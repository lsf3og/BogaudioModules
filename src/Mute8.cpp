
#include "Mute8.hpp"
#include "mixer.hpp"

const float Mute8::maxDecibels = 0.0f;
const float Mute8::minDecibels = Amplifier::minDecibels;
const float Mute8::slewTimeMS = 5.0f;

void Mute8::onReset() {
	for (int i = 0; i < 8; ++i) {
		_triggers[i].reset();
	}
}

void Mute8::onSampleRateChange() {
	float sampleRate = engineGetSampleRate();
	for (int i = 0; i < 8; ++i) {
		_slewLimiters[i].setParams(sampleRate, slewTimeMS, maxDecibels - minDecibels);
	}
}

void Mute8::step() {
	for (int i = 0; i < 8; ++i) {
		stepChannel(i);
	}
}

void Mute8::stepChannel(int i) {
	_triggers[i].process(inputs[MUTE1_INPUT + i].value);
	if (params[MUTE1_PARAM + i].value > 0.5f || _triggers[i].isHigh()) {
		lights[MUTE1_LIGHT + i].value = 1.0f;
		_amplifiers[i].setLevel(_slewLimiters[i].next(minDecibels));
	}
	else {
		lights[MUTE1_LIGHT + i].value = 0.0f;
		_amplifiers[i].setLevel(_slewLimiters[i].next(maxDecibels));
	}
	outputs[OUTPUT1_OUTPUT + i].value = _amplifiers[i].next(inputs[INPUT1_INPUT + i].active ? inputs[INPUT1_INPUT + i].value : 5.0f);
}

struct Mute8Widget : ModuleWidget {
	static constexpr int hp = 10;

	Mute8Widget(Mute8* module) : ModuleWidget(module) {
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/Mute8.svg")));
			addChild(panel);
		}

