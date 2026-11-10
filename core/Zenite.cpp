#include "Zenite.h"

#include <algorithm>
#include <cmath>

namespace zenite {

// ------------------------------------------------------------------ nomes e textos
const char* genreName(Genre g) {
    switch (g) {
        case Genre::FutureBass: return "Future Bass";
        case Genre::ProgHouse: return "Progressive House";
        case Genre::BigRoom: return "Big Room";
    }
    return "";
}

const char* sectionName(SectionType t) {
    switch (t) {
        case SectionType::Intro: return "Intro";
        case SectionType::Build: return "Build-up";
        case SectionType::Drop: return "Drop";
        case SectionType::Breakdown: return "Breakdown";
        case SectionType::Outro: return "Outro";
    }
    return "";
}

int defaultBpm(Genre g) {
    switch (g) {
        case Genre::FutureBass: return 150;
        case Genre::ProgHouse: return 124;
        case Genre::BigRoom: return 128;
    }
    return 128;
}

const char* bpmHint(Genre g) {
    switch (g) {
        case Genre::FutureBass: return "Zona habitual: 140-160 BPM (sensação de meio tempo: a caixa cai no 3.º tempo).";
        case Genre::ProgHouse: return "Zona habitual: 120-128 BPM (bombo a 4 por compasso, ritmo contínuo).";
        case Genre::BigRoom: return "Zona habitual: 126-130 BPM (bombo forte a 4 por compasso, palmas no 2 e no 4).";
    }
    return "";
}

int gmNote(Piece p) {
    switch (p) {
        case Piece::Kick: return 36;
        case Piece::Snare: return 38;
        case Piece::Clap: return 39;
        case Piece::HatC: return 42;
        case Piece::HatO: return 46;
        case Piece::Rim: return 37;
        case Piece::TomL: return 45;
        case Piece::TomM: return 47;
        case Piece::TomH: return 50;
        case Piece::Crash: return 49;
        case Piece::Shaker: return 70;
        case Piece::Impact: return 55;
        case Piece::RevCrash: return 119;
    }
    return 36;
}

static ArrangementTemplate T(const char* n, std::vector<SectionDef> s) { return ArrangementTemplate{n, std::move(s)}; }

const std::vector<ArrangementTemplate>& arrangementTemplates(Genre g) {
    using S = SectionType;
    static const std::vector<ArrangementTemplate> fb = {
        T("Padrão (7 secções)", {{S::Intro, 8}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Outro, 8}}),
        T("Compacto", {{S::Intro, 4}, {S::Build, 4}, {S::Drop, 8}, {S::Breakdown, 8}, {S::Build, 4}, {S::Drop, 8}, {S::Outro, 4}}),
        T("Extendido", {{S::Intro, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 8}, {S::Build, 8}, {S::Drop, 16}, {S::Outro, 16}}),
    };
    static const std::vector<ArrangementTemplate> ph = {
        T("Padrão (7 secções)", {{S::Intro, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Outro, 16}}),
        T("Compacto", {{S::Intro, 8}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 8}, {S::Build, 8}, {S::Drop, 16}, {S::Outro, 8}}),
        T("Extendido", {{S::Intro, 32}, {S::Build, 16}, {S::Drop, 32}, {S::Breakdown, 16}, {S::Build, 16}, {S::Drop, 32}, {S::Outro, 32}}),
    };
    static const std::vector<ArrangementTemplate> br = {
        T("Padrão (7 secções)", {{S::Intro, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Outro, 16}}),
        T("Compacto", {{S::Intro, 8}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 8}, {S::Build, 8}, {S::Drop, 16}, {S::Outro, 8}}),
        T("Extendido", {{S::Intro, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 16}, {S::Build, 8}, {S::Drop, 16}, {S::Breakdown, 8}, {S::Build, 8}, {S::Drop, 16}, {S::Outro, 16}}),
    };
    switch (g) {
        case Genre::FutureBass: return fb;
        case Genre::ProgHouse: return ph;
        case Genre::BigRoom: return br;
    }
    return fb;
}

