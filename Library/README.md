# Banque de modules MOOGER

`modules.json` est le miroir généré du registre exécutable `Source/ModuleRegistry.h`. Il décrit les types, les plages de paramètres et les prises. Signal : 0 = audio, 1 = CV, 2 = pitch MIDI, 3 = gate.

La banque est intégrée dans le plugin ; les modules et réglages propres à un rack sont enregistrés dans son fichier `.mooger` et dans l'état du DAW. Ce fichier de catalogue n'est pas un moteur de synthèse interprété.

Régénération : `mooger_rack --catalog > Library/modules.json`. Contrôle : `mooger_rack --check-catalog Library/modules.json`.
