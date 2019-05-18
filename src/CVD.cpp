
#include "CVD.hpp"

void CVD::onSampleRateChange() {
	_delay.setSampleRate(engineGetSampleRate());
}

void CVD::process(const ProcessArgs& args) {
	float time = params[TIME_PARAM].value;
	if (inputs[TIME_INPUT].active) {
		time *= clamp(inputs[TIME_INPUT].value / 10.0f, 0.0f, 1.0f);
	}
	switch ((int)params[TIME_SCALE_PARAM].value) {
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

	float mix = params[MIX_PARAM].value;
	if (inputs[MIX_INPUT].active) {
		mix = clamp(mix + inputs[MIX_INPUT].value / 5.0f, -1.0f, 1.0f);
	}
	_mix.setParams(mix);

	float in = inputs[IN_INPUT].value;
	float delayed = _delay.next(in);
	outputs[OUT_OUTPUT].value = _mix.next(in, delayed);
}

struct CVDWidget : ModuleWidget {
	static constexpr int hp = 3;

	CVDWidget(CVD* module) : ModuleWidget(module) {
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(asset::plugin(pluginInstance, "res/CVD.svg")));
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

		addParam(createParam<Knob29>(timeParamPosition, module, CVD::TIME_PARAM, 0.0, 1.0, 0.5));
		{
			auto w = createParam<Knob16>(timeScaleParamPosition, module, CVD::TIME_SCALE_PARAM, 0.0, 2.0, 1.0);
			auto k = dynamic_cast<SVGKnob*>(w);
			k->snap = true;
			k->minAngle = -M_PI / 4.0f;
			k->maxAngle = M_PI / 4.0f;
			k->speed = 3.0;
			addParam(w);
		}
		addParam(createParam<Knob29>(mixParamPosition, module, CVD::MIX_PARAM, -1.0, 1.0, 0.0));

		addInput(createPort<Port24>(timeInputPosition, PortWidget::INPUT, module, CVD::TIME_INPUT));
		addInput(createPort<Port24>(mixInputPosition, PortWidget::INPUT, module, CVD::MIX_INPUT));
		addInput(createPort<Port24>(inInputPosition, PortWidget::INPUT, module, CVD::IN_INPUT));

		addOutput(createPort<Port24>(outOutputPosition, PortWidget::OUTPUT, module, CVD::OUT_OUTPUT));
	}
};

Model* modelCVD = bogaudio::createModel<CVD, CVDWidget>("Bogaudio-CVD", "CVD",  "CV delay", DELAY_TAG);
