#!/bin/bash

GHIDRA=C:/soft/ghidra_10.3.3_PUBLIC/support/analyzeHeadless
PROJECT=fidb
WD=$(pwd)
TMP=${WD}/tmp
INPUT=${WD}/input
RESULT=result.fidb
SCRIPTS=${WD}/scripts
DUPLOG=${TMP}/duplicates.log
CPU="x86:LE:16:Real Mode"
COMPILER=default

mkdir -p "${TMP}"

echo "=== Loading from ./input into ${TMP}/${PROJECT}"
$GHIDRA "${TMP}" "${PROJECT}" \
        -import "${INPUT}" -recursive \
        -noanalysis \
        -processor "${CPU}" \
        -cspec "${COMPILER}"
#$GHIDRA "${TMP}" "${PROJECT}" \
#        -import "${INPUT}"/* \
#        -noanalysis \
#        -processor "${CPU}" \
#        -cspec "${COMPILER}"


echo "=== Analyzing loaded objects"
$GHIDRA "${TMP}" "${PROJECT}" \
        -process "{PROJECT}:/*" -recursive \
        -scriptPath "${SCRIPTS}" \
        -preScript "FunctionIDHeadlessPrescriptMinimal.java" \
        -postScript "FunctionIDHeadlessPostscript.java"

touch "${DUPLOG}"

echo "=== Generating ${TMP}/${RESULT}"
$GHIDRA "${TMP}" "${PROJECT}" \
        -noanalysis \
        -scriptPath "${SCRIPTS}" \
        -preScript "AutoCreateMultipleLibraries.java" \
        "${DUPLOG}" "${TMP}" "${RESULT}" \
        "/" "${CPU}"
