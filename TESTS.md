# MOOGER V0.4 — validation

## Localement sur Linux

- CMake Release sans JUCE : 3/3 tests DSP réussis.
- Régression du cœur mélodique et des deux VCO/mixer : réussite.
- Percussion à 44,1/48/96 kHz : huit pas, hauteurs/vélocités, silences, déclenchement manuel, arrêt avec fin de résonance, bus débranché et sortie repatchée, tempos liés/indépendants, coexistence avec une note mélodique tenue, extrêmes FM/bruit/résonance/niveaux.
- Vérification C++17 des sources JUCE et du test d'intégration contre JUCE 8.0.17-security.

## Réalisé sur Windows 2022 / MSVC x64

[Build validé](https://github.com/olivershadd-star/MOOGER/actions/runs/44971297135), source `ea2f5c50721847efc72622e0b528a88934fc477e` : **4/4 tests réussis**.

- MSVC x64 : VST3, Standalone et test d'intégration JUCE.
- Quatre tests CTest en Release : réussite.
- Test processeur à 44,1/48/96 kHz et blocs 64/128/256/512.
- Sauvegarde/restauration des nouveaux paramètres et câbles, chargement d'un état V0.3 reconstruit sans paramètres de percussion.
- Ouverture de l'éditeur sans modification de la résonance, du sustain ou du niveau VCA : réussite.
- Captures Windows du rack à sa taille normale et réduite dans MOOGER-Rack-Preview, relues visuellement.
- ZIP téléchargé et intégrité SHA-256 contrôlée ; présence de la VST3 et de l'EXE, signatures PE x64 vérifiées.
- Test de percussion local avec AddressSanitizer et UndefinedBehaviorSanitizer : réussite.

Le rendu et la lecture audio du plugin dans Ableton restent à vérifier sur une machine Windows équipée du DAW. Les tests ne mesurent pas une fidélité au circuit DFAM et ne prouvent pas l'absence de tout clic audible.
