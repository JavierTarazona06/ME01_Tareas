# Simulación de Modelo de Movilidad Boids

## Descripción General

Este repositorio implementa un **modelo de movilidad basado en Boids** para simular el movimiento y la coordinación de nodos (agentes) en una red ad hoc inalámbrica, utilizando el simulador de redes ns-3. La simulación modela tanto nodos **líderes** como **seguidores**, su agrupamiento en clústeres, la comunicación inalámbrica y su respuesta colectiva a eventos dinámicos (como la aparición y extinción de "fuegos") en un entorno 2D.

El proyecto está diseñado para fines de investigación y educativos, especialmente en el estudio de **redes auto-organizadas**, **algoritmos de agrupamiento** y **modelos de movilidad bio-inspirados**.

---

## Características Principales

- **Modelo Boids**: Implementa las reglas clásicas de Boids (separación, alineamiento, cohesión) para el movimiento de los nodos, con extensiones para la dinámica líder-seguidor.
- **Agrupamiento (Clustering)**: Los nodos se organizan en clústeres, cada uno con un líder (Cluster-Head) y varios seguidores.
- **Liderazgo Dinámico**: El liderazgo puede cambiar dinámicamente según métricas del nodo (energía, conectividad, proximidad a objetivos, movilidad) usando un Algoritmo de Agrupamiento Ponderado (WCA).
- **Comunicación Inalámbrica**: Los nodos están equipados con interfaces Wi-Fi y se comunican en modo ad hoc.
- **Simulación de Eventos**: Aparecen "fuegos" aleatorios en el entorno, y los líderes coordinan la respuesta para extinguirlos.
- **Registro de Métricas**: La simulación registra posiciones de los nodos, estado de liderazgo y eventos de fuego en archivos CSV para su posterior análisis y visualización.
- **Visualización**: Un script en Python (`simulate/showNodes.py`) anima los resultados de la simulación, mostrando el movimiento de los nodos, zonas de liderazgo y ubicaciones de los fuegos.

---

## Componentes Principales

### 1. **boids.cc** (Punto de Entrada de la Simulación)

- **Creación de Nodos**: Inicializa nodos líderes y seguidores, asignándolos a clústeres.
- **Configuración Wi-Fi**: Configura la comunicación Wi-Fi ad hoc para todos los nodos.
- **Asignación de Movilidad**: Aplica el BoidsMobilityModel a todos los nodos, con posiciones iniciales aleatorias.
- **Distribución en Clústeres**: Distribuye los seguidores entre los líderes, formando clústeres.
- **Programación de Eventos**: Los fuegos se generan en intervalos y ubicaciones aleatorias usando un proceso espacial de Poisson.
- **Bucle de Simulación**: Ejecuta la simulación, actualizando posiciones, liderazgo y respuesta a los fuegos.
- **Salida de Métricas**: Escribe posiciones de nodos y eventos de fuego en `boids_positions.csv`, y métricas resumidas en `boids_summary.csv`.

### 2. **boids-mobility-model.h / .cc** (Modelo de Movilidad Boids)

- **Reglas Boids**: Implementa separación, alineamiento y cohesión para los seguidores; los líderes son atraídos hacia los fuegos.
- **Evaluación de Liderazgo**: Utiliza un Algoritmo de Agrupamiento Ponderado (WCA) para asignar o revocar dinámicamente el liderazgo según:
  - Energía residual
  - Conectividad del nodo (número de vecinos)
  - Proximidad a los fuegos (objetivos)
  - Movilidad del nodo (estabilidad)
- **Gestión de Fuegos**: Los fuegos se generan y asignan a los líderes, quienes coordinan su extinción.
- **Cálculo de Métricas**: Registra y calcula métricas como el número de fuegos extinguidos y el tiempo promedio de extinción.

### 3. **simulate/showNodes.py** (Visualización)

- **Lee**: `boids_positions.csv` generado por la simulación.
- **Anima**: El movimiento de los nodos, zonas de liderazgo y eventos de fuego a lo largo del tiempo.
- **Controles Interactivos**: Permite alternar la visibilidad de los fuegos y las zonas de los líderes.

---

## ¿Cómo Funciona?

1. **Inicialización**: La simulación comienza creando un número especificado de nodos líderes y seguidores, asignándolos a clústeres.
2. **Modelo de Movilidad**: Cada nodo utiliza el BoidsMobilityModel, que gobierna su movimiento según reglas locales y el estado de liderazgo.
3. **Configuración Inalámbrica**: Los nodos se configuran para comunicarse por Wi-Fi en modo ad hoc, permitiendo comunicación directa entre pares.
4. **Eventos de Fuego**: Los fuegos aparecen en ubicaciones y momentos aleatorios. Los líderes los detectan y se mueven para extinguirlos, coordinando a sus seguidores.
5. **Liderazgo Dinámico**: Si un líder queda aislado o hay otro nodo con mejor puntuación WCA, el liderazgo puede cambiar dinámicamente.
6. **Registro y Visualización**: Todos los eventos y posiciones relevantes se registran para su análisis y visualización posterior.

---

## Archivos de Salida

- **boids_positions.csv**: Contiene posiciones de todos los nodos con marca de tiempo, estado de liderazgo y eventos de fuego.
- **boids_summary.csv**: Resume métricas clave como el número total de fuegos extinguidos y el tiempo promedio de extinción.

---

## Visualización

Para visualizar la simulación:

```bash
cd simulate
python showNodes.py
```

Esto animará el movimiento de los nodos, mostrará las zonas de los líderes y los eventos de fuego a medida que ocurren.

---

## Casos de Uso

- **Investigación en Redes Ad Hoc Móviles (MANETs)**
- **Estudio de Algoritmos Bio-inspirados**
- **Dinámica de Agrupamiento y Liderazgo**
- **Simulación de Respuesta a Desastres y Robótica de Enjambre**

---

## Requisitos

- **ns-3** (para ejecutar la simulación)
- **Python 3** con `pandas`, `matplotlib` y `numpy` (para la visualización)

---

## Estructura de Archivos

- `boids.cc` — Script principal de simulación
- `boids-mobility-model.h/.cc` — Implementación del modelo de movilidad Boids
- `simulate/showNodes.py` — Script de visualización
- `simulate/boids_positions.csv` — Datos de salida (generados)
- `simulate/boids_summary.csv` — Resumen de salida (generado)
- `CMakeLists.txt` — Configuración de compilación

---

## Referencias

- Reynolds, C. W. (1987). "Flocks, herds and schools: A distributed behavioral model." *ACM SIGGRAPH Computer Graphics*.
- Chatterjee, M., Das, S. K., & Turgut, D. (2002). "WCA: A Weighted Clustering Algorithm for Mobile Ad Hoc Networks." *Cluster Computing*. 