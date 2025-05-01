@echo off
title Lancement des processus vectoriels
echo Compilation des fichiers Java...
javac MessageVectoriel.java Prog1Vectoriel.java Prog2Vectoriel.java Prog3Vectoriel.java Prog4Vectoriel.java

echo Lancement des processus...

start "Processus P1" cmd /k "java Prog1Vectoriel"
timeout /t 1 >nul
start "Processus P2" cmd /k "java Prog2Vectoriel"
timeout /t 1 >nul
start "Processus P3" cmd /k "java Prog3Vectoriel"
timeout /t 1 >nul
start "Processus P4" cmd /k "java Prog4Vectoriel"

echo Tous les processus sont en cours d'exécution.
pause
