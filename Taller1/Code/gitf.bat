@echo off
setlocal EnableDelayedExpansion

REM --------------------------------------------------------------------------
REM Script de automatización para flujos básicos de Git y GitHub CLI (Batch)
REM Uso:
REM   gitf.bat modo [parametros]
REM Modos:
REM   statusFetch           : Hace un status y un fetch
REM   sync                : Actualiza la rama actual con origin.
REM   commit <mensaje>    : Agrega, comitea y sube cambios con el mensaje dado (mensaje todo pegado).
REM   compMain            : Compara la rama actual con main (en ambos sentidos).
REM   pullReqToMain       : Crea un Pull Request de la rama actual a main.
REM   upToMain            : Fusiona (merge) main en la rama actual.
REM   openRepo            : Abre el repositorio en GitHub en el navegador.
REM   viewPRmain          : Lista los Pull Requests abiertos hacia main.
REM   mergePR <ID>        : Fusiona un Pull Request dado por su ID.
REM --------------------------------------------------------------------------

REM Obtener rama actual
for /f %%i in ('git rev-parse --abbrev-ref HEAD') do set MIRAMA=%%i

set MODO=%1

REM statusFetch
if /i "%MODO%"=="statusFetch" (
    git status
    git fetch origin
    git status
    goto fin
)

REM sync
if /i "%MODO%"=="sync" (
    echo [SYNC] Actualizando rama "!MIRAMA!" desde origin...
    git pull origin !MIRAMA!
    goto fin
)

REM commit
if /i "%MODO%"=="commit" (
    set "MESSAGE=%2"
    if "!MESSAGE!"=="" (
        set /p MESSAGE=Mensaje del commit:
    )
    if "!MESSAGE!"=="" (
        echo Error: El mensaje del commit no puede estar vacío.
        goto fin
    )
    git add .
    git commit -m "!MESSAGE!"
    goto fin
)

REM compMain
if /i "!MODO!"=="compMain" (
    echo Comparando cambios de "!MIRAMA!" a "main". Pendientes de tu rama para main
    git rev-list origin/main..!MIRAMA!
    echo.
    echo Comparando cambios de "main" a "!MIRAMA!". Pendientes de main para tu rama
    git rev-list !MIRAMA!..origin/main
    goto fin
)

REM pullReqToMain
if /i "%MODO%"=="pullReqToMain" (
    set "TITLE=%2"
    set "BODY=%3"
    if "!TITLE!"=="" (
        set /p TITLE=Titulo del Pull Request:
    )
    if "%BODY%"=="" (
        set /p BODY=Descripcion del Pull Request:
    )
    echo Creando Pull Request desde "!MIRAMA!" hacia "main"...
    gh pr create --base main --head !MIRAMA! --title "!TITLE!" --body "!BODY!"
    goto fin
)

REM upToMain
if /i "%MODO%"=="upToMain" (
    echo [MERGE] Fusionando "main" en "!MIRAMA!"...
    git merge origin/main
    goto fin
)

REM openRepo
if /i "%MODO%"=="openRepo" (
    echo Abriendo el repositorio en GitHub...
    gh repo view --web
    goto fin
)

REM viewPRmain
if /i "%MODO%"=="viewPRmain" (
    echo Listando Pull Requests abiertos hacia "main"...
    gh pr list --base main --state open --json number -q ".[].number"
    goto fin
)

REM mergePR
if /i "%MODO%"=="mergePR" (
    if "%2"=="" (
        echo Error: Debes proveer el ID del Pull Request a fusionar.
        echo Uso: gitf.bat mergePR ID
        goto fin
    )
    for /f %%i in ('gh pr view %2 --json mergeable --jq .mergeable') do set MERGEABLEI=%%i

    if /i "!MERGEABLEI!"=="MERGEABLE" (
        echo Pull request #%2 está mergeable. Ejecutando merge...
        gh pr merge %2
        goto fin
    ) else (
        echo Pull request #%2 NO está mergeable "!MERGEABLEI!".
        REM Aquí podrías abortar el script o tomar otra acción
        goto fin
    )
)

REM Ayuda/uso por defecto
echo Uso: gitf.bat ^<sync^|statusFetch^|commit^|compMain^|pullReqToMain^|upToMain^|openRepo^|viewPRmain^|mergePR^>
goto fin

:fin
exit /b
