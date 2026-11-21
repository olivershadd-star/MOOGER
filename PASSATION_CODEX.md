# Passation — MOOGER V0.3

Le nom produit est MOOGER. Cette version dérive de ANKS V0.2 corrigé et ajoute uniquement les deux premiers compléments validés : VCO 2 et mixer audio. Les trois autres compléments restent une feuille de route.

Sources : SynthEngine.h (DSP), PluginProcessor.* (paramètres/état/MIDI), PluginEditor.* (rack fixe natif), CMakeLists.txt et workflow Windows.

VCO2 : quatre ondes, tune2, fine2, wave2. Mixer : mix1, mix2, mixLevel, gains lissés. Les cinq nouveaux ports sont ajoutés après outputIn pour préserver les anciens indices. Neuf câbles par défaut. Le routage reste acyclique et prédéfini.

Les anciens noms de classes, options CMake ANKS et identifiants plugin sont conservés pour limiter les changements techniques. Produit et cibles CMake : MOOGER. Les états utilisent encore ANKSModularState ; version=3. Les six nouveaux paramètres reçoivent leurs défauts quand ils manquent dans un ancien état.

Vérifications effectuées : compilation et CTest du DSP Release, contrôle syntaxique JUCE du processeur et de l'éditeur. Voir TESTS.md. Le scan VST3, la compatibilité d'état, le rendu GUI et l'écoute dans Ableton restent à vérifier sous Windows. Aucun binaire Windows validé n'est fourni.

Prochaine étape concrète : exécuter le workflow Windows, ouvrir dans Ableton Live 12, vérifier le patch neuf, puis VCO2/mixer et sauvegarde/restauration. L'architecture ajout/déplacement de modules reste à développer avant d'annoncer un rack extensible.
