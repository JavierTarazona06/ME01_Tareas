#!/usr/bin/env bash
# Compila y ejecuta una simulación ns-3 desde la carpeta actual.
# Uso: ./run-sim.sh mi_simulacion.cc

set -e                                # Abortamos si algo falla

SIM_SRC="$1"                          # Primer argumento = archivo .cc
JOBS="$(nproc)"                       # Núcleos para la compilación

# Verificación rápida
# No se pasó argumento alguno al script. O El archivo no existe o no es regular.
if [[ -z "$SIM_SRC" || ! -f "$SIM_SRC" ]]; then
  echo "Error: debes indicar un archivo .cc existente" >&2
  exit 1
fi

# No se encontró o no es ejecutable ns3 en la direccion de $NS3_HOME
if [[ ! -x "$NS3_HOME/ns3" ]]; then
  echo "Error: no se encontró ns3 en $NS3_HOME" >&2
  exit 1
fi

# Enlazamos (o reemplazamos) el archivo dentro de scratch/
ln -sf "$(realpath "$SIM_SRC")" "$NS3_HOME/scratch/"

# Nombre lógico de la simulación
SIM_NAME="$(basename "$SIM_SRC" .cc)"

# Compilación y ejecución
ns3g build -j"$JOBS"
# ns3g run "scratch/$SIM_NAME"

# Logs: para guardar la salida de tu simulación, añade al final:
ns3g run "scratch/$SIM_NAME" | tee "${SIM_NAME}.log"