# MOOGER V0.3 — laboratoire semi-modulaire natif

Projet ANKS Labs, C++17/JUCE, VST3 Windows x64 et application autonome. Première extension du cœur inspiré du Mother-32 : second oscillateur et mixer audio. Les sources ne constituent pas un binaire Windows validé. Le moteur est un synthétiseur natif original ; aucune mesure de fidélité au circuit Mother-32 n'a été effectuée.

## Cette version

- Rack compact fixe de deux rangées, façade sombre, repères crème et commandes ambre.
- Cœur : séquenceur huit pas, VCO 1, filtre résonant, VCA, ADSR, LFO et sortie limitée doucement.
- VCO 2 : sinus, triangle, dent de scie, carré ; accord de -24 à +24 demi-tons, réglage fin de -50 à +50 cents ; entrée pitch et sortie audio indépendantes.
- Mixer : deux entrées patchables, deux atténuateurs 0–1, niveau de sortie 0–1 ; lissage des trois gains sur une constante de temps de 10 ms.
- Par défaut les deux VCO suivent le séquenceur/MIDI ; VCO 2 est désaccordé de +7 cents. Ils alimentent le mixer (65 % / 35 %, sortie à 80 %), puis le filtre, le VCA et la sortie.
- Neuf câbles initiaux, dix-neuf prises. Les entrées mixer acceptent VCO 1 ou VCO 2. Le mixer peut alimenter le filtre, le VCA ou directement la sortie ; les boucles de rétroaction sont refusées.
- Accord VCO 1 et VCO 2 mis à jour pendant une note tenue, avec glissement de fréquence de 5 ms.

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
cmake --build build --config Release --target MOOGER_VST3 MOOGER_Standalone anks_audio_smoke mooger_modules
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

Les anciens identifiants de paramètres et les quatorze indices de prises V0.2 sont conservés. Les identifiants techniques du plugin et la racine XML ANKSModularState sont également conservés pour préparer la reprise des anciens états ; la reprise effective dans un DAW reste à tester. Les paramètres nouveaux absents d'un état V0.2 reçoivent leurs valeurs par défaut. Un ancien patch branché directement VCO 1 → filtre conserve ce trajet et contourne le nouveau mixer.

Le rack reste fixe et monophonique, avec horloge interne et routage prédéfini. Les modules ne sont pas encore ajoutables ou déplaçables. La seconde modulation, l'enveloppe/VCA auxiliaire et la voix de percussion sont les trois prochains compléments proposés ; ils ne sont pas implémentés ici. Pas de synchronisation au transport du DAW, de FM ni de synchronisation dure entre VCO dans cette version.

Les gains du mixer sont lissés. Cela ne garantit pas une absence de clics lors des changements de forme d'onde, des débranchements ou de tous les autres paramètres.

JUCE : AGPLv3 ou licence commerciale selon les conditions de son éditeur. Les anciennes références HTML sont conservées sous Reference à titre historique.
