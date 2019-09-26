
#include "LFO.hpp"

void LFO::Engine::reset() {
	resetTrigger.reset();
	sampleStep = phasor._sampleRate;
}

void LFO::Engine::sampleRateChange() {
	phasor.setSampleRate(APP->engine->getSampleRate());
	sampleStep = phasor._sampleRate;
}

void LFO::reset() {
	for (int c = 0; c < _channels; ++c) {
		_engines[c]->reset();
	}
}

void LFO::sampleRateChange() {
	for (int c = 0; c < _channels; ++c) {
		_engines[c]->sampleRateChange();
	}
}

bool LFO::active() {
	return (
		outputs[SINE_OUTPUT].isConnected() ||
		outputs[TRIANGLE_OUTPUT].isConnected() ||
		outputs[RAMP_UP_OUTPUT].isConnected() ||
		outputs[RAMP_DOWN_OUTPUT].isConnected() ||
		outputs[SQUARE_OUTPUT].isConnected()
	);
}

int LFO::channels() {
	return std::max(1, inputs[PITCH_INPUT].getChannels());
}

void LFO::addEngine(int c) {
	_engines[c] = new Engine();
	_engines[c]->reset();
	_engines[c]->sampleRateChange();
}

void LFO::removeEngine(int c) {
	delete _engines[c];
	_engines[c] = NULL;
}

void LFO::modulateChannel(int c) {
	Engine& e = *_engines[c];

	setFrequency(params[FREQUENCY_PARAM], inputs[PITCH_INPUT], e.phasor, c);

	float pw = params[PW_PARAM].getValue();
	if (inputs[PW_INPUT].isConnected()) {
		pw *= clamp(inputs[PW_INPUT].getPolyVoltage(c) / 5.0f, -1.0f, 1.0f);
	}
	pw *= 1.0f - 2.0f * e.square.minPulseWidth;
	pw *= 0.5f;
	pw += 0.5f;
	e.square.setPulseWidth(pw);

	float sample = params[SAMPLE_PARAM].getValue();
	if (inputs[SAMPLE_INPUT].isConnected()) {
		sample *= clamp(inputs[SAMPLE_INPUT].getPolyVoltage(c) / 10.0f, 0.0f, 1.0f);
	}
	float maxSampleSteps = (e.phasor._sampleRate / e.phasor._frequency) / 4.0f;
	e.sampleSteps = clamp((int)(sample * maxSampleSteps), 1, (int)maxSampleSteps);

	e.offset = params[OFFSET_PARAM].getValue();
	if (inputs[OFFSET_INPUT].isConnected()) {
		e.offset *= clamp(inputs[OFFSET_INPUT].getPolyVoltage(c) / 5.0f, -1.0f, 1.0f);
	}
	e.offset *= 5.0f;

	e.scale = params[SCALE_PARAM].getValue();
	if (inputs[SCALE_INPUT].isConnected()) {
		e.scale *= clamp(inputs[SCALE_INPUT].getPolyVoltage(c) / 10.0f, 0.0f, 1.0f);
	}
}

void LFO::always(const ProcessArgs& args) {
	lights[SLOW_LIGHT].value = _slowMode = params[SLOW_PARAM].getValue() > 0.5f;
}

void LFO::processChannel(const ProcessArgs& args, int c) {
	Engine& e = *_engines[c];

	if (e.resetTrigger.next(inputs[RESET_INPUT].getPolyVoltage(c))) {
		e.phasor.resetPhase();
	}

	e.phasor.advancePhase();
	bool useSample = false;
	if (e.sampleSteps > 1) {
		++e.sampleStep;
		if (e.sampleStep >= e.sampleSteps) {
			e.sampleStep = 0;
		}
		else {
			useSample = true;
		}
	}
	updateOutput(c, e.sine, useSample, false, outputs[SINE_OUTPUT], e.sineSample, e.sineActive);
	updateOutput(c, e.triangle, useSample, false, outputs[TRIANGLE_OUTPUT], e.triangleSample, e.triangleActive);
	updateOutput(c, e.ramp, useSample, false, outputs[RAMP_UP_OUTPUT], e.rampUpSample, e.rampUpActive);
	updateOutput(c, e.ramp, useSample, true, outputs[RAMP_DOWN_OUTPUT], e.rampDownSample, e.rampDownActive);
	updateOutput(c, e.square, false, false, outputs[SQUARE_OUTPUT], e.squareSample, e.squareActive);
}

