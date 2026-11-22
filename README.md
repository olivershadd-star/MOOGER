# MOOGER V0.4 — laboratoire semi-modulaire natif

Projet ANKS Labs, C++17/JUCE, VST3 Windows x64 et application autonome. Cœur inspiré du Mother-32, second oscillateur, mixer et voix rythmique indépendante. La compilation Windows et les quatre tests automatisés ont réussi ; le chargement et l'écoute dans Ableton restent à vérifier. Le moteur est un synthétiseur natif original ; aucune mesure de fidélité au circuit Mother-32 n'a été effectuée.

[Télécharger la VST3 et l’EXE Windows x64](https://github.com/olivershadd-star/MOOGER/actions/runs/44971297135/artifacts/14733424346) · [Captures du rack](https://github.com/olivershadd-star/MOOGER/actions/runs/44971297135/artifacts/14734201045)

## Cette version

- Rack fixe de trois rangées, redimensionnable de 784 × 665 à 1344 × 1140, façade sombre, repères crème et commandes ambre.
- Cœur : séquenceur huit pas, VCO 1, filtre résonant, VCA, ADSR, LFO et sortie limitée doucement.
- VCO 2 : sinus, triangle, dent de scie, carré ; accord de -24 à +24 demi-tons, réglage fin de -50 à +50 cents ; entrée pitch et sortie audio indépendantes.
- Mixer : deux entrées patchables, deux atténuateurs 0–1, niveau de sortie 0–1 ; lissage des trois gains sur une constante de temps de 10 ms.
- Par défaut les deux VCO suivent le séquenceur/MIDI ; VCO 2 est désaccordé de +7 cents. Ils alimentent le mixer (65 % / 35 %, sortie à 80 %), puis le filtre, le VCA et la sortie.
- Dix câbles initiaux, vingt et une prises. Les entrées mixer acceptent VCO 1 ou VCO 2. Le mixer peut alimenter le filtre, le VCA ou directement la sortie ; les boucles de rétroaction sont refusées.
- Accord VCO 1 et VCO 2 mis à jour pendant une note tenue, avec glissement de fréquence de 5 ms.

## Nouvelle voix de percussion

- Deux oscillateurs partageant le choix triangle/carré, mélange du second oscillateur, rapport de fréquences 0,25–8, FM linéaire et bruit.
- Filtre passe-bas résonant propre à la percussion ; enveloppes de hauteur et d'amplitude séparées du cœur mélodique.
- Séquenceur huit pas en doubles croches : hauteur -24 à +24 demi-tons, vélocité 0–1. Zéro produit un silence de déclenchement et laisse finir le son précédent.
- **DRUM RUN** démarre uniquement les percussions, **HIT** joue un coup même à l'arrêt.
- **LINK BPM** utilise le BPM du cœur ; sinon **FREE BPM** règle le tempo indépendant. Cette option ne synchronise ni la phase des séquenceurs ni le transport du DAW.
- **DRUM LEVEL** dose le volume, puis le master contrôle les deux voix. À l'ouverture les deux séquenceurs sont arrêtés.
- Parcours prébranché : PERCUSSION OUT → OUTPUT DRUM IN. Cette entrée s'ajoute à l'entrée mélodique avant le limiteur. Débrancher l'entrée mélodique seule ne coupe donc pas les percussions.
- La sortie percussion peut aussi alimenter une entrée du mixer, le filtre, le VCA ou la sortie mélodique. Pour éviter de la doubler, débrancher OUTPUT DRUM IN quand on utilise ce second trajet.
- Décroissance d'amplitude 20 ms–2 s (temps jusqu'à environ -60 dB après l'attaque), décroissance de hauteur 5 ms–1 s et profondeur 0–48 demi-tons. STOP laisse finir le coup en cours.

Le moteur est une création originale inspirée du rôle musical du DFAM. Il ne reproduit pas l'ensemble de son architecture ni ses circuits.

## Utilisation

1. En mode SEQ, cliquer RUN pour entendre le patch initial.
2. Tourner IN 2 du mixer pour doser le second VCO ; FINE règle les battements et SEMITONES les intervalles.
3. Cliquer une sortie, puis une entrée compatible, ou glisser entre les prises. Cliquer une entrée seule la débranche.
4. Débrancher PITCH IN du VCO 2 pour le faire jouer à une hauteur indépendante, basée sur la note MIDI 48 plus son accord.
5. Ajouter LFO CV → VCF CV pour animer le filtre. RESET CABLES restaure le patch initial.
6. Désactiver SEQ / MIDI pour recevoir les notes d'un clavier ou d'un clip. Les commandes et câbles sont sauvegardés dans l'état du plugin.

## Compiler sous Windows

Visual Studio 2022, charge Développement Desktop en C++, et CMake 3.22+. Exécuter `compile_windows.bat`.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target MOOGER_VST3 MOOGER_Standalone anks_audio_smoke mooger_modules mooger_percussion mooger_processor_tests
ctest --test-dir build -C Release --output-on-failure
```

JUCE 8.0.17-security est récupéré depuis son dépôt officiel. Pour fournir une copie locale : `-DANKS_JUCE_DIR=C:/JUCE`.

Le workflow `.github/workflows/build-windows.yml` effectue les mêmes opérations et prépare l'artefact `MOOGER-Windows-x64`. Placer le contenu du dossier à la racine d'un dépôt, puis lancer **Build Windows VST3 + Standalone** dans Actions.

Sorties : `build/MOOGER_artefacts/Release/VST3/MOOGER.vst3/` et `build/MOOGER_artefacts/Release/Standalone/MOOGER.exe`. Conserver le bundle VST3 complet pour l'installation dans `C:/Program Files/Common Files/VST3/`.

## Tests DSP sans JUCE

```sh
cmake -S . -B build-dsp -DANKS_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

Voir TESTS.md pour les résultats et leurs limites.

## Compatibilité et limites

Les anciens identifiants de paramètres et les quatorze indices de prises V0.2 sont conservés. Les identifiants techniques du plugin et la racine XML ANKSModularState sont également conservés pour préparer la reprise des anciens états ; la reprise effective dans un DAW reste à tester. Tous les paramètres absents d'un ancien état reçoivent leurs valeurs par défaut, notamment DRUM RUN = arrêt pour V0.2/V0.3. Un ancien patch branché directement VCO 1 → filtre conserve ce trajet et contourne le nouveau mixer.

Le rack reste fixe : une voix mélodique monophonique et une voix de percussion indépendante, horloges internes et routage prédéfini. Les modules ne sont pas encore ajoutables ou déplaçables. Le panneau de modulation, la section sous-harmonique, le wavefolder et la génération de séquences restent à développer. Pas de synchronisation au transport du DAW ni de synchronisation dure entre les VCO. La FM est disponible dans la voix de percussion seulement.

Les commandes sans courbe explicite utilisent désormais une courbe linéaire : cela évite la perte de précision observée à l'ouverture de l'éditeur sur la résonance et le sustain. Les valeurs physiques sauvegardées sont conservées ; les anciennes automations enregistrées en valeurs normalisées pour ces commandes restent à vérifier dans le DAW. Les affichages numériques utilisent une précision adaptée à chaque commande.

Les gains du mixer sont lissés. Cela ne garantit pas une absence de clics lors des changements de forme d'onde, des débranchements ou de tous les autres paramètres.

JUCE : AGPLv3 ou licence commerciale selon les conditions de son éditeur. Les anciennes références HTML sont conservées sous Reference à titre historique.
