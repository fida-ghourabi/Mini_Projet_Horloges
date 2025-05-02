@echo off
setlocal

:: Compilation des fichiers Java
echo Compilation...
javac MessageMatriciel.java Prog1Matricielle.java Prog2Matricielle.java Prog3Matricielle.java Prog4Matricielle.java
if errorlevel 1 (
    echo Erreur de compilation.
    pause
    exit /b
)

:: Lancement des 4 programmes dans des fenêtres séparées
echo Lancement des processus...
start "MonitorGUI" cmd /k "javac -encoding UTF-8 MonitorGUI.java && java MonitorGUI"
start "MessageMatriciel" cmd /k "javac MessageMatriciel.java && java MessageMatriciel"

start "Prog1" cmd /k "javac Prog1Matricielle.java && java Prog1Matricielle"
start "Prog2" cmd /k "javac Prog2Matricielle.java && java Prog2Matricielle"
start "Prog3" cmd /k "javac Prog3Matricielle.java && java Prog3Matricielle"
start "Prog4" cmd /k "javac Prog4Matricielle.java && java Prog4Matricielle"

echo Tous les processus sont en cours d'exécution.
pause
