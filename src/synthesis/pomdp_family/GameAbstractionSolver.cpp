#include "GameAbstractionSolver.h"

#include <queue>

namespace synthesis {
        
    template<typename ValueType>
    void print_matrix(storm::storage::SparseMatrix<ValueType> matrix) {
        auto const& row_group_indices = matrix.getRowGroupIndices();
        for(uint64_t state=0; state < matrix.getRowGroupCount(); state++) {
            std::cout << "state " << state << ": " << std::endl;
            for(uint64_t row=row_group_indices[state]; row<row_group_indices[state+1]; row++) {
                for(auto const &entry: matrix.getRow(row)) {
                    std::cout << state << "-> "  << entry.getColumn() << " ["  << entry.getValue() << "];";
                }
                std::cout << std::endl;
            }
        }
        std::cout << "-----" << std::endl;
    }

    template void print_matrix<storm::storage::sparse::state_type>(storm::storage::SparseMatrix<storm::storage::sparse::state_type> matrix);
    template void print_matrix<double>(storm::storage::SparseMatrix<double> matrix);

    template<typename ValueType>
    GameAbstractionSolver<ValueType>::GameAbstractionSolver(
        storm::models::sparse::Model<ValueType> const& quotient,
        uint64_t quotient_num_actions,
        std::vector<uint64_t> const& choice_to_action,
        std::string const& target_label
    ) : quotient(quotient), quotient_num_actions(quotient_num_actions), choice_to_action(choice_to_action) {

        auto quotient_num_states = quotient.getNumberOfStates();
        auto quotient_num_choices = quotient.getNumberOfChoices();

        this->choice_to_destinations.resize(quotient_num_choices);
        for(uint64_t choice=0; choice<quotient_num_choices; choice++) {
            for(auto const &entry: quotient.getTransitionMatrix().getRow(choice)) {
                this->choice_to_destinations[choice].push_back(entry.getColumn());
            }
        }
        this->setupSolverEnvironment();

        // identify target states
        this->state_is_target = storm::storage::BitVector(quotient_num_states,false);
        for(auto state: quotient.getStateLabeling().getStates(target_label)) {
            this->state_is_target.set(state,true);
        }

        this->solution_state_values = std::vector<double>(quotient_num_states,0);
        this->solution_state_to_player1_action = std::vector<uint64_t>(quotient_num_states,quotient_num_actions);
        this->solution_all_choices = storm::storage::BitVector(quotient_num_choices,false);
        this->solution_reachable_choices = storm::storage::BitVector(quotient_num_choices,false);
    }

    
    template<typename ValueType>
    void GameAbstractionSolver<ValueType>::setupSolverEnvironment() {
        this->env.solver().game().setPrecision(storm::utility::convertNumber<storm::RationalNumber>(1e-8));

        // value iteration
        // this->env.solver().game().setMethod(storm::solver::GameMethod::ValueIteration);
        
        // policy iteration
        this->env.solver().game().setMethod(storm::solver::GameMethod::PolicyIteration);
        this->env.solver().setLinearEquationSolverType(storm::solver::EquationSolverType::Native);
        this->env.solver().native().setMethod(storm::solver::NativeLinearEquationSolverMethod::Jacobi);
        this->env.solver().setLinearEquationSolverPrecision(env.solver().game().getPrecision());
    }

    
    template<typename ValueType>
    storm::OptimizationDirection GameAbstractionSolver<ValueType>::getOptimizationDirection(bool maximizing) {
        return maximizing ? storm::OptimizationDirection::Maximize : storm::OptimizationDirection::Minimize;
    }


