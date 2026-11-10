#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>

#include "PluginEditor.h"
#include "Util.h"

using namespace zenite;

const char* const kKeyNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

namespace {
juce::String barsId(int i) { return "bars" + juce::String(i); }
}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout ZeniteProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto choice = [&](const String& id, const String& name, StringArray items, int def) {
        l.add(std::make_unique<AudioParameterChoice>(ParameterID{id, 1}, name, items, def));
    };
    auto flt = [&](const String& id, const String& name, float lo, float hi, float def) {
        l.add(std::make_unique<AudioParameterFloat>(ParameterID{id, 1}, name, NormalisableRange<float>(lo, hi), def));
    };
    auto boo = [&](const String& id, const String& name, bool def) { l.add(std::make_unique<AudioParameterBool>(ParameterID{id, 1}, name, def)); };
    auto intp = [&](const String& id, const String& name, int lo, int hi, int def) {
        l.add(std::make_unique<AudioParameterInt>(ParameterID{id, 1}, name, lo, hi, def));
    };
    StringArray keys;
    for (auto* k : kKeyNames) keys.add(k);

    choice("genre", TR("Subgénero"), StringArray{"Future Bass", "Progressive House", "Big Room"}, 0);
    choice("template", TR("Modelo de arranjo"), StringArray{TR("Padrão"), "Compacto", "Extendido"}, 0);
    choice("mode", "Modo", StringArray{"Arranjo completo", "Build-up", "Drums (loop)"}, 0);
    intp("var", TR("Variação dos drums"), 1, 4, 1);
    choice("fills", "Fills", StringArray{"Sem fills", "Poucos", "Normais", "Muitos"}, 2);
    choice("key", TR("Tom (riser termina na tónica)"), keys, 9);
    choice("buildBars", TR("Compassos do build-up"), StringArray{"4", "8", "16"}, 1);
    choice("loopBars", TR("Compassos do loop"), StringArray{"4", "8", "16"}, 1);
    flt("bpm", "BPM", 60.f, 200.f, 150.f);
    for (int i = 0; i < kMaxSections; ++i) intp(barsId(i), TR("Compassos da secção ") + String(i + 1), 0, 64, 0);

    choice("curve", "Rampa de snare", StringArray{"Escalonada", TR("Contínua")}, 0);
    choice("ioi", TR("Início da rampa"), StringArray{TR("1/4 (semínimas)"), TR("1/8 (colcheias)")}, 0);
    boo("kicks", TR("Bombo durante o build"), true);
    boo("silence", TR("Silêncio antes do drop"), true);
    boo("revcrash", TR("Prato invertido"), true);
    choice("riser", "Riser", StringArray{TR("Ruído"), "Saw", "Saw + ruído"}, 2);
    intp("range", TR("Subida do riser (semitons)"), 12, 36, 24);
    flt("filt", TR("Abertura do filtro"), 0.f, 1.f, 0.7f);

    boo("audition", TR("Ouvir (loop)"), false);
    flt("gKick", "Bombo", 0.f, 1.f, 0.8f);
    flt("gSnare", "Caixa/Palmas", 0.f, 1.f, 0.7f);
    flt("gHats", "Hats/Perc", 0.f, 1.f, 0.6f);
    flt("gFx", "Pratos/FX", 0.f, 1.f, 0.6f);
    flt("gRiser", "Riser", 0.f, 1.f, 0.6f);
    flt("gMaster", "Volume", 0.f, 1.f, 0.8f);
    return l;
}

ZeniteProcessor::ZeniteProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "ZENITE", createLayout()) {
    pAudition = apvts.getRawParameterValue("audition");
    pKey = apvts.getRawParameterValue("key");
    pRiserVol = apvts.getRawParameterValue("gRiser");
    pMasterVol = apvts.getRawParameterValue("gMaster");
    pBpm = apvts.getRawParameterValue("bpm");
    const char* gids[4] = {"gKick", "gSnare", "gHats", "gFx"};
    for (int i = 0; i < 4; ++i) pGroup[i] = apvts.getRawParameterValue(gids[i]);

    juce::StringArray ids{"genre", "template", "mode", "var", "fills", "buildBars", "loopBars", "curve", "ioi", "kicks", "silence", "revcrash", "riser", "range", "filt"};
    for (int i = 0; i < kMaxSections; ++i) ids.add(barsId(i));
    for (auto& id : ids) apvts.addParameterListener(id, this);
    listenIds = ids;
    regenerate();
}

