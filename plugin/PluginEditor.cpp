#include "PluginEditor.h"

#include "Util.h"

using namespace zenite;

namespace {
const juce::Colour kBg(0xff14161f), kPanel(0xff1d2130), kEdge(0xff2d3348), kText(0xffe2e8f0), kMuted(0xff8a94a8), kAccent(0xff4fd1c5);

juce::Colour sectionColour(SectionType t) {
    switch (t) {
        case SectionType::Intro: return juce::Colour(0xff5a7fb5);
        case SectionType::Build: return juce::Colour(0xfff6ad55);
        case SectionType::Drop: return juce::Colour(0xffe05a9a);
        case SectionType::Breakdown: return juce::Colour(0xff4fd1c5);
        case SectionType::Outro: return juce::Colour(0xff8a94a8);
    }
    return kMuted;
}
int rowOf(Piece p) {
    switch (p) {
        case Piece::Kick: return 0;
        case Piece::Snare: case Piece::Clap: case Piece::TomL: case Piece::TomM: case Piece::TomH: return 1;
        case Piece::HatC: case Piece::HatO: case Piece::Rim: case Piece::Shaker: return 2;
        default: return 3;
    }
}
}  // namespace

// =============================================================== DragHandle
void DragHandle::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(1);
    g.setColour(kAccent.withAlpha(0.16f));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(kAccent);
    g.drawRoundedRectangle(r, 7.0f, 1.2f);
    g.setColour(kText);
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawText(text, getLocalBounds(), juce::Justification::centred);
}
void DragHandle::mouseDrag(const juce::MouseEvent& e) {
    if (dragging || e.getDistanceFromDragStart() < 6 || !makeFile) return;
    auto f = makeFile();
    if (!f.existsAsFile()) return;
    dragging = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles({f.getFullPathName()}, false, this, [this] { dragging = false; });
}

// =============================================================== TimelineView
void TimelineView::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(kPanel);
    g.fillRoundedRectangle(r, 8.0f);
    auto v = proc.getView();
    if (!v || v->plan.length <= 0) return;
    const auto& plan = v->plan;
    const float x0 = r.getX() + 10, w = r.getWidth() - 20;
    auto X = [&](double b) { return x0 + (float) (b / plan.length) * w; };
    // régua
    const int totalBars = (int) std::lround(plan.length / 4.0);
    const int step = totalBars > 96 ? 16 : totalBars > 48 ? 8 : 4;
    g.setFont(juce::FontOptions(10.5f));
    for (int b = 0; b < totalBars; b += step) {
        g.setColour(kEdge);
        g.drawVerticalLine((int) X(b * 4.0), r.getY() + 6, r.getBottom() - 6);
        g.setColour(kMuted);
        g.drawText(juce::String(b + 1), (int) X(b * 4.0) + 3, (int) r.getY() + 4, 40, 14, juce::Justification::left);
    }
    const float by = r.getY() + 22, bh = 56;
    for (const auto& s : plan.sections) {
        auto rc = juce::Rectangle<float>(X(s.start) + 1, by, X(s.start + s.length) - X(s.start) - 2, bh);
        g.setColour(sectionColour(s.type).withAlpha(0.32f));
        g.fillRoundedRectangle(rc, 5.0f);
        g.setColour(sectionColour(s.type));
        g.drawRoundedRectangle(rc, 5.0f, 1.4f);
        g.setColour(kText);
        g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        g.drawText(U8(s.label), rc.reduced(5, 3).removeFromTop(18), juce::Justification::topLeft, true);
        g.setColour(kMuted);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(juce::String(s.bars) + TR(" comp."), rc.reduced(5, 3).removeFromBottom(16), juce::Justification::bottomLeft, true);
    }
    // curvas do riser (filtro = branco, tom = amarelo)
    for (const auto& rs : plan.risers) {
        juce::Path pf, pp;
        for (int i = 0; i <= 40; ++i) {
            double t = i / 40.0;
            float x = X(rs.start + t * rs.length);
            double cd = (1.0 - rs.p.filterAmt) + rs.p.filterAmt * riserCutoff01(t);
            float yf = by + bh - 4 - (float) cd * (bh - 8);
            float yp = by + bh - 4 - (float) riserPitch01(t) * (bh - 8);
            if (i == 0) { pf.startNewSubPath(x, yf); pp.startNewSubPath(x, yp); }
            else { pf.lineTo(x, yf); pp.lineTo(x, yp); }
        }
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.strokePath(pf, juce::PathStrokeType(1.5f));
        g.setColour(juce::Colour(0xfffaf089));
        g.strokePath(pp, juce::PathStrokeType(1.5f));
    }
    // mini piano-roll dos drums
    const float ry = by + bh + 10, rh = 13;
    static const juce::Colour rc[4] = {juce::Colour(0xfff56565), juce::Colour(0xff63b3ed), juce::Colour(0xff9ae6b4), juce::Colour(0xfffaf089)};
    const char* names[4] = {"Bombo", "Caixa", "Hats", "Pratos/FX"};
    g.setFont(juce::FontOptions(9.5f));
    for (int i = 0; i < 4; ++i) {
        g.setColour(kEdge.withAlpha(0.5f));
        g.fillRect(x0, ry + i * (rh + 2), w, rh);
        g.setColour(kMuted);
        g.drawText(names[i], (int) x0 + 2, (int) (ry + i * (rh + 2)), 70, (int) rh, juce::Justification::left);
    }
    for (const auto& h : plan.hits) {
        int row = rowOf(h.piece);
        g.setColour(rc[row].withAlpha(0.35f + 0.65f * h.vel / 127.0f));
        g.fillRect(X(h.start), ry + row * (rh + 2) + 1, std::max(1.2f, w / (float) plan.length * 0.25f), rh - 2);
    }
    g.setColour(kMuted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(TR("Build-up: linha branca = filtro a abrir, amarela = tom do riser a subir."), (int) x0, (int) (r.getBottom() - 16), (int) w, 14,
               juce::Justification::right);
    if (playhead >= 0) {
        g.setColour(juce::Colours::white);
        g.drawVerticalLine((int) X(playhead), r.getY() + 4, r.getBottom() - 4);
    }
}

