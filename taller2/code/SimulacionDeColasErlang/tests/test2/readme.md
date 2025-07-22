# Caso de Prueba 2: Erlang C (con cola)

## Parámetros del sistema
- **Media entre llegadas (λ):** 5 minutos
- **Media de atención (μ):** 6 minutos
- **Número de clientes a simular:** 100
- **Modo:** 1 (Erlang C)
- **Número de servidores (m):** 2

## Cálculo teórico

La fórmula de Erlang C para la probabilidad de que un cliente tenga que esperar (haya cola) es:

\[
p(\text{cola}) = \frac{\frac{1}{m!}\left(\frac{\lambda}{\mu}\right)^m \left( \frac{1}{1-\rho} \right)}{1 + \sum_{n=1}^{m-1} \frac{1}{n!}\left(\frac{\lambda}{\mu}\right)^n + \frac{1}{m!}\left(\frac{\lambda}{\mu}\right)^m \left( \frac{1}{1-\rho} \right)}
\]

donde:
- $\lambda$ es la tasa de llegadas
- $\mu$ es la tasa de servicio
- $m$ es el número de servidores
- $\rho = \frac{\lambda}{m\mu}$ es la utilización del sistema

**Cálculo:**
- $\lambda = 1/5$ clientes por minuto
- $\mu = 1/6$ clientes por minuto
- $m = 2$
- $\rho = \frac{1/5}{2 \times 1/6} = \frac{1/5}{1/3} = 0.6$

\[
p(\text{cola}) = \frac{\frac{1}{2!}\left(1.2\right)^2 \left( \frac{1}{1-0.6} \right)}{1 + \frac{1}{1!}(1.2)^1 + \frac{1}{2!}(1.2)^2 \left( \frac{1}{1-0.6} \right)}
= \frac{0.72 \times 2.5}{1 + 1.2 + 0.72 \times 2.5}
= \frac{1.8}{4.0} = 0.45
\]

## Ejecución del simulador

1. Configura el archivo `param.txt` así:
   ```
   5.0
   6.0
   100
   1
   2
   ```
2. Ejecuta el simulador:
   ```bash
   ./SistemaDeColas
   ```
3. Revisa el archivo `result.txt` y anota la “Probabilidad de espera estimada (P_c)” o el porcentaje de clientes que esperaron.

## Comparación
- Compara el valor teórico (\(0.45\)) con el valor simulado.
- Con un número pequeño de clientes, la variabilidad en los resultados simulados será mayor.
- Diferencias notables pueden deberse a la aleatoriedad de la simulación y al bajo tamaño de muestra.

# Resultados obtenidos

Sistema de Colas Simple

Tiempo promedio de llegada      5.000 minutos

Tiempo promedio de atencion           6.000 minutos

Numero de clientes           100

Modo de simulación:              Erlang C (con cola)
Número de servidores (m):        2


Espera promedio en la cola      7.972 minutos

Numero promedio en cola     1.330

Uso de Servidores

Uso del servidor 1 :           0.026
Uso del servidor 2 :           0.782
Tiempo de terminacion de la simulacion     599.268 minutos>>> Modo Erlang C (con cola)
Clientes que debieron esperar:              70
Probabilidad de espera estimada (P_w):           0.7761


# Análisis de resultados obtenidos

- Dado que el número de clientes simulados es bajo (100), es esperable que la probabilidad de espera simulada difiera del valor teórico (0.45) debido a la alta variabilidad estadística inherente a muestras pequeñas. Esto puede provocar que el valor simulado sea mayor o menor que el teórico, y que la dispersión entre ejecuciones sea significativa.
- Para obtener una mejor aproximación al valor teórico, se recomienda aumentar el número de clientes simulados (por ejemplo, a 10,000 o más), lo que reduce la variabilidad y permite que la simulación converja hacia el valor esperado por la teoría de colas.
- Si la diferencia entre el valor simulado y el teórico persiste incluso con muestras grandes, se recomienda revisar la correcta interpretación de los parámetros y la implementación del simulador.