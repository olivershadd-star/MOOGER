<div align="center">

# ANKS LABS / MOOGER
### SEMI-MODULAR SYNTH · PATCHING · CV MODULATION

![Status](https://img.shields.io/badge/STATUS-PROTOTYPE_V0.5-EF343E?style=for-the-badge&labelColor=10151E)
![Audio](https://img.shields.io/badge/AUDIO-DEVELOPMENT-19B9FF?style=for-the-badge&labelColor=10151E)

**BUILD. BREAK. REWIRE. LISTEN.**

[![ANKS LABS](https://img.shields.io/badge/ANKS_LABS-GITHUB_PROFILE-19B9FF?style=for-the-badge&labelColor=10151E)](https://github.com/olivershadd-star)

</div>

---

## 01 // LE PROJET

**MOOGER** est une création originale d'ANKS Labs : un synthétiseur semi-modulaire natif inspiré par l'approche analogique, avec deux VCO, patching virtuel, VCF/VCA, séquenceur huit pas, contrôles CV et une voix de percussion indépendante. C'est un laboratoire de synthèse, **pas** une émulation de circuits Moog.

> **Statut : prototype v0.5 / test build.** La compilation Windows et les tests automatisés documentés ont réussi ; l'écoute et l'intégration dans Ableton restent à confirmer. Ne pas considérer ce build comme stable.

**[Sources & documentation](https://github.com/olivershadd-star/MOOGER)** · **[Builds de test / GitHub Actions](https://github.com/olivershadd-star/MOOGER/actions)**

---

## Documentation technique existante — MOOGER v0.5


Projet ANKS Labs, C++17/JUCE, VST3 Windows x64 et application autonome. Cœur inspiré du Mother-32, second oscillateur, mixer voix rythmique indépendante et laboratoire de modulation CV. La V0.5 ajoute le panneau de modulation ; compilation Windows et cinq tests automatisés réussis (voir TESTS.md). Le chargement et l'écoute dans Ableton restent à vérifier. Le moteur est un synthétiseur natif original ; aucune mesure de fidélité au circuit Mother-32 n'a été effectuée.

[Télécharger la VST3 et l’EXE Windows x64](https://github.com/olivershadd-star/MOOGER/actions/runs/44973665441/artifacts/14734628815) · [Captures des deux vues du rack](https://github.com/olivershadd-star/MOOGER/actions/runs/44973665441/artifacts/14734588908)

## Cette version

- Rack fixe de trois rangées, dernière rangée commutable entre modulation et percussion, redimensionnable de 784 × 665 à 1344 × 1140, façade sombre, repères crème et commandes ambre.
- Cœur : séquenceur huit pas, VCO 1, filtre résonant, VCA, ADSR, LFO et sortie limitée doucement.
- VCO 2 : sinus, triangle, dent de scie, carré ; accord de -24 à +24 demi-tons, réglage fin de -50 à +50 cents ; entrée pitch et sortie audio indépendantes.
- Mixer : deux entrées patchables, deux atténuateurs 0–1, niveau de sortie 0–1 ; lissage des trois gains sur une constante de temps de 10 ms.
- Par défaut les deux VCO suivent le séquenceur/MIDI ; VCO 2 est désaccordé de +7 cents. Ils alimentent le mixer (65 % / 35 %, sortie à 80 %), puis le filtre, le VCA et la sortie.
- Dix-sept câbles initiaux, quarante et une prises. Les entrées mixer acceptent VCO 1 ou VCO 2. Le mixer peut alimenter le filtre, le VCA ou directement la sortie ; les boucles de rétroaction sont refusées.
- Accord VCO 1 et VCO 2 mis à jour pendant une note tenue, avec glissement de fréquence de 5 ms.

## Panneau de modulation V0.5

Le bouton **DRUM VIEW / MOD VIEW** change la dernière rangée sans arrêter ni modifier le traitement audio. Le panneau de modulation apparaît par défaut. Les prises et câbles de la vue cachée restent actifs.

- LFO indépendant 0,05–30 Hz, sorties triangle et carré simultanées.
- Source de bruit indépendante ; sample & hold capturé sur front montant du carré ou du gate SEQ/MIDI, ou avec le bouton **SAMPLE**.
- Deux atténuateurs 0–1 et mélangeur CV quatre entrées. MIX 1 et MIX 2 règlent les deux premières entrées ; les entrées 3 et 4 sont à gain unité. OFFSET décale le mélange, CV LEVEL règle son niveau. MIX + et MIX - fournissent des sorties opposées.
- LAG possède des constantes de temps de montée et descente séparées (1 ms–2 s). À une constante de temps, environ 63 % de la variation a été parcourue.
- Les CV sont normalisés entre -1 et +1 ; la somme du mixer est limitée à cette plage. Il ne s'agit pas de tensions matérielles.
- Les gains CV sont lissés sur 5 ms. Les nouvelles commandes de filtre et de VCA ont un lissage de 2 ms ; la hauteur conserve le glissement de 5 ms.

La chaîne utilitaire est prébranchée :

```text
NOISE → S&H IN ; LFO SQR → CLOCK IN
S&H OUT → ATT1 IN ; ATT1 OUT → MIX IN1
LFO CV du cœur → ATT2 IN ; ATT2 OUT → MIX IN2
MIX + → LAG IN
```

Aucune destination sonore n'est ajoutée automatiquement : le patch précédent conserve son son. Pour essayer, cliquer **RUN / STOP**, puis **ANIMATE FILTER** : LAG OUT se branche sur VCF CUTOFF CV. Les commandes RISE/FALL et FILTER Hz règlent la douceur et l'amplitude du mouvement.

Autres essais : S&H OUT → PITCH CV du VCO 2 pour des hauteurs par paliers ; LFO TRI → AMP CV pour une modulation d'amplitude ; MIX - pour inverser un mouvement. PITCH DEPTH règle la profondeur commune des deux nouvelles entrées de hauteur (0–24 demi-tons), FILTER Hz celle des nouvelles sources vers le filtre (0–8000 Hz), AMP DEPTH celle des nouvelles sources vers le VCA. Les branchements LFO/ENV historiques conservent leur comportement et leurs réglages.

À AMP DEPTH = 1, une nouvelle source CV à -1 ferme le VCA, 0 donne un demi-niveau et +1 l’ouvre. Le CV du VCA remplace son enveloppe quand on change son câble : une modulation peut donc ouvrir le VCA et produire un drone même lorsque le séquenceur est arrêté.

Le chemin CV est dirigé : sources → S&H → atténuateurs → mixer → lag → destinations. Les liaisons vers des étapes précédentes et les sources audio sur les entrées CV sont refusées. Toute sortie peut alimenter plusieurs entrées compatibles, sans module multiple supplémentaire. Cette version s'inspire des fonctions du CP-251 ; ses plages et son routage sont propres à MOOGER.

## Voix de percussion

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
cmake --build build --config Release --target MOOGER_VST3 MOOGER_Standalone anks_audio_smoke mooger_modules mooger_percussion mooger_modulation mooger_processor_tests
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

Les anciens identifiants de paramètres et les quatorze indices de prises V0.2 sont conservés. Les identifiants techniques du plugin et la racine XML ANKSModularState sont également conservés pour préparer la reprise des anciens états ; la reprise effective dans un DAW reste à tester. Tous les paramètres absents d'un ancien état reçoivent leurs valeurs par défaut, notamment DRUM RUN = arrêt pour V0.2/V0.3. Les indices et paramètres V0.4 sont conservés, et un état V0.4 ne reçoit pas de nouvelle modulation sur ses destinations sonores. Un ancien patch branché directement VCO 1 → filtre conserve ce trajet et contourne le nouveau mixer.

Le rack reste fixe : une voix mélodique monophonique et une voix de percussion indépendante, horloges internes et routage prédéfini. Les modules ne sont pas encore ajoutables ou déplaçables. La section sous-harmonique, le wavefolder et la génération de séquences restent à développer. Pas de synchronisation au transport du DAW ni de synchronisation dure entre les VCO. La FM est disponible dans la voix de percussion seulement.

Les commandes sans courbe explicite utilisent désormais une courbe linéaire : cela évite la perte de précision observée à l'ouverture de l'éditeur sur la résonance et le sustain. Les valeurs physiques sauvegardées sont conservées ; les anciennes automations enregistrées en valeurs normalisées pour ces commandes restent à vérifier dans le DAW. Les affichages numériques utilisent une précision adaptée à chaque commande.

Les gains du mixer sont lissés. Cela ne garantit pas une absence de clics lors des changements de forme d'onde, des débranchements ou de tous les autres paramètres.

JUCE : AGPLv3 ou licence commerciale selon les conditions de son éditeur. Les anciennes références HTML sont conservées sous Reference à titre historique.