ZeniteProcessor::~ZeniteProcessor() {
    cancelPendingUpdate();
    for (auto& id : listenIds) apvts.removeParameterListener(id, this);
}

void ZeniteProcessor::prepareToPlay(double, int samplesPerBlock) {
    const size_t sz = (size_t) std::max(4096, samplesPerBlock);
    rAmp.assign(sz, 0.0);
    rCut.assign(sz, 0.0);
    rPit.assign(sz, 0.0);
    kit.reset();
    riser.reset();
    seqRunning = false;
}

void ZeniteProcessor::setParam(const juce::String& id, float v) {
    if (auto* p = apvts.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}

void ZeniteProcessor::parameterChanged(const juce::String& id, float) {
    if (id == "genre") { genreChanged = true; resetBars = true; }
    if (id == "template") resetBars = true;
    triggerAsyncUpdate();
}

void ZeniteProcessor::handleAsyncUpdate() {
    if (genreChanged.exchange(false)) setParam("bpm", (float) defaultBpm((Genre) pi("genre")));
    if (resetBars.exchange(false))
        for (int i = 0; i < kMaxSections; ++i)
            if (pi(barsId(i)) != 0) setParam(barsId(i), 0.f);
    regenerate();
    sendChangeMessage();
}

double ZeniteProcessor::currentBpm() const {
    double h = lastHostBpm.load();
    return h > 0 ? h : (double) pf("bpm");
}

void ZeniteProcessor::regenerate() {
    DrumParams dp;
    dp.genre = (Genre) juce::jlimit(0, kNumGenres - 1, pi("genre"));
    dp.variation = juce::jlimit(0, 3, pi("var") - 1);
    dp.fills = juce::jlimit(0, 3, pi("fills"));
    BuildParams bp;
    bp.curve = (RollCurve) juce::jlimit(0, 1, pi("curve"));
    bp.startIoi = pi("ioi") == 0 ? 1.0 : 0.5;
    bp.kicks = pb("kicks");
    bp.silenceEnd = pb("silence");
    bp.revCrash = pb("revcrash");
    bp.riser = (RiserType) juce::jlimit(0, 2, pi("riser"));
    bp.rangeSemis = pi("range");
    bp.filterAmt = pf("filt");
    const Mode mode = (Mode) juce::jlimit(0, 2, pi("mode"));
    const int ti = juce::jlimit(0, 2, pi("template"));
    const auto& defs = arrangementTemplates(dp.genre)[(size_t) ti].sections;
    std::vector<int> ov;
    for (size_t i = 0; i < defs.size(); ++i) {
        int b = i < (size_t) kMaxSections ? pi(barsId((int) i)) : 0;
        ov.push_back(b > 0 ? b : defs[i].bars);
    }
    static const int opts[3] = {4, 8, 16};
    auto v = std::make_shared<View>();
    v->plan = makePlan(mode, dp, bp, ti, ov, opts[juce::jlimit(0, 2, pi("buildBars"))], opts[juce::jlimit(0, 2, pi("loopBars"))]);
    v->wrap = mode != Mode::Arrangement;
    v->genre = dp.genre;
    v->mode = mode;
    {
        juce::SpinLock::ScopedLockType sl(dataLock);
        if (published) graveyard.push_back(published);
        published = v;
    }
    graveyard.erase(std::remove_if(graveyard.begin(), graveyard.end(), [](const auto& p) { return p.use_count() == 1; }), graveyard.end());
}

std::shared_ptr<const View> ZeniteProcessor::getView() const {
    juce::SpinLock::ScopedLockType sl(dataLock);
    return published;
}

// ------------------------------------------------------------ áudio
void ZeniteProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    {
        juce::SpinLock::ScopedTryLockType tl(dataLock);
        if (tl.isLocked()) audioData = published;
    }
    bool playing = false, hasPpq = false;
    double ppq = 0.0, bpm = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppq = *q; hasPpq = true; }
            if (auto b = pos->getBpm()) bpm = *b;
        }
    if (bpm > 0) lastHostBpm = bpm;
    else bpm = (double) pBpm->load();
    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    const double bps = bpm / 60.0 / sr;  // tempos por amostra
    const bool hostPlaying = playing && hasPpq;

    outMidi.clear();
    const bool audition = pAudition->load() > 0.5f;
    bool useClock = false;
    double p0 = 0.0;
    if (hostPlaying) { useClock = true; p0 = ppq; }
    else if (audition) {
        if (!lastAudition) auditionBeat = 0.0;
        useClock = true;
        p0 = auditionBeat;
        auditionBeat += n * bps;
    }
    lastAudition = audition;
    const double p1 = p0 + n * bps;
    double from = p0;
    if (useClock) {
        const bool cont = seqRunning && std::abs(p0 - expectedP0) <= 0.05;
        if (cont) from = expectedP0;
        else kit.reset();
        expectedP0 = p1;
        seqRunning = true;
    } else seqRunning = false;

    nTrig = 0;
    const View* vw = audioData.get();
    const bool ok = useClock && vw && vw->plan.length > 0.0;
    double L = 0;
    bool wrapNow = false;
    if (ok) {
        L = vw->plan.length;
        wrapNow = vw->wrap || !hostPlaying;
        const auto& hits = vw->plan.hits;
        const long long k0 = wrapNow ? (long long) std::floor(from / L) : 0;
        const long long k1 = wrapNow ? k0 + 1 : 0;
        for (long long k = k0; k <= k1; ++k) {
            const double lo = from - (double) k * L, hi = p1 - (double) k * L;
            auto it = std::lower_bound(hits.begin(), hits.end(), lo, [](const Hit& h, double v) { return h.start < v; });
            for (; it != hits.end() && it->start < hi; ++it) {
                const double abs = it->start + (double) k * L;
                const int off = juce::jlimit(0, n - 1, (int) ((abs - p0) / bps));
                if (nTrig < 512) trig[nTrig++] = {off, (int) it->piece, it->vel, it->length / bps};
                if (it->piece != Piece::RevCrash) {
                    outMidi.addEvent(juce::MidiMessage::noteOn(10, gmNote(it->piece), (juce::uint8) it->vel), off);
                    outMidi.addEvent(juce::MidiMessage::noteOff(10, gmNote(it->piece)), juce::jmin(n - 1, off + 1));
                }
            }
        }
    }
    // pads ao vivo: notas GM do teclado tocam a peça correspondente
    for (const auto meta : midi) {
        auto m = meta.getMessage();
        if (!m.isNoteOn()) continue;
        for (int pc = 0; pc < kNumPieces; ++pc)
            if (gmNote((Piece) pc) == m.getNoteNumber() && nTrig < 512) {
                Trig t{meta.samplePosition, pc, (int) m.getVelocity(), 0.5 * sr};
                int j = nTrig++;
                while (j > 0 && trig[j - 1].off > t.off) { trig[j] = trig[j - 1]; --j; }
                trig[j] = t;
            }
    }

    float* L0 = buffer.getWritePointer(0);
    float* R0 = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L0;
    const float master = pMasterVol->load();
    for (int g = 0; g < 4; ++g) kit.groupGain[(size_t) g] = pGroup[g]->load() * pGroup[g]->load() * 1.4f;
    int pos = 0;
    for (int i = 0; i < nTrig; ++i) {
        if (trig[i].off > pos) { kit.render(L0 + pos, R0 + pos, trig[i].off - pos, sr); pos = trig[i].off; }
        kit.trigger((Piece) trig[i].piece, trig[i].vel, trig[i].len);
    }
    if (n > pos) kit.render(L0 + pos, R0 + pos, n - pos, sr);

    if (ok && !vw->plan.risers.empty()) {
        const int key = juce::jlimit(0, 11, (int) pKey->load());
        const int type = (int) vw->plan.risers[0].p.riser;
        const float rg = pRiserVol->load() * pRiserVol->load() * 0.9f;
        const int chunk = (int) rAmp.size();
        for (int c = 0; c < n; c += chunk) {
            const int m = std::min(chunk, n - c);
            for (int i = 0; i < m; ++i) {
                double b = p0 + (c + i) * bps;
                if (wrapNow) b -= std::floor(b / L) * L;
                double a = 0, cut = 300, pit = 440;
                for (const auto& r : vw->plan.risers)
                    if (b >= r.start && b < r.start + r.length) {
                        const double t = (b - r.start) / r.length;
                        a = riserAmp(t);
                        if (r.p.silenceEnd && (b - r.start) > r.length - 0.5) a = 0;
                        const double cd = (1.0 - r.p.filterAmt) + r.p.filterAmt * riserCutoff01(t);
                        cut = 250.0 * std::pow(64.0, cd);
                        const double semis = -(1.0 - riserPitch01(t)) * r.p.rangeSemis;
                        pit = 440.0 * std::pow(2.0, (60 + key - 69 + semis) / 12.0);
                        break;
                    }
                rAmp[(size_t) i] = a; rCut[(size_t) i] = cut; rPit[(size_t) i] = pit;
            }
            riser.render(L0 + c, R0 + c, m, sr, rAmp.data(), rCut.data(), rPit.data(), type, rg);
        }
    }

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* d = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i) d[i] = std::tanh(d[i] * master * 1.2f);
    }
    uiBeat = ok ? (wrapNow ? p0 - std::floor(p0 / L) * L : p0) : -1.0;
    midi.swapWith(outMidi);
}

