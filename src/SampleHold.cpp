
#include "SampleHold.hpp"

#define POLY_INPUT "poly_input"
#define NOISE_TYPE "noise_type"
#define RANGE_OFFSET "range_offset"
#define RANGE_SCALE "range_scale"

void SampleHold::reset() {
	for (int i = 0; i < maxChannels; ++i) {
		_trigger1[i].reset();
		_value1[i] = 0.0f;
		_trigger2[i].reset();
		_value2[i] = 0.0f;
	}
}

json_t* SampleHold::dataToJson() {
	json_t* root = json_object();
	json_object_set_new(root, POLY_INPUT, json_integer(_polyInputID));
	json_object_set_new(root, NOISE_TYPE, json_integer((int)_noiseType));
	json_object_set_new(root, RANGE_OFFSET, json_real(_rangeOffset));
	json_object_set_new(root, RANGE_SCALE, json_real(_rangeScale));
	return root;
}

void SampleHold::dataFromJson(json_t* root) {
	json_t* p = json_object_get(root, POLY_INPUT);
	if (p) {
		_polyInputID = json_integer_value(p);
	}

	json_t* nt = json_object_get(root, NOISE_TYPE);
	if (nt) {
		_noiseType = (NoiseType)json_integer_value(nt);
	}

	json_t* ro = json_object_get(root, RANGE_OFFSET);
	if (ro) {
		_rangeOffset = json_real_value(ro);
	}

	json_t* rs = json_object_get(root, RANGE_SCALE);
	if (rs) {
		_rangeScale = json_real_value(rs);
	}
}

void SampleHold::processAll(const ProcessArgs& args) {
	handleChannel(
		params[TRACK1_PARAM],
		params[INVERT1_PARAM],
		_trigger1,
		params[TRIGGER1_PARAM],
		inputs[TRIGGER1_INPUT],
		NULL,
		inputs[IN1_INPUT],
		_value1,
		outputs[OUT1_OUTPUT]
	);
	handleChannel(
		params[TRACK2_PARAM],
		params[INVERT2_PARAM],
		_trigger2,
		params[TRIGGER2_PARAM],
		inputs[TRIGGER2_INPUT],
		&inputs[TRIGGER1_INPUT],
		inputs[IN2_INPUT],
		_value2,
		outputs[OUT2_OUTPUT]
	);
}

void SampleHold::handleChannel(
	Param& trackParam,
	Param& invertParam,
	Trigger* trigger,
	Param& triggerParam,
	Input& triggerInput,
	Input* altTriggerInput,
	Input& in,
	float* value,
	Output& out
) {
	int n = std::max(1, _polyInputID == IN1_INPUT ? in.getChannels() : triggerInput.getChannels());
	out.setChannels(n);
	for (int i = 0; i < n; ++i) {
		float triggerIn = 0.0f;
		if (triggerInput.isConnected()) {
			triggerIn = triggerInput.getPolyVoltage(i);
		} else if (altTriggerInput) {
			triggerIn = altTriggerInput->getPolyVoltage(i);
		}

		bool triggered = trigger[i].process(triggerParam.getValue() + triggerIn);
		if (trackParam.getValue() > 0.5f ? trigger[i].isHigh() : triggered) {
			if (in.isConnected()) {
				value[i] = in.getPolyVoltage(i);
			}
			else {
				value[i] = (noise() + _rangeOffset) * _rangeScale;
			}
		}

		float o = value[i];
		if (invertParam.getValue() > 0.5f) {
			o = -o;
		}
		out.setVoltage(o, i);
	}
}

