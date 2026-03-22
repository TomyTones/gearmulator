#pragma once

#include "jeEditor.h"
#include "jePluginEditorState.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace jeMorphPlugin
{
	class MorphProcessor;
	class MorphContainer;

	class MorphEditorState : public jeJucePlugin::PluginEditorState
	{
	public:
		explicit MorphEditorState(MorphProcessor& _processor);
		~MorphEditorState() override;

		juce::Component* getUiRoot() const override;
		int getWidth() const override;
		int getHeight() const override;
		bool resizeEditor(int _width, int _height) const override;

		static constexpr int kSliderWidth = 80;

	private:
		void buildSecondEditorAndContainer(MorphProcessor& _processor);

		std::unique_ptr<jeJucePlugin::Editor> m_editorB;
		std::unique_ptr<MorphContainer> m_container;
	};
}
