#include "verification.h"
#include <storm-pomdp/api/verification.h>

template<typename ValueType>
void define_verification(py::module& m, std::string const& vtSuffix) {
    typedef storm::pomdp::modelchecker::BeliefExplorationPomdpModelCheckerOptions<ValueType> Options;
    py::class_<typename storm::pomdp::modelchecker::BeliefExplorationPomdpModelChecker<storm::models::sparse::Pomdp<ValueType>>> bepmc(m, ("BeliefExplorationModelChecker" + vtSuffix).c_str());
    bepmc.def(py::init<std::shared_ptr<storm::models::sparse::Pomdp<ValueType>>, Options>(), py::arg("model"), py::arg("options"));
    bepmc.def("check", &storm::pomdp::modelchecker::BeliefExplorationPomdpModelChecker<storm::models::sparse::Pomdp<ValueType>>::check, py::arg("task"), py::arg("values"));

    py::class_<Options> bepmcoptions(m, ("BeliefExplorationModelCheckerOptions" + vtSuffix).c_str());
    bepmcoptions.def(py::init<bool, bool>(), py::arg("discretize"), py::arg("unfold"));
    bepmcoptions.def_readwrite("use_explicit_cutoff", &Options::useExplicitCutoff);
    bepmcoptions.def_readwrite("size_threshold_init", &Options::sizeThresholdInit);
    bepmcoptions.def_readwrite("use_grid_clipping", &Options::useGridClipping);
    bepmcoptions.def_readwrite("exploration_time_limit", &Options::explorationTimeLimit);
    bepmcoptions.def_readwrite("clipping_threshold_init", &Options::clippingThresholdInit);
    bepmcoptions.def_readwrite("clipping_grid_res", &Options::clippingGridRes);
    bepmcoptions.def_readwrite("gap_threshold_init", &Options::gapThresholdInit);
    bepmcoptions.def_readwrite("size_threshold_factor", &Options::sizeThresholdFactor);
    bepmcoptions.def_readwrite("refine_precision", &Options::refinePrecision);
    bepmcoptions.def_readwrite("refine_step_limit", &Options::refineStepLimit);
    bepmcoptions.def_readwrite("refine", &Options::refine);
    bepmcoptions.def_readwrite("exploration_heuristic", &Options::explorationHeuristic);
    bepmcoptions.def_readwrite("preproc_minmax_method", &Options::preProcMinMaxMethod);
    
    py::class_<typename storm::pomdp::modelchecker::BeliefExplorationPomdpModelChecker<storm::models::sparse::Pomdp<ValueType>>::Result> bepmcres(m, ("BeliefExplorationPomdpModelCheckerResult" + vtSuffix).c_str());
    bepmcres.def_readonly("induced_mc_from_scheduler", &storm::pomdp::modelchecker::BeliefExplorationPomdpModelChecker<storm::models::sparse::Pomdp<ValueType>>::Result::schedulerAsMarkovChain);
    bepmcres.def_readonly("cutoff_schedulers", &storm::pomdp::modelchecker::BeliefExplorationPomdpModelChecker<storm::models::sparse::Pomdp<ValueType>>::Result::cutoffSchedulers);
    bepmcres.def_readonly("lower_bound", &storm::pomdp::modelchecker::BeliefExplorationPomdpModelChecker<storm::models::sparse::Pomdp<ValueType>>::Result::lowerBound);
    bepmcres.def_readonly("upper_bound", &storm::pomdp::modelchecker::BeliefExplorationPomdpModelChecker<storm::models::sparse::Pomdp<ValueType>>::Result::upperBound);
}

template void define_verification<double>(py::module& m, std::string const& vtSuffix);