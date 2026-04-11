#include "ProjectEC3Component.h"

ProjectEC3Component::ProjectEC3Component()
{
    buildChooseTable(20);

    // --- Degree slider ---
    degreeLabel.setText("Degree", juce::dontSendNotification);
    degreeLabel.setJustificationType(juce::Justification::centredLeft);

    degreeSlider.setRange(1, 20, 1);
    degreeSlider.setValue(1, juce::dontSendNotification);
    degreeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    degreeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);

    degreeSlider.onValueChange = [this]()
    {
        setDegree(static_cast<int>(degreeSlider.getValue()));
    };

    // --- Method dropdown ---
    methodLabel.setText("Method", juce::dontSendNotification);
    methodLabel.setJustificationType(juce::Justification::centredLeft);
    methodBox.addItem("NLI (De Casteljau)", 1);
    methodBox.addItem("BB-form (Bernstein Sum)", 2);
    methodBox.setSelectedId(1);

    methodBox.onChange = [this]()
    {
        method = (methodBox.getSelectedId() == 1) ? GraphComponent::EvalMethod::NLI
                                                  : GraphComponent::EvalMethod::BB;
        graph.setMethod(method);
        rebuildWavetable();
    };

    // --- Reset button ---
    resetButton.setButtonText("Reset");
    resetButton.onClick = [this]()
    {
        std::fill(a.begin(), a.end(), 1.0);
        graph.repaint();
        rebuildWavetable();
    };

    muteButton.setClickingTogglesState(true);
    muteButton.onClick = [this]()
    {
        muted = muteButton.getToggleState();
    };

    // --- Frequency slider ---
    freqLabel.setText("Frequency (Hz)", juce::dontSendNotification);
    freqLabel.setJustificationType(juce::Justification::centredLeft);

    freqSlider.setRange(55.0, 2000.0, 1.0);
    freqSlider.setValue(441.0, juce::dontSendNotification);
    freqSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    freqSlider.setNumDecimalPlacesToDisplay(0);

    freqSlider.onValueChange = [this]()
    {
        frequencyHz = freqSlider.getValue();
        rebuildWavetable();
    };

    addAndMakeVisible(degreeLabel);
    addAndMakeVisible(degreeSlider);
    addAndMakeVisible(methodLabel);
    addAndMakeVisible(methodBox);
    addAndMakeVisible(resetButton);
    addAndMakeVisible(muteButton);
    addAndMakeVisible(freqLabel);
    addAndMakeVisible(freqSlider);
    addAndMakeVisible(graph);

    // Hook graph drag → wavetable rebuild
    graph.onCoeffChanged = [this]() { rebuildWavetable(); };

    setDegree(1);

    // Open default audio output (0 inputs, 2 outputs)
    deviceManager.initialiseWithDefaultDevices(0, 2);
    deviceManager.addAudioCallback(this);

    setSize(1280, 720);
}

ProjectEC3Component::~ProjectEC3Component()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

// ─────────────────────────────────────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────────────────────────────────────

void ProjectEC3Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void ProjectEC3Component::resized()
{
    constexpr int pad     = 10;
    constexpr int barH    = 40;
    constexpr int gapX    = 10;
    constexpr int gapY    = 10;

    auto area = getLocalBounds().reduced(pad);
    auto top  = area.removeFromTop(barH);

    degreeLabel.setBounds(top.removeFromLeft(70));
    degreeSlider.setBounds(top.removeFromLeft(250));
    top.removeFromLeft(gapX);

    methodLabel.setBounds(top.removeFromLeft(60));
    methodBox.setBounds(top.removeFromLeft(190));
    top.removeFromLeft(gapX);

    resetButton.setBounds(top.removeFromLeft(80));
    top.removeFromLeft(gapX);
    muteButton.setBounds(top.removeFromLeft(70));
    top.removeFromLeft(gapX);

    freqLabel.setBounds(top.removeFromLeft(110));
    freqSlider.setBounds(top.removeFromLeft(220));

    area.removeFromTop(gapY);
    graph.setBounds(area);
}

// ─────────────────────────────────────────────────────────────────────────────
// Audio callbacks
// ─────────────────────────────────────────────────────────────────────────────

