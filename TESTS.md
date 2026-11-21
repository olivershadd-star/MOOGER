# MOOGER V0.3 — vérifications du 20 novembre 2026

## Réalisées

- Configuration CMake Release et compilation indépendante du DSP : réussite.
- CTest : 2/2 tests réussis, ANKS_DSP_Smoke et MOOGER_Modules.
- Séquence huit pas, stabilité, sortie non silencieuse, marge de sortie, déconnexion OUTPUT, LFO/filtre et relâchement des notes : réussite à 44,1 / 48 / 96 kHz.
- VCO2 : note MIDI 69 à 440 Hz, octave réglée pendant la note à 880 Hz, désaccord fin +50 cents, pitch indépendant après déconnexion, indépendance par rapport à VCO1 : réussite aux trois fréquences.
- Mixer : contribution de chaque oscillateur, quatre formes d'onde VCO2, déconnexion de son entrée, branchement alternatif VCO1, mute du niveau général, refus des boucles et d'un câble CV sur entrée audio : réussite.
- Balayages brusques des gains et du niveau sur sources sinusoïdales : variations entre échantillons inférieures à 0,02 dans ce scénario. Ce contrôle vise les gains du mixer et ne garantit pas l'absence de tout clic audible.
- Vérification syntaxique C++17 de PluginProcessor.cpp et PluginEditor.cpp contre JUCE 8.0.17-security, commit dabd5ad13cd869cba41046947fffc8e5ce5cd662 : réussite. Avertissement existant pour MidiBuffer::Iterator déprécié.

## Non vérifiées

- Compilation/liaison complète de JUCE et exécution de l'interface : dépendances de développement Linux absentes (notamment X11/Xlib.h dans la tentative précédente).
- Exécution du workflow Windows MSVC x64, bundle VST3, EXE autonome et scan Ableton Live 12.
- Rendu graphique, confort des nouvelles commandes, audio matériel et écoute réelle.
- Restauration d'état V0.2/V0.3 dans un DAW ; migration préparée dans le code seulement.
- Traitement MIDI de la couche JUCE, tailles de blocs 64/128/256/512 et charge CPU en DAW.

Le ZIP contient les sources et les scripts, aucun VST3 Windows validé.