std::string sectionTip(Genre g, SectionType t) {
    int gi = (int) g, ti = (int) t;
    static const char* tips[kNumGenres][5] = {
        {   // Future Bass
            "Acordes suaves com pad e arpejo filtrado, sem bombo ou só com hats leves. Apresenta o motivo da melodia e deixa espaço para a voz/chop.",
            "Sobe a energia: snare roll, riser e filtro a abrir. Podes parar o baixo e deixar só os acordes a subir de tom.",
            "Supersaws largos, baixo/sub a alternar com o bombo, arpejo rápido. A caixa cai no 3.º tempo; mantém espaço entre a melodia e o baixo.",
            "Tira bombo e baixo: piano, pad e voz/pluck suave. É aqui que a música respira e mostra a emoção da progressão.",
            "Desmonta os elementos um a um (baixo, arpejo, drums) para uma saída fácil de misturar noutra faixa."},
        {   // Progressive House
            "Bombo e hats a entrar aos poucos; só 2-3 elementos de cada vez. Acrescenta um elemento novo a cada 4 compassos.",
            "Filtro passa-baixo a abrir devagar, snare roll nos últimos 4 compassos e riser contínuo. Tira o baixo para o drop bater mais.",
            "Baixo rolante e acordes em stab ou pad, bombo constante. Faz crescer a energia a cada 8 compassos com camadas novas.",
            "Sem bombo e sem baixo: acordes, pad e melodia. Prepara a segunda subida reentrando elementos aos poucos.",
            "Vai tirando camadas de 8 em 8 compassos até ficar só bombo e percussão, para facilitar a mistura."},
        {   // Big Room
            "Bombo forte e hats; um motivo curto na melodia. Mantém a intro funcional para o DJ.",
            "Snare roll bem marcado, riser agudo e uma pausa curta antes do drop. Mantém o bombo a 4 por compasso até ao fim.",
            "Lead grande e curto em cima do bombo; baixo curto entre os bombos. Crash e impacto no 1.º tempo do drop.",
            "Sem bombo: acordes, melodia e voz. Faz a segunda subida mais intensa do que a primeira.",
            "Tira o lead e deixa bombo, hats e percussão para a mistura de saída."},
    };
    return tips[gi][ti];
}

std::string barsTip(SectionType t, int bars) {
    std::string base;
    switch (t) {
        case SectionType::Intro: base = "Intro: 8 a 16 compassos funcionam bem; 32 só para música de pista."; break;
        case SectionType::Build: base = "Build-up: 8 compassos é o padrão; 4 dá algo mais direto; 16 só se a energia for crescendo."; break;
        case SectionType::Drop: base = "Drop: 16 compassos é o padrão; 32 só se houver variação a cada 8."; break;
        case SectionType::Breakdown: base = "Breakdown: 8 a 16 compassos; mais do que isso pesa."; break;
        case SectionType::Outro: base = "Outro: 8 a 16 compassos chegam para a mistura de saída."; break;
    }
    if (bars % 4 != 0) base += " Atenção: usa múltiplos de 4 compassos para as frases fecharem bem.";
    return base;
}

// ------------------------------------------------------------------ padrões de drums (16 passos = 1 compasso)
struct Pat { const char *kA, *kB, *sn, *cl, *hc, *ho, *sh, *rim; };

static const Pat dropPats[kNumGenres][4] = {
    {   // Future Bass (meio tempo)
        {"x.........x.....", "x.....x.........", "........x.......", "........x.......", "x.x.x.x.x.x.x.x.", "", ".......x.......x", ""},
        {"x...........x...", "x.....x...x.....", "........x.......", "........x.......", "x.xxx.x.x.xxx.x.", "", "..x...x...x...x.", ""},
        {"x.....x.....x...", "x..x......x.....", "........x.......", "........x.......", "xoxoxoxoxoxoxoxo", "", "", ".......x.......x"},
        {"x.....x...x.....", "x.........x.x...", "........x.......", "........x.......", "x.x.x.xxx.x.x.x.", "", "", "..x.......x....."},
    },
    {   // Progressive House
        {"x...x...x...x...", "x...x...x...x...", "", "....x.......x...", "...x...x...x...x", "..x...x...x...x.", "", ""},
        {"x...x...x...x...", "x...x...x...x...", "", "....x.......x...", ".x.x.x.x.x.x.x.x", "..x...x...x...x.", "", ""},
        {"x...x...x...x...", "x...x...x...x...", "", "....x.......x...", "x.x...x.x.x...x.", "......x.......x.", "", ""},
        {"x...x...x...x...", "x...x...x...x...", "", "....x.......x...", "...x...x...x...x", "..x...x...x...x.", "xoxoxoxoxoxoxoxo", ".......x.......x"},
    },
    {   // Big Room
        {"x...x...x...x...", "x...x...x...x...", "", "....x.......x...", "", "..x...x...x...x.", "", ""},
        {"x...x...x...x...", "x...x...x...x...", "....x.......x...", "....x.......x...", "...x...x...x...x", "..x...x...x...x.", "", ""},
        {"x...x...x...x...", "x...x...x...x..x", "....x.......x...", "....x.......x...", "", "..x...x...x...x.", "", ""},
        {"x...x...x...x...", "x...x...x...x..x", "....x.......x...", "....x.......x...", "...x...x...x...x", "..x...x...x...x.", "", ".......x.......x"},
    },
};