void LFO::updateOutput(int c, Phasor& wave, bool useSample, bool invert, Output& output, float& sample, bool& active) {
	if (output.isConnected()) {
		output.setChannels(_channels);
		if (useSample && active) {
			output.setVoltage(sample, c);
		}
		else {
			sample = wave.nextFromPhasor(_engines[c]->phasor) * amplitude * _engines[c]->scale;
			if (invert) {
				sample = -sample;
			}
			sample += _engines[c]->offset;
			output.setVoltage(sample, c);
		}
		active = true;
	}
	else {
		active = false;
	}
}

struct LFOWidget : ModuleWidget {
	static constexpr int hp = 10;

	LFOWidget(LFO* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SvgPanel *panel = new SvgPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LFO.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto frequencyParamPosition = Vec(41.0, 45.0);
		auto slowParamPosition = Vec(120.0, 249.0);
		auto sampleParamPosition = Vec(37.0, 150.0);
		auto pwParamPosition = Vec(102.0, 150.0);
		auto offsetParamPosition = Vec(42.0, 196.0);
		auto scaleParamPosition = Vec(107.0, 196.0);

		auto sampleInputPosition = Vec(15.0, 230.0);
		auto pwInputPosition = Vec(47.0, 230.0);
		auto offsetInputPosition = Vec(15.0, 274.0);
		auto scaleInputPosition = Vec(47.0, 274.0);
		auto pitchInputPosition = Vec(15.0, 318.0);
		auto resetInputPosition = Vec(47.0, 318.0);

		auto rampDownOutputPosition = Vec(79.0, 230.0);
		auto rampUpOutputPosition = Vec(79.0, 274.0);
		auto squareOutputPosition = Vec(111.0, 274.0);
		auto triangleOutputPosition = Vec(79.0, 318.0);
		auto sineOutputPosition = Vec(111.0, 318.0);

		auto slowLightPosition = Vec(111.0, 240.0);
		// end generated by svg_widgets.rb

		addParam(createParam<Knob68>(frequencyParamPosition, module, LFO::FREQUENCY_PARAM));
		addParam(createParam<StatefulButton9>(slowParamPosition, module, LFO::SLOW_PARAM));
		addParam(createParam<Knob26>(sampleParamPosition, module, LFO::SAMPLE_PARAM));
		addParam(createParam<Knob26>(pwParamPosition, module, LFO::PW_PARAM));
		addParam(createParam<Knob16>(offsetParamPosition, module, LFO::OFFSET_PARAM));
		addParam(createParam<Knob16>(scaleParamPosition, module, LFO::SCALE_PARAM));

		addInput(createInput<Port24>(sampleInputPosition, module, LFO::SAMPLE_INPUT));
		addInput(createInput<Port24>(pwInputPosition, module, LFO::PW_INPUT));
		addInput(createInput<Port24>(offsetInputPosition, module, LFO::OFFSET_INPUT));
		addInput(createInput<Port24>(scaleInputPosition, module, LFO::SCALE_INPUT));
		addInput(createInput<Port24>(pitchInputPosition, module, LFO::PITCH_INPUT));
		addInput(createInput<Port24>(resetInputPosition, module, LFO::RESET_INPUT));

		addOutput(createOutput<Port24>(rampUpOutputPosition, module, LFO::RAMP_UP_OUTPUT));
		addOutput(createOutput<Port24>(rampDownOutputPosition, module, LFO::RAMP_DOWN_OUTPUT));
		addOutput(createOutput<Port24>(squareOutputPosition, module, LFO::SQUARE_OUTPUT));
		addOutput(createOutput<Port24>(triangleOutputPosition, module, LFO::TRIANGLE_OUTPUT));
		addOutput(createOutput<Port24>(sineOutputPosition, module, LFO::SINE_OUTPUT));

		addChild(createLight<SmallLight<GreenLight>>(slowLightPosition, module, LFO::SLOW_LIGHT));
	}
};

Model* modelLFO = bogaudio::createModel<LFO, LFOWidget>("Bogaudio-LFO", "LFO",  "low-frequency oscillator", "LFO");
