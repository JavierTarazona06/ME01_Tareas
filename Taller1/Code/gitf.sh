#!/bin/bash

# -----------------------------------------------------------------------------
# Script de automatización para flujos básicos de Git y GitHub CLI.
#
# Uso:
#   ./git-helper.sh <modo> [parametros]
#
# Modos disponibles:
#   sync                : Actualiza la rama actual con origin.
#   commit <mensaje>    : Agrega, comitea y sube cambios con el mensaje dado.
#   compMain            : Compara la rama actual con main (en ambos sentidos).
#   pullReqToMain       : Crea un Pull Request de la rama actual a main.
#   upToMain            : Fusiona (merge) main en la rama actual.
#   openRepo            : Abre el repositorio en GitHub en el navegador.
#   viewPRmain          : Lista los Pull Requests abiertos hacia main.
#   mergePR <ID>        : Fusiona un Pull Request dado por su ID.
#
# Argumentos:
#   modo        (string)  : Operación a realizar.
#   mensaje     (string)  : Mensaje de commit (obligatorio en 'commit').
#
# -----------------------------------------------------------------------------

set -e  # Abortar en cualquier error

readonly MIRAMA="$(git rev-parse --abbrev-ref HEAD)"

git fetch origin
git status

case "$1" in
    sync)
        echo "[SYNC] Actualizando rama '$MIRAMA' desde origin..."
        git pull origin "$MIRAMA"
        ;;
    commit)
        if [[ -z "$2" ]]; then
            echo "Error: Debes proveer un mensaje de commit."
            echo "Uso: $0 commit \"Mensaje del commit\""
            exit 2
        fi
        git add .
        git commit -m "$2"
        git push origin "$MIRAMA"
        ;;
    compMain)
        echo "Comparando cambios de '$MIRAMA' a 'main' pendientes de tu rama para main"
        git rev-list origin/main.."$MIRAMA"
        echo
        echo "Comparando cambios de 'main' a '$MIRAMA' pendientes de main para tu rama"
        git rev-list "$MIRAMA"..origin/main
        ;;
    pullReqToMain)
        TITLE="$2"
        BODY="$3"
        if [[ -z "$TITLE" ]]; then
            read -p "Título del Pull Request: " TITLE
        fi
        if [[ -z "$BODY" ]]; then
            read -p "Descripción del Pull Request: " BODY
        fi
        echo "Creando Pull Request desde '$MIRAMA' hacia 'main'..."
        gh pr create --base main --head "$MIRAMA" --title "$TITLE" --body "$BODY"
        ;;
    upToMain)
        echo "[MERGE] Fusionando 'main' en '$MIRAMA'..."
        git merge origin/main
        ;;
    openRepo)
        echo "Abriendo el repositorio en GitHub..."
        gh repo view --web
        ;;
    viewPRmain)
        echo "Listando Pull Requests abiertos hacia 'main'..."
        gh pr list --base main --state open --json number -q '.[].number'
        ;;
    mergePR)
        if [[ -z "$2" ]]; then
            echo "Error: Debes proveer el ID del Pull Request a fusionar."
            echo "Uso: $0 mergePR <ID>"
            exit 2
        fi
        gh pr merge [ID]
        ;;
    *)
        echo "Uso: $0 {sync|commit <mensaje>|compMain|pullReqToMain [titulo] [descripcion]|
            upToMain|openRepo|viewPRmain|mergePR <ID>}"
        exit 1
        ;;
esac