static void addPat(std::vector<Hit>& out, Piece p, const char* pat, double barStart, double velScale, int baseVel, double len = 0.125) {
    if (!pat || !*pat) return;
    for (int i = 0; i < 16 && pat[i]; ++i) {
        if (pat[i] == '.') continue;
        double v = baseVel * velScale * (pat[i] == 'o' ? 0.6 : 1.0);
        Hit h;
        h.piece = p;
        h.start = barStart + i * 0.25;
        h.length = len;
        h.vel = std::max(1, std::min(127, (int) std::lround(v)));
        out.push_back(h);
    }
}

// Tabelas leves para Intro/Outro/Breakdown (padrão de cada género)
static void lightBar(std::vector<Hit>& out, Genre g, SectionType t, int bar, int bars, double bs) {
    double vs = (t == SectionType::Breakdown) ? 0.6 : 0.75;
    bool kickOn = true;
    if (t == SectionType::Breakdown) kickOn = false;
    if (t == SectionType::Intro && g != Genre::FutureBass && bars >= 8 && bar < bars / 2) kickOn = false;
    if (g == Genre::FutureBass) {
        if (t == SectionType::Breakdown) {
            addPat(out, Piece::Rim, "........x.......", bs, vs, 80);
            addPat(out, Piece::Shaker, "x.x.x.x.x.x.x.x.", bs, vs, 70);
        } else {
            addPat(out, Piece::HatC, "x.x.x.x.x.x.x.x.", bs, vs, 80);
            addPat(out, Piece::Rim, "........x.......", bs, vs, 80);
            if (kickOn && bar >= bars / 2) addPat(out, Piece::Kick, "x.........x.....", bs, vs, 100);
        }
    } else if (g == Genre::ProgHouse) {
        if (kickOn) addPat(out, Piece::Kick, "x...x...x...x...", bs, vs, 100);
        if (t == SectionType::Breakdown) addPat(out, Piece::Shaker, "xoxoxoxoxoxoxoxo", bs, vs, 70);
        else addPat(out, Piece::HatO, "..x...x...x...x.", bs, vs, 80);
        if (t != SectionType::Breakdown && bar >= bars / 2) addPat(out, Piece::HatC, "...x...x...x...x", bs, vs, 70);
    } else {
        if (kickOn) addPat(out, Piece::Kick, "x...x...x...x...", bs, vs, 105);
        if (t == SectionType::Breakdown) addPat(out, Piece::HatC, "x.x.x.x.x.x.x.x.", bs, vs, 70);
        else {
            addPat(out, Piece::HatO, "..x...x...x...x.", bs, vs, 80);
            if (bar >= bars / 2) addPat(out, Piece::Clap, "....x.......x...", bs, vs, 90);
        }
    }
}

static void addFill(std::vector<Hit>& out, int model, double bs) {
    static const double win[4] = {3.0, 2.5, 2.0, 3.5};
    double w = win[model];
    out.erase(std::remove_if(out.begin(), out.end(), [&](const Hit& h) {
                  return h.start >= bs + w - 1e-9 && h.start < bs + 4.0 && h.piece != Piece::Kick && h.piece != Piece::Crash;
              }), out.end());
    auto add = [&](Piece p, double pos, int v, double len = 0.2) {
        Hit h; h.piece = p; h.start = bs + pos; h.length = len; h.vel = v; out.push_back(h);
    };
    switch (model) {
        case 0:  // snare em semicolcheias no último tempo, a crescer
            for (int i = 0; i < 4; ++i) add(Piece::Snare, 3.0 + i * 0.25, 75 + i * 16);
            break;
        case 1:  // descida de tons
            add(Piece::TomH, 2.5, 100); add(Piece::TomH, 3.0, 105); add(Piece::TomM, 3.25, 105);
            add(Piece::TomL, 3.5, 110); add(Piece::Snare, 3.75, 120);
            break;
        case 2:  // snare em colcheias e depois semicolcheias
            for (int i = 0; i < 4; ++i) add(Piece::Snare, 2.0 + i * 0.5, 70 + i * 8);
            for (int i = 0; i < 4; ++i) add(Piece::Snare, 3.0 + i * 0.25, 100 + i * 8);
            break;
        default:  // pausa curta e duas pancadas finais
            add(Piece::Snare, 3.5, 100); add(Piece::Snare, 3.75, 120);
            break;
    }
}

