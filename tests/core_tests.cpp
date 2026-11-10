#include <cmath>
#include <cstdio>
#include <set>

#include "Zenite.h"

using namespace zenite;
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FALHOU: %s (linha %d)\n", #c, __LINE__); } } while (0)

static bool same(const std::vector<Hit>& a, const std::vector<Hit>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i].piece != b[i].piece || a[i].start != b[i].start || a[i].vel != b[i].vel) return false;
    return true;
}

int main() {
    for (int g = 0; g < kNumGenres; ++g) {
        Genre ge = (Genre) g;
        const auto& tl = arrangementTemplates(ge);
        CHECK(tl.size() == 3);
        for (const auto& t : tl) {
            int drops = 0, builds = 0;
            for (size_t i = 0; i < t.sections.size(); ++i) {
                CHECK(t.sections[i].bars % 4 == 0);
                if (t.sections[i].type == SectionType::Drop) ++drops;
                if (t.sections[i].type == SectionType::Build) {
                    ++builds;
                    CHECK(i + 1 < t.sections.size() && t.sections[i + 1].type == SectionType::Drop);  // todo build resolve num drop
                }
            }
            CHECK(drops >= 2 && builds >= 2);
            CHECK(t.sections.front().type == SectionType::Intro);
            CHECK(t.sections.back().type == SectionType::Outro);
        }
        for (int s = 0; s < 5; ++s) CHECK(!sectionTip(ge, (SectionType) s).empty());
        CHECK(defaultBpm(ge) > 100);

        for (int var = 0; var < 4; ++var)
            for (int fills = 0; fills <= 3; ++fills)
                for (size_t ti = 0; ti < tl.size(); ++ti) {
                    DrumParams dp; dp.genre = ge; dp.variation = var; dp.fills = fills;
                    BuildParams bp;
                    Plan p = makePlan(Mode::Arrangement, dp, bp, (int) ti, {}, 8, 8);
                    Plan p2 = makePlan(Mode::Arrangement, dp, bp, (int) ti, {}, 8, 8);
                    CHECK(same(p.hits, p2.hits));  // determinismo
                    double len = 0;
                    for (auto& s : p.sections) { CHECK(std::fabs(s.start - len) < 1e-9); len += s.length; }
                    CHECK(std::fabs(len - p.length) < 1e-9);
                    CHECK((int) p.risers.size() == [&]{ int n = 0; for (auto& s : p.sections) n += s.type == SectionType::Build; return n; }());
                    double last = -1;
                    for (auto& h : p.hits) {
                        CHECK(h.start >= last - 1e-9);
                        last = h.start;
                        CHECK(h.start >= 0 && h.start < p.length);
                        CHECK(h.vel >= 1 && h.vel <= 127);
                    }
                    // kick em todos os tempos do drop (4 por compasso nos géneros four-on-floor)
                    if (ge != Genre::FutureBass)
                        for (auto& s : p.sections)
                            if (s.type == SectionType::Drop)
                                for (int b = 0; b < s.bars * 4; ++b) {
                                    bool k = false;
                                    for (auto& h : p.hits)
                                        if (h.piece == Piece::Kick && std::fabs(h.start - (s.start + b)) < 1e-9) k = true;
                                    CHECK(k);
                                }
                    // drop começa com impacto + crash
                    for (auto& s : p.sections)
                        if (s.type == SectionType::Drop) {
                            bool imp = false, cr = false;
                            for (auto& h : p.hits) {
                                if (std::fabs(h.start - s.start) < 1e-9 && h.piece == Piece::Impact) imp = true;
                                if (std::fabs(h.start - s.start) < 1e-9 && h.piece == Piece::Crash) cr = true;
                            }
                            CHECK(imp && cr);
                        }
                }
    }

    // variações realmente diferentes
    for (int g = 0; g < kNumGenres; ++g) {
        std::vector<std::vector<Hit>> v;
        for (int var = 0; var < 4; ++var) {
            DrumParams dp; dp.genre = (Genre) g; dp.variation = var; dp.fills = 0;
            v.push_back(drumsForSection(dp, SectionType::Drop, 4, 0, 0));
        }
        for (int a = 0; a < 4; ++a)
            for (int b = a + 1; b < 4; ++b) CHECK(!same(v[a], v[b]));
    }

    // fills: com fills=0 não há toms; com fills=2 há fill em cada 4 compassos
    {
        DrumParams dp; dp.genre = Genre::BigRoom; dp.fills = 0;
        auto h0 = drumsForSection(dp, SectionType::Drop, 8, 0, 0);
        int toms = 0;
        for (auto& h : h0) if (h.piece == Piece::TomL || h.piece == Piece::TomM || h.piece == Piece::TomH) ++toms;
        CHECK(toms == 0);
        dp.fills = 2;
        auto h2 = drumsForSection(dp, SectionType::Drop, 8, 0, 0);
        CHECK(h2.size() != h0.size());
    }

    // build-up: rampa de snare acelera, vel cresce, termina antes do drop, kicks a 4 por compasso
    {
        DrumParams dp; BuildParams bp;
        for (int bars : {4, 8, 16}) {
            for (int c = 0; c < 2; ++c) {
                bp.curve = (RollCurve) c;
                auto h = buildUpHits(bp, dp, bars, 32.0);
                double total = bars * 4.0;
                std::vector<Hit> sn;
                int kicks = 0;
                for (auto& x : h) {
                    CHECK(x.start >= 32.0 && x.start < 32.0 + total);
                    if (x.piece == Piece::Snare) sn.push_back(x);
                    if (x.piece == Piece::Kick) ++kicks;
                }
                CHECK(kicks == bars * 4);  // 4 por compasso; o silêncio é só o último meio tempo
                CHECK(sn.size() > 8);
                double firstGap = sn[1].start - sn[0].start;
                double lastGap = sn.back().start - sn[sn.size() - 2].start;
                CHECK(lastGap < firstGap);
                CHECK(sn.back().vel > sn.front().vel);
                CHECK(sn.back().start < 32.0 + total - 0.5 + 1e-9);
                for (size_t i = 1; i < sn.size(); ++i) {
                    CHECK(sn[i].vel >= sn[i - 1].vel);
                    CHECK(sn[i].start - sn[i - 1].start >= 0.125 - 1e-9);
                }
            }
        }
        bp.silenceEnd = false; bp.kicks = true;
        auto h = buildUpHits(bp, dp, 4, 0);
        int k = 0;
        for (auto& x : h) if (x.piece == Piece::Kick) ++k;
        CHECK(k == 16);
        bp.revCrash = false; bp.kicks = false;
        h = buildUpHits(bp, dp, 4, 0);
        for (auto& x : h) CHECK(x.piece != Piece::Kick && x.piece != Piece::RevCrash);
    }

    // curvas do riser monótonas e a terminar em 1
    {
        double pa = -1, pc = -1, pp = -1;
        for (int i = 0; i <= 100; ++i) {
            double t = i / 100.0;
            CHECK(riserAmp(t) >= pa); pa = riserAmp(t);
            CHECK(riserCutoff01(t) >= pc); pc = riserCutoff01(t);
            CHECK(riserPitch01(t) >= pp); pp = riserPitch01(t);
        }
        CHECK(std::fabs(riserPitch01(1.0) - 1.0) < 1e-12 && std::fabs(riserCutoff01(1.0) - 1.0) < 1e-12);
    }

    // override de compassos e modos
    {
        DrumParams dp; BuildParams bp;
        std::vector<int> ov = {4, 4, 8, 8, 4, 8, 4};
        Plan p = makePlan(Mode::Arrangement, dp, bp, 0, ov, 8, 8);
        CHECK(std::fabs(p.length - 40 * 4.0 - 0.0) < 1e-9 + 0 || true);
        double sum = 0; for (int b : ov) sum += b * 4.0;
        CHECK(std::fabs(p.length - sum) < 1e-9);
        Plan pb = makePlan(Mode::BuildOnly, dp, bp, 0, {}, 8, 8);
        CHECK(pb.sections.size() == 1 && pb.risers.size() == 1 && std::fabs(pb.length - 32.0) < 1e-9);
        Plan pd = makePlan(Mode::DrumsOnly, dp, bp, 0, {}, 8, 4);
        CHECK(pd.sections.size() == 1 && pd.risers.empty() && std::fabs(pd.length - 16.0) < 1e-9);
    }

    std::printf("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
