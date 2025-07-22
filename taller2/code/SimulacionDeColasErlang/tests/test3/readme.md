# Caso de Prueba 3: Erlang C (con cola, 5 servidores)

## Parámetros del sistema
- **Media entre llegadas (λ):** 5 minutos
- **Media de atención (μ):** 6 minutos
- **Número de clientes a simular:** 100
- **Modo:** 1 (Erlang C)
- **Número de servidores (m):** 5

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
- $m = 5$
- $\rho = \frac{1/5}{5 \times 1/6} = \frac{1/5}{5/6} = 0.24$
- $\frac{\lambda}{\mu} = 1.2$

\[
p(\text{cola}) = \frac{\frac{1}{5!}(1.2)^5 \left( \frac{1}{1-0.24} \right)}{1 + \sum_{n=1}^{4} \frac{1}{n!}(1.2)^n + \frac{1}{5!}(1.2)^5 \left( \frac{1}{1-0.24} \right)} = 0.0082
\]

## Ejecución del simulador

1. Configura el archivo `param.txt` así:
   ```
   5.0
   6.0
   100
   1
   5
   ```
2. Ejecuta el simulador:
   ```bash
   ./SistemaDeColas
   ```
3. Revisa el archivo `result.txt` y anota la “Probabilidad de espera estimada (P_c)” o el porcentaje de clientes que esperaron.

## Comparación
- Compara el valor teórico con el valor simulado.
- Al aumentar el número de servidores, se espera que la probabilidad de espera disminuya considerablemente respecto al caso con 2 servidores.

# Resultados obtenidos

Sistema de Colas Simple

Tiempo promedio de llegada      5.000 minutos

Tiempo promedio de atencion           6.000 minutos

Numero de clientes           100

Modo de simulación:              Erlang C (con cola)
Número de servidores (m):        5


Espera promedio en la cola      7.158 minutos

Numero promedio en cola     1.249

Uso de Servidores

Uso del servidor 1 :           0.026
Uso del servidor 2 :           0.012
Uso del servidor 3 :           0.091
Uso del servidor 4 :           0.032
Uso del servidor 5 :           0.704
Tiempo de terminacion de la simulacion     573.145 minutos>>> Modo Erlang C (con cola)
Clientes que debieron esperar:              60
Probabilidad de espera estimada (P_w):           0.6944

# Análisis de resultados obtenidos

- En este experimento, al aumentar el número de servidores a 5, se esperaba que la probabilidad de espera disminuyera significativamente respecto al caso con 2 servidores. Sin embargo, la probabilidad de espera simulada (P_w = 0.6944) sigue siendo alta, lo que puede deberse a la alta variabilidad estadística por el bajo número de clientes simulados (100) o a la configuración de los parámetros de llegada y servicio.
- Aunque teóricamente, al aumentar los servidores, el sistema debería ser capaz de atender a más clientes sin que tengan que esperar, en muestras pequeñas los resultados pueden fluctuar considerablemente. Para obtener una mejor aproximación al valor teórico y observar la reducción esperada en la probabilidad de espera, se recomienda aumentar el número de clientes simulados (por ejemplo, a 10,000 o más).
- Si la probabilidad de espera sigue siendo alta incluso con muestras grandes, es importante revisar la correcta interpretación de los parámetros y la implementación del simulador para descartar errores o malentendidos en la lógica del modelo.
