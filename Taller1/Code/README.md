# Estructura del Directorio `Code`

Este directorio contiene el código fuente y scripts relacionados con simulaciones y modelos de movilidad, organizados en varias carpetas según su propósito y tipo de experimento.

## Estructura de Carpetas

```
Code/
├── boids/
│   ├── boids.cc
│   ├── boids-mobility-model.h
│   ├── boids-mobility-model.cc
│   ├── CMakeLists.txt
│   └── simulate/
│       ├── boids_positions.csv
│       └── showNodes.py
├── ideas/
│   ├── umanet.cc
│   ├── simulation1.cc
│   └── umanet.cc
├── tutorial/
│   ├── sim_first.cc
│   ├── sim_second.cc
│   ├── sim_third.cc
│   ├── sim_fourth.cc
│   ├── sim_fifth.cc
│   └── sim_sixth.cc
└── run-sim.sh
```

## Descripción de Carpetas y Archivos

### `boids/`
Contiene la implementación principal del modelo de movilidad tipo "boids" (simulación de comportamiento de enjambre). Incluye:
- **boids.cc, boids-mobility-model.h, boids-mobility-model.cc**: Archivos fuente y de cabecera para la simulación y lógica del modelo boids.
- **CMakeLists.txt**: Archivo de configuración para compilar el proyecto con CMake.
- **simulate/**: Subcarpeta con archivos auxiliares para análisis y visualización:
  - `boids_positions.csv`: Resultados de posiciones de los boids generados por la simulación.
  - `showNodes.py`: Script en Python para visualizar las posiciones de los boids.

### `ideas/`
Contiene implementaciones y experimentos alternativos o preliminares:
- **umanet.cc, 0umanet.cc**: Versiones de modelos de movilidad o simulaciones de redes.
- **simulation1.cc**: Script de simulación específico o de prueba.

### `tutorial/`
Incluye ejemplos y scripts de simulación para propósitos de aprendizaje o referencia:
- **sim_first.cc** a **sim_sixth.cc**: Diferentes ejemplos de simulaciones, posiblemente incrementales en complejidad o características.

### `run-sim.sh`
Script de shell para ejecutar simulaciones, compilar o automatizar tareas relacionadas con los modelos.

---

Este README proporciona una visión general para facilitar la navegación y comprensión del propósito de cada carpeta y archivo principal dentro del directorio `Code`. Si necesitas detalles sobre un archivo específico, revisa los comentarios dentro del código fuente correspondiente. 