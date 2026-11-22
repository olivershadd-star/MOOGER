# MOOGER V0.5 — validation

## Réalisé localement sur Linux

- CMake Release sans JUCE : quatre tests DSP réussis (cœur, VCO/mixer, percussion et modulation).
- Modulation à 44,1/48/96 kHz : captures sur front montant, maintien entre fronts, SAMPLE manuel, gains des deux atténuateurs, quatre entrées/offset/inversion du mixer, montée/descente du lag, LFO et bruit bornés.
- Panneau non relié à une destination sonore : comparaison binaire contre le moteur V0.4 publié, 1 504 800 échantillons identiques (trois fréquences, quatre ondes, mélodie et percussion actives).
- CV constant : VCO1 passant de 440 à 880 Hz avec une profondeur d'une octave.
- Neuf sorties utilitaires vers filtre, VCA et hauteur des deux VCO : stabilité numérique. Refus de câbles audio/CV et de boucles interdites.
- AddressSanitizer et UndefinedBehaviorSanitizer sur le test de modulation : réussite.
- Vérification syntaxique C++17 de l'éditeur, du processeur et du test d'intégration JUCE.

## Prévu par le workflow Windows

- Compilation VST3, EXE et cinq tests CTest en Release.
- Traitement JUCE aux trois fréquences et blocs 64/128/256/512, sauvegarde des paramètres et câbles CV, migrations V0.3 et V0.4.
- Vérification de SAMPLE, maintien du S&H, ouverture de l'éditeur sans modification des paramètres et changement de vue sans changement de patch.
- Captures Windows de la vue modulation, de la vue percussion et de la taille réduite.

L'écoute et le chargement dans Ableton Live 12 ne sont pas couverts par ces tests. Pas de mesure de fidélité sonore au CP-251, ni de garantie d'absence de tout transitoire lors d'un changement de câble ou de source.
