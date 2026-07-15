# Pokemon FireRed: Pokedex Tracker

This is a ROM hack of Pokemon FireRed (built using the 'pokefirered' decompilation project) that includes an in-game Pokedex Tracker Screen to help players track uncaught, caught, and search-filtered Pokemon, as well as their location with dynamic mapping on their Town/Sevii Island Maps.

## Features

Pokedex Tracker Screen: Custom UI accessible in-game to see which Pokemon you have caught and still need to catch.

Search/Filter Feature: Interactive search bar and status filters (All, Caught, Uncaught) to quickly sort and find Pokemon within the list.

Location Mapping: By selecting a Pokemon and pressing A on their LOCATION button, the Town Map is opened. On the Town Map, sprite icons appear everywhere the Pokemon can be found in order to make it easier for the user to catch all Pokemon.

Map Warnings: Warnings appear if users try to identify locations of Pokemon before having the Town/Sevii Island Maps. 


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