		addChild(Widget::create<ScrewSilver>(Vec(0, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(0, 365)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto mute1ParamPosition = Vec(78.2, 40.7);
		auto mute2ParamPosition = Vec(78.2, 80.7);
		auto mute3ParamPosition = Vec(78.2, 120.7);
		auto mute4ParamPosition = Vec(78.2, 160.7);
		auto mute5ParamPosition = Vec(78.2, 200.7);
		auto mute6ParamPosition = Vec(78.2, 240.7);
		auto mute7ParamPosition = Vec(78.2, 280.7);
		auto mute8ParamPosition = Vec(78.2, 318.7);

		auto input1InputPosition = Vec(11.0, 36.0);
		auto input2InputPosition = Vec(11.0, 76.0);
		auto input3InputPosition = Vec(11.0, 116.0);
		auto input4InputPosition = Vec(11.0, 156.0);
		auto input5InputPosition = Vec(11.0, 196.0);
		auto input6InputPosition = Vec(11.0, 236.0);
		auto input7InputPosition = Vec(11.0, 276.0);
		auto input8InputPosition = Vec(11.0, 316.0);
		auto mute1InputPosition = Vec(46.0, 36.0);
		auto mute2InputPosition = Vec(46.0, 76.0);
		auto mute3InputPosition = Vec(46.0, 116.0);
		auto mute4InputPosition = Vec(46.0, 156.0);
		auto mute5InputPosition = Vec(46.0, 196.0);
		auto mute6InputPosition = Vec(46.0, 236.0);
		auto mute7InputPosition = Vec(46.0, 276.0);
		auto mute8InputPosition = Vec(46.0, 316.0);

		auto output1OutputPosition = Vec(115.0, 36.0);
		auto output2OutputPosition = Vec(115.0, 76.0);
		auto output3OutputPosition = Vec(115.0, 116.0);
		auto output4OutputPosition = Vec(115.0, 156.0);
		auto output5OutputPosition = Vec(115.0, 196.0);
		auto output6OutputPosition = Vec(115.0, 236.0);
		auto output7OutputPosition = Vec(115.0, 276.0);
		auto output8OutputPosition = Vec(115.0, 316.0);

		auto mute1LightPosition = Vec(100.5, 46.8);
		auto mute2LightPosition = Vec(100.5, 86.8);
		auto mute3LightPosition = Vec(100.5, 126.8);
		auto mute4LightPosition = Vec(100.5, 166.8);
		auto mute5LightPosition = Vec(100.5, 206.8);
		auto mute6LightPosition = Vec(100.5, 246.8);
		auto mute7LightPosition = Vec(100.5, 286.8);
		auto mute8LightPosition = Vec(100.5, 324.8);
		// end generated by svg_widgets.rb

		addParam(ParamWidget::create<MuteButton>(mute1ParamPosition, module, Mute8::MUTE1_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<MuteButton>(mute2ParamPosition, module, Mute8::MUTE2_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<MuteButton>(mute3ParamPosition, module, Mute8::MUTE3_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<MuteButton>(mute4ParamPosition, module, Mute8::MUTE4_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<MuteButton>(mute5ParamPosition, module, Mute8::MUTE5_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<MuteButton>(mute6ParamPosition, module, Mute8::MUTE6_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<MuteButton>(mute7ParamPosition, module, Mute8::MUTE7_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<MuteButton>(mute8ParamPosition, module, Mute8::MUTE8_PARAM, 0.0, 1.0, 0.0));

		addInput(Port::create<Port24>(input1InputPosition, Port::INPUT, module, Mute8::INPUT1_INPUT));
		addInput(Port::create<Port24>(input2InputPosition, Port::INPUT, module, Mute8::INPUT2_INPUT));
		addInput(Port::create<Port24>(input3InputPosition, Port::INPUT, module, Mute8::INPUT3_INPUT));
		addInput(Port::create<Port24>(input4InputPosition, Port::INPUT, module, Mute8::INPUT4_INPUT));
		addInput(Port::create<Port24>(input5InputPosition, Port::INPUT, module, Mute8::INPUT5_INPUT));
		addInput(Port::create<Port24>(input6InputPosition, Port::INPUT, module, Mute8::INPUT6_INPUT));
		addInput(Port::create<Port24>(input7InputPosition, Port::INPUT, module, Mute8::INPUT7_INPUT));
		addInput(Port::create<Port24>(input8InputPosition, Port::INPUT, module, Mute8::INPUT8_INPUT));
		addInput(Port::create<Port24>(mute1InputPosition, Port::INPUT, module, Mute8::MUTE1_INPUT));
		addInput(Port::create<Port24>(mute2InputPosition, Port::INPUT, module, Mute8::MUTE2_INPUT));
		addInput(Port::create<Port24>(mute3InputPosition, Port::INPUT, module, Mute8::MUTE3_INPUT));
		addInput(Port::create<Port24>(mute4InputPosition, Port::INPUT, module, Mute8::MUTE4_INPUT));
		addInput(Port::create<Port24>(mute5InputPosition, Port::INPUT, module, Mute8::MUTE5_INPUT));
		addInput(Port::create<Port24>(mute6InputPosition, Port::INPUT, module, Mute8::MUTE6_INPUT));
		addInput(Port::create<Port24>(mute7InputPosition, Port::INPUT, module, Mute8::MUTE7_INPUT));
		addInput(Port::create<Port24>(mute8InputPosition, Port::INPUT, module, Mute8::MUTE8_INPUT));

		addOutput(Port::create<Port24>(output1OutputPosition, Port::OUTPUT, module, Mute8::OUTPUT1_OUTPUT));
		addOutput(Port::create<Port24>(output2OutputPosition, Port::OUTPUT, module, Mute8::OUTPUT2_OUTPUT));
		addOutput(Port::create<Port24>(output3OutputPosition, Port::OUTPUT, module, Mute8::OUTPUT3_OUTPUT));
		addOutput(Port::create<Port24>(output4OutputPosition, Port::OUTPUT, module, Mute8::OUTPUT4_OUTPUT));
		addOutput(Port::create<Port24>(output5OutputPosition, Port::OUTPUT, module, Mute8::OUTPUT5_OUTPUT));
		addOutput(Port::create<Port24>(output6OutputPosition, Port::OUTPUT, module, Mute8::OUTPUT6_OUTPUT));
		addOutput(Port::create<Port24>(output7OutputPosition, Port::OUTPUT, module, Mute8::OUTPUT7_OUTPUT));
		addOutput(Port::create<Port24>(output8OutputPosition, Port::OUTPUT, module, Mute8::OUTPUT8_OUTPUT));

		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mute1LightPosition, module, Mute8::MUTE1_LIGHT));
		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mute2LightPosition, module, Mute8::MUTE2_LIGHT));
		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mute3LightPosition, module, Mute8::MUTE3_LIGHT));
		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mute4LightPosition, module, Mute8::MUTE4_LIGHT));
		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mute5LightPosition, module, Mute8::MUTE5_LIGHT));
		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mute6LightPosition, module, Mute8::MUTE6_LIGHT));
		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mute7LightPosition, module, Mute8::MUTE7_LIGHT));
		addChild(ModuleLightWidget::create<SmallLight<GreenLight>>(mute8LightPosition, module, Mute8::MUTE8_LIGHT));
	}
};

Model* modelMute8 = createModel<Mute8, Mute8Widget>("Bogaudio-Mute8", "Mute8", "eight mutes with CV", UTILITY_TAG);