std::vector<Hit> drumsForSection(const DrumParams& dp, SectionType t, int bars, double startBeat, int sectionIndex) {
    std::vector<Hit> out;
    if (bars <= 0) return out;
    if (t == SectionType::Build) return buildUpHits(BuildParams(), dp, bars, startBeat);
    int var = std::max(0, std::min(3, dp.variation));
    int fills = std::max(0, std::min(3, dp.fills));
    for (int bar = 0; bar < bars; ++bar) {
        double bs = startBeat + bar * 4.0;
        std::vector<Hit> bh;
        if (t == SectionType::Drop) {
            const Pat& p = dropPats[(int) dp.genre][var];
            addPat(bh, Piece::Kick, (bar % 2 == 0) ? p.kA : p.kB, bs, 1.0, 112);
            addPat(bh, Piece::Snare, p.sn, bs, 1.0, 108);
            addPat(bh, Piece::Clap, p.cl, bs, 1.0, 104);
            addPat(bh, Piece::HatC, p.hc, bs, 1.0, 82);
            addPat(bh, Piece::HatO, p.ho, bs, 1.0, 88, 0.2);
            addPat(bh, Piece::Shaker, p.sh, bs, 1.0, 70);
            addPat(bh, Piece::Rim, p.rim, bs, 1.0, 84);
            bool first = (bar == 0);
            if (first) {
                Hit c; c.piece = Piece::Crash; c.start = bs; c.length = 2; c.vel = 118; bh.push_back(c);
                Hit im; im.piece = Piece::Impact; im.start = bs; im.length = 1; im.vel = 120; bh.push_back(im);
                if (std::none_of(bh.begin(), bh.end(), [&](const Hit& h) { return h.piece == Piece::Kick && h.start == bs; })) {
                    Hit k; k.piece = Piece::Kick; k.start = bs; k.length = 0.25; k.vel = 118; bh.push_back(k);
                }
            } else if (fills > 0 && bar % 4 == 0) {
                Hit c; c.piece = Piece::Crash; c.start = bs; c.length = 2; c.vel = 105; bh.push_back(c);
            }
            bool last = (bar == bars - 1);
            bool doFill = false;
            if (fills >= 1 && (last || (bar + 1) % 8 == 0)) doFill = true;
            if (fills >= 2 && (bar + 1) % 4 == 0) doFill = true;
            if (fills >= 3 && (bar + 1) % 2 == 0) doFill = true;
            if (doFill) {
                int model = (sectionIndex + (bar / 4) + var) % 4;
                if (fills >= 3 && (bar + 1) % 4 != 0) model = 3;  // mini-fill nos compassos pares
                addFill(bh, model, bs);
            }
        } else {
            lightBar(bh, dp.genre, t, bar, bars, bs);
        }
        out.insert(out.end(), bh.begin(), bh.end());
    }
    std::stable_sort(out.begin(), out.end(), [](const Hit& a, const Hit& b) { return a.start < b.start; });
    return out;
}

