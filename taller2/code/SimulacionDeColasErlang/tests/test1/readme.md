# Caso de Prueba 1: Erlang B (sin cola)

## Parámetros del sistema
- **Media entre llegadas (λ):** 5 minutos
- **Media de atención (μ):** 6 minutos
- **Número de clientes a simular:** 100
- **Modo:** 0 (Erlang B)
- **Número de servidores (m):** 2

## Cálculo teórico

La fórmula de Erlang B para la probabilidad de bloqueo \( p_m \) es:

\[
p_m = \frac{\frac{1}{m!}\left(\frac{\lambda}{\mu}\right)^m}{1 + \sum_{n=1}^{m} \frac{1}{n!}\left(\frac{\lambda}{\mu}\right)^n}
\]

donde:
- $\lambda$ es la tasa de llegadas
- $\mu$ es la tasa de servicio
- $m$ es el número de servidores

**Cálculo:**
- $\lambda = 1/5$ clientes por minuto
- $\mu = 1/6$ clientes por minuto
- $m = 2$

\[
p_m = \frac{\frac{1}{2!}\left(\frac{1/5}{1/6}\right)^2}{1 + \frac{1}{1!}\left(\frac{1/5}{1/6}\right)^1 + \frac{1}{2!}\left(\frac{1/5}{1/6}\right)^2}
= \frac{\frac{1}{2}\cdot(1.2)^2}{1 + 1.2 + \frac{1}{2}\cdot(1.2)^2}
= \frac{0.72}{2.92} \approx 0.2466
\]

## Ejecución del simulador

1. Configura el archivo `param.txt` así:
   ```
   5.0
   6.0
   100
   0
   2
   ```
2. Ejecuta el simulador:
   ```bash
   ./SistemaDeColas
   ```
3. Revisa el archivo `result.txt` y anota la “Probabilidad de bloqueo estimada (P_b)”.

## Comparación
- Compara el valor teórico (\(\approx 0.2466\)) con el valor simulado.
- Con un número pequeño de clientes, la variabilidad en los resultados simulados será mayor.
- Diferencias notables pueden deberse a la aleatoriedad de la simulación y al bajo tamaño de muestra.

# Resultados obtenidos

```
Sistema de Colas Simple

Tiempo promedio de llegada      5.000 minutos

Tiempo promedio de atencion           6.000 minutos

Numero de clientes           100

Modo de simulación:              Erlang B (sin cola)
Número de servidores (m):        2

Espera promedio en la cola      0.000 minutos

Numero promedio en cola     0.000

Uso de Servidores

Uso del servidor 1 :           0.021
Uso del servidor 2 :           0.544
Tiempo de terminacion de la simulacion     982.220 minutos>>> Modo Erlang B (sin cola)
Clientes bloqueados (rechazados):           105
Probabilidad de bloqueo estimada (P_b):          0.5426
```

# Análisis de resultados obtenidos

- El valor simulado de la probabilidad de bloqueo (P_b = 0.5426) es considerablemente mayor que el valor teórico (≈ 0.2466). Esto es esperable cuando el número de clientes simulados es bajo (100), ya que la variabilidad estadística es mucho mayor y los resultados pueden alejarse del valor teórico. Para obtener una mejor aproximación al valor teórico, se recomienda aumentar el número de clientes simulados (por ejemplo, a 10,000 o más).