// ------------------------------------------------------------ exportação
juce::String ZeniteProcessor::suggestedFileName(const juce::String& what) const {
    auto v = getView();
    juce::String g = v ? juce::String(genreName(v->genre)).removeCharacters(" ") : "Zenite";
    return "Zenite_" + g + "_" + what;
}

bool ZeniteProcessor::writeMidiFile(const juce::File& f, bool drums, bool risers) const {
    auto v = getView();
    if (!v) return false;
    const int tpq = 960;
    auto T = [&](double beats) { return beats * tpq; };
    juce::MidiMessageSequence conductor;
    conductor.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4));
    conductor.addEvent(juce::MidiMessage::tempoMetaEvent((int) std::lround(60000000.0 / currentBpm())));
    for (const auto& s : v->plan.sections) {
        juce::String name = juce::String::fromUTF8(s.label.c_str());
        conductor.addEvent(juce::MidiMessage::textMetaEvent(6, name), T(s.start));  // marcador
    }
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote(tpq);
    mf.addTrack(conductor);
    bool any = false;
    if (drums) {
        juce::MidiMessageSequence seq;
        seq.addEvent(juce::MidiMessage::textMetaEvent(3, "Drums"));
        for (const auto& h : v->plan.hits) {
            seq.addEvent(juce::MidiMessage::noteOn(10, gmNote(h.piece), (juce::uint8) h.vel), T(h.start));
            seq.addEvent(juce::MidiMessage::noteOff(10, gmNote(h.piece)), T(h.start + std::max(0.05, h.length)) - 2);
        }
        seq.updateMatchedPairs();
        mf.addTrack(seq);
        any = !v->plan.hits.empty();
    }
    if (risers && !v->plan.risers.empty()) {
        juce::MidiMessageSequence seq;
        seq.addEvent(juce::MidiMessage::textMetaEvent(3, "Riser"));
        const int top = 60 + juce::jlimit(0, 11, (int) pKey->load());
        for (const auto& r : v->plan.risers) {
            const int R = std::max(1, r.p.rangeSemis);
            // notas em degraus que seguem a curva de pitch; a última (tónica) dura até ao drop
            for (int k = 0; k <= R; ++k) {
                double a = std::pow(std::max(0.0, (k - 0.5) / R), 1.0 / 1.5);
                double b = std::pow(std::min(1.0, (k + 0.5) / R), 1.0 / 1.5);
                if (k == R) b = 1.0;
                const int pitch = top - R + k;
                seq.addEvent(juce::MidiMessage::noteOn(1, pitch, (juce::uint8) 100), T(r.start + a * r.length));
                seq.addEvent(juce::MidiMessage::noteOff(1, pitch), T(r.start + b * r.length) - 2);
            }
            // automação de filtro (CC74) e volume (CC11), de 1/4 em 1/4 de tempo
            for (double b = 0; b <= r.length + 1e-9; b += 0.25) {
                double t = std::min(1.0, b / r.length);
                double cd = (1.0 - r.p.filterAmt) + r.p.filterAmt * riserCutoff01(t);
                seq.addEvent(juce::MidiMessage::controllerEvent(1, 74, juce::jlimit(0, 127, (int) std::lround(cd * 127))), T(r.start + b));
                seq.addEvent(juce::MidiMessage::controllerEvent(1, 11, juce::jlimit(0, 127, (int) std::lround(riserAmp(t) * 127))), T(r.start + b));
            }
        }
        seq.updateMatchedPairs();
        mf.addTrack(seq);
        any = true;
    }
    if (!any) return false;
    f.deleteFile();
    juce::FileOutputStream os(f);
    return os.openedOk() && mf.writeTo(os);
}

