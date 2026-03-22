#include "jeMorphProcessor.h"
#include "jeMorphEditorState.h"

// ReSharper disable once CppUnusedIncludeDirective
#include "BinaryData.h"
#include "jeLib/device.h"
#include "synthLib/deviceException.h"

namespace jeMorphPlugin
{
	jucePluginEditorLib::PluginEditorState* MorphProcessor::createEditorState()
	{
		return new MorphEditorState(*this);
	}

	MorphProcessor::MorphProcessor()
	{
		// Base class constructor has already loaded ROMs and initialised engine A.
		// Create engine B from the same ROM so both start with the same firmware.
		const auto& rom = getSelectedRom();
		if (rom.isValid())
		{
			synthLib::DeviceCreateParams params;
			params.romData = rom.getData();
			params.romName = rom.getName();
			params.homePath = getDataFolder();

			auto* dev = new jeLib::Device(params);
			if (dev->isValid())
			{
				m_deviceB.reset(dev);
				m_pluginB = std::make_unique<synthLib::Plugin>(
					m_deviceB.get(),
					[](synthLib::Device*) -> synthLib::Device* { return nullptr; }
				);
			}
			else
			{
				delete dev;
			}
		}

		// Expose morph as an automatable JUCE parameter so Logic can record it.
		m_morphParam = new juce::AudioParameterFloat(
			juce::ParameterID{"MorphAB", 1},
			"Morph A\u2194B",
			juce::NormalisableRange<float>(0.0f, 1.0f),
			0.0f
		);
		addParameter(m_morphParam);
	}

	MorphProcessor::~MorphProcessor()
	{
		// Destroy the editor before tearing down engine B so no UI callbacks fire
		// into a dangling plugin pointer. The base class destructor will call
		// destroyEditorState() again; that second call is a safe no-op.
		destroyEditorState();
		m_pluginB.reset();
		m_deviceB.reset();
	}

	void MorphProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock)
	{
		// Let the base class configure engine A.
		jucePluginEditorLib::Processor::prepareToPlay(sampleRate, samplesPerBlock);

		// Mirror the same settings to engine B.
		if (m_pluginB)
		{
			m_pluginB->setHostSamplerate(static_cast<float>(sampleRate), 0.0f);
			m_pluginB->setBlockSize(samplesPerBlock);
		}
	}

	void MorphProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
	{
		if (!m_pluginB || !m_morphParam)
		{
			// Engine B not ready — fall back to single-engine mode.
			jucePluginEditorLib::Processor::processBlock(buffer, midiMessages);
			return;
		}

		// Save incoming MIDI so we can replay it into engine B after the base
		// class clears midiMessages.
		const juce::MidiBuffer midiCopy = midiMessages;

		// ── Engine A ─────────────────────────────────────────────────────────
		// The base class handles MIDI routing, BPM sync, output gain, and fills
		// 'buffer' with engine A's audio.
		jucePluginEditorLib::Processor::processBlock(buffer, midiMessages);

		// ── Engine B — MIDI ───────────────────────────────────────────────────
		// Forward note / CC events. Skip SysEx: it carries patch data meant for
		// engine A's controller and should not be sent to engine B.
		for (const auto meta : midiCopy)
		{
			const auto msg = meta.getMessage();
			if (msg.isSysEx() || msg.getRawDataSize() > 3)
				continue;

			synthLib::SMidiEvent ev(synthLib::MidiEventSource::Host);
			ev.a = msg.getRawData()[0];
			ev.b = msg.getRawDataSize() > 1 ? msg.getRawData()[1] : 0;
			ev.c = msg.getRawDataSize() > 2 ? msg.getRawData()[2] : 0;
			ev.offset = std::max(0, meta.samplePosition);
			m_pluginB->addMidiEvent(ev);
		}

		// ── Engine B — Audio ──────────────────────────────────────────────────
		const int numSamples  = buffer.getNumSamples();
		const int numChannels = getTotalNumOutputChannels();

		juce::AudioBuffer<float> bufferB(numChannels, numSamples);
		bufferB.clear();

		synthLib::TAudioInputs  inputsB{};
		synthLib::TAudioOutputs outputsB{};

		const int maxCh = std::min(numChannels, static_cast<int>(outputsB.size()));
		for (int ch = 0; ch < maxCh; ++ch)
			outputsB[ch] = bufferB.getWritePointer(ch);

		m_pluginB->process(inputsB, outputsB, static_cast<size_t>(numSamples), 0.0f, 0.0f, true);

		// ── Crossfade ─────────────────────────────────────────────────────────
		// output = A * (1 - morph)  +  B * morph
		const float morph = m_morphParam->get();
		for (int ch = 0; ch < numChannels; ++ch)
		{
			buffer.applyGain(ch, 0, numSamples, 1.0f - morph);
			buffer.addFrom(ch, 0, bufferB, ch, 0, numSamples, morph);
		}
	}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new jeMorphPlugin::MorphProcessor();
}
