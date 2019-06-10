#pragma once

#include "bogaudio.hpp"
#include "disable_output_limit.hpp"
#include "dsp/signal.hpp"

using namespace bogaudio::dsp;

extern Model* modelVCM;

namespace bogaudio {

struct VCMLevelParamQuantity;

struct VCM : DisableOutputLimitModule {
	enum ParamsIds {
		LEVEL1_PARAM,
		LEVEL2_PARAM,
		LEVEL3_PARAM,
		LEVEL4_PARAM,
		MIX_PARAM,
		LINEAR_PARAM,
		NUM_PARAMS
	};

	enum InputsIds {
		IN1_INPUT,
		CV1_INPUT,
		IN2_INPUT,
		CV2_INPUT,
		IN3_INPUT,
		CV3_INPUT,
		IN4_INPUT,
		CV4_INPUT,
		MIX_CV_INPUT,
		NUM_INPUTS
	};

	enum OutputsIds {
		MIX_OUTPUT,
		NUM_OUTPUTS
	};

	enum LightsIds {
		LINEAR_LIGHT,
		NUM_LIGHTS
	};

	Amplifier _amplifier1;
	Amplifier _amplifier2;
	Amplifier _amplifier3;
	Amplifier _amplifier4;

	VCM() : DisableOutputLimitModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		configParam<VCMLevelParamQuantity>(LEVEL1_PARAM, 0.0f, 1.0f, 0.8f, "Level 1");
		configParam<VCMLevelParamQuantity>(LEVEL2_PARAM, 0.0f, 1.0f, 0.8f, "Level 2");
		configParam<VCMLevelParamQuantity>(LEVEL3_PARAM, 0.0f, 1.0f, 0.8f, "Level 3");
		configParam<VCMLevelParamQuantity>(LEVEL4_PARAM, 0.0f, 1.0f, 0.8f, "Level 4");
		configParam<VCMLevelParamQuantity>(MIX_PARAM, 0.0f, 1.0f, 0.8f, "Mix level");
		configParam(LINEAR_PARAM, 0.0f, 1.0f, 0.0f, "Linear");
		onReset();
	}

	inline bool isLinear() { return params[LINEAR_PARAM].getValue() > 0.5f; }
	void process(const ProcessArgs& args) override;
	float channelStep(Input& input, Param& knob, Input& cv, Amplifier& amplifier, bool linear);
};

struct VCMLevelParamQuantity : AmpliferParamQuantity {
	bool isLinear() override {
		return static_cast<VCM*>(module)->isLinear();
	}
};

} // namespace bogaudio