void ProjectEC3Component::audioDeviceIOCallbackWithContext(
    const float* const*, int,
    float* const* outputChannelData, int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    if (muted)
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch])
                juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        return;
    }

    const int  buf       = readBuf.load(std::memory_order_acquire);
    const auto& table    = wavetable[buf];
    const int  tableSize = static_cast<int>(table.size());

    if (tableSize == 0)
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch])
                juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        return;
    }

    // If the cycle length changed, keep phase in range
    if (sizeChanged.exchange(false, std::memory_order_acq_rel))
        phase = phase % tableSize;

    for (int s = 0; s < numSamples; ++s)
    {
        const float sample = table[static_cast<size_t>(phase)];
        phase = (phase + 1) % tableSize;

        for (int ch = 0; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch])
                outputChannelData[ch][s] = sample;
    }
}

void ProjectEC3Component::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    rebuildWavetable();
}

void ProjectEC3Component::audioDeviceStopped()
{
    phase = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Model helpers
// ─────────────────────────────────────────────────────────────────────────────

void ProjectEC3Component::buildChooseTable(int maxDegree)
{
    choose.assign(static_cast<size_t>(maxDegree) + 1,
                  std::vector<double>(static_cast<size_t>(maxDegree) + 1, 0.0));

    for (int n = 0; n <= maxDegree; ++n)
    {
        choose[static_cast<size_t>(n)][0] = 1.0;
        choose[static_cast<size_t>(n)][static_cast<size_t>(n)] = 1.0;

        for (int k = 1; k < n; ++k)
            choose[static_cast<size_t>(n)][static_cast<size_t>(k)] =
                choose[static_cast<size_t>(n - 1)][static_cast<size_t>(k - 1)] +
                choose[static_cast<size_t>(n - 1)][static_cast<size_t>(k)];
    }
}

void ProjectEC3Component::setDegree(int d)
{
    degree = juce::jlimit(1, 20, d);
    a.assign(static_cast<size_t>(degree) + 1, 1.0);
    graph.setModel(degree, &a, method, &choose);
    rebuildWavetable();
}

double ProjectEC3Component::evalPoly(double t) const
{
    if (a.empty()) return 0.0;
    if (degree <= 0) return a[0];
    if (t <= 0.0)   return a[0];
    if (t >= 1.0)   return a[static_cast<size_t>(degree)];

    if (method == GraphComponent::EvalMethod::NLI)
    {
        std::vector<double> tmp = a;
        for (int r = 1; r <= degree; ++r)
            for (int i = 0; i <= degree - r; ++i)
                tmp[static_cast<size_t>(i)] =
                    (1.0 - t) * tmp[static_cast<size_t>(i)] +
                    t          * tmp[static_cast<size_t>(i) + 1];
        return tmp[0];
    }

    // BB-form
    const double u = 1.0 - t;
    double sum = 0.0;
    for (int i = 0; i <= degree; ++i)
    {
        const double c     = choose[static_cast<size_t>(degree)][static_cast<size_t>(i)];
        const double basis = c * std::pow(u, degree - i) * std::pow(t, i);
        sum += a[static_cast<size_t>(i)] * basis;
    }
    return sum;
}

void ProjectEC3Component::rebuildWavetable()
{
    const int N = juce::jmax(1, static_cast<int>(std::round(sampleRate / frequencyHz)));

    const int writeBuf = 1 - readBuf.load(std::memory_order_relaxed);
    auto& buf = wavetable[writeBuf];
    buf.resize(static_cast<size_t>(N));

    double maxAbs = 0.0;
    for (int i = 0; i < N; ++i)
    {
        const double tCycle = (2.0 * i) / N;
        const double val = (tCycle < 1.0) ? evalPoly(tCycle)
                                           : -evalPoly(2.0 - tCycle);
        buf[static_cast<size_t>(i)] = static_cast<float>(val);
        maxAbs = std::max(maxAbs, std::abs(val));
    }

    if (maxAbs > 0.0)
    {
        const float scale = static_cast<float>(0.85 / maxAbs);
        for (auto& s : buf) s *= scale;
    }

    sizeChanged.store(true,    std::memory_order_release);
    readBuf.store   (writeBuf, std::memory_order_release);
}
