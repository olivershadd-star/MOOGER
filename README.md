<div align="center">

# ANKS LABS / MOOGER
### FREE MODULAR RACK · FRONT / BACK · AUDIO + CV

![Status](https://img.shields.io/badge/STATUS-PROTOTYPE_V0.6-EF343E?style=for-the-badge&labelColor=10151E)
![Audio](https://img.shields.io/badge/AUDIO-DEVELOPMENT-19B9FF?style=for-the-badge&labelColor=10151E)

**BUILD. BREAK. REWIRE. LISTEN.**

[![ANKS LABS](https://img.shields.io/badge/ANKS_LABS-GITHUB_PROFILE-19B9FF?style=for-the-badge&labelColor=10151E)](https://github.com/olivershadd-star)

</div>

---

## ⬇ TÉLÉCHARGER ET INSTALLER — DOWNLOAD

[![TÉLÉCHARGER LA DERNIÈRE VERSION](https://img.shields.io/github/v/release/olivershadd-star/MOOGER?include_prereleases&sort=date&display_name=tag&style=for-the-badge&logo=github&logoColor=F7FAFF&label=T%C3%89L%C3%89CHARGER&labelColor=10151E&color=EF343E)](../../releases) [![TOUTES LES VERSIONS](https://img.shields.io/badge/TOUTES_LES-VERSIONS-F5C343?style=for-the-badge&labelColor=10151E)](../../releases)

1. Clique sur **TÉLÉCHARGER**, puis, dans la rubrique *Assets*, télécharge `MOOGER-Windows-x64-X.Y.Z.zip` (X.Y.Z = numéro de version : prends toujours le plus grand).
2. Clic droit sur le ZIP → **Extraire tout**.
3. **Plugin pour Ableton** : copie le dossier `VST3\MOOGER.vst3` dans `C:\Program Files\Common Files\VST3`.
4. Dans Live : **Préférences → Plug-ins → Dossier système VST3 = On**, puis **Relancer l'analyse**.
5. **Sans Ableton** : lance `Application\MOOGER.exe`. Aucune installation.

*Windows peut afficher « Windows a protégé votre ordinateur » : clique sur **Informations complémentaires → Exécuter quand même** (programme gratuit non signé, c'est normal).*

**EN —** Click **DOWNLOAD**, get `MOOGER-Windows-x64-X.Y.Z.zip` from *Assets*, extract it and copy `VST3\MOOGER.vst3` to `C:\Program Files\Common Files\VST3`, then rescan plug-ins. Or run `Application\MOOGER.exe` without any DAW.

---

## 01 // LE PROJET

**MOOGER V0.6** est un rack modulaire natif C++/JUCE. Chaque module vient d'une banque intégrée et devient une instance indépendante : on peut ajouter, dupliquer, retirer, remplacer et déplacer ses modules, puis définir le trajet du son avec les câbles.

**RETOURNER LE RACK** bascule entre deux faces des mêmes machines : commandes à l'avant, prises et câbles à l'arrière. Le changement de face ne modifie ni le son, ni le patch, ni la disposition.

La V0.6 est une version de développement. Voir TESTS.md pour les vérifications effectuées ; le chargement, l'écoute et l'ergonomie dans Ableton restent à vérifier sur une machine équipée du DAW.

## 02 // CONSTRUIRE SON RACK

1. Dans le menu de départ, choisir **Rack vide**, **Initial séquenceur** ou **Initial MIDI**, puis **CRÉER**. Le rack vide est silencieux.
2. Prendre un type dans la **BANQUE DE MODULES** et le glisser dans le rack, ou double-cliquer son nom / cliquer **AJOUTER**. Plusieurs exemplaires du même type sont autorisés.
3. Déplacer une machine en glissant son en-tête. Les emplacements s'échangent si l'on dépose un module sur un autre.
4. **2x** duplique les réglages dans une instance indépendante, sans recopier les câbles. **X** retire le module et ses câbles. Pour remplacer une machine : sélectionner son en-tête, choisir un type dans la banque, puis **REMPLACER**. Les numéros de prises encore valides sont conservés ; leur sens peut changer avec le nouveau type.
5. Cliquer **RETOURNER LE RACK**. Choisir une sortie, puis une entrée, ou glisser directement entre les prises. Cliquer une entrée seule retire son câble. Une sortie peut alimenter plusieurs entrées.
6. Retourner à l'avant pour accéder aux réglages. Le bouton **SEQ RUN / STOP** commande les séquenceurs et les percussions internes ; les oscillateurs et LFO restent continus, comme dans un modulaire. Le volume du module SORTIE AUDIO contrôle le niveau final.
7. **ENREGISTRER / OUVRIR** conserve le rack complet dans un fichier `.mooger`. Modules, réglages, positions et connexions sont aussi enregistrés dans le projet du DAW.

Un VCO et un séquenceur ne sont pas la même fonction : le VCO produit la vibration ; le séquenceur fournit les hauteurs et les gates qui le commandent.

### Exemple simple, entièrement câblé

```text
SEQ 8 PITCH → VCO PITCH
SEQ 8 GATE → ENV ADSR GATE
VCO AUDIO → VCF AUDIO
VCF LOW PASS → VCA AUDIO
ENV ADSR ENVELOPPE → VCA GAIN CV
VCA AUDIO → SORTIE AUDIO LEFT / MONO
```

Pour jouer au clavier ou avec un clip Ableton, choisir **Initial MIDI**. Le module MIDI expose les notes, le gate et la vélocité de l'entrée MIDI commune du plugin. Les note-off sont gérés sans relâcher une autre note encore tenue.

Pour un retour d'effet : alimenter un MIX AUDIO avec une source et le retour d'un DELAY, envoyer le mélange dans l'effet, puis vers SORTIE AUDIO. Les boucles sont permises ; le moteur retarde les liaisons de retour d'un échantillon pour conserver un calcul causal et borne les signaux.

## 03 // BANQUE INTÉGRÉE

| Module | Fonction |
|---|---|
| VCO | Sinus, triangle, dent de scie, carré, accord et modulation audio/CV |
| VCF | Passe-bas et passe-haut résonants, cutoff modulable |
| VCA | Amplitude contrôlée par CV, niveau et bias |
| ENV ADSR | Enveloppe indépendante déclenchée par gate |
| LFO | Triangle, carré et sinus simultanés, profondeur et offset |
| SEQ 8 | Huit hauteurs, tempo interne ou horloge externe, pitch et gate |
| MIX AUDIO | Mélangeur audio quatre entrées |
| SORTIE AUDIO | Sortie stéréo du rack vers le plugin, volume et panoramique |
| BRUIT | Source de bruit audio ou de modulation |
| SAMPLE & HOLD | Capture d'un signal sur horloge ou bouton IMPULSION |
| ATTENUVERT. | Gain CV/audio positif ou inversé |
| LISSAGE | Montée et descente indépendantes |
| PERCUSSION | Voix rythmique à deux oscillateurs et bruit, déclenchement externe ou séquence interne |
| DELAY | Retard jusqu'à 1,5 s, feedback et mélange sec/effet |
| MIDI | Source de notes, gate et vélocité du DAW/clavier |
| MIX CV | Quatre signaux, offset, somme et sortie inversée |

Le catalogue est défini dans `Source/ModuleRegistry.h`, miroir lisible dans `Library/modules.json`. Le plugin embarque la banque ; il fonctionne sans base distante, compte ou réseau. Ajouter du texte dans le JSON ne crée pas un nouveau moteur DSP : de nouveaux types s'ajoutent dans le code et sont compilés. Ce n'est pas un chargeur de VST tiers.

## 04 // COMPATIBILITÉ ET LIMITES

- Première capacité : **32 modules et 128 câbles**, une source par entrée. Vue défilante et emplacements sur grille.
- Chaque instance a son identifiant, ses paramètres, ses oscillateurs/enveloppes/filtres/buffers. Déplacer un module ne modifie pas son ordre de traitement ni son état sonore.
- Tous les signaux des prises peuvent être essayés ensemble. Le code applique des limites et une conversion de hauteur : une sortie PITCH transporte une note MIDI ; un autre signal branché sur PITCH module autour de MIDI 48 à raison de 12 demi-tons par unité. MOD / FM est une entrée supplémentaire de modulation de hauteur.
- Le moteur traite les chemins acycliques dans l'ordre des dépendances ; les arcs de retour d'une boucle utilisent l'échantillon précédent. Les retards explicites ont leurs propres buffers. Les changements structurels bénéficient d'une transition courte ; cela ne garantit pas l'absence de tout transitoire dans un patch expérimental.
- Les automations du rack libre sont des contrôles normalisés attachés aux emplacements (16 contrôles réservés par emplacement). Remplacer ou réutiliser un emplacement peut changer la signification de son automation ; les automations V0.5 conservent leurs identifiants historiques.
- Les anciens états V0.2–V0.5 sont relus dans leur moteur classique. Ils ne sont pas convertis automatiquement en graphes libres. Le bouton **NOUVEAU RACK LIBRE** permet de repartir sur la nouvelle architecture. Voir `Docs/LegacyV05.md` pour les anciennes commandes.
- Les presets `.mooger` du nouvel éditeur sont ceux du rack libre. Un ancien projet chargé par le DAW peut nécessiter de fermer et rouvrir l'éditeur pour montrer les commandes classiques.
- Pas encore de synchronisation au transport/PPQ du DAW, de polyphonie par voix, d'import de modules tiers ou de création de nouveaux moteurs par l'utilisateur. Les horloges et le câblage restent internes au plugin.

## 05 // COMPILER ET TESTER

Visual Studio 2022, charge Développement Desktop en C++, CMake 3.22+. Exécuter `compile_windows.bat`. JUCE 8.0.17-security est téléchargé à la configuration ; fournir `-DANKS_JUCE_DIR=C:/JUCE` pour une copie locale. La publication automatique dans Releases est conservée.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target MOOGER_VST3 MOOGER_Standalone anks_audio_smoke mooger_modules mooger_percussion mooger_modulation mooger_rack mooger_processor_tests
ctest --test-dir build -C Release --output-on-failure
```

Tests du moteur sans JUCE :

```sh
cmake -S . -B build-dsp -DANKS_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
```

Pour régénérer la banque JSON après modification du registre : `mooger_rack --catalog > Library/modules.json`. Le test MOOGER_Catalog vérifie sa correspondance avec la banque du plugin.

Projet original ANKS Labs, inspiré par la synthèse modulaire et l'organisation avant/arrière des racks. Pas une émulation de circuits Moog. JUCE est disponible sous AGPLv3 ou licence commerciale selon ses conditions.
