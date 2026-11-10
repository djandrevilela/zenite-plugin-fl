# Zenite (VST3) — estrutura e energia para FL Studio

Plugin irmão do **Harmonia** (acordes) e do **Alvorada** (melodia/baixo/synth). Tudo é **determinístico e baseado em convenções de arranjo**
(sem aleatoriedade, sem IA): mesmas definições => mesmo resultado. "Variação" escolhe outro modelo de uma lista curada.
Sem padrões copiados de músicas, sem nomes de artistas nem de produtos.

## O que faz
| Módulo | O que dá |
|---|---|
| **Template de arranjo** | Intro, Build-up, Drop, Breakdown, segundo Build-up e Drop, Outro, com 3 modelos por subgénero (Padrão, Compacto, Extendido). Todas as secções em múltiplos de 4 compassos e **cada build-up resolve num drop**. Podes mudar os compassos de cada secção (aviso se não for múltiplo de 4). Cada secção traz **dicas** (o que pôr lá e quantos compassos usar). |
| **Marcadores** | O MIDI exportado leva **marcadores** no início de cada secção; o **guia .txt** lista os compassos onde pôr marcadores na régua da playlist. |
| **Build-up builder** | Rampa de snare (escalonada 1/4→1/8→1/16→1/32 ou contínua, sempre a acelerar e a subir de velocidade), bombo a 4 por compasso, silêncio de meio tempo antes do drop, prato invertido, **riser** (ruído/saw) com **filtro a abrir e tom a subir até à tónica** do tom escolhido, tudo preso ao tempo. Exporta MIDI do riser com notas em degraus + automação CC74 (filtro) e CC11 (volume), e o guia traz a tabela de automação. |
| **Drums por subgénero** | Future Bass (meio tempo), Progressive House, Big Room: 4 variações cada, fills (Sem / Poucos / Normais / Muitos) no fim das frases, crash + impacto no 1.º tempo de cada drop. |

## Modos
**Arranjo completo**, **Build-up** (só o build, em loop) e **Drums (loop)**. Com o FL a tocar o plugin segue o transporte; sem isso usa
**▶ Ouvir (loop)**. O BPM usado é o do anfitrião; o campo BPM serve quando o anfitrião não o fornece (e muda para o habitual do subgénero ao trocares de subgénero).

## Usar no FL Studio
1. Copia `Zenite.vst3` para `C:\Program Files\Common Files\VST3\` e faz *Find installed plugins*.
2. Adiciona o Zenite como gerador e carrega em **▶ Ouvir (loop)** para ouvires.
3. Arrasta o **MIDI: drums + marcadores** para a playlist/piano roll (drums no canal 10, notas GM). Arrasta o **MIDI: riser + automação** para o piano roll do teu synth de riser.
4. Também podes tocar pads: as notas GM do teu teclado (36 bombo, 38 caixa, 39 palmas, 42/46 hats…) tocam os sons do plugin.
5. A saída MIDI do plugin leva os drums no canal 10 para roteares para outro instrumento/kit.

## Compilar
- **GitHub Actions**: push da pasta, separador *Actions* → artefacto `Zenite-VST3`.
- **Windows local** (VS 2022 + CMake + Git):
  ```
  cmake -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release --target Zenite_VST3
  ```
- **Só motor + testes**: `cmake -B build -DZENITE_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build`
- **Teste de áudio offline** (opcional): `-DZENITE_BUILD_RENDER_TEST=ON` e correr `ZeniteRenderTest`.

## Estado e limites desta versão
- Motor testado (579 mil verificações: determinismo, rampa sempre a acelerar, bombo a 4, build→drop, fills, curvas monótonas). Compila em Linux e o teste de áudio offline passa nos 3 subgéneros e 3 modos (sem NaN/clipping).
- **Não foi ouvido nem testado dentro do FL Studio.** Por verificar: se o FL importa os **marcadores** do MIDI como marcadores da playlist (se não, usa o guia .txt); o aspeto da interface; o carácter dos sons (são sons de pré-escuta aproximados, usa os teus próprios kits e risers no mix final).
- O riser MIDI usa notas em degraus que seguem a curva de tom: com glide/portamento no teu synth soa contínuo.
- JUCE é AGPLv3 ou comercial. O nome "Zenite" é provisório: confirma que está livre antes de lançar.
