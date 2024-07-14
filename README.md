# DeSt
WIP decompilation of the 1993 AD&D Stronghold game for DOS.

The project's goal is recovering original code in a form compilable by Borland C++ 3.0.

# Milestones
- Decompiled small part of the init code and mapped most of the asset init code.
- Recovered the raw asset files (LBMs, VOCs, etc), which can be assembled back into the STRONG.DAT stash file by going to assets and typing `st.exe grab`.


# Development:

See dest/gpr for the Ghidra project file

See the sister's devroomm/mzap project for the utility to convert the Borland FBOV overlayed STRONG.EXE into a plain MZ format loadable by Ghidra and IDA.

See dest/doc/journey.txt for the explanation on how to prepare Ghidra.
