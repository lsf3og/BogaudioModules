
#include "Lmtr.hpp"

void Lmtr::onReset() {
	_modulationStep = modulationSteps;
}

void Lmtr::onSampleRateChange() {
	float sampleRate = APP->engine->getSampleRate();
	_detector.setSampleRate(sampleRate);
	_attackSL.setParams(sampleRate, 150.0f);
	_releaseSL.setParams(sampleRate, 600.0f);
	_modulationStep = modulationSteps;
}

void Lmtr::process(const ProcessArgs& args) {
	if (!(outputs[LEFT_OUTPUT].isConnected() || outputs[RIGHT_OUTPUT].isConnected())) {
		return;
	}

	++_modulationStep;
	if (_modulationStep >= modulationSteps) {
		_modulationStep = 0;

		_thresholdDb = params[THRESHOLD_PARAM].getValue();
		if (inputs[THRESHOLD_INPUT].isConnected()) {
			_thresholdDb *= clamp(inputs[THRESHOLD_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		}
		_thresholdDb *= 30.0f;
		_thresholdDb -= 24.0f;

		float outGain = params[OUTPUT_GAIN_PARAM].getValue();
		if (inputs[OUTPUT_GAIN_INPUT].isConnected()) {
			outGain = clamp(outGain + inputs[OUTPUT_GAIN_INPUT].getVoltage() / 5.0f, 0.0f, 1.0f);
		}
		outGain *= 24.0f;
		if (_outGain != outGain) {
			_outGain = outGain;
			_outLevel = decibelsToAmplitude(_outGain);
		}

		_softKnee = params[KNEE_PARAM].getValue() > 0.97f;
	}

	float leftInput = inputs[LEFT_INPUT].getVoltage();
	float rightInput = inputs[RIGHT_INPUT].getVoltage();
	float env = _detector.next(leftInput + rightInput);
	if (env > _lastEnv) {
		env = _attackSL.next(env, _lastEnv);
	}
	else {
		env = _releaseSL.next(env, _lastEnv);
	}
	_lastEnv = env;

	float detectorDb = amplitudeToDecibels(env / 5.0f);
	float compressionDb = _compressor.compressionDb(detectorDb, _thresholdDb, Compressor::maxEffectiveRatio, _softKnee);
	_amplifier.setLevel(-compressionDb);
	if (outputs[LEFT_OUTPUT].isConnected()) {
		outputs[LEFT_OUTPUT].setVoltage(_saturator.next(_amplifier.next(leftInput) * _outLevel));
	}
	if (outputs[RIGHT_OUTPUT].isConnected()) {
		outputs[RIGHT_OUTPUT].setVoltage(_saturator.next(_amplifier.next(rightInput) * _outLevel));
	}
}

struct LmtrWidget : ModuleWidget {
	static constexpr int hp = 6;

	LmtrWidget(Lmtr* module) {
		setModule(module);
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SvgPanel *panel = new SvgPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Lmtr.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto thresholdParamPosition = Vec(26.0, 52.0);
		auto outputGainParamPosition = Vec(26.0, 134.0);
		auto kneeParamPosition = Vec(39.5, 199.5);

		auto leftInputPosition = Vec(16.0, 244.0);
		auto rightInputPosition = Vec(50.0, 244.0);
		auto thresholdInputPosition = Vec(16.0, 280.0);
		auto outputGainInputPosition = Vec(50.0, 280.0);

		auto leftOutputPosition = Vec(16.0, 320.0);
		auto rightOutputPosition = Vec(50.0, 320.0);
		// end generated by svg_widgets.rb

		addParam(createParam<Knob38>(thresholdParamPosition, module, Lmtr::THRESHOLD_PARAM));
		addParam(createParam<Knob38>(outputGainParamPosition, module, Lmtr::OUTPUT_GAIN_PARAM));
		addParam(createParam<SliderSwitch2State14>(kneeParamPosition, module, Lmtr::KNEE_PARAM));

		addInput(createInput<Port24>(leftInputPosition, module, Lmtr::LEFT_INPUT));
		addInput(createInput<Port24>(rightInputPosition, module, Lmtr::RIGHT_INPUT));
		addInput(createInput<Port24>(thresholdInputPosition, module, Lmtr::THRESHOLD_INPUT));
		addInput(createInput<Port24>(outputGainInputPosition, module, Lmtr::OUTPUT_GAIN_INPUT));

		addOutput(createOutput<Port24>(leftOutputPosition, module, Lmtr::LEFT_OUTPUT));
		addOutput(createOutput<Port24>(rightOutputPosition, module, Lmtr::RIGHT_OUTPUT));
	}
};

Model* modelLmtr = bogaudio::createModel<Lmtr, LmtrWidget>("Bogaudio-Lmtr", "LMTR", "limiter", "Dynamics");
