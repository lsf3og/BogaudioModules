
#include "Sums.hpp"

void Sums::process(const ProcessArgs& args) {
	float a = inputs[A_INPUT].value;
	float b = inputs[B_INPUT].value;
	if (_disableOutputLimit) {
		outputs[SUM_OUTPUT].value = a + b,
		outputs[DIFFERENCE_OUTPUT].value = a - b;
		outputs[MAX_OUTPUT].value = std::max(a, b);
		outputs[MIN_OUTPUT].value = std::min(a, b);

		if (inputs[NEGATE_INPUT].active) {
			outputs[NEGATE_OUTPUT].value = -inputs[NEGATE_INPUT].value;
		}
		else {
			outputs[NEGATE_OUTPUT].value = 0.0f;
		}
	}
	else {
		outputs[SUM_OUTPUT].value = clamp(a + b, -12.0f, 12.0f);
		outputs[DIFFERENCE_OUTPUT].value = clamp(a - b, -12.0f, 12.0f);
		outputs[MAX_OUTPUT].value = clamp(std::max(a, b), -12.0f, 12.0f);
		outputs[MIN_OUTPUT].value = clamp(std::min(a, b), -12.0f, 12.0f);

		if (inputs[NEGATE_INPUT].active) {
			outputs[NEGATE_OUTPUT].value = clamp(-inputs[NEGATE_INPUT].value, -12.0f, 12.0f);
		}
		else {
			outputs[NEGATE_OUTPUT].value = 0.0f;
		}
	}
}

struct SumsWidget : DisableOutputLimitModuleWidget {
	static constexpr int hp = 3;

	SumsWidget(Sums* module) : DisableOutputLimitModuleWidget(module) {
		box.size = Vec(RACK_GRID_WIDTH * hp, RACK_GRID_HEIGHT);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(asset::plugin(pluginInstance, "res/Sums.svg")));
			addChild(panel);
		}

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		// generated by svg_widgets.rb
		auto aInputPosition = Vec(10.5, 23.0);
		auto bInputPosition = Vec(10.5, 53.0);
		auto negateInputPosition = Vec(10.5, 262.0);

		auto sumOutputPosition = Vec(10.5, 86.0);
		auto differenceOutputPosition = Vec(10.5, 126.0);
		auto maxOutputPosition = Vec(10.5, 166.0);
		auto minOutputPosition = Vec(10.5, 206.0);
		auto negateOutputPosition = Vec(10.5, 295.0);
		// end generated by svg_widgets.rb

		addInput(createPort<Port24>(aInputPosition, PortWidget::INPUT, module, Sums::A_INPUT));
		addInput(createPort<Port24>(bInputPosition, PortWidget::INPUT, module, Sums::B_INPUT));
		addInput(createPort<Port24>(negateInputPosition, PortWidget::INPUT, module, Sums::NEGATE_INPUT));

		addOutput(createPort<Port24>(sumOutputPosition, PortWidget::OUTPUT, module, Sums::SUM_OUTPUT));
		addOutput(createPort<Port24>(differenceOutputPosition, PortWidget::OUTPUT, module, Sums::DIFFERENCE_OUTPUT));
		addOutput(createPort<Port24>(maxOutputPosition, PortWidget::OUTPUT, module, Sums::MAX_OUTPUT));
		addOutput(createPort<Port24>(minOutputPosition, PortWidget::OUTPUT, module, Sums::MIN_OUTPUT));
		addOutput(createPort<Port24>(negateOutputPosition, PortWidget::OUTPUT, module, Sums::NEGATE_OUTPUT));
	}
};

Model* modelSums = bogaudio::createModel<Sums, SumsWidget>("Bogaudio-Sums", "Sums",  "arithmetic logic", LOGIC_TAG);
