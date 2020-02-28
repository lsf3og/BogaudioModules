
#include "EQ.hpp"

bool EQ::active() {
	return outputs[OUT_OUTPUT].isConnected();
}

int EQ::channels() {
	return inputs[IN_INPUT].getChannels();
}

void EQ::addChannel(int c) {
	_engines[c] = new Engine();
}

void EQ::removeChannel(int c) {
	delete _engines[c];
	_engines[c] = NULL;
}

void EQ::modulate() {
	_lowDb = clamp(params[LOW_PARAM].getValue(), Engine::cutDb, Engine::gainDb);
	_midDb = clamp(params[MID_PARAM].getValue(), Engine::cutDb, Engine::gainDb);
	_highDb = clamp(params[HIGH_PARAM].getValue(), Engine::cutDb, Engine::gainDb);
}

void EQ::modulateChannel(int c) {
	_engines[c]->setParams(
		APP->engine->getSampleRate(),
		_lowDb,
		_midDb,
		_highDb
	);
}

void EQ::processAll(const ProcessArgs& args) {
	outputs[OUT_OUTPUT].setChannels(_channels);
}

void EQ::processChannel(const ProcessArgs& args, int c) {
	outputs[OUT_OUTPUT].setVoltage(_engines[c]->next(inputs[IN_INPUT].getVoltage(c)), c);
}

struct EQWidget : ModuleWidget {
	static constexpr int hp = 3;

	EQWidget(EQ* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SvgPanel *panel = new SvgPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EQ.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto lowParamPosition = Vec(8.0, 47.0);
		auto midParamPosition = Vec(8.0, 125.0);
		auto highParamPosition = Vec(8.0, 203.0);

		auto inInputPosition = Vec(10.5, 267.0);

		auto outOutputPosition = Vec(10.5, 305.0);
		// end generated by svg_widgets.rb

		addParam(createParam<Knob29>(lowParamPosition, module, EQ::LOW_PARAM));
		addParam(createParam<Knob29>(midParamPosition, module, EQ::MID_PARAM));
		addParam(createParam<Knob29>(highParamPosition, module, EQ::HIGH_PARAM));

		addInput(createInput<Port24>(inInputPosition, module, EQ::IN_INPUT));

		addOutput(createOutput<Port24>(outOutputPosition, module, EQ::OUT_OUTPUT));
	}
};

Model* modelEQ = createModel<EQ, EQWidget>("Bogaudio-EQ", "EQ", "Compact 3-channel equalizer", "Equalizer", "Polyphonic");