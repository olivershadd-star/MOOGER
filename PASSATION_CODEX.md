# Passation — MOOGER V0.6, rack libre

Architecture nouvelle pour les projets neufs : `ModuleRegistry.h` (16 types, miroir Library/modules.json), `RackModel.h` (32 slots, 128 câbles, UID et disposition), `RackEngine.h` (états DSP par instance et graphe audio/CV), `RackProcessor.cpp` (édition, publication du graphe, sérialisation), `FreeRackEditor.*` (banque, déplacement/duplication/remplacement/suppression, face avant et face arrière).

Les contrôles du rack sont 512 paramètres normalisés APVTS, ajoutés après les paramètres historiques. Ils sont liés aux emplacements, pas à des identifiants créés dynamiquement dans le DAW. Le nom/sens de l'automation dépend donc du module installé dans le slot.

Les threads qui éditent le modèle sont sérialisés côté état/UI. Le callback audio reçoit un instantané POD par triple buffer, ne verrouille pas le modèle et n'alloue pas dans le moteur du graphe. Les buffers de delay sont réservés à prepareToPlay. Le tri des dépendances est indépendant des positions ; les arcs de retour lisent l'échantillon précédent. Paramètres et sorties sont bornés, sortie avec rejet DC et limitation douce.

État XML version 6, racine ANKSModularState conservée, sous-arbre FreeRack. Préservation de la compatibilité V0.5 : un état ancien active le moteur et l'éditeur classiques. Un bouton ouvre un nouveau rack libre, sans conversion automatique du vieux patch. L'ancien moteur et ses tests sont conservés.

Le nouvel éditeur affiche uniquement les commandes à l'avant et les prises/câbles à l'arrière. Retourner le rack ne change pas le modèle. Le geste de déplacement échange les emplacements occupés ; une duplication copie les réglages mais pas les câbles. Retirer une instance retire ses câbles. Remplacer conserve les indices de prises encore valides.

Tests : moteurs historiques, nouveau graphe, catalogue, instances VCO/LFO/enveloppes indépendantes, déplacement sans changement de phase, MIDI, boucles, absence de new pendant le rendu, publication concurrente. Tests JUCE : état complet, presets invalides, banque glisser-déposer, geste de déplacement, clics de câblage, faces sans commandes visibles à l'arrière, captures Windows.

La publication automatique des versions dans Releases ajoutée au dépôt est conservée. Le chargement, l'écoute et l'ergonomie dans Ableton restent à confirmer. Voir TESTS.md pour les résultats effectifs.
