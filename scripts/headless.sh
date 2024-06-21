#!/bin/bash

GHIDRA=C:/soft/ghidra_10.3.3_PUBLIC/support/analyzeHeadless
PROJECT=tc30
WD=$(pwd)

echo "=== Analyzing preloaded objects"
$GHIDRA "${WD}" "${PROJECT}" -process "${PROJECT}:/*" -recursive
