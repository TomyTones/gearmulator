#include "jeMorphEditorState.h"

#include "jeMorphProcessor.h"

#include "jeEditor.h"
#include "jePluginProcessor.h"

#include "juceRmlUi/juceRmlComponent.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace jeMorphPlugin
{

// ─────────────────────────────────────────────────────────────────────────────
// MorphContainer — lays out [EditorA] [vertical morph slider] [EditorB]
// ─────────────────────────────────────────────────────────────────────────────
class MorphContainer : public juce::Component
{
public:
	MorphContainer(juce::Component* _editorA,
	               juce::Component* _editorB,
	               juce::AudioParameterFloat* _morphParam)
		: m_editorA(_editorA)
		, m_editorB(_editorB)
		, m_slider(juce::Slider::LinearVertical, juce::Slider::NoTextBox)
		, m_attachment(*_morphParam, m_slider, nullptr)
	{
		// Invert so dragging up → morph toward B (more intuitive)
		m_slider.setRange(0.0, 1.0);
		m_slider.setDoubleClickReturnValue(true, 0.0);
		m_slider.setPopupDisplayEnabled(true, false, nullptr);
		m_slider.setTextValueSuffix("");
		m_slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffccaaff));
		m_slider.setColour(juce::Slider::trackColourId,  juce::Colour(0xff6644aa));
		m_slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff222222));

		m_labelA.setFont(juce::Font(22.0f, juce::Font::bold));
		m_labelB.setFont(juce::Font(22.0f, juce::Font::bold));
		m_labelA.setJustificationType(juce::Justification::centred);
		m_labelB.setJustificationType(juce::Justification::centred);
		m_labelA.setColour(juce::Label::textColourId, juce::Colours::white);
		m_labelB.setColour(juce::Label::textColourId, juce::Colours::white);
		m_labelA.setText("A", juce::dontSendNotification);
		m_labelB.setText("B", juce::dontSendNotification);

		m_title.setFont(juce::Font(14.0f));
		m_title.setJustificationType(juce::Justification::centred);
		m_title.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
		m_title.setText("MORPH", juce::dontSendNotification);

		addAndMakeVisible(m_editorA);
		addAndMakeVisible(m_editorB);
		addAndMakeVisible(m_slider);
		addAndMakeVisible(m_labelA);
		addAndMakeVisible(m_labelB);
		addAndMakeVisible(m_title);
	}

	void paint(juce::Graphics& g) override
	{
		// Dark strip behind the morph controls
		const int editorW = (getWidth() - MorphEditorState::kSliderWidth) / 2;
		g.setColour(juce::Colour(0xff111111));
		g.fillRect(editorW, 0, MorphEditorState::kSliderWidth, getHeight());
	}

	void resized() override
	{
		const int w = getWidth();
		const int h = getHeight();
		const int sw = MorphEditorState::kSliderWidth;
		const int editorW = (w - sw) / 2;
		const int labelH  = 36;

		m_editorA->setBounds(0, 0, editorW, h);
		m_editorB->setBounds(editorW + sw, 0, editorW, h);

		const int sliderX = editorW;
		m_labelB.setBounds(sliderX, 0, sw, labelH);
		m_slider.setBounds(sliderX, labelH, sw, h - labelH * 3);
		m_title.setBounds(sliderX, h - labelH * 2, sw, labelH);
		m_labelA.setBounds(sliderX, h - labelH, sw, labelH);
	}

private:
	juce::Component* m_editorA;
	juce::Component* m_editorB;

	juce::Slider m_slider;
	juce::SliderParameterAttachment m_attachment;

	juce::Label m_labelA;
	juce::Label m_labelB;
	juce::Label m_title;
};


// ─────────────────────────────────────────────────────────────────────────────
// MorphEditorState
// ─────────────────────────────────────────────────────────────────────────────
MorphEditorState::MorphEditorState(MorphProcessor& _processor)
	: jeJucePlugin::PluginEditorState(_processor)   // creates editor A + loads skin
{
	buildSecondEditorAndContainer(_processor);
}

MorphEditorState::~MorphEditorState()
{
	// m_container is destroyed before m_editorB (declaration order → reverse destroy)
	// so MorphContainer's raw pointers are never dangling during teardown.
}

void MorphEditorState::buildSecondEditorAndContainer(MorphProcessor& _processor)
{
	auto* editorA = getEditor();
	if (!editorA)
		return;

	auto* morphParam = _processor.getMorphParam();
	if (!morphParam)
		return;

	const auto& skin = getCurrentSkin();
	if (!skin.isValid())
		return;

	// Create a second independent editor view bound to the same processor.
	// Both views reflect the same controller state — this is intentional for
	// the Phase-1 prototype where engine B shares engine A's parameter state.
	m_editorB = std::make_unique<jeJucePlugin::Editor>(_processor, skin);
	m_editorB->create();

	// Build the container: [EditorA RmlComponent] [slider] [EditorB RmlComponent]
	m_container = std::make_unique<MorphContainer>(
		editorA->getRmlComponent(),
		m_editorB->getRmlComponent(),
		morphParam
	);
}

juce::Component* MorphEditorState::getUiRoot() const
{
	if (m_container)
		return m_container.get();
	return jeJucePlugin::PluginEditorState::getUiRoot();
}

int MorphEditorState::getWidth() const
{
	return jeJucePlugin::PluginEditorState::getWidth() * 2 + kSliderWidth;
}

int MorphEditorState::getHeight() const
{
	return jeJucePlugin::PluginEditorState::getHeight();
}

bool MorphEditorState::resizeEditor(const int _width, const int _height) const
{
	if (!m_container || !m_editorB)
		return jeJucePlugin::PluginEditorState::resizeEditor(_width, _height);

	m_container->setSize(_width, _height);

	const int editorW = (_width - kSliderWidth) / 2;

	// Resize each RmlUi context to its panel's pixel size
	if (auto* editorA = getEditor())
		editorA->setSize(editorW, _height);
	m_editorB->setSize(editorW, _height);

	return true;
}

} // namespace jeMorphPlugin
