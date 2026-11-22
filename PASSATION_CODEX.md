# Passation — MOOGER V0.4

Développement natif C++17/JUCE. Cœur mélodique V0.3 conservé, ajout d'une voix rythmique inspirée du rôle du DFAM, sans prétendre émuler ses circuits.

- PercussionEngine.h : deux oscillateurs, triangle/carré, bruit déterministe, FM, filtre TPT, enveloppes amplitude/hauteur et huit pas pitch/vélocité. Pas d'allocations ou verrous dans ce moteur.
- SynthEngine.h : deux nouvelles prises ajoutées après les 19 existantes ; sortie percussion routable, bus indépendant vers le master. Dix câbles par défaut. Indices historiques conservés.
- PluginProcessor.* : 31 paramètres ajoutés à la fin de la liste, migration des IDs manquants vers leurs défauts, état XML version 4 avec racine historique inchangée. DRUM RUN démarre seulement la percussion ; HIT est un déclenchement indépendant.
- PluginEditor.* : troisième rangée percussion, huit hauteurs/vélocités, commande LINK BPM, redimensionnement proportionnel du rack et de ses prises.
- Tests/Percussion.cpp : séquenceur, silence, patching, enveloppes, clocks et extrêmes aux trois fréquences.
- Tests/Processor.cpp : traitement JUCE aux trois fréquences et quatre tailles de blocs, sauvegarde des paramètres/câbles, migration V0.3, HIT, captures du rack à 1120×950 et 784×665.
- Workflow Windows : VST3 + EXE, quatre tests et artefact séparé MOOGER-Rack-Preview.

LINK BPM partage uniquement la valeur du BPM. Il ne verrouille pas la phase, le transport Ableton ou le départ des deux séquenceurs. STOP laisse finir le son. Vélocité zéro laisse finir le coup précédent.

Les fonctions de la voix rythmique sont une implémentation originale : filtre TPT, enveloppes et plage de paramètres propres à MOOGER, pas une copie DFAM.

Reste à vérifier après build Windows : scan et écoute dans Ableton Live 12, ergonomie, sauvegarde d'un vrai projet DAW et charge CPU. La fidélité sonore à un Moog matériel n'a pas été mesurée. Le rack dynamique ajoutable/déplaçable reste à développer.
