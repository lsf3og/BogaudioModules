
#include "SampleHold.hpp"

void SampleHold::onReset() {
	_trigger1.reset();
	_value1 = 0.0f;
	_trigger2.reset();
	_value2 = 0.0f;
}

void SampleHold::process(const ProcessArgs& args) {
	{
		lights[TRACK1_LIGHT].value = params[TRACK1_PARAM].getValue();
		bool triggered = _trigger1.process(params[TRIGGER1_PARAM].getValue() + inputs[TRIGGER1_INPUT].getVoltage());
		if (params[TRACK1_PARAM].getValue() > 0.5f ? _trigger1.isHigh() : triggered) {
			if (inputs[IN1_INPUT].isConnected()) {
				_value1 = inputs[IN1_INPUT].getVoltage();
			}
			else {
				_value1 = fabsf(_noise.next()) * 10.0;
			}
		}
		outputs[OUT1_OUTPUT].setVoltage(_value1);
	}

	{
		lights[TRACK2_LIGHT].value = params[TRACK2_PARAM].getValue();
		bool triggered = _trigger2.process(params[TRIGGER2_PARAM].getValue() + inputs[TRIGGER2_INPUT].getVoltage());
		if (params[TRACK2_PARAM].getValue() > 0.5f ? _trigger2.isHigh() : triggered) {
			if (inputs[IN2_INPUT].isConnected()) {
				_value2 = inputs[IN2_INPUT].getVoltage();
			}
			else {
				_value2 = fabsf(_noise.next()) * 10.0;
			}
		}
		outputs[OUT2_OUTPUT].setVoltage(_value2);
	}
}

struct SampleHoldWidget : ModuleWidget {
	static constexpr int hp = 3;

	SampleHoldWidget(SampleHold* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SvgPanel *panel = new SvgPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SampleHold.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto trigger1ParamPosition = Vec(13.5, 27.0);
		auto track1ParamPosition = Vec(29.0, 122.7);
		auto trigger2ParamPosition = Vec(13.5, 190.0);
		auto track2ParamPosition = Vec(29.0, 285.7);

		auto trigger1InputPosition = Vec(10.5, 49.0);
		auto in1InputPosition = Vec(10.5, 86.0);
		auto trigger2InputPosition = Vec(10.5, 212.0);
		auto in2InputPosition = Vec(10.5, 249.0);

		auto out1OutputPosition = Vec(10.5, 137.0);
		auto out2OutputPosition = Vec(10.5, 300.0);

		auto track1LightPosition = Vec(7.0, 124.0);
		auto track2LightPosition = Vec(7.0, 287.0);
		// end generated by svg_widgets.rb

		addParam(createParam<Button18>(trigger1ParamPosition, module, SampleHold::TRIGGER1_PARAM));
		addParam(createParam<Button18>(trigger2ParamPosition, module, SampleHold::TRIGGER2_PARAM));
		addParam(createParam<StatefulButton9>(track1ParamPosition, module, SampleHold::TRACK1_PARAM));
		addParam(createParam<StatefulButton9>(track2ParamPosition, module, SampleHold::TRACK2_PARAM));

		addInput(createInput<Port24>(trigger1InputPosition, module, SampleHold::TRIGGER1_INPUT));
		addInput(createInput<Port24>(in1InputPosition, module, SampleHold::IN1_INPUT));
		addInput(createInput<Port24>(trigger2InputPosition, module, SampleHold::TRIGGER2_INPUT));
		addInput(createInput<Port24>(in2InputPosition, module, SampleHold::IN2_INPUT));

		addOutput(createOutput<Port24>(out1OutputPosition, module, SampleHold::OUT1_OUTPUT));
		addOutput(createOutput<Port24>(out2OutputPosition, module, SampleHold::OUT2_OUTPUT));

		addChild(createLight<SmallLight<GreenLight>>(track1LightPosition, module, SampleHold::TRACK1_LIGHT));
		addChild(createLight<SmallLight<GreenLight>>(track2LightPosition, module, SampleHold::TRACK2_LIGHT));
	}
};

Model* modelSampleHold = bogaudio::createModel<SampleHold, SampleHoldWidget>("Bogaudio-SampleHold", "S&H",  "dual sample (or track) and hold", "Sample and hold", "Dual");
