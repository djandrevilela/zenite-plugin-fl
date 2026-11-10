#pragma once
// Zenite core: arranjo, drums por subgénero, build-up e plano temporal. C++17 puro, determinístico.
// Tempo em tempos (beats) de 4/4. Um compasso = 4 tempos.
#include <string>
#include <vector>

namespace zenite {

enum class Genre { FutureBass, ProgHouse, BigRoom };
constexpr int kNumGenres = 3;
enum class SectionType { Intro, Build, Drop, Breakdown, Outro };
enum class Piece { Kick, Snare, Clap, HatC, HatO, Rim, TomL, TomM, TomH, Crash, Shaker, Impact, RevCrash };
constexpr int kNumPieces = 13;

struct Hit {
    Piece piece = Piece::Kick;
    double start = 0;
    double length = 0.125;
    int vel = 100;
};

struct SectionDef { SectionType type; int bars; };
struct ArrangementTemplate {
    const char* name;
    std::vector<SectionDef> sections;
};

const char* genreName(Genre g);
const char* sectionName(SectionType t);
int defaultBpm(Genre g);
const char* bpmHint(Genre g);
const std::vector<ArrangementTemplate>& arrangementTemplates(Genre g);  // 3 variantes: Padrão, Compacto, Extendido
std::string sectionTip(Genre g, SectionType t);                          // o que pôr na secção (pt-PT)
std::string barsTip(SectionType t, int bars);                            // porquê esta duração (pt-PT)
int gmNote(Piece p);                                                     // mapa GM (canal 10)

// ---- Build-up ----
enum class RollCurve { Stepped, Smooth };
enum class RiserType { Noise, Saw, Both };
struct BuildParams {
    RollCurve curve = RollCurve::Stepped;
    double startIoi = 1.0;       // intervalo inicial entre pancadas (1 = semínima, 0.5 = colcheia)
    bool kicks = true;           // mantém o bombo a 4 por compasso durante o build
    bool silenceEnd = true;      // meio tempo de silêncio antes do drop
    bool revCrash = true;        // prato invertido no fim
    RiserType riser = RiserType::Both;
    int rangeSemis = 24;         // quantos semitons o riser sobe até à tónica
    double filterAmt = 0.7;      // 0..1: quanto o filtro abre
};

struct DrumParams {
    Genre genre = Genre::FutureBass;
    int variation = 0;           // 0..3
    int fills = 2;               // 0 sem fills .. 3 fortes
};

std::vector<Hit> drumsForSection(const DrumParams&, SectionType, int bars, double startBeat, int sectionIndex);
std::vector<Hit> buildUpHits(const BuildParams&, const DrumParams&, int bars, double startBeat);

// Curvas do riser (t em 0..1)
double riserAmp(double t);
double riserCutoff01(double t);  // 0..1 (abertura do filtro)
double riserPitch01(double t);   // 0..1: 0 = tom mais grave (tónica - range), 1 = tónica

// ---- Plano completo ----
struct RiserSpan { double start = 0, length = 0; BuildParams p; };
struct SectionSpan {
    SectionType type = SectionType::Intro;
    double start = 0, length = 0;
    int bars = 0;
    std::string label, tip, barsTip;
};
enum class Mode { Arrangement, BuildOnly, DrumsOnly };
struct Plan {
    std::vector<Hit> hits;  // ordenados por início
    std::vector<RiserSpan> risers;
    std::vector<SectionSpan> sections;
    double length = 0;
};

// barsOverride: compassos por secção (se o tamanho não coincidir com o modelo, usa o modelo).
Plan makePlan(Mode, const DrumParams&, const BuildParams&, int templateIndex, const std::vector<int>& barsOverride,
              int buildBars, int loopBars);

}  // namespace zenite
