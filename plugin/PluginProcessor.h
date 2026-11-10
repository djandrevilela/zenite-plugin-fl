#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <memory>
#include <vector>

#include "DrumSynth.h"
#include "Zenite.h"

constexpr int kMaxSections = 10;
extern const char* const kKeyNames[12];

// Dados imutáveis partilhados com a thread de áudio.
struct View {
    zenite::Plan plan;
    bool wrap = false;  // modos Build-up / Drums repetem em loop
    zenite::Genre genre = zenite::Genre::FutureBass;
    zenite::Mode mode = zenite::Mode::Arrangement;
};

class ZeniteProcessor : public juce::AudioProcessor,
                        public juce::ChangeBroadcaster,
                        private juce::AsyncUpdater,
                        private juce::AudioProcessorValueTreeState::Listener {
public:
    ZeniteProcessor();
    ~ZeniteProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Zenite"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- API do editor ----
    juce::AudioProcessorValueTreeState apvts;
    std::shared_ptr<const View> getView() const;
    double getUiBeat() const { return uiBeat.load(); }  // < 0 = parado
    double currentBpm() const;
    void regenerate();  // lê os parâmetros e publica um plano novo

    bool writeMidiFile(const juce::File&, bool drums, bool riser) const;
    bool writeGuideFile(const juce::File&) const;
    juce::String suggestedFileName(const juce::String& what) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged(const juce::String&, float) override;
    void handleAsyncUpdate() override;

    float pf(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }
    int pi(const juce::String& id) const { return (int) std::lround(pf(id)); }
    bool pb(const juce::String& id) const { return pf(id) > 0.5f; }
    void setParam(const juce::String& id, float realValue);

    mutable juce::SpinLock dataLock;
    std::shared_ptr<const View> published, audioData;
    std::vector<std::shared_ptr<const View>> graveyard;
    std::atomic<bool> resetBars{false}, genreChanged{false};
    juce::StringArray listenIds;

    // áudio
    std::atomic<float>* pAudition = nullptr;
    std::atomic<float>* pKey = nullptr;
    std::atomic<float>* pRiserVol = nullptr;
    std::atomic<float>* pMasterVol = nullptr;
    std::atomic<float>* pBpm = nullptr;
    struct Trig { int off; int piece; int vel; double len; };
    Trig trig[512];
    int nTrig = 0;
    std::atomic<float>* pGroup[4] = {};
    zdsp::DrumKit kit;
    zdsp::RiserSynth riser;
    std::vector<double> rAmp, rCut, rPit;
    juce::MidiBuffer outMidi;
    bool seqRunning = false, lastAudition = false;
    double expectedP0 = 0.0, auditionBeat = 0.0;
    std::atomic<double> uiBeat{-1.0};
    std::atomic<double> lastHostBpm{0.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZeniteProcessor)
};
