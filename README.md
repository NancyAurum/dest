# DeSt
WIP decompilation of the 1993 AD&D Stronghold game for DOS.

The project's goal is recovering original code in a form compilable by Borland C++ 3.0.

![Dragon Image](https://github.com/NancyAurum/dest/blob/main/dragon.png)


# Milestones
* Decompiled small part of the init code and mapped most of the asset init code.
* Recovered the raw asset files (LBMs, VOCs, etc), which can be assembled back into the STRONG.DAT stash file by going to assets and typing `st.exe grab`. The LBM files can be edited with [Deluxe Paint](https://winworldpc.com/product/deluxepaint/2x). These are PBM LBMs, so not many program support them.


# TODO

* Fix skylines palettes and and entity frame rect markers, which are used to determine sprite centering.
* Fix RLE encoding for PBM LBM files.
* Decompile the rest of the game, starting with these huge assets init routines.


# Development

gpr - the Ghidra project file

tools/ungrab - utility which recovers files from the English STRONG.DAT.

tools/mzap - utility which converts the Borland FBOV overlayed STRONG.EXE into a plain MZ format loadable by Ghidra and IDA.

See doc/journey.txt for the explanation on how to prepare Ghidra.

See doc/switchfix.txt on how to fix Ghidra's switch decompilation.
