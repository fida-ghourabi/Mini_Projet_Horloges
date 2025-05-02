@echo off
title Lancement des processus vectoriels

echo Lancement des processus...   
start "MonitorGUI" cmd /k "javac -encoding UTF-8 MonitorGUI.java && java MonitorGUI"

start "Processus P1" cmd /k "javac Prog1Vectoriel.java && java Prog1Vectoriel"
start "Processus P2" cmd /k "javac Prog2Vectoriel.java && java Prog2Vectoriel"
start "Processus P3" cmd /k "javac Prog3Vectoriel.java && java Prog3Vectoriel"
start "Processus P4" cmd /k "javac Prog4Vectoriel.java && java Prog4Vectoriel"

echo Tous les processus sont en cours d'exécution.
pause
