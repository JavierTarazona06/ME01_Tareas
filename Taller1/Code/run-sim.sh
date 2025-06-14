#!/usr/bin/env bash
# Compila y ejecuta una simulación ns-3 desde la carpeta actual.
# Uso: ./run-sim.sh mi_simulacion.cc

set -e                                  # Abortamos si algo falla

SIM_SRC="$1"                            # Archivo .cc recibido
SIM_NAME="$(basename "$SIM_SRC" .cc)"   # Nombre lógico
JOBS="$(nproc)"                         # Núcleos disponibles

# 1. Verificación de argumentos
if [[ -z "$SIM_SRC" || ! -f "$SIM_SRC" ]]; then
  echo "Error: debes indicar un archivo .cc existente" >&2
  exit 1
fi

# 2. Verificación del wrapper ns3
if [[ ! -x "$NS3_HOME/ns3" ]]; then
  echo "Error: no se encontró ns3 en $NS3_HOME" >&2
  exit 1
fi

# 3. Enlace simbólico (si no existe)
if [[ ! -e "$NS3_HOME/scratch/$SIM_NAME.cc" ]]; then
  ln -sf "$(realpath "$SIM_SRC")" "$NS3_HOME/scratch/"
fi

# 4. Compilación y ejecución ────────────────
while true; do
  read -rp "¿Deseas compilar '${SIM_NAME}.cc'? [Y/n]: " yn
  yn="${yn:-Y}"  # Y por defecto si el usuario solo presiona ENTER

  case "$yn" in
    [Yy]* )
        "$NS3_HOME/ns3" build -j"$JOBS"
      break
      ;;
    [Nn]* )
      echo "Solo se va a correr la simulación sin compilar." 
      break
      ;;
    * )
      echo "Respuesta inválida. Por favor, ingresa Y (sí) o N (no)."
      ;;
  esac
done


"$NS3_HOME/ns3" run "scratch/$SIM_NAME" | tee "${SIM_NAME}.log"

echo "Simulación terminada; log guardado en ${SIM_NAME}.log"

# 5. Visualizar animación ────────────────
# Pregunta al usuario si desea visualizar NetAnim
while true; do
  read -rp "¿Deseas abrir NetAnim para visualizar '${SIM_NAME}.xml'? [Y/n]: " yn
  yn="${yn:-Y}"  # Y por defecto si el usuario solo presiona ENTER

  case "$yn" in
    [Yy]* )
      NETANIM_BIN="$(find "${NS3_HOME}/build" -type f -executable -name netanim | head -n1)"
      if [[ -x "$NETANIM_BIN" ]]; then
        echo "Iniciando NetAnim..."
        "$NETANIM_BIN" "${NS3_HOME}/${SIM_NAME}.xml"
      else
        echo "No se encontró NetAnim en '${NS3_HOME}/build'"
        exit 1
      fi
      break
      ;;
    [Nn]* )
      echo "Visualización cancelada." 
      break
      ;;
    * )
      echo "Respuesta inválida. Por favor, ingresa Y (sí) o N (no)."
      ;;
  esac
done