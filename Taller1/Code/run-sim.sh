#!/usr/bin/env bash
# Compila y ejecuta una simulación ns-3 desde la carpeta actual.
# Uso: ./run-sim.sh mi_simulacion.cc
# Uso con GDB: ./run-sim.sh mi_simulacion.cc gdb

set -e                                  # Abortamos si algo falla

SIM_SRC="$1"                            # Archivo .cc recibido
SIM_NAME="$(basename "$SIM_SRC" .cc)"   # Nombre lógico
JOBS="$(nproc)"                         # Núcleos disponibles
GDB="$2"                             # Opcional: si se pasa, se ejecuta con GDB

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

# 4. Compilación ────────────────
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

# 4. Ejecución ────────────────
while true; do
  read -rp "¿Deseas ejecutar el programa? [Y/n]: " yn
  yn="${yn:-Y}"  # Valor por defecto es Y si el usuario solo presiona ENTER

  case "$yn" in
    [Yy]* )
      if [[ -n "$GDB" ]]; then
        echo "Ejecutando con GDB..."
        "$NS3_HOME/ns3" run --gdb "$SIM_NAME"
        exit $?
      else
        "$NS3_HOME/ns3" run "scratch/$SIM_NAME" | tee "${SIM_NAME}.log"
        echo "Simulación terminada; log guardado en ${SIM_NAME}.log"
        break
      fi
      ;;
    [Nn]* )
      echo "Ejecución cancelada."
      break
      ;;
    * )
      echo "Por favor, responde con Y (sí) o N (no)."
      ;;
  esac
done

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