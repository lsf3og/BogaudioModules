
#include "CVD.hpp"

void CVD::onSampleRateChange() {
	_delay.setSampleRate(APP->engine->getSampleRate());
}

void CVD::process(const ProcessArgs& args) {
	float time = params[TIME_PARAM].getValue();
	if (inputs[TIME_INPUT].isConnected()) {
		time *= clamp(inputs[TIME_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
	}
	switch ((int)params[TIME_SCALE_PARAM].getValue()) {
		case 0: {
			time /= 100.f;
			break;
		}
		case 1: {
			time /= 10.f;
			break;
		}
	}
	_delay.setTime(time);

	float mix = params[MIX_PARAM].getValue();
	if (inputs[MIX_INPUT].isConnected()) {
		mix = clamp(mix + inputs[MIX_INPUT].getVoltage() / 5.0f, -1.0f, 1.0f);
	}
	_mix.setParams(mix);

	float in = inputs[IN_INPUT].getVoltage();
	float delayed = _delay.next(in);
	outputs[OUT_OUTPUT].setVoltage(_mix.next(in, delayed));
}

struct CVDWidget : ModuleWidget {
	static constexpr int hp = 3;

	CVDWidget(CVD* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SvgPanel *panel = new SvgPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CVD.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto timeParamPosition = Vec(8.0, 36.0);
		auto timeScaleParamPosition = Vec(14.5, 84.0);
		auto mixParamPosition = Vec(8.0, 176.0);

		auto timeInputPosition = Vec(10.5, 107.0);
		auto mixInputPosition = Vec(10.5, 217.0);
		auto inInputPosition = Vec(10.5, 267.0);

		auto outOutputPosition = Vec(10.5, 305.0);
		// end generated by svg_widgets.rb

		addParam(createParam<Knob29>(timeParamPosition, module, CVD::TIME_PARAM));
		{
			auto w = createParam<Knob16>(timeScaleParamPosition, module, CVD::TIME_SCALE_PARAM);
			auto k = dynamic_cast<SvgKnob*>(w);
			k->snap = true;
			k->minAngle = -M_PI / 4.0f;
			k->maxAngle = M_PI / 4.0f;
			k->speed = 3.0;
			addParam(w);
		}
		addParam(createParam<Knob29>(mixParamPosition, module, CVD::MIX_PARAM));

		addInput(createInput<Port24>(timeInputPosition, module, CVD::TIME_INPUT));
		addInput(createInput<Port24>(mixInputPosition, module, CVD::MIX_INPUT));
		addInput(createInput<Port24>(inInputPosition, module, CVD::IN_INPUT));

		addOutput(createOutput<Port24>(outOutputPosition, module, CVD::OUT_OUTPUT));
	}
};

Model* modelCVD = bogaudio::createModel<CVD, CVDWidget>("Bogaudio-CVD", "CVD",  "CV delay", "Delay");
