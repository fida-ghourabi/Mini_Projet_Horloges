@echo off
title Lancement des 4 programmes scalaire

start "Prog1Scalar" cmd /k "javac Prog1Scalar.java && java Prog1Scalar"
start "Prog2Scalar" cmd /k "javac Prog2Scalar.java && java Prog2Scalar"
start "Prog3Scalar" cmd /k "javac Prog3Scalar.java && java Prog3Scalar"
start "Prog4Scalar" cmd /k "javac Prog4Scalar.java && java Prog4Scalar"

echo Tous les processus ont ete lances dans des fenetres separees.
exit
