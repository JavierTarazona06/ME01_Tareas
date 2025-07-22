# Simulador de Sistemas de Colas Erlang B y C

Este programa, desarrollado en C++, permite simular sistemas de colas tipo **Erlang B** (sin cola) y **Erlang C** (con cola) con múltiples servidores. Es fácilmente extensible para otros modelos y experimentos.

## Características principales

- Simulación de sistemas de colas M/M/m bajo los modelos Erlang B y Erlang C.
- Configuración sencilla mediante archivo de parámetros.
- Resultados claros y detallados en archivo de salida.
- Código comentado y variables en español para facilitar su comprensión y modificación.

## Archivos principales

- `Sistema de Colas.cpp`: Código fuente principal del simulador.
- `lcgrand.cpp`: Generador de números aleatorios (tomado de [Simlib.c](http://www.sju.edu/~sforman/courses/2000F_CSC_5835/)).
- `param.txt`: Archivo de entrada con los parámetros de simulación.
- `result.txt`: Archivo de salida con los resultados de la simulación.

## Configuración de parámetros

El archivo `param.txt` debe estar en la misma carpeta que el ejecutable y debe contener, en orden, los siguientes valores (uno por línea):

1. **Media entre llegadas** (float): Tiempo promedio entre llegadas de clientes.
2. **Media de atención** (float): Tiempo promedio de atención por servidor.
3. **Número de clientes a simular** (entero): Total de clientes a procesar.
4. **Modo** (entero): `0` para Erlang B (sin cola), `1` para Erlang C (con cola).
5. **Número de servidores** (entero): Cantidad de servidores en el sistema.

**Ejemplo de `param.txt`:**
```
5.0
6.0
8
0
2
```

## Compilación y ejecución

1. **Compilar el programa:**
   ```bash
   g++ Sistema\ de\ Colas.cpp -o SistemaDeColas
   ```
2. **Ejecutar el simulador:**
   ```bash
   ./SistemaDeColas
   ```
   Esto generará el archivo `result.txt` con los resultados.

## Formato de salida (`result.txt`)

El archivo de resultados incluye:

- Parámetros de entrada utilizados.
- Modo de simulación y número de servidores.
- Métricas principales:
  - Probabilidad de bloqueo (Erlang B) o de espera (Erlang C).
  - Uso de cada servidor.
  - Tiempos promedio de espera y ocupación.
  - Número de clientes bloqueados o que esperaron.
  - Tiempo total de simulación.

**Ejemplo de salida:**
```
Sistema de Colas Simple

Tiempo promedio de llegada      5.000 minutos
Tiempo promedio de atencion     6.000 minutos
Numero de clientes              8
Modo de simulación:             Erlang B (sin cola)
Número de servidores (m):       2

Espera promedio en la cola      0.000 minutos
Numero promedio en cola         0.000

Uso de Servidores
Uso del servidor 1 :            0.257
Uso del servidor 2 :            0.257

Tiempo de terminacion de la simulacion      32.383 minutos
Clientes bloqueados (rechazados):           1
Probabilidad de bloqueo estimada (P_b):     0.2103
```

## Créditos

- El generador de números aleatorios `lcgrand.cpp` fue tomado de [Simlib.c](http://www.sju.edu/~sforman/courses/2000F_CSC_5835/).