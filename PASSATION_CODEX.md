# Passation — MOOGER V0.5

Ajout demandé : panneau de modulation inspiré des fonctions du CP-251. Moteur original à CV normalisés, pas une émulation de circuit.

ModulationEngine.h : LFO triangle/carré, bruit déterministe, S&H sur front montant et commande SAMPLE, deux atténuateurs, mixer quatre entrées avec offset et sortie inversée, lag à montée/descente indépendantes. Douze paramètres ajoutés à la fin de la liste APVTS.

SynthEngine.h : vingt prises ajoutées après les vingt et une V0.4, anciennes valeurs d'enum conservées. Routage CV acyclique avec rangs d'étapes. Deux entrées de hauteur additionnelles aux VCO, sources utilitaires routables vers filtre/VCA. Dix-sept câbles prébranchés, aucune nouvelle destination sonore connectée par défaut. Les voix V0.4 sont conservées ; tests de sortie identique pour le panneau utilitaire non relié au son.

PluginEditor.* : la dernière rangée affiche MODULATION ou PERCUSSION ; toutes deux traitent l'audio en continu. Les prises de la vue cachée ne réagissent pas aux clics. ANIMATE FILTER connecte LAG OUT au cutoff. Le changement de vue n'est pas un paramètre musical ; la vue MODULATION est utilisée à l'ouverture.

État XML : version 5, racine historique ANKSModularState conservée. Toutes les valeurs de paramètres et connexions sont sauvegardées ; phases LFO et état instantané du S&H ne sont pas sérialisés. Migration de paramètres manquants V0.3/V0.4 vers leurs défauts.

Tests/Modulation.cpp : S&H (fronts/maintien/SAMPLE), gains et inversion du mixer, constantes de temps du lag, préservation du son sans cible, intervalle d'une octave par CV et routage des neuf sorties utilitaires vers quatre cibles aux trois fréquences. Tests/Processor.cpp étendu à la sauvegarde CV, à la migration V0.4 et aux captures des deux vues plus taille réduite.

Le rack reste fixe, les câbles suivent un graphe prédéfini, les horloges ne sont pas synchronisées au transport du DAW. Voir README.md et TESTS.md. Le test final dans Ableton Live 12 reste à réaliser.
