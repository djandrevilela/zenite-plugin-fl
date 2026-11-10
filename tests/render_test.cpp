// Teste de áudio offline (opcional): arranjo completo nos 3 subgéneros, sem NaN, com som e sem clipping.
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <cstdio>

#include "PluginProcessor.h"

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    ZeniteProcessor proc;
    const double sr = 44100.0;
    const int bs = 512;
    proc.setPlayConfigDetails(0, 2, sr, bs);
    proc.prepareToPlay(sr, bs);
    juce::AudioBuffer<float> buf(2, bs);
    int fails = 0;
    if (auto* q = proc.apvts.getParameter("audition")) q->setValueNotifyingHost(1.f);
    std::printf("%-20s %8s %8s %6s\n", "subgénero/modo", "pico", "rms", "hits");
    for (int g = 0; g < 3; ++g)
        for (int mode = 0; mode < 3; ++mode) {
            proc.apvts.getParameter("genre")->setValueNotifyingHost(proc.apvts.getParameter("genre")->convertTo0to1((float) g));
            proc.apvts.getParameter("mode")->setValueNotifyingHost(proc.apvts.getParameter("mode")->convertTo0to1((float) mode));
            proc.apvts.getParameter("bars0")->setValueNotifyingHost(0.f);
            proc.regenerate();
            auto v = proc.getView();
            double peak = 0, sumSq = 0;
            long cnt = 0, notes = 0;
            bool bad = false;
            const double secs = std::min(40.0, v->plan.length / (proc.currentBpm() / 60.0));
            const int blocks = (int) (secs * sr / bs);
            for (int b = 0; b < blocks; ++b) {
                juce::MidiBuffer mb;
                buf.clear();
                proc.processBlock(buf, mb);
                for (const auto m : mb) if (m.getMessage().isNoteOn()) ++notes;
                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < bs; ++s) {
                        float x = buf.getSample(ch, s);
                        if (!std::isfinite(x)) bad = true;
                        peak = std::max(peak, (double) std::abs(x));
                        sumSq += x * x;
                        ++cnt;
                    }
            }
            double rms = std::sqrt(sumSq / (double) cnt);
            bool ok = !bad && peak > 0.05 && peak < 1.0 && rms > 0.005 && notes > 0;
            std::printf("%-12s modo %d  %8.3f %8.4f %6ld %s\n", zenite::genreName((zenite::Genre) g), mode, peak, rms, notes, ok ? "" : " <-- PROBLEMA");
            if (!ok) ++fails;
        }
    auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("zen_test.mid");
    auto gd = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("zen_test.txt");
    proc.apvts.getParameter("mode")->setValueNotifyingHost(0.f);
    proc.regenerate();
    bool w1 = proc.writeMidiFile(f, true, true), w2 = proc.writeGuideFile(gd);
    std::printf("MIDI: %s (%lld bytes) | guia: %s (%lld bytes)\n", w1 ? "ok" : "FALHOU", (long long) f.getSize(), w2 ? "ok" : "FALHOU", (long long) gd.getSize());
    if (!w1 || !w2) ++fails;
    juce::MidiFile mf;
    juce::FileInputStream is(f);
    if (!is.openedOk() || !mf.readFrom(is) || mf.getNumTracks() != 3) { std::printf("MIDI ilegível\n"); ++fails; }
    std::printf(fails == 0 ? "\nTudo OK.\n" : "\n%d problema(s).\n", fails);
    return fails == 0 ? 0 : 1;
}
