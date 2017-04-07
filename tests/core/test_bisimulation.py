import stormpy
import stormpy.logic
from helpers.helper import get_example_path
import math

import math

class TestBisimulation:
    def test_bisimulation(self):
        program = stormpy.parse_prism_program(get_example_path("dtmc", "crowds5_5.pm"))
        assert program.nr_modules == 1
        assert program.model_type == stormpy.PrismModelType.DTMC

        prop = "P=? [F \"observe0Greater1\"]"
        properties = stormpy.parse_properties_for_prism_program(prop, program)
        model = stormpy.build_model(program, properties)
        assert model.nr_states == 7403
        assert model.nr_transitions == 13041
        assert model.model_type == stormpy.ModelType.DTMC
        assert not model.supports_parameters
        initial_state = model.initial_states[0]
        assert initial_state == 0
        result = stormpy.model_checking(model, properties[0])
        model_bisim = stormpy.perform_bisimulation(model, properties, stormpy.BisimulationType.STRONG)
        assert model_bisim.nr_states == 64
        assert model_bisim.nr_transitions == 104
        assert model_bisim.model_type == stormpy.ModelType.DTMC
        assert not model_bisim.supports_parameters
        result_bisim = stormpy.model_checking(model_bisim, properties[0])
        initial_state_bisim = model_bisim.initial_states[0]
        assert initial_state_bisim == 34
        assert math.isclose(result.at(initial_state), result_bisim.at(initial_state_bisim), rel_tol=1e-4)

    def test_parametric_bisimulation(self):
        import pycarl
        program = stormpy.parse_prism_program(get_example_path("pdtmc", "crowds3_5.pm"))
        assert program.nr_modules == 1
        assert program.model_type == stormpy.PrismModelType.DTMC
        assert program.has_undefined_constants

        prop = "P=? [F \"observe0Greater1\"]"
        properties = stormpy.parse_properties_for_prism_program(prop, program)
        model = stormpy.build_parametric_model(program, properties)
        assert model.nr_states == 1367
        assert model.nr_transitions == 2027
        assert model.model_type == stormpy.ModelType.DTMC
        assert model.has_parameters
        result = stormpy.model_checking(model, properties[0])
        initial_state = model.initial_states[0]
        assert initial_state == 0
        ratFunc = result.result.at(initial_state)
        model_bisim = stormpy.perform_bisimulation(model, properties, stormpy.BisimulationType.STRONG)
        assert model_bisim.nr_states == 80
        assert model_bisim.nr_transitions == 120
        assert model_bisim.model_type == stormpy.ModelType.DTMC
        assert model_bisim.has_parameters
        result_bisim = stormpy.model_checking(model_bisim, properties[0])
        initial_state_bisim = model_bisim.initial_states[0]
        assert initial_state_bisim == 48
        ratFunc_bisim = result_bisim.result.at(initial_state_bisim)
        assert ratFunc == ratFunc_bisim