// =============================================================== SectionRow
SectionRow::SectionRow(ZeniteProcessor& p, int i) : idx(i) {
    bars.setSliderStyle(juce::Slider::IncDecButtons);
    bars.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 24);
    bars.setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_Vertical);
    bars.setNumDecimalPlacesToDisplay(0);
    bars.textFromValueFunction = [this](double v) { return juce::String(v < 0.5 ? defBars : (int) v); };
    addAndMakeVisible(bars);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "bars" + juce::String(i), bars);
    tip.setFont(juce::FontOptions(11.5f));
    tip.setColour(juce::Label::textColourId, kMuted);
    tip.setJustificationType(juce::Justification::topLeft);
    tip.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(tip);
}
void SectionRow::setSection(const SectionSpan* s, int def, bool editable) {
    setVisible(s != nullptr);
    if (!s) return;
    defBars = def;
    bars.setVisible(editable);
    bars.updateText();
    name = U8(s->label);
    colour = sectionColour(s->type);
    tip.setText(TR((s->tip + "  \xE2\x80\xA2  " + s->barsTip).c_str()), juce::dontSendNotification);
    repaint();
}
void SectionRow::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(0, 2);
    g.setColour(kPanel);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(colour);
    g.fillRoundedRectangle(r.removeFromLeft(5), 2.0f);
    g.setColour(kText);
    g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    g.drawText(name, 14, 4, 100, 20, juce::Justification::left);
}
void SectionRow::resized() {
    auto r = getLocalBounds().reduced(8, 4);
    r.removeFromLeft(6);
    auto left = r.removeFromLeft(118);
    bars.setBounds(left.removeFromBottom(24));
    r.removeFromLeft(6);
    tip.setBounds(r);
}

// =============================================================== ZeniteEditor
juce::ComboBox* ZeniteEditor::addCombo(const char* id, const juce::String& label, std::vector<juce::Component*>& col) {
    auto* c = own(new juce::ComboBox());
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id))) c->addItemList(p->choices, 1);
    cAtt.push_back(std::make_unique<CA>(proc.apvts, id, *c));
    auto* l = new juce::Label({}, label);
    l->setColour(juce::Label::textColourId, kMuted);
    l->setFont(juce::FontOptions(12.0f));
    labels.add(l);
    addAndMakeVisible(l);
    col.push_back(l);
    col.push_back(c);
    return c;
}
juce::Slider* ZeniteEditor::addSlider(const char* id, const juce::String& label, std::vector<juce::Component*>& col, bool incDec) {
    auto* s = own(new juce::Slider());
    if (incDec) {
        s->setSliderStyle(juce::Slider::IncDecButtons);
        s->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 24);
    } else {
        s->setSliderStyle(juce::Slider::LinearHorizontal);
        s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 20);
    }
    sAtt.push_back(std::make_unique<SA>(proc.apvts, id, *s));
    auto* l = new juce::Label({}, label);
    l->setColour(juce::Label::textColourId, kMuted);
    l->setFont(juce::FontOptions(12.0f));
    labels.add(l);
    addAndMakeVisible(l);
    col.push_back(l);
    col.push_back(s);
    return s;
}
juce::ToggleButton* ZeniteEditor::addToggle(const char* id, const juce::String& text, std::vector<juce::Component*>& col) {
    auto* t = own(new juce::ToggleButton(text));
    bAtt.push_back(std::make_unique<BA>(proc.apvts, id, *t));
    col.push_back(t);
    return t;
}

ZeniteEditor::ZeniteEditor(ZeniteProcessor& p) : juce::AudioProcessorEditor(&p), proc(p), timeline(p) {
    setLookAndFeel(&lnf);
    lnf.setColour(juce::ComboBox::backgroundColourId, kPanel);
    lnf.setColour(juce::PopupMenu::backgroundColourId, kPanel);
    lnf.setColour(juce::Slider::thumbColourId, kAccent);
    lnf.setColour(juce::Slider::trackColourId, kAccent.withAlpha(0.5f));
    lnf.setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    lnf.setColour(juce::ToggleButton::tickColourId, kAccent);
    lnf.setColour(juce::TextButton::buttonColourId, kPanel);

    // barra de topo
    addCombo("genre", TR("Subgénero"), topRow);
    modeBox = addCombo("mode", "Modo", topRow);
    addCombo("template", TR("Modelo de arranjo"), topRow);
    addCombo("key", "Tom do riser", topRow);
    addSlider("bpm", "BPM", topRow);
    addToggle("audition", TR("\xE2\x96\xB6 Ouvir (loop)"), topRow);

    addAndMakeVisible(timeline);
    for (int i = 0; i < kMaxSections; ++i) addAndMakeVisible(rows.add(new SectionRow(p, i)));

    // drums
    addSlider("var", TR("Variação (1-4)"), drumCol, true);
    addCombo("fills", "Fills", drumCol);
    // build-up
    addCombo("buildBars", TR("Compassos do build (modo Build-up)"), buildCol);
    addCombo("curve", "Rampa de snare", buildCol);
    addCombo("ioi", TR("Início da rampa"), buildCol);
    addToggle("kicks", TR("Bombo durante o build"), buildCol);
    addToggle("silence", TR("Silêncio antes do drop"), buildCol);
    addToggle("revcrash", TR("Prato invertido"), buildCol);
    addCombo("riser", "Riser", buildCol);
    addSlider("range", TR("Subida (semitons)"), buildCol);
    addSlider("filt", TR("Abertura do filtro"), buildCol);
    addCombo("loopBars", TR("Compassos do loop (modo Drums)"), buildCol);

    // mixer
    struct M { const char* id; const char* name; };
    for (M m : {M{"gKick", "Bombo"}, M{"gSnare", "Caixa/Palmas"}, M{"gHats", "Hats/Perc"}, M{"gFx", "Pratos/FX"}, M{"gRiser", "Riser"}, M{"gMaster", "Volume"}}) {
        auto* s = own(new juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox));
        sAtt.push_back(std::make_unique<SA>(proc.apvts, m.id, *s));
        auto* l = new juce::Label({}, m.name);
        l->setJustificationType(juce::Justification::centred);
        l->setColour(juce::Label::textColourId, kMuted);
        l->setFont(juce::FontOptions(11.5f));
        labels.add(l);
        addAndMakeVisible(l);
        mixRow.push_back(s);
        mixLabels.push_back(l);
    }

    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory);
    dragDrums.text = TR("Arrastar MIDI: drums + marcadores");
    dragDrums.makeFile = [this, tmp] {
        auto f = tmp.getChildFile(proc.suggestedFileName("Arranjo") + ".mid");
        return proc.writeMidiFile(f, true, false) ? f : juce::File();
    };
    dragRiser.text = TR("Arrastar MIDI: riser + automação");
    dragRiser.makeFile = [this, tmp] {
        auto f = tmp.getChildFile(proc.suggestedFileName("Riser") + ".mid");
        return proc.writeMidiFile(f, false, true) ? f : juce::File();
    };
    dragGuide.text = TR("Arrastar guia (.txt)");
    dragGuide.makeFile = [this, tmp] {
        auto f = tmp.getChildFile(proc.suggestedFileName("Guia") + ".txt");
        return proc.writeGuideFile(f) ? f : juce::File();
    };
    addAndMakeVisible(dragDrums);
    addAndMakeVisible(dragRiser);
    addAndMakeVisible(dragGuide);

    hint.setText(TR("Sem aleatoriedade nem IA: padrões e durações curados por teoria de arranjo. Os marcadores vão dentro do MIDI e no guia."),
                 juce::dontSendNotification);
    hint.setFont(juce::FontOptions(11.0f));
    hint.setColour(juce::Label::textColourId, kMuted);
    addAndMakeVisible(hint);

    setSize(1120, 840);
    proc.addChangeListener(this);
    refresh();
    startTimerHz(30);
}

