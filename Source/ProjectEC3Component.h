#pragma once
#include <JuceHeader.h>
#include "GraphComponent.h"

class ProjectEC3Component : public juce::Component,
                             public juce::AudioIODeviceCallback
{
public:
    ProjectEC3Component();
    ~ProjectEC3Component() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    // UI
    juce::Slider      degreeSlider;
    juce::Label       degreeLabel;
    juce::ComboBox    methodBox;
    juce::Label       methodLabel;
    juce::TextButton  resetButton;
    juce::TextButton  muteButton  { "Mute" };
    std::atomic<bool> muted       { false };
    juce::Slider      freqSlider;
    juce::Label       freqLabel;
    GraphComponent    graph;

    // Model
    int degree = 1;
    std::vector<double> a;
    std::vector<std::vector<double>> choose;
    GraphComponent::EvalMethod method = GraphComponent::EvalMethod::NLI;

    // Audio state (message thread only, except sampleRate)
    double frequencyHz = 441.0;
    double sampleRate  = 44100.0;

    juce::AudioDeviceManager deviceManager;

    // Double-buffered wavetable for lock-free handoff
    std::vector<float> wavetable[2];
    std::atomic<int>  readBuf    { 0 };
    std::atomic<bool> sizeChanged { false };

    // Audio thread state
    int phase = 0;

    void buildChooseTable(int maxDegree);
    void setDegree(int d);
    double evalPoly(double t) const;
    void rebuildWavetable();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectEC3Component)
};
