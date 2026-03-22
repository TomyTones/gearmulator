#pragma once

#include "jePluginProcessor.h"

#include "synthLib/plugin.h"

namespace jeMorphPlugin
{
	class MorphProcessor : public jeJucePlugin::AudioPluginAudioProcessor
	{
	public:
		MorphProcessor();
		~MorphProcessor() override;

		jucePluginEditorLib::PluginEditorState* createEditorState() override;

		void prepareToPlay(double sampleRate, int samplesPerBlock) override;
		void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

		juce::AudioParameterFloat* getMorphParam() const { return m_morphParam; }

	private:
		std::unique_ptr<synthLib::Device> m_deviceB;
		std::unique_ptr<synthLib::Plugin> m_pluginB;
		juce::AudioParameterFloat* m_morphParam = nullptr;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphProcessor)
	};
}