bool ZeniteProcessor::writeGuideFile(const juce::File& f) const {
    auto v = getView();
    if (!v) return false;
    juce::String t;
    t << "ZENITE - guia de arranjo e energia\n";
    t << juce::String::fromUTF8(genreName(v->genre)) << " | " << juce::String(currentBpm(), 0) << " BPM\n";
    t << TR(bpmHint(v->genre)) << "\n\n";
    t << TR("SECÇÕES (compassos a contar do 1)\n");
    for (const auto& s : v->plan.sections) {
        const int b0 = (int) std::lround(s.start / 4.0) + 1, b1 = b0 + s.bars - 1;
        t << "  " << juce::String(b0).paddedLeft(' ', 3) << "-" << juce::String(b1).paddedRight(' ', 4) << " " << U8(s.label)
          << " (" << s.bars << TR(" compassos)\n     ") << U8(s.tip) << "\n     " << U8(s.barsTip) << "\n";
    }
    t << TR("\nMARCADORES: na régua da playlist, coloca marcadores de tempo nestes compassos: ");
    for (const auto& s : v->plan.sections) t << (int) std::lround(s.start / 4.0) + 1 << " (" << U8(s.label) << ")  ";
    t << "\n";
    for (const auto& r : v->plan.risers) {
        const int b0 = (int) std::lround(r.start / 4.0) + 1;
        t << TR("\nBUILD-UP a partir do compasso ") << b0 << " (" << (int) std::lround(r.length / 4.0) << TR(" compassos)\n");
        t << TR("  Snare roll: ") << (r.p.curve == RollCurve::Stepped ? TR("escalonada (1/4 > 1/8 > 1/16 > 1/32)") : TR("contínua (acelera sem degraus)"))
          << TR(", velocidade a subir. Nos últimos 4 compassos do build.\n");
        t << TR("  Automação sugerida (posição, filtro passa-baixo, tom, volume):\n");
        for (int i = 0; i <= 8; ++i) {
            double tt = i / 8.0;
            double cd = (1.0 - r.p.filterAmt) + r.p.filterAmt * riserCutoff01(tt);
            double semis = -(1.0 - riserPitch01(tt)) * r.p.rangeSemis;
            double beat = r.length * tt;
            t << "    " << TR("compasso ") << juce::String(b0 + (int) std::floor(beat / 4.0)) << "." << juce::String(1 + (int) std::floor(std::fmod(beat, 4.0)))
              << ": " << TR("filtro ") << (int) std::lround(cd * 100) << "%, " << TR("tom ") << juce::String(semis, 1) << " st, " << TR("volume ")
              << (int) std::lround(riserAmp(tt) * 100) << "%\n";
        }
        t << TR("  O tom do riser termina na tónica (") << kKeyNames[juce::jlimit(0, 11, (int) pKey->load())] << TR("). Liga o filtro e o pitch do teu riser a estas curvas.\n");
    }
    f.deleteFile();
    return f.replaceWithText(t, false, false, "\n");
}

void ZeniteProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void ZeniteProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (!xml || !xml->hasTagName(apvts.state.getType())) return;
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    cancelPendingUpdate();
    genreChanged = false;  // carregar um projeto não deve repor BPM nem compassos
    resetBars = false;
    regenerate();
    sendChangeMessage();
}

juce::AudioProcessorEditor* ZeniteProcessor::createEditor() { return new ZeniteEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ZeniteProcessor(); }