float SampleHold::noise() {
	switch (_noiseType) {
		case BLUE_NOISE_TYPE: {
			return clamp(2.0f * _blue.next(), -1.0f, 1.0f);
		}
		case PINK_NOISE_TYPE: {
			return clamp(1.5f * _pink.next(), -1.0f, 1.0f);
		}
		case RED_NOISE_TYPE: {
			return clamp(2.0f * _red.next(), -1.0f, 1.0f);
		}
		default: {
			return clamp(_white.next(), -1.0f, 1.0f);
		}
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
		auto track1ParamPosition = Vec(26.5, 122.7);
		auto invert1ParamPosition = Vec(26.5, 133.7);
		auto trigger2ParamPosition = Vec(13.5, 190.0);
		auto track2ParamPosition = Vec(26.5, 285.7);
		auto invert2ParamPosition = Vec(26.5, 296.7);

		auto trigger1InputPosition = Vec(10.5, 49.0);
		auto in1InputPosition = Vec(10.5, 86.0);
		auto trigger2InputPosition = Vec(10.5, 212.0);
		auto in2InputPosition = Vec(10.5, 249.0);

		auto out1OutputPosition = Vec(10.5, 147.0);
		auto out2OutputPosition = Vec(10.5, 310.0);
		// end generated by svg_widgets.rb

		addParam(createParam<Button18>(trigger1ParamPosition, module, SampleHold::TRIGGER1_PARAM));
		addParam(createParam<Button18>(trigger2ParamPosition, module, SampleHold::TRIGGER2_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(track1ParamPosition, module, SampleHold::TRACK1_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(track2ParamPosition, module, SampleHold::TRACK2_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(invert1ParamPosition, module, SampleHold::INVERT1_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(invert2ParamPosition, module, SampleHold::INVERT2_PARAM));

		addInput(createInput<Port24>(trigger1InputPosition, module, SampleHold::TRIGGER1_INPUT));
		addInput(createInput<Port24>(in1InputPosition, module, SampleHold::IN1_INPUT));
		addInput(createInput<Port24>(trigger2InputPosition, module, SampleHold::TRIGGER2_INPUT));
		addInput(createInput<Port24>(in2InputPosition, module, SampleHold::IN2_INPUT));

		addOutput(createOutput<Port24>(out1OutputPosition, module, SampleHold::OUT1_OUTPUT));
		addOutput(createOutput<Port24>(out2OutputPosition, module, SampleHold::OUT2_OUTPUT));
	}

	struct RangeOptionMenuItem : OptionMenuItem {
		RangeOptionMenuItem(SampleHold* module, const char* label, float offset, float scale)
		: OptionMenuItem(
			label,
			[=]() { return module->_rangeOffset == offset && module->_rangeScale == scale; },
			[=]() {
				module->_rangeOffset = offset;
				module->_rangeScale = scale;
			}
		)
		{}
	};

	void appendContextMenu(Menu* menu) override {
		SampleHold* m = dynamic_cast<SampleHold*>(module);
		assert(m);
		menu->addChild(new MenuLabel());
		{
			OptionsMenuItem* p = new OptionsMenuItem("Polyphony channels from");
			p->addItem(OptionMenuItem("GATE input", [m]() { return m->_polyInputID == SampleHold::TRIGGER1_INPUT; }, [m]() { m->_polyInputID = SampleHold::TRIGGER1_INPUT; }));
			p->addItem(OptionMenuItem("IN input", [m]() { return m->_polyInputID == SampleHold::IN1_INPUT; }, [m]() { m->_polyInputID = SampleHold::IN1_INPUT; }));
			OptionsMenuItem::addToMenu(p, menu);
		}
		{
			OptionsMenuItem* mi = new OptionsMenuItem("Normal noise");
			mi->addItem(OptionMenuItem("Blue", [m]() { return m->_noiseType == SampleHold::BLUE_NOISE_TYPE; }, [m]() { m->_noiseType = SampleHold::BLUE_NOISE_TYPE; }));
			mi->addItem(OptionMenuItem("White", [m]() { return m->_noiseType == SampleHold::WHITE_NOISE_TYPE; }, [m]() { m->_noiseType = SampleHold::WHITE_NOISE_TYPE; }));
			mi->addItem(OptionMenuItem("Pink", [m]() { return m->_noiseType == SampleHold::PINK_NOISE_TYPE; }, [m]() { m->_noiseType = SampleHold::PINK_NOISE_TYPE; }));
			mi->addItem(OptionMenuItem("Red", [m]() { return m->_noiseType == SampleHold::RED_NOISE_TYPE; }, [m]() { m->_noiseType = SampleHold::RED_NOISE_TYPE; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
		{
			OptionsMenuItem* mi = new OptionsMenuItem("Normal range");
			mi->addItem(RangeOptionMenuItem(m, "+/-10V", 0.0f, 10.0f));
			mi->addItem(RangeOptionMenuItem(m, "+/-5V", 0.0f, 5.0f));
			mi->addItem(RangeOptionMenuItem(m, "+/-3V", 0.0f, 3.0f));
			mi->addItem(RangeOptionMenuItem(m, "+/-1V", 0.0f, 1.0f));
			mi->addItem(RangeOptionMenuItem(m, "0V-10V", 1.0f, 5.0f));
			mi->addItem(RangeOptionMenuItem(m, "0V-5V", 1.0f, 2.5f));
			mi->addItem(RangeOptionMenuItem(m, "0V-3V", 1.0f, 1.5f));
			mi->addItem(RangeOptionMenuItem(m, "0V-1V", 1.0f, 0.5f));
			OptionsMenuItem::addToMenu(mi, menu);
		}
	}
};

Model* modelSampleHold = bogaudio::createModel<SampleHold, SampleHoldWidget>("Bogaudio-SampleHold", "S&H", "Dual sample (or track) and hold", "Sample and hold", "Dual", "Polyphonic");
