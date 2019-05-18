
#include "Noise.hpp"

void Noise::process(const ProcessArgs& args) {
	if (outputs[BLUE_OUTPUT].active) {
		outputs[BLUE_OUTPUT].value = clamp(_blue.next() * 20.0f, -10.0f, 10.f);
	}
	if (outputs[WHITE_OUTPUT].active) {
		outputs[WHITE_OUTPUT].value = clamp(_white.next() * 10.0f, -10.0f, 10.f);
	}
	if (outputs[PINK_OUTPUT].active) {
		outputs[PINK_OUTPUT].value = clamp(_pink.next() * 15.0f, -10.0f, 10.f);
	}
	if (outputs[RED_OUTPUT].active) {
		outputs[RED_OUTPUT].value = clamp(_red.next() * 20.0f, -10.0f, 10.f);
	}
	if (outputs[GAUSS_OUTPUT].active) {
		outputs[GAUSS_OUTPUT].value = clamp(_gauss.next(), -10.0f, 10.f);
	}

	float in = 0.0;
	if (inputs[ABS_INPUT].active) {
		in = inputs[ABS_INPUT].value;
		if (in < 0.0) {
			in = -in;
		}
	}
	outputs[ABS_OUTPUT].value = in;
}

struct NoiseWidget : ModuleWidget {
	static constexpr int hp = 3;

	NoiseWidget(Noise* module) : ModuleWidget(module) {
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Noise.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto absInputPosition = Vec(10.5, 253.0);

		auto blueOutputPosition = Vec(10.5, 24.0);
		auto whiteOutputPosition = Vec(10.5, 65.0);
		auto pinkOutputPosition = Vec(10.5, 106.0);
		auto redOutputPosition = Vec(10.5, 147.0);
		auto gaussOutputPosition = Vec(10.5, 188.0);
		auto absOutputPosition = Vec(10.5, 291.0);
		// end generated by svg_widgets.rb

		addInput(createPort<Port24>(absInputPosition, PortWidget::INPUT, module, Noise::ABS_INPUT));

		addOutput(createPort<Port24>(blueOutputPosition, PortWidget::OUTPUT, module, Noise::BLUE_OUTPUT));
		addOutput(createPort<Port24>(whiteOutputPosition, PortWidget::OUTPUT, module, Noise::WHITE_OUTPUT));
		addOutput(createPort<Port24>(pinkOutputPosition, PortWidget::OUTPUT, module, Noise::PINK_OUTPUT));
		addOutput(createPort<Port24>(redOutputPosition, PortWidget::OUTPUT, module, Noise::RED_OUTPUT));
		addOutput(createPort<Port24>(gaussOutputPosition, PortWidget::OUTPUT, module, Noise::GAUSS_OUTPUT));
		addOutput(createPort<Port24>(absOutputPosition, PortWidget::OUTPUT, module, Noise::ABS_OUTPUT));
	}
};

Model* modelNoise = bogaudio::createModel<Noise, NoiseWidget>("Bogaudio-Noise", "Noise",  "noise source", NOISE_TAG, RANDOM_TAG);
