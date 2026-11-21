# ANKS Modular Lab / Mother 01 — V0.1

## Objectif
Un rack semi-modulaire pédagogique, monophonique, jouable dans le navigateur et sans installation.

## Démarrage rapide
1. Double-cliquer sur `index.html` (Windows : Chrome ou Edge de préférence).
2. Cliquer sur **Lancer le séquenceur**.
3. Modifier les notes des huit pas, puis la forme d'onde du VCO et la coupure du VCF.
4. Pour moduler le filtre : cliquer la prise **CV** du LFO, puis **CUTOFF CV** du VCF. On peut également tirer le câble à la souris.
5. Cliquer sur un câble pour le retirer ; utiliser **Rack d'origine** pour revenir aux six câbles initiaux.
6. Exporter un patch en JSON pour le recharger ensuite.

## Le circuit par défaut
SEQ PITCH → VCO PITCH ; SEQ GATE → ENV GATE ; VCO AUDIO → VCF AUDIO ; VCF AUDIO → VCA AUDIO ; ENV CV → VCA CV ; VCA AUDIO → OUT AUDIO.

## Architecture technique
Application indépendante dans un seul fichier `index.html`. HTML, CSS et JavaScript natifs ; synthèse en temps réel via `AudioContext`, `OscillatorNode`, `BiquadFilterNode`, `GainNode` et `DynamicsCompressorNode`. Les prises AUDIO reconfigurent effectivement les connexions des nœuds audio. La modulation LFO vers la coupure du filtre utilise une connexion audio-vers-paramètre. Les connexions PITCH, GATE et ENV sont distribuées sous forme d'événements de contrôle qui pilotent les paramètres Web Audio.

C'est un prototype original autonome, **pas un fork ni un portage** du dépôt externe cité ci-dessous. Il sert de premier banc d'essai. La V0.1 n'a pas encore de mise en page de rack libre, de modules ajoutables à la volée, de CV générique, ni d'horloge musicale à scheduling haute précision.

## Évolutions proposées (une seule à la fois)
1. Séparer le moteur audio, les modules, les connexions et la vue en fichiers distincts, puis ajouter des tests automatiques.
2. Ajouter la gestion d'un véritable rack avec insertion/retrait de modules et mémorisation des emplacements.
3. Ajouter un deuxième oscillateur et un mixer pour le mélanger au premier.
4. Ajouter des modulations CV génériques, puis un oscilloscope pédagogique.
5. Ajouter un séquenceur 16 pas, le MIDI et éventuellement une sortie WAV.

## Pistes open source examinées
- https://github.com/craigdanj/Modular-Synth-JS — MIT, HTML autonome, code JavaScript Web Audio sans dépendances d'exécution, projet encore jeune.
- https://github.com/michael-graute/modular-web-syntheziser — MIT, plus avancé, TypeScript, interface patchable.
- https://cdn.inmusicbrands.com/Moog/Mother32/Mother_32_Users_Manual.pdf — référence du principe semi-modulaire à liaisons pré-câblées.

## Publication internet
Glisser un dossier contenant `index.html`, ou le ZIP fourni, sur https://app.netlify.com/drop. Vous recevrez un lien public `netlify.app`. Aucun serveur audio n'est nécessaire, le navigateur fait le calcul du son.

## Sécurité
Le volume démarre coupé, augmente uniquement au lancement, et la sortie est limitée par un compresseur. Commencer avec un volume d'écoute modéré, surtout au casque.
