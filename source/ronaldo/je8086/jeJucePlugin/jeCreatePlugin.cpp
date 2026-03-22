#include "jePluginProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new jeJucePlugin::AudioPluginAudioProcessor();
}
