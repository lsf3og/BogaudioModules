
#include "Mix8.hpp"

#define POLY_OFFSET "poly_channel_offset"

json_t* Mix8::dataToJson() {
	json_t* root = json_object();
	json_object_set_new(root, POLY_OFFSET, json_integer(_polyChannelOffset));
	return root;
}

void Mix8::dataFromJson(json_t* root) {
	json_t* o = json_object_get(root, POLY_OFFSET);
	if (o) {
		_polyChannelOffset = json_integer_value(o);
	}
}

void Mix8::sampleRateChange() {
	float sr = APP->engine->getSampleRate();
	for (int i = 0; i < 8; ++i) {
		_channels[i]->setSampleRate(sr);
		_panSLs[i].setParams(sr, MIXER_PAN_SLEW_MS, 2.0f);
	}
	_slewLimiter.setParams(sr, MixerChannel::levelSlewTimeMS, MixerChannel::maxDecibels - MixerChannel::minDecibels);
	_rms.setSampleRate(sr);
}

void Mix8::processAll(const ProcessArgs& args) {
	Mix8ExpanderMessage* toExp = &_dummyExpanderMessage;
	Mix8ExpanderMessage* fromExp = &_dummyExpanderMessage;
	if (expanderConnected()) {
		toExp = toExpander();
		fromExp = fromExpander();
	}

	if (!(
		inputs[IN1_INPUT].isConnected() ||
		inputs[IN2_INPUT].isConnected() ||
		inputs[IN3_INPUT].isConnected() ||
		inputs[IN4_INPUT].isConnected() ||
		inputs[IN5_INPUT].isConnected() ||
		inputs[IN6_INPUT].isConnected() ||
		inputs[IN7_INPUT].isConnected() ||
		inputs[IN8_INPUT].isConnected()
	)) {
		if (_wasActive > 0) {
			--_wasActive;
			for (int i = 0; i < 8; ++i) {
				_channels[i]->reset();
				toExp->active[i] = false;
			}
			_rmsLevel = 0.0f;
			outputs[L_OUTPUT].setVoltage(0.0f);
			outputs[R_OUTPUT].setVoltage(0.0f);
		}
		return;
	}
	_wasActive = 2;

	bool solo =
		params[MUTE1_PARAM].getValue() > 1.5f ||
		params[MUTE2_PARAM].getValue() > 1.5f ||
		params[MUTE3_PARAM].getValue() > 1.5f ||
		params[MUTE4_PARAM].getValue() > 1.5f ||
		params[MUTE5_PARAM].getValue() > 1.5f ||
		params[MUTE6_PARAM].getValue() > 1.5f ||
		params[MUTE7_PARAM].getValue() > 1.5f ||
		params[MUTE8_PARAM].getValue() > 1.5f;

	{
		float sample = 0.0f;
		if (_polyChannelOffset >= 0) {
			sample = inputs[IN1_INPUT].getPolyVoltage(_polyChannelOffset);
		} else {
			sample = inputs[IN1_INPUT].getVoltageSum();
		}
		_channels[0]->next(sample, solo);
		toExp->preFader[0] = sample;
		toExp->active[0] = inputs[IN1_INPUT].isConnected();

		for (int i = 1; i < 8; ++i) {
			float sample = 0.0f;
			bool channelActive = false;
			if (inputs[IN1_INPUT + 3 * i].isConnected()) {
				sample = inputs[IN1_INPUT + 3 * i].getVoltageSum();
				_channels[i]->next(sample, solo);
				channelActive = true;
			}
			else if (_polyChannelOffset >= 0) {
				sample = inputs[IN1_INPUT].getPolyVoltage(_polyChannelOffset + i);
				_channels[i]->next(sample, solo);
				channelActive = true;
			}
			else {
				_channels[i]->out = 0.0f;
				_channels[i]->rms = 0.0f;
			}
			toExp->preFader[i] = sample;
			toExp->active[i] = channelActive;
		}
	}

	float level = Amplifier::minDecibels;
	if (params[MIX_MUTE_PARAM].getValue() < 0.5f) {
		level = params[MIX_PARAM].getValue();
		if (inputs[MIX_CV_INPUT].isConnected()) {
			level *= clamp(inputs[MIX_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		}
		level *= MixerChannel::maxDecibels - MixerChannel::minDecibels;
		level += MixerChannel::minDecibels;
	}
	_amplifier.setLevel(_slewLimiter.next(level));

	float outs[8];
	for (int i = 0; i < 8; ++i) {
		toExp->postFader[i] = outs[i] = _channels[i]->out;
	}

	float mono = 0.0f;
	float left = 0.0f;
	float right = 0.0f;
	if (expanderConnected()) {
		mono += fromExp->returnA[0] + fromExp->returnB[0];
		left += fromExp->returnA[0] + fromExp->returnB[0];
		right += fromExp->returnA[1] + fromExp->returnB[1];
		std::copy(fromExp->postEQ, fromExp->postEQ + 8, outs);
	}

	for (int i = 0; i < 8; ++i) {
		mono += outs[i];
	}
	mono = _amplifier.next(mono);
	mono = _saturator.next(mono);
	_rmsLevel = _rms.next(mono) / 5.0f;

	if (outputs[L_OUTPUT].isConnected() && outputs[R_OUTPUT].isConnected()) {
		for (int i = 0; i < 8; ++i) {
			float pan = clamp(params[PAN1_PARAM + 3 * i].getValue(), -1.0f, 1.0f);
			if (inputs[PAN1_INPUT + 3 * i].isConnected()) {
				pan *= clamp(inputs[PAN1_INPUT + 3 * i].getVoltage() / 5.0f, -1.0f, 1.0f);
			}
			_panners[i].setPan(_panSLs[i].next(pan));
			float l, r;
			_panners[i].next(outs[i], l, r);
			left += l;
			right += r;
		}

		left = _amplifier.next(left);
		left = _saturator.next(left);
		outputs[L_OUTPUT].setVoltage(left);

		right = _amplifier.next(right);
		right = _saturator.next(right);
		outputs[R_OUTPUT].setVoltage(right);
	}
	else {
		outputs[L_OUTPUT].setVoltage(mono);
		outputs[R_OUTPUT].setVoltage(mono);
	}
}

struct Mix8Widget : ModuleWidget {
	static constexpr int hp = 27;

	Mix8Widget(Mix8* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SvgPanel *panel = new SvgPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mix8.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

		// generated by svg_widgets.rb
		auto level1ParamPosition = Vec(17.5, 32.0);
		auto mute1ParamPosition = Vec(17.2, 185.7);
		auto pan1ParamPosition = Vec(18.5, 223.0);
		auto level2ParamPosition = Vec(61.5, 32.0);
		auto mute2ParamPosition = Vec(61.2, 185.7);
		auto pan2ParamPosition = Vec(62.5, 223.0);
		auto level3ParamPosition = Vec(105.5, 32.0);
		auto mute3ParamPosition = Vec(105.2, 185.7);
		auto pan3ParamPosition = Vec(106.5, 223.0);
		auto level4ParamPosition = Vec(149.5, 32.0);
		auto mute4ParamPosition = Vec(149.2, 185.7);
		auto pan4ParamPosition = Vec(150.5, 223.0);
		auto level5ParamPosition = Vec(193.5, 32.0);
		auto mute5ParamPosition = Vec(193.2, 185.7);
		auto pan5ParamPosition = Vec(194.5, 223.0);
		auto level6ParamPosition = Vec(237.5, 32.0);
		auto mute6ParamPosition = Vec(237.2, 185.7);
		auto pan6ParamPosition = Vec(238.5, 223.0);
		auto level7ParamPosition = Vec(281.5, 32.0);
		auto mute7ParamPosition = Vec(281.2, 185.7);
		auto pan7ParamPosition = Vec(282.5, 223.0);
		auto level8ParamPosition = Vec(325.5, 32.0);
		auto mute8ParamPosition = Vec(325.2, 185.7);
		auto pan8ParamPosition = Vec(326.5, 223.0);
		auto mixParamPosition = Vec(369.5, 32.0);
		auto mixMuteParamPosition = Vec(369.2, 185.7);

		auto cv1InputPosition = Vec(14.5, 255.0);
		auto pan1InputPosition = Vec(14.5, 290.0);
		auto in1InputPosition = Vec(14.5, 325.0);
		auto cv2InputPosition = Vec(58.5, 255.0);
		auto pan2InputPosition = Vec(58.5, 290.0);
		auto in2InputPosition = Vec(58.5, 325.0);
		auto cv3InputPosition = Vec(102.5, 255.0);
		auto pan3InputPosition = Vec(102.5, 290.0);
		auto in3InputPosition = Vec(102.5, 325.0);
		auto cv4InputPosition = Vec(146.5, 255.0);
		auto pan4InputPosition = Vec(146.5, 290.0);
		auto in4InputPosition = Vec(146.5, 325.0);
		auto cv5InputPosition = Vec(190.5, 255.0);
		auto pan5InputPosition = Vec(190.5, 290.0);
		auto in5InputPosition = Vec(190.5, 325.0);
		auto cv6InputPosition = Vec(234.5, 255.0);
		auto pan6InputPosition = Vec(234.5, 290.0);
		auto in6InputPosition = Vec(234.5, 325.0);
		auto cv7InputPosition = Vec(278.5, 255.0);
		auto pan7InputPosition = Vec(278.5, 290.0);
		auto in7InputPosition = Vec(278.5, 325.0);
		auto cv8InputPosition = Vec(322.5, 255.0);
		auto pan8InputPosition = Vec(322.5, 290.0);
		auto in8InputPosition = Vec(322.5, 325.0);
		auto mixCvInputPosition = Vec(366.5, 252.0);

		auto lOutputPosition = Vec(366.5, 290.0);
		auto rOutputPosition = Vec(366.5, 325.0);
		// end generated by svg_widgets.rb

		addSlider(level1ParamPosition, module, Mix8::LEVEL1_PARAM, module ? &module->_channels[0]->rms : NULL);
		addParam(createParam<SoloMuteButton>(mute1ParamPosition, module, Mix8::MUTE1_PARAM));
		addParam(createParam<Knob16>(pan1ParamPosition, module, Mix8::PAN1_PARAM));
		addSlider(level2ParamPosition, module, Mix8::LEVEL2_PARAM, module ? &module->_channels[1]->rms : NULL);
		addParam(createParam<SoloMuteButton>(mute2ParamPosition, module, Mix8::MUTE2_PARAM));
		addParam(createParam<Knob16>(pan2ParamPosition, module, Mix8::PAN2_PARAM));
		addSlider(level3ParamPosition, module, Mix8::LEVEL3_PARAM, module ? &module->_channels[2]->rms : NULL);
		addParam(createParam<SoloMuteButton>(mute3ParamPosition, module, Mix8::MUTE3_PARAM));
		addParam(createParam<Knob16>(pan3ParamPosition, module, Mix8::PAN3_PARAM));
		addSlider(level4ParamPosition, module, Mix8::LEVEL4_PARAM, module ? &module->_channels[3]->rms : NULL);
		addParam(createParam<SoloMuteButton>(mute4ParamPosition, module, Mix8::MUTE4_PARAM));
		addParam(createParam<Knob16>(pan4ParamPosition, module, Mix8::PAN4_PARAM));
		addSlider(level5ParamPosition, module, Mix8::LEVEL5_PARAM, module ? &module->_channels[4]->rms : NULL);
		addParam(createParam<SoloMuteButton>(mute5ParamPosition, module, Mix8::MUTE5_PARAM));
		addParam(createParam<Knob16>(pan5ParamPosition, module, Mix8::PAN5_PARAM));
		addSlider(level6ParamPosition, module, Mix8::LEVEL6_PARAM, module ? &module->_channels[5]->rms : NULL);
		addParam(createParam<SoloMuteButton>(mute6ParamPosition, module, Mix8::MUTE6_PARAM));
		addParam(createParam<Knob16>(pan6ParamPosition, module, Mix8::PAN6_PARAM));
		addSlider(level7ParamPosition, module, Mix8::LEVEL7_PARAM, module ? &module->_channels[6]->rms : NULL);
		addParam(createParam<SoloMuteButton>(mute7ParamPosition, module, Mix8::MUTE7_PARAM));
		addParam(createParam<Knob16>(pan7ParamPosition, module, Mix8::PAN7_PARAM));
		addSlider(level8ParamPosition, module, Mix8::LEVEL8_PARAM, module ? &module->_channels[7]->rms : NULL);
		addParam(createParam<SoloMuteButton>(mute8ParamPosition, module, Mix8::MUTE8_PARAM));
		addParam(createParam<Knob16>(pan8ParamPosition, module, Mix8::PAN8_PARAM));
		addSlider(mixParamPosition, module, Mix8::MIX_PARAM, module ? &module->_rmsLevel : NULL);
		addParam(createParam<MuteButton>(mixMuteParamPosition, module, Mix8::MIX_MUTE_PARAM));

		addInput(createInput<Port24>(cv1InputPosition, module, Mix8::CV1_INPUT));
		addInput(createInput<Port24>(pan1InputPosition, module, Mix8::PAN1_INPUT));
		addInput(createInput<Port24>(in1InputPosition, module, Mix8::IN1_INPUT));
		addInput(createInput<Port24>(cv2InputPosition, module, Mix8::CV2_INPUT));
		addInput(createInput<Port24>(pan2InputPosition, module, Mix8::PAN2_INPUT));
		addInput(createInput<Port24>(in2InputPosition, module, Mix8::IN2_INPUT));
		addInput(createInput<Port24>(cv3InputPosition, module, Mix8::CV3_INPUT));
		addInput(createInput<Port24>(pan3InputPosition, module, Mix8::PAN3_INPUT));
		addInput(createInput<Port24>(in3InputPosition, module, Mix8::IN3_INPUT));
		addInput(createInput<Port24>(cv4InputPosition, module, Mix8::CV4_INPUT));
		addInput(createInput<Port24>(pan4InputPosition, module, Mix8::PAN4_INPUT));
		addInput(createInput<Port24>(in4InputPosition, module, Mix8::IN4_INPUT));
		addInput(createInput<Port24>(cv5InputPosition, module, Mix8::CV5_INPUT));
		addInput(createInput<Port24>(pan5InputPosition, module, Mix8::PAN5_INPUT));
		addInput(createInput<Port24>(in5InputPosition, module, Mix8::IN5_INPUT));
		addInput(createInput<Port24>(cv6InputPosition, module, Mix8::CV6_INPUT));
		addInput(createInput<Port24>(pan6InputPosition, module, Mix8::PAN6_INPUT));
		addInput(createInput<Port24>(in6InputPosition, module, Mix8::IN6_INPUT));
		addInput(createInput<Port24>(cv7InputPosition, module, Mix8::CV7_INPUT));
		addInput(createInput<Port24>(pan7InputPosition, module, Mix8::PAN7_INPUT));
		addInput(createInput<Port24>(in7InputPosition, module, Mix8::IN7_INPUT));
		addInput(createInput<Port24>(cv8InputPosition, module, Mix8::CV8_INPUT));
		addInput(createInput<Port24>(pan8InputPosition, module, Mix8::PAN8_INPUT));
		addInput(createInput<Port24>(in8InputPosition, module, Mix8::IN8_INPUT));
		addInput(createInput<Port24>(mixCvInputPosition, module, Mix8::MIX_CV_INPUT));

		addOutput(createOutput<Port24>(lOutputPosition, module, Mix8::L_OUTPUT));
		addOutput(createOutput<Port24>(rOutputPosition, module, Mix8::R_OUTPUT));
	}

	void addSlider(Vec position, Mix8* module, int id, float* rms) {
		auto slider = createParam<VUSlider151>(position, module, id);
		if (rms) {
			dynamic_cast<VUSlider*>(slider)->setVULevel(rms);
		}
		addParam(slider);
	}

	void appendContextMenu(Menu* menu) override {
		Mix8* m = dynamic_cast<Mix8*>(module);
		assert(m);
		menu->addChild(new MenuLabel());
		OptionsMenuItem* mi = new OptionsMenuItem("Input 1 poly spread");
		mi->addItem(OptionMenuItem("None", [m]() { return m->_polyChannelOffset == -1; }, [m]() { m->_polyChannelOffset = -1; }));
		mi->addItem(OptionMenuItem("Channels 1-8", [m]() { return m->_polyChannelOffset == 0; }, [m]() { m->_polyChannelOffset = 0; }));
		mi->addItem(OptionMenuItem("Channels 9-16", [m]() { return m->_polyChannelOffset == 8; }, [m]() { m->_polyChannelOffset = 8; }));
		OptionsMenuItem::addToMenu(mi, menu);
	}
};

Model* modelMix8 = bogaudio::createModel<Mix8, Mix8Widget>("Bogaudio-Mix8", "MIX8", "8-channel mixer and panner", "Mixer", "Panning");


void Mix8x::sampleRateChange() {
	float sr = APP->engine->getSampleRate();
	for (int i = 0; i < 8; ++i) {
		_channels[i]->setSampleRate(sr);
	}
	_returnASL.setParams(sr, MixerChannel::levelSlewTimeMS, MixerChannel::maxDecibels - MixerChannel::minDecibels);
	_returnBSL.setParams(sr, MixerChannel::levelSlewTimeMS, MixerChannel::maxDecibels - MixerChannel::minDecibels);
}

void Mix8x::modulate() {
	for (int i = 0; i < 8; ++i) {
		_channels[i]->modulate();
	}
}

void Mix8x::processAll(const ProcessArgs& args) {
	if (!baseConnected()) {
		outputs[SEND_A_OUTPUT].setVoltage(0.0f);
		outputs[SEND_B_OUTPUT].setVoltage(0.0f);
		return;
	}

	Mix8ExpanderMessage* from = fromBase();
	Mix8ExpanderMessage* to = toBase();
	float sendA = 0.0f;
	float sendB = 0.0f;
	bool sendAActive = outputs[SEND_A_OUTPUT].isConnected();
	bool sendBActive = outputs[SEND_B_OUTPUT].isConnected();
	for (int i = 0; i < 8; ++i) {
		if (from->active[i]) {
			_channels[i]->next(from->preFader[i], from->postFader[i], sendAActive, sendBActive);
			to->postEQ[i] = _channels[i]->postEQ;
			sendA += _channels[i]->sendA;
			sendB += _channels[i]->sendB;
		}
		else {
			to->postEQ[i] = from->preFader[i];
		}
	}
	outputs[SEND_A_OUTPUT].setVoltage(_saturatorA.next(sendA));
	outputs[SEND_B_OUTPUT].setVoltage(_saturatorA.next(sendB));

	bool lAActive = inputs[L_A_INPUT].isConnected();
	bool rAActive = inputs[R_A_INPUT].isConnected();
	if (lAActive || rAActive) {
		float levelA = clamp(params[LEVEL_A_PARAM].getValue(), 0.0f, 1.0f);
		if (inputs[LEVEL_A_INPUT].isConnected()) {
			levelA *= clamp(inputs[LEVEL_A_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		}
		levelA = 1.0f - levelA;
		levelA *= Amplifier::minDecibels;
		_returnAAmp.setLevel(_returnASL.next(levelA));
		if (lAActive) {
			to->returnA[0] = _returnAAmp.next(inputs[L_A_INPUT].getVoltage());
		}
		else {
			to->returnA[0] = 0.0f;
		}
		if (rAActive) {
			to->returnA[1] = _returnAAmp.next(inputs[R_A_INPUT].getVoltage());
		}
		else {
			to->returnA[1] = to->returnA[0];
		}
	}

	bool lBActive = inputs[L_B_INPUT].isConnected();
	bool rBActive = inputs[R_B_INPUT].isConnected();
	if (lBActive || rBActive) {
		float levelB = clamp(params[LEVEL_B_PARAM].getValue(), 0.0f, 1.0f);
		levelB = 1.0f - levelB;
		levelB *= Amplifier::minDecibels;
		_returnBAmp.setLevel(_returnBSL.next(levelB));
		if (lBActive) {
			to->returnB[0] = _returnBAmp.next(inputs[L_B_INPUT].getVoltage());
		}
		else {
			to->returnB[0] = 0.0f;
		}
		if (rBActive) {
			to->returnB[1] = _returnBAmp.next(inputs[R_B_INPUT].getVoltage());
		}
		else {
			to->returnB[1] = to->returnB[0];
		}
	}
}

struct Mix8xWidget : ModuleWidget {
	static constexpr int hp = 27;

	Mix8xWidget(Mix8x* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SvgPanel *panel = new SvgPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mix8x.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

		// generated by svg_widgets.rb
		auto low1ParamPosition = Vec(18.5, 43.0);
		auto mid1ParamPosition = Vec(18.5, 89.0);
		auto high1ParamPosition = Vec(18.5, 135.0);
		auto a1ParamPosition = Vec(18.5, 180.0);
		auto preA1ParamPosition = Vec(30.0, 208.0);
		auto b1ParamPosition = Vec(18.5, 236.0);
		auto preB1ParamPosition = Vec(30.0, 264.0);
		auto low2ParamPosition = Vec(62.5, 43.0);
		auto mid2ParamPosition = Vec(62.5, 89.0);
		auto high2ParamPosition = Vec(62.5, 135.0);
		auto a2ParamPosition = Vec(62.5, 180.0);
		auto preA2ParamPosition = Vec(74.0, 208.0);
		auto b2ParamPosition = Vec(62.5, 236.0);
		auto preB2ParamPosition = Vec(74.0, 264.0);
		auto low3ParamPosition = Vec(106.5, 43.0);
		auto mid3ParamPosition = Vec(106.5, 89.0);
		auto high3ParamPosition = Vec(106.5, 135.0);
		auto a3ParamPosition = Vec(106.5, 180.0);
		auto preA3ParamPosition = Vec(118.0, 208.0);
		auto b3ParamPosition = Vec(106.5, 236.0);
		auto preB3ParamPosition = Vec(118.0, 264.0);
		auto low4ParamPosition = Vec(150.5, 43.0);
		auto mid4ParamPosition = Vec(150.5, 89.0);
		auto high4ParamPosition = Vec(150.5, 135.0);
		auto a4ParamPosition = Vec(150.5, 180.0);
		auto preA4ParamPosition = Vec(162.0, 208.0);
		auto b4ParamPosition = Vec(150.5, 236.0);
		auto preB4ParamPosition = Vec(162.0, 264.0);
		auto low5ParamPosition = Vec(194.5, 43.0);
		auto mid5ParamPosition = Vec(194.5, 89.0);
		auto high5ParamPosition = Vec(194.5, 135.0);
		auto a5ParamPosition = Vec(194.5, 180.0);
		auto preA5ParamPosition = Vec(206.0, 208.0);
		auto b5ParamPosition = Vec(194.5, 236.0);
		auto preB5ParamPosition = Vec(206.0, 264.0);
		auto low6ParamPosition = Vec(238.5, 43.0);
		auto mid6ParamPosition = Vec(238.5, 89.0);
		auto high6ParamPosition = Vec(238.5, 135.0);
		auto a6ParamPosition = Vec(238.5, 180.0);
		auto preA6ParamPosition = Vec(250.0, 208.0);
		auto b6ParamPosition = Vec(238.5, 236.0);
		auto preB6ParamPosition = Vec(250.0, 264.0);
		auto low7ParamPosition = Vec(282.5, 43.0);
		auto mid7ParamPosition = Vec(282.5, 89.0);
		auto high7ParamPosition = Vec(282.5, 135.0);
		auto a7ParamPosition = Vec(282.5, 180.0);
		auto preA7ParamPosition = Vec(294.0, 208.0);
		auto b7ParamPosition = Vec(282.5, 236.0);
		auto preB7ParamPosition = Vec(294.0, 264.0);
		auto low8ParamPosition = Vec(326.5, 43.0);
		auto mid8ParamPosition = Vec(326.5, 89.0);
		auto high8ParamPosition = Vec(326.5, 135.0);
		auto a8ParamPosition = Vec(326.5, 180.0);
		auto preA8ParamPosition = Vec(338.0, 208.0);
		auto b8ParamPosition = Vec(326.5, 236.0);
		auto preB8ParamPosition = Vec(338.0, 264.0);
		auto levelAParamPosition = Vec(370.5, 138.0);
		auto levelBParamPosition = Vec(370.5, 328.0);

		auto a1InputPosition = Vec(14.5, 290.0);
		auto b1InputPosition = Vec(14.5, 325.0);
		auto a2InputPosition = Vec(58.5, 290.0);
		auto b2InputPosition = Vec(58.5, 325.0);
		auto a3InputPosition = Vec(102.5, 290.0);
		auto b3InputPosition = Vec(102.5, 325.0);
		auto a4InputPosition = Vec(146.5, 290.0);
		auto b4InputPosition = Vec(146.5, 325.0);
		auto a5InputPosition = Vec(190.5, 290.0);
		auto b5InputPosition = Vec(190.5, 325.0);
		auto a6InputPosition = Vec(234.5, 290.0);
		auto b6InputPosition = Vec(234.5, 325.0);
		auto a7InputPosition = Vec(278.5, 290.0);
		auto b7InputPosition = Vec(278.5, 325.0);
		auto a8InputPosition = Vec(322.5, 290.0);
		auto b8InputPosition = Vec(322.5, 325.0);
		auto lAInputPosition = Vec(366.5, 62.0);
		auto rAInputPosition = Vec(366.5, 97.0);
		auto levelAInputPosition = Vec(366.5, 170.0);
		auto lBInputPosition = Vec(366.5, 252.0);
		auto rBInputPosition = Vec(366.5, 287.0);

		auto sendAOutputPosition = Vec(366.5, 24.0);
		auto sendBOutputPosition = Vec(366.5, 214.0);
		// end generated by svg_widgets.rb

		addParam(createParam<Knob16>(low1ParamPosition, module, Mix8x::LOW1_PARAM));
		addParam(createParam<Knob16>(mid1ParamPosition, module, Mix8x::MID1_PARAM));
		addParam(createParam<Knob16>(high1ParamPosition, module, Mix8x::HIGH1_PARAM));
		addParam(createParam<Knob16>(a1ParamPosition, module, Mix8x::A1_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preA1ParamPosition, module, Mix8x::PRE_A1_PARAM));
		addParam(createParam<Knob16>(b1ParamPosition, module, Mix8x::B1_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preB1ParamPosition, module, Mix8x::PRE_B1_PARAM));
		addParam(createParam<Knob16>(low2ParamPosition, module, Mix8x::LOW2_PARAM));
		addParam(createParam<Knob16>(mid2ParamPosition, module, Mix8x::MID2_PARAM));
		addParam(createParam<Knob16>(high2ParamPosition, module, Mix8x::HIGH2_PARAM));
		addParam(createParam<Knob16>(a2ParamPosition, module, Mix8x::A2_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preA2ParamPosition, module, Mix8x::PRE_A2_PARAM));
		addParam(createParam<Knob16>(b2ParamPosition, module, Mix8x::B2_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preB2ParamPosition, module, Mix8x::PRE_B2_PARAM));
		addParam(createParam<Knob16>(low3ParamPosition, module, Mix8x::LOW3_PARAM));
		addParam(createParam<Knob16>(mid3ParamPosition, module, Mix8x::MID3_PARAM));
		addParam(createParam<Knob16>(high3ParamPosition, module, Mix8x::HIGH3_PARAM));
		addParam(createParam<Knob16>(a3ParamPosition, module, Mix8x::A3_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preA3ParamPosition, module, Mix8x::PRE_A3_PARAM));
		addParam(createParam<Knob16>(b3ParamPosition, module, Mix8x::B3_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preB3ParamPosition, module, Mix8x::PRE_B3_PARAM));
		addParam(createParam<Knob16>(low4ParamPosition, module, Mix8x::LOW4_PARAM));
		addParam(createParam<Knob16>(mid4ParamPosition, module, Mix8x::MID4_PARAM));
		addParam(createParam<Knob16>(high4ParamPosition, module, Mix8x::HIGH4_PARAM));
		addParam(createParam<Knob16>(a4ParamPosition, module, Mix8x::A4_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preA4ParamPosition, module, Mix8x::PRE_A4_PARAM));
		addParam(createParam<Knob16>(b4ParamPosition, module, Mix8x::B4_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preB4ParamPosition, module, Mix8x::PRE_B4_PARAM));
		addParam(createParam<Knob16>(low5ParamPosition, module, Mix8x::LOW5_PARAM));
		addParam(createParam<Knob16>(mid5ParamPosition, module, Mix8x::MID5_PARAM));
		addParam(createParam<Knob16>(high5ParamPosition, module, Mix8x::HIGH5_PARAM));
		addParam(createParam<Knob16>(a5ParamPosition, module, Mix8x::A5_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preA5ParamPosition, module, Mix8x::PRE_A5_PARAM));
		addParam(createParam<Knob16>(b5ParamPosition, module, Mix8x::B5_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preB5ParamPosition, module, Mix8x::PRE_B5_PARAM));
		addParam(createParam<Knob16>(low6ParamPosition, module, Mix8x::LOW6_PARAM));
		addParam(createParam<Knob16>(mid6ParamPosition, module, Mix8x::MID6_PARAM));
		addParam(createParam<Knob16>(high6ParamPosition, module, Mix8x::HIGH6_PARAM));
		addParam(createParam<Knob16>(a6ParamPosition, module, Mix8x::A6_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preA6ParamPosition, module, Mix8x::PRE_A6_PARAM));
		addParam(createParam<Knob16>(b6ParamPosition, module, Mix8x::B6_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preB6ParamPosition, module, Mix8x::PRE_B6_PARAM));
		addParam(createParam<Knob16>(low7ParamPosition, module, Mix8x::LOW7_PARAM));
		addParam(createParam<Knob16>(mid7ParamPosition, module, Mix8x::MID7_PARAM));
		addParam(createParam<Knob16>(high7ParamPosition, module, Mix8x::HIGH7_PARAM));
		addParam(createParam<Knob16>(a7ParamPosition, module, Mix8x::A7_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preA7ParamPosition, module, Mix8x::PRE_A7_PARAM));
		addParam(createParam<Knob16>(b7ParamPosition, module, Mix8x::B7_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preB7ParamPosition, module, Mix8x::PRE_B7_PARAM));
		addParam(createParam<Knob16>(low8ParamPosition, module, Mix8x::LOW8_PARAM));
		addParam(createParam<Knob16>(mid8ParamPosition, module, Mix8x::MID8_PARAM));
		addParam(createParam<Knob16>(high8ParamPosition, module, Mix8x::HIGH8_PARAM));
		addParam(createParam<Knob16>(a8ParamPosition, module, Mix8x::A8_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preA8ParamPosition, module, Mix8x::PRE_A8_PARAM));
		addParam(createParam<Knob16>(b8ParamPosition, module, Mix8x::B8_PARAM));
		addParam(createParam<IndicatorButtonGreen9>(preB8ParamPosition, module, Mix8x::PRE_B8_PARAM));
		addParam(createParam<Knob16>(levelAParamPosition, module, Mix8x::LEVEL_A_PARAM));
		addParam(createParam<Knob16>(levelBParamPosition, module, Mix8x::LEVEL_B_PARAM));

		addInput(createInput<Port24>(a1InputPosition, module, Mix8x::A1_INPUT));
		addInput(createInput<Port24>(b1InputPosition, module, Mix8x::B1_INPUT));
		addInput(createInput<Port24>(a2InputPosition, module, Mix8x::A2_INPUT));
		addInput(createInput<Port24>(b2InputPosition, module, Mix8x::B2_INPUT));
		addInput(createInput<Port24>(a3InputPosition, module, Mix8x::A3_INPUT));
		addInput(createInput<Port24>(b3InputPosition, module, Mix8x::B3_INPUT));
		addInput(createInput<Port24>(a4InputPosition, module, Mix8x::A4_INPUT));
		addInput(createInput<Port24>(b4InputPosition, module, Mix8x::B4_INPUT));
		addInput(createInput<Port24>(a5InputPosition, module, Mix8x::A5_INPUT));
		addInput(createInput<Port24>(b5InputPosition, module, Mix8x::B5_INPUT));
		addInput(createInput<Port24>(a6InputPosition, module, Mix8x::A6_INPUT));
		addInput(createInput<Port24>(b6InputPosition, module, Mix8x::B6_INPUT));
		addInput(createInput<Port24>(a7InputPosition, module, Mix8x::A7_INPUT));
		addInput(createInput<Port24>(b7InputPosition, module, Mix8x::B7_INPUT));
		addInput(createInput<Port24>(a8InputPosition, module, Mix8x::A8_INPUT));
		addInput(createInput<Port24>(b8InputPosition, module, Mix8x::B8_INPUT));
		addInput(createInput<Port24>(lAInputPosition, module, Mix8x::L_A_INPUT));
		addInput(createInput<Port24>(rAInputPosition, module, Mix8x::R_A_INPUT));
		addInput(createInput<Port24>(levelAInputPosition, module, Mix8x::LEVEL_A_INPUT));
		addInput(createInput<Port24>(lBInputPosition, module, Mix8x::L_B_INPUT));
		addInput(createInput<Port24>(rBInputPosition, module, Mix8x::R_B_INPUT));

		addOutput(createOutput<Port24>(sendAOutputPosition, module, Mix8x::SEND_A_OUTPUT));
		addOutput(createOutput<Port24>(sendBOutputPosition, module, Mix8x::SEND_B_OUTPUT));
	}
};

Model* modelMix8x = createModel<Mix8x, Mix8xWidget>("Bogaudio-Mix8x", "MIX8X", "Expander for MIX8, adds EQs and sends", "Mixer", "Expander");
