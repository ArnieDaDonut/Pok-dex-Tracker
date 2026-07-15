# Pokemon FireRed: Pokedex Tracker

https://github.com/user-attachments/assets/df75c051-63ea-4d6f-b0d8-6fd672a74e9a

This is a ROM hack of Pokemon FireRed (built using the 'pokefirered' decompilation project) that includes an in-game Pokedex Tracker Screen to help players track uncaught, caught, and search-filtered Pokemon, as well as their location with dynamic mapping on their Town/Sevii Island Maps.

## Features

Pokedex Tracker Screen: Custom UI accessible in-game to see which Pokemon you have caught and still need to catch.

<img width="1512" height="982" alt="Image" src="https://github.com/user-attachments/assets/6248758f-ba56-4535-b950-3aca451f8abc" />

Search/Filter Feature: Interactive search bar and status filters (All, Caught, Uncaught) to quickly sort and find Pokemon within the list.

<img width="1512" height="982" alt="Image" src="https://github.com/user-attachments/assets/6171f456-8d45-4fde-8186-3ce6cde21c55" />

<img width="1512" height="982" alt="Image" src="https://github.com/user-attachments/assets/c965a400-0f61-46ab-9855-3c4a360dfe89" />

Location Mapping: By selecting a Pokemon and pressing A on their LOCATION button, the Town Map is opened. On the Town Map, sprite icons appear everywhere the Pokemon can be found in order to make it easier for the user to catch all Pokemon.

<img width="1512" height="982" alt="Image" src="https://github.com/user-attachments/assets/ac0e1f22-e704-4bab-9d54-3656e89cf09b" />

Map Warnings: Warnings appear if users try to identify locations of Pokemon before having the Town/Sevii Island Maps. 

<img width="1512" height="982" alt="Image" src="https://github.com/user-attachments/assets/e8a1d087-1daf-4c81-ac74-850b19ab499d" />


## AI Disclosure

AI was used to:
- Research on how to code a Pokemon game.
- Set up the repository with the decompilation project.
- Set up base code for the game to edit.
- Debug complex code.
- Assist in building complex features such as the Location button.

## How to Build

Prerequisites:

- devkitRAM - compiler used to build decompilation projects. Follow the official pokefirered installation guide for more information (https://github.com/pret/pokefirered/blob/master/INSTALL.md).

Compiling:

Run make -j4 in your root directory.

This will make and output the modified ROM: pokefirered.gba

Simply load the ROM on an emulator (like VBA) and enjoy!


## Legality

This repository adheres to all copyright and modding guidelines. 
- No ROMS or copyrighted ROM files, assets, or binaries are distributed.
- The code requires a copy of Pokemon FireRed in order to run and create the .gba file to run and play on.
