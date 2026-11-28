# MOOGER V0.6 — vérifications

## Localement sur Linux

- CMake Release sans JUCE : 6/6 tests réussis (quatre historiques, nouveau rack et catalogue).
- Banque : 16 types, 32 instances, refus du dépassement, contrôle des identifiants et du catalogue JSON.
- À 44,1/48/96 kHz : starter huit pas audible, deux VCO accordés indépendamment, deux LFO/enveloppes indépendants, déplacement sans changement d'un échantillon, priorité MIDI et relâchement, boucles audio via filtre/delay, tous les types instanciables, reconfiguration et reprise après prepare.
- Compteur de `new` pendant le rendu : zéro allocation C++ dans le moteur de graphe.
- Publication concurrente : 10 000 modèles, aucun instantané incohérent observé dans ce test.
- AddressSanitizer + UndefinedBehaviorSanitizer sur le test du nouveau moteur : réussite.
- Vérification syntaxique C++17 des sources JUCE et des nouveaux tests d'intégration.

## Prévu dans la compilation Windows

- VST3, Standalone et sept tests CTest.
- Tests du processeur classique conservés et test du rack libre à trois fréquences et quatre tailles de blocs.
- Duplication, remplacement, déplacement et suppression de modules/câbles ; sauvegarde/restauration des instances, paramètres et positions ; refus d'un type de module inconnu sans destruction du rack courant.
- Événements MIDI à leur offset dans le bloc, ouverture avant/arrière sans changement des réglages, absence de commandes musicales visibles à l'arrière.
- Gestionnaires UI de déplacement, dépôt depuis la banque et clics de connexion/déconnexion des prises.
- Captures des faces avant/arrière et de la taille réduite.

Ces vérifications ne remplacent pas le scan et l'écoute dans Ableton Live 12, la mesure CPU en DAW et les essais d'ergonomie réels. Les anciens patches sont relus dans le mode classique, sans conversion automatique en graphes libres.
