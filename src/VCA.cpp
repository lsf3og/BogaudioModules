
#include "VCA.hpp"

void VCA::onSampleRateChange() {
	float sampleRate = APP->engine->getSampleRate();
	_levelSL1.setParams(sampleRate, 5.0f, 1.0f);
	_levelSL2.setParams(sampleRate, 5.0f, 1.0f);
}

void VCA::process(const ProcessArgs& args) {
	bool linear = isLinear();
	lights[LINEAR_LIGHT].value = linear;
	channelStep(inputs[IN1_INPUT], outputs[OUT1_OUTPUT], params[LEVEL1_PARAM], inputs[CV1_INPUT], _amplifier1, _levelSL1, linear);
	channelStep(inputs[IN2_INPUT], outputs[OUT2_OUTPUT], params[LEVEL2_PARAM], inputs[CV2_INPUT], _amplifier2, _levelSL2, linear);
}

void VCA::channelStep(Input& input, Output& output, Param& knob, Input& cv, Amplifier& amplifier, bogaudio::dsp::SlewLimiter& levelSL, bool linear) {
	if (input.isConnected() && output.isConnected()) {
		float level = knob.value;
		if (cv.isConnected()) {
			level *= clamp(cv.getVoltage() / 10.0f, 0.0f, 1.0f);
		}
		level = levelSL.next(level);
		if (linear) {
			output.setVoltage(level * input.value);
		}
		else {
			level = 1.0f - level;
			level *= Amplifier::minDecibels;
			amplifier.setLevel(level);
			output.setVoltage(amplifier.next(input.value));
		}
	}
}

struct VCAWidget : ModuleWidget {
	static constexpr int hp = 3;

	VCAWidget(VCA* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SvgPanel *panel = new SvgPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VCA.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto level1ParamPosition = Vec(9.5, 22.5);
		auto level2ParamPosition = Vec(9.5, 181.5);
		auto linearParamPosition = Vec(29.4, 332.9);

		auto cv1InputPosition = Vec(10.5, 60.0);
		auto in1InputPosition = Vec(10.5, 95.0);
		auto cv2InputPosition = Vec(10.5, 219.0);
		auto in2InputPosition = Vec(10.5, 254.0);

		auto out1OutputPosition = Vec(10.5, 133.0);
		auto out2OutputPosition = Vec(10.5, 292.0);

		auto linearLightPosition = Vec(6.5, 334.5);
		// end generated by svg_widgets.rb

		addParam(createParam<Knob26>(level1ParamPosition, module, VCA::LEVEL1_PARAM));
		addParam(createParam<Knob26>(level2ParamPosition, module, VCA::LEVEL2_PARAM));
		addParam(createParam<StatefulButton9>(linearParamPosition, module, VCA::LINEAR_PARAM));

		addInput(createInput<Port24>(cv1InputPosition, module, VCA::CV1_INPUT));
		addInput(createInput<Port24>(in1InputPosition, module, VCA::IN1_INPUT));
		addInput(createInput<Port24>(cv2InputPosition, module, VCA::CV2_INPUT));
		addInput(createInput<Port24>(in2InputPosition, module, VCA::IN2_INPUT));

		addOutput(createOutput<Port24>(out1OutputPosition, module, VCA::OUT1_OUTPUT));
		addOutput(createOutput<Port24>(out2OutputPosition, module, VCA::OUT2_OUTPUT));

		addChild(createLight<SmallLight<GreenLight>>(linearLightPosition, module, VCA::LINEAR_LIGHT));
	}
};

Model* modelVCA = bogaudio::createModel<VCA, VCAWidget>("Bogaudio-VCA", "VCA",  "dual attenuator", "VCA", "Dual");
