# Zenite — princípios do projeto (ler sempre antes de mexer no código)

Plugin VST3 (JUCE) de estrutura e energia para EDM: template de arranjo, build-up builder e drums por subgénero.
Plugin irmão do Harmonia (acordes) e do Alvorada (melodia/baixo/synth).

## Regras inegociáveis (definidas pelo autor)
1. **Nada de geração aleatória nem IA.** Tudo é determinístico: padrões curados à mão + regras (fases de 4 compassos,
   acelerações do snare roll, curvas do riser). "Variação" = escolher outro modelo de uma lista, nunca sorteio.
   Proibido `rand`, `std::random_*`, ML. (O ruído dos sons de bateria usa LCG de semente fixa: é DSP, não geração musical.)
2. **Só coisas que funcionam**: estruturas em múltiplos de 4/8 compassos, fills no fim das frases, builds que resolvem no drop,
   riser que termina na tónica do tom escolhido.
3. **Sem infringir copyright**: sem padrões/estruturas copiados de músicas concretas, sem nomes de artistas ou de produtos.
   Só convenções genéricas de género (não protegíveis).
4. Interface e textos em português europeu (pt-PT).
5. `core/` é C++17 puro, testado em `tests/`. `plugin/` só orquestra (JUCE).

## Estrutura
- `core/Zenite`          arranjo, drums, build-up, plano temporal (secções, hits, risers)
- `plugin/DrumSynth.h`   bateria sintetizada + riser (sons internos para pré-escuta)
- `plugin/PluginProcessor` sequenciador sincronizado com o anfitrião (ou loop interno), export MIDI com marcadores
- `plugin/PluginEditor`  timeline + painéis
