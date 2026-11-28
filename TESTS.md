# MOOGER V0.6 — vérifications

## Localement sur Linux

- CMake Release sans JUCE : 6/6 tests réussis (quatre historiques, nouveau rack et catalogue).
- Banque : 16 types, 32 instances, refus du dépassement, contrôle des identifiants et du catalogue JSON.
- À 44,1/48/96 kHz : starter huit pas audible, deux VCO accordés indépendamment, deux LFO/enveloppes indépendants, déplacement sans changement d'un échantillon, priorité MIDI et relâchement, boucles audio via filtre/delay, tous les types instanciables, reconfiguration et reprise après prepare.
- Compteur de `new` pendant le rendu : zéro allocation C++ dans le moteur de graphe.
- Publication concurrente : 10 000 modèles, aucun instantané incohérent observé dans ce test.
- AddressSanitizer + UndefinedBehaviorSanitizer sur le test du nouveau moteur : réussite.
- Vérification syntaxique C++17 des sources JUCE et des nouveaux tests d'intégration.

## Windows x64 — compilation réussie

- [Build Windows réussi](https://github.com/olivershadd-star/MOOGER/actions/runs/46117707632), commit `1eab5014de4346527ee9edae3f061d513bd8c58e` : VST3, Standalone et **7/7 tests CTest réussis**, 7,61 s.
- Tests du processeur classique conservés et test du rack libre à trois fréquences et quatre tailles de blocs.
- Duplication, remplacement, déplacement et suppression de modules/câbles ; sauvegarde/restauration des instances, paramètres et positions ; refus d'un type de module inconnu sans destruction du rack courant.
- Événements MIDI à leur offset dans le bloc, ouverture avant/arrière sans changement des réglages, absence de commandes musicales visibles à l'arrière.
- Gestionnaires UI de déplacement, dépôt depuis la banque et clics de connexion/déconnexion des prises.
- Captures des faces avant/arrière et de la taille réduite inspectées : commandes visibles à l’avant, prises et câbles à l’arrière. Un libellé de statut accentué présente un défaut d’encodage visuel sous Windows ; les commandes et le câblage fonctionnent dans les tests.
- [ZIP publié V0.6.0](https://github.com/olivershadd-star/MOOGER/releases/tag/v0.6.0) téléchargé et contrôlé : archive intègre, application et VST3 PE x64, catalogue inclus. SHA-256 du ZIP : `62ad20a8ce133dd6fe81e4a5948fcb86bcaa75418849187354127646b073a343`.

Ces vérifications ne remplacent pas le scan et l'écoute dans Ableton Live 12, la mesure CPU en DAW et les essais d'ergonomie réels. Les anciens patches sont relus dans le mode classique, sans conversion automatique en graphes libres.
