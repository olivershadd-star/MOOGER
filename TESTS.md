# MOOGER V0.4 — validation

## Localement sur Linux

- CMake Release sans JUCE : 3/3 tests DSP réussis.
- Régression du cœur mélodique et des deux VCO/mixer : réussite.
- Percussion à 44,1/48/96 kHz : huit pas, hauteurs/vélocités, silences, déclenchement manuel, arrêt avec fin de résonance, bus débranché et sortie repatchée, tempos liés/indépendants, coexistence avec une note mélodique tenue, extrêmes FM/bruit/résonance/niveaux.
- Vérification C++17 des sources JUCE et du test d'intégration contre JUCE 8.0.17-security.

## Vérification Windows prévue par le workflow

- MSVC x64 : VST3, Standalone et test d'intégration JUCE.
- Quatre tests CTest en Release.
- Test processeur à 44,1/48/96 kHz et blocs 64/128/256/512.
- Sauvegarde/restauration des nouveaux paramètres et câbles, chargement d'un état V0.3 reconstruit sans paramètres de percussion.
- Captures du rack à sa taille normale et réduite dans MOOGER-Rack-Preview.

Le rendu et la lecture audio du plugin dans Ableton restent à vérifier sur une machine Windows équipée du DAW. Les tests ne mesurent pas une fidélité au circuit DFAM et ne prouvent pas l'absence de tout clic audible.
