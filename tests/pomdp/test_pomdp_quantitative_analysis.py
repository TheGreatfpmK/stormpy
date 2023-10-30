import stormpy

from configurations import pomdp

from helpers.helper import get_example_path


@pomdp
class TestPomdpQuantitative:
    def test_underapprox_mc_maze_pmax(self):
        program = stormpy.parse_prism_program(get_example_path("pomdp", "maze_2.prism"))
        formulas = stormpy.parse_properties_for_prism_program("Pmax=? [ !\"bad\" U \"goal\" ]", program)
        model = stormpy.build_model(program, formulas)
        model = stormpy.pomdp.make_canonic(model)
        options = stormpy.pomdp.BeliefExplorationModelCheckerOptionsDouble(False, True)
        options.use_state_elimination_cutoff = False
        options.size_threshold_init = 10
        options.use_clipping = False
        belmc = stormpy.pomdp.BeliefExplorationModelCheckerDouble(model, options)
        result = belmc.check(formulas[0].raw_formula, [])
        assert round(result.lower_bound, 3) == 0.352
        assert result.induced_mc_from_scheduler.nr_states == 10
        assert result.induced_mc_from_scheduler.nr_transitions == 23
        assert result.cutoff_schedulers[0].get_choice(1).__str__() == "{[0.25: 0], [0.25: 1], [0.25: 2], [0.25: 3], }"

    def test_underapprox_mc_maze_rmin(self):
        program = stormpy.parse_prism_program(get_example_path("pomdp", "maze_2.prism"))
        formulas = stormpy.parse_properties_for_prism_program("Rmin=? [ F \"goal\" ]", program)
        model = stormpy.build_model(program, formulas)
        model = stormpy.pomdp.make_canonic(model)
        options = stormpy.pomdp.BeliefExplorationModelCheckerOptionsDouble(False, True)
        options.use_state_elimination_cutoff = False
        options.size_threshold_init = 10
        options.use_clipping = False
        belmc = stormpy.pomdp.BeliefExplorationModelCheckerDouble(model, options)
        result = belmc.check(formulas[0].raw_formula, [])
        assert round(result.upper_bound, 3) == 12.270
        assert result.induced_mc_from_scheduler.nr_states == 9
        assert result.induced_mc_from_scheduler.nr_transitions == 17
        assert result.cutoff_schedulers[0].get_choice(2).__str__() == "{[0.5: 0], [0.5: 1], }"

    def test_underapprox_mc_cmaze_rmin(self):
        program = stormpy.parse_prism_program(get_example_path("pomdp", "maze-concise.prism"))
        formulas = stormpy.parse_properties_for_prism_program("R{\"steps\"}min=? [F ((x = 2) & (y = 0))]", program)
        model = stormpy.build_model(program, formulas)
        model = stormpy.pomdp.make_canonic(model)
        options = stormpy.pomdp.BeliefExplorationModelCheckerOptionsDouble(False, True)
        options.use_state_elimination_cutoff = False
        options.size_threshold_init = 10
        options.use_clipping = False
        belmc = stormpy.pomdp.BeliefExplorationModelCheckerDouble(model, options)
        result = belmc.check(formulas[0].raw_formula, [])
        assert round(result.upper_bound, 3) == 19.781
        assert result.induced_mc_from_scheduler.nr_states == 9
        assert result.induced_mc_from_scheduler.nr_transitions == 15
        assert result.cutoff_schedulers[0].get_choice(5).__str__() == "{[0.5: 1], [0.5: 3], }"