    template<typename ValueType>
    void GameAbstractionSolver<ValueType>::solve(
        storm::storage::BitVector quotient_choice_mask,
        bool player1_maximizing, bool player2_maximizing
    ) {
        auto quotient_num_states = this->quotient.getNumberOfStates();
        auto quotient_num_choices = this->quotient.getNumberOfChoices();
        auto quotient_initial_state = *(this->quotient.getInitialStates().begin());

        ItemTranslator state_to_player1_state(quotient_num_states);
        ItemKeyTranslator<uint64_t> state_action_to_player2_state(quotient_num_states);
        std::vector<std::set<uint64_t>> player1_state_to_actions;
        std::vector<std::vector<uint64_t>> player2_state_to_choices;
        
        std::queue<uint64_t> unexplored_states;
        storm::storage::BitVector state_is_encountered(quotient_num_states);
        unexplored_states.push(quotient_initial_state);
        state_is_encountered.set(quotient_initial_state,true);
        auto const& quotient_row_group_indices = this->quotient.getTransitionMatrix().getRowGroupIndices();
        while(not unexplored_states.empty()) {
            auto state = unexplored_states.front();
            unexplored_states.pop();
            auto player1_state = state_to_player1_state.translate(state);
            player1_state_to_actions.resize(state_to_player1_state.numTranslations());
            for(auto choice = quotient_row_group_indices[state]; choice < quotient_row_group_indices[state+1]; choice++) {
                if(not quotient_choice_mask[choice]) {
                    continue;
                }
                auto action = choice_to_action[choice];
                player1_state_to_actions[player1_state].insert(action);
                auto player2_state = state_action_to_player2_state.translate(state,action);
                player2_state_to_choices.resize(state_action_to_player2_state.numTranslations());
                player2_state_to_choices[player2_state].push_back(choice);
                for(auto state_dst: this->choice_to_destinations[choice]) {
                    if(state_is_encountered[state_dst]) {
                        continue;
                    }
                    unexplored_states.push(state_dst);
                    state_is_encountered.set(state_dst,true);
                }
            }
        }
        auto player1_num_states = state_to_player1_state.numTranslations();
        auto player2_num_states = state_action_to_player2_state.numTranslations();
        
        // add fresh target states
        auto player1_target_state = player1_num_states++;
        auto player2_target_state = player2_num_states++;

        // build the matrix of Player 1
        std::vector<uint64_t> player1_choice_to_action;
        storm::storage::SparseMatrixBuilder<storm::storage::sparse::state_type> player1_matrix_builder(0,0,0,false,true);
        uint64_t player1_num_rows = 0;
        for(uint64_t player1_state=0; player1_state<player1_num_states-1; player1_state++) {
            player1_matrix_builder.newRowGroup(player1_num_rows);
            auto state = state_to_player1_state.retrieve(player1_state);
            for(auto action: player1_state_to_actions[player1_state]) {
                player1_choice_to_action.push_back(action);
                auto player2_state = state_action_to_player2_state.translate(state,action);
                player1_matrix_builder.addNextValue(player1_num_rows,player2_state,1);
                player1_num_rows++;
            }
        }
        // fresh target state of Player 1
        player1_matrix_builder.newRowGroup(player1_num_rows);
        player1_matrix_builder.addNextValue(player1_num_rows,player2_target_state,1);
        player1_num_rows++;
        auto player1_matrix = player1_matrix_builder.build();

        
        // build the matrix of Player 2
        std::vector<uint64_t> player2_choice_to_quotient_choice;
        storm::storage::SparseMatrixBuilder<ValueType> player2_matrix_builder(0,0,0,false,true);
        // build the reward vector
        std::vector<double> player2_row_rewards;
        uint64_t player2_num_rows = 0;
        for(uint64_t player2_state=0; player2_state<player2_num_states-1; player2_state++) {
            player2_matrix_builder.newRowGroup(player2_num_rows);
            auto [state,action] = state_action_to_player2_state.retrieve(player2_state);
            if(state_is_target[state]) {
                // target state, transition to the target state of Player 1 and gain unit reward
                player2_matrix_builder.addNextValue(player2_num_rows,player1_target_state,1);
                player2_choice_to_quotient_choice.push_back(quotient_num_choices);
                player2_row_rewards.push_back(1);
                player2_num_rows++;
            } else {
                // state is not target
                for(auto choice: player2_state_to_choices[player2_state]) {
                    // transition to the corresponding states of Player 1 and gain zero reward
                    for(auto const& entry: this->quotient.getTransitionMatrix().getRow(choice)) {
                        auto state_dst = entry.getColumn();
                        auto player1_state_dst = state_to_player1_state.translate(state_dst);
                        player2_matrix_builder.addNextValue(player2_num_rows,player1_state_dst,entry.getValue());
                    }
                    player2_choice_to_quotient_choice.push_back(choice);
                    player2_row_rewards.push_back(0);
                    player2_num_rows++;
                }
            }
        }
        // fresh target state of Player 2: transition to the target state of Player 1 with zero reward
        player2_matrix_builder.newRowGroup(player2_num_rows);
        player2_matrix_builder.addNextValue(player2_num_rows,player1_target_state,1);
        player2_choice_to_quotient_choice.push_back(quotient_num_choices);
        player2_row_rewards.push_back(0);
        player2_num_rows++;
        auto player2_matrix = player2_matrix_builder.build();

        // solve the game
        auto solver = storm::solver::GameSolverFactory<ValueType>().create(env, player1_matrix, player2_matrix);
        solver->setTrackSchedulers(true);
        auto player1_direction = this->getOptimizationDirection(player1_maximizing);
        auto player2_direction = this->getOptimizationDirection(player2_maximizing);
        std::vector<double> player1_state_values(player1_num_states,0);
        solver->solveGame(this->env, player1_direction, player2_direction, player1_state_values, player2_row_rewards);
        auto player1_choices = solver->getPlayer1SchedulerChoices();
        auto player2_choices = solver->getPlayer2SchedulerChoices();

        // collect all the results
        std::fill(this->solution_state_values.begin(),this->solution_state_values.end(),0);
        std::fill(this->solution_state_to_player1_action.begin(),this->solution_state_to_player1_action.end(),quotient_num_actions);
        this->solution_all_choices.clear();
        this->solution_reachable_choices.clear();

        auto const& player1_matrix_row_group_indices = player1_matrix.getRowGroupIndices();
        auto const& player2_matrix_row_group_indices = player2_matrix.getRowGroupIndices();

        for(uint64_t player1_state=0; player1_state<player1_num_states-1; player1_state++) {
            auto state = state_to_player1_state.retrieve(player1_state);
            this->solution_state_values[state] = player1_state_values[player1_state];

            // get action selected by Player 1
            auto player1_choice = player1_matrix_row_group_indices[player1_state] + player1_choices[player1_state];
            auto player1_action = player1_choice_to_action[player1_choice];
            this->solution_state_to_player1_action[state] = player1_action;

            if(this->state_is_target[state]) {
                // if the state is target, there is exactly one action that is performed
                auto state_only_choice = quotient_row_group_indices[state];
                this->solution_all_choices.set(state_only_choice,true);
                this->solution_reachable_choices.set(state_only_choice,true);
                continue;
            }

            // scan through all actions available to Player 1
            for(auto action: player1_state_to_actions[player1_state]) {
                // get action selected by Player 2
                auto player2_state = state_action_to_player2_state.translate(state,action);
                auto player2_choice = player2_matrix_row_group_indices[player2_state]+player2_choices[player2_state];
                auto quotient_choice = player2_choice_to_quotient_choice[player2_choice];
                this->solution_all_choices.set(quotient_choice,true);
                if(action == player1_action) {
                    this->solution_reachable_choices.set(quotient_choice,true);
                }
            }
        }
        this->solution_value = this->solution_state_values[quotient_initial_state];

    }


    template class GameAbstractionSolver<double>;
}