// ------------------------------------------------------------------ build-up
std::vector<Hit> buildUpHits(const BuildParams& bp, const DrumParams& dp, int bars, double startBeat) {
    (void) dp;
    std::vector<Hit> out;
    if (bars <= 0) return out;
    const double total = bars * 4.0;
    const double endLimit = total - (bp.silenceEnd ? 0.5 : 0.0);
    const double rollLen = std::min(total, 16.0);
    const double rollStart = total - rollLen;
    const double s0 = std::max(0.125, std::min(2.0, bp.startIoi));

    if (bp.kicks)
        for (double b = 0; b < endLimit - 1e-9; b += 1.0) {
            Hit h; h.piece = Piece::Kick; h.start = startBeat + b; h.length = 0.25; h.vel = 100; out.push_back(h);
        }
    for (double b = 0; b < rollStart - 1e-9; b += 0.5) {
        Hit h; h.piece = Piece::HatC; h.start = startBeat + b; h.length = 0.125; h.vel = 70; out.push_back(h);
    }
    // rampa de snare
    double pos = rollStart;
    int guard = 0;
    while (pos < endLimit - 1e-9 && guard++ < 4000) {
        double t = (pos - rollStart) / rollLen;
        double ioi;
        if (bp.curve == RollCurve::Stepped) {
            double div = (t < 0.5) ? 1.0 : (t < 0.75) ? 2.0 : (t < 0.875) ? 4.0 : 8.0;
            ioi = std::max(0.125, s0 / div);
        } else {
            ioi = std::max(0.125, s0 * std::pow(0.125 / s0, t));
        }
        Hit h; h.piece = Piece::Snare; h.start = startBeat + pos; h.length = std::min(ioi, 0.25) * 0.9;
        h.vel = std::max(1, std::min(127, (int) std::lround(45 + 82 * t)));
        out.push_back(h);
        pos += ioi;
    }
    if (bp.revCrash && total >= 4.0) {
        Hit h; h.piece = Piece::RevCrash; h.start = startBeat + total - 4.0; h.length = 4.0; h.vel = 100; out.push_back(h);
    }
    std::stable_sort(out.begin(), out.end(), [](const Hit& a, const Hit& b) { return a.start < b.start; });
    return out;
}

double riserAmp(double t) { t = std::max(0.0, std::min(1.0, t)); return 0.15 + 0.85 * t * t; }
double riserCutoff01(double t) { t = std::max(0.0, std::min(1.0, t)); return t * t; }
double riserPitch01(double t) { t = std::max(0.0, std::min(1.0, t)); return std::pow(t, 1.5); }

// ------------------------------------------------------------------ plano
Plan makePlan(Mode mode, const DrumParams& dp, const BuildParams& bp, int templateIndex, const std::vector<int>& barsOverride,
              int buildBars, int loopBars) {
    Plan plan;
    std::vector<SectionDef> defs;
    if (mode == Mode::Arrangement) {
        const auto& tl = arrangementTemplates(dp.genre);
        int ti = std::max(0, std::min((int) tl.size() - 1, templateIndex));
        defs = tl[(size_t) ti].sections;
        if (barsOverride.size() == defs.size())
            for (size_t i = 0; i < defs.size(); ++i) defs[i].bars = std::max(1, std::min(64, barsOverride[i]));
    } else if (mode == Mode::BuildOnly) {
        defs.push_back({SectionType::Build, std::max(1, std::min(32, buildBars))});
    } else {
        defs.push_back({SectionType::Drop, std::max(1, std::min(32, loopBars))});
    }

    double pos = 0;
    int dropCount = 0, buildCount = 0, bdCount = 0;
    for (size_t i = 0; i < defs.size(); ++i) {
        const auto& d = defs[i];
        SectionSpan s;
        s.type = d.type;
        s.start = pos;
        s.bars = d.bars;
        s.length = d.bars * 4.0;
        std::string nm = sectionName(d.type);
        if (d.type == SectionType::Drop) { ++dropCount; nm += " " + std::to_string(dropCount); }
        if (d.type == SectionType::Build) { ++buildCount; nm += " " + std::to_string(buildCount); }
        if (d.type == SectionType::Breakdown) { ++bdCount; if (bdCount > 1) nm += " " + std::to_string(bdCount); }
        s.label = nm;
        s.tip = sectionTip(dp.genre, d.type);
        s.barsTip = barsTip(d.type, d.bars);
        plan.sections.push_back(s);

        std::vector<Hit> hs;
        if (d.type == SectionType::Build) {
            hs = buildUpHits(bp, dp, d.bars, pos);
            RiserSpan r; r.start = pos; r.length = s.length; r.p = bp;
            plan.risers.push_back(r);
        } else {
            hs = drumsForSection(dp, d.type, d.bars, pos, (int) i);
        }
        plan.hits.insert(plan.hits.end(), hs.begin(), hs.end());
        pos += s.length;
    }
    plan.length = pos;
    std::stable_sort(plan.hits.begin(), plan.hits.end(), [](const Hit& a, const Hit& b) { return a.start < b.start; });
    return plan;
}

}  // namespace zenite