ZeniteEditor::~ZeniteEditor() {
    proc.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void ZeniteEditor::timerCallback() { timeline.setPlayhead(proc.getUiBeat()); }

void ZeniteEditor::refresh() {
    auto v = proc.getView();
    const bool arr = v && v->mode == Mode::Arrangement;
    const auto& tl = arrangementTemplates(v ? v->genre : Genre::FutureBass);
    (void) tl;
    for (int i = 0; i < rows.size(); ++i) {
        const SectionSpan* s = (v && i < (int) v->plan.sections.size()) ? &v->plan.sections[(size_t) i] : nullptr;
        // compassos por omissão do modelo (para mostrar quando o parâmetro está a 0)
        int def = s ? s->bars : 4;
        if (s && arr) {
            int ti = juce::jlimit(0, 2, (int) std::lround(proc.apvts.getRawParameterValue("template")->load()));
            const auto& defs = arrangementTemplates(v->genre)[(size_t) ti].sections;
            if (i < (int) defs.size()) def = defs[(size_t) i].bars;
        }
        rows[i]->setSection(s, def, arr);
    }
    timeline.repaint();
    repaint();
}

void ZeniteEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    g.setColour(kPanel);
    g.fillRoundedRectangle(juce::Rectangle<float>(742, 240, 366, 500), 8.0f);
    g.setColour(kPanel);
    g.fillRoundedRectangle(juce::Rectangle<float>(12, 748, 1096, 82), 8.0f);
    g.setColour(kAccent);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("ZENITE", 756, 244, 120, 20, juce::Justification::left);
    g.setColour(kMuted);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText("DRUMS", 756, 268, 120, 16, juce::Justification::left);
    g.drawText("BUILD-UP", 756, 366, 200, 16, juce::Justification::left);
    auto v = proc.getView();
    if (v) {
        g.setFont(juce::FontOptions(11.5f));
        g.drawText(TR(bpmHint(v->genre)), 12, 36, 1096, 14, juce::Justification::left);
    }
}

void ZeniteEditor::resized() {
    // topo: etiqueta em cima, controlo por baixo (6 blocos)
    {
        int x = 12;
        const int widths[6] = {170, 170, 190, 120, 200, 160};
        for (int i = 0; i < 6; ++i) {
            auto* lab = topRow[(size_t) i * 2];
            auto* c = topRow[(size_t) i * 2 + 1];
            if (i == 5) { c->setBounds(x, 8, widths[i], 28); lab->setBounds(0, 0, 0, 0); }
            else { lab->setBounds(x, 0, widths[i], 0); c->setBounds(x, 8, widths[i] - 8, 28); lab->setVisible(false); }
            x += widths[i] + 6;
        }
    }
    timeline.setBounds(12, 56, 1096, 176);
    for (int i = 0; i < rows.size(); ++i) rows[i]->setBounds(12, 240 + i * 50, 720, 48);

    // coluna de drums
    int y = 288;
    for (size_t i = 0; i + 1 < drumCol.size(); i += 2) {
        drumCol[i]->setBounds(756, y, 130, 24);
        drumCol[i + 1]->setBounds(890, y, 200, 24);
        y += 30;
    }
    y = 388;
    // build: alterna etiquetas (par) + controlos; toggles não têm etiqueta
    for (size_t i = 0; i < buildCol.size();) {
        if (dynamic_cast<juce::Label*>(buildCol[i])) {
            buildCol[i]->setBounds(756, y, 150, 24);
            buildCol[i + 1]->setBounds(910, y, 186, 24);
            i += 2;
        } else {
            buildCol[i]->setBounds(756, y, 330, 24);
            i += 1;
        }
        y += 30;
    }
    // mixer
    for (size_t i = 0; i < mixRow.size(); ++i) {
        int x = 30 + (int) i * 110;
        mixRow[i]->setBounds(x + 18, 754, 56, 50);
        mixLabels[i]->setBounds(x - 4, 806, 100, 18);
    }
    dragDrums.setBounds(710, 756, 390, 22);
    dragRiser.setBounds(710, 782, 390, 22);
    dragGuide.setBounds(710, 808, 390, 20);
    hint.setBounds(12, 832 - 2, 1000, 12);
}
