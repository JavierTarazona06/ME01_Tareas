# Presentación: Simulación de Redes MANET Jerárquicas con Liderazgo Adaptativo y Movilidad Boids

## 1. Portada y Contexto
- **Universidad Nacional de Colombia**
- Taller 1: Modelos estocásticos y simulación en computación y comunicaciones
- Integrantes: Javier Andrés Tarazona Jiménez, Yenifer Yulieth Mora Segura, Juan Esteban Carranza Salazar, Grevy Joner Rincon Mejia, Jefferson Duvan Ramirez Castañeda, Javier Andres Carrillo Carrasco, Diego Nicolas Ramirez Maldonado
- Fecha: Junio 16 de 2025

---

## 2. Motivación e Introducción
- **Problema:** En escenarios de extinción de incendios forestales con drones cooperativos, la desconexión temporal de un dron seguidor del líder afecta la coordinación y la eficiencia.
- **Solución propuesta:** Uso de redes MANET jerárquicas donde los nodos aislados pueden autoproclamarse líderes temporales (CH-temporal) para mantener la coordinación y respuesta.

---

## 3. Marco Teórico
### ¿Qué es una red MANET?
- Red inalámbrica móvil, descentralizada y auto-configurable.
- Cada nodo es terminal y enrutador.
- Útil en desastres, operaciones militares, zonas rurales, VANETs, IoT.

### Jerarquía en MANET
- Organización en clústeres con líderes (Cluster Heads, CH).
- Beneficios: escalabilidad, reducción de sobrecarga, tolerancia a fallos, eficiencia energética.
- Problemas: movilidad grupal, costos de re-clustering, escalabilidad, falta de trazas reales.

### Protocolo Cluster-Based y Liderazgo Dinámico
- Organización jerárquica, elección de CH por métrica WCA (energía, conectividad, distancia a objetivos, movilidad).
- Novedad: nodo CH-temporal para resiliencia local.

### Propuesta de Indagación

¿No será que la **efectividad global en la extinción de fuegos** (medida por métricas como el tiempo promedio de extinción, el número total de fuegos extinguidos en un periodo dado) **se incrementa** si se permite que un nodo seguidor que previamente estaba **aislado** (sin un líder cercano dentro de un radio efectivo de influencia) **se autoproclame temporalmente como líder hasta que se encuentre con uno con un mayor WCA**?

---

## 4. Simulación en NS-3
- **Herramienta:** Network Simulator 3 (ns-3), versión 3.45.
- Modelos de movilidad: RandomWaypoint, Hierarchical, Boids.
- Visualización: PyViz.

### Modelo Boids
- Reglas: Separación, Alineación, Cohesión.
- Descentralizado, simula enjambres y equipos cooperativos.
- Limitaciones: calibración de parámetros, imprevisibilidad, carga computacional.

---

## 5. Propuesta de Solución
- **Mecanismo de liderazgo temporal:**
  - Nodo aislado detecta evento crítico (fuego) y se autoproclama líder temporal (tempCH).
  - Coordina respuesta local hasta encontrar un líder con mayor WCA.
- **Beneficios:**
  - Reduce latencia de respuesta.
  - Mejora cobertura y resiliencia.
  - Solución local, bajo costo computacional.

---

## 6. Diseño y Metodología
- **Sistema basado en Boids:**
  - Nodos con roles de líder o seguidor.
  - Líderes buscan eventos (fuegos), seguidores aplican reglas Boids y siguen a líderes.
- **Espacio de simulación:** 1000x1000 unidades, frontera toroidal.
- **Actualización periódica:** Posiciones, roles, energía, eventos.
- **Visualización:** Diagramas de flujo, UML, animaciones.

---

## 7. Manual de Usuario (Resumen)
- Instalación de NS-3 en WSL (Ubuntu recomendado).
- Copia y configuración de archivos: boids.cc, boids-mobility-model.cc/h.
- Compilación y ejecución de la simulación.
- Visualización de resultados con showNodes.py (matplotlib).

---

## 8. Manual Técnico (Resumen)
- Componentes principales: boids-mobility-model.h/cc, main.cc.
- Reglas implementadas: separación, alineación, cohesión.
- Jerarquía líder-seguidor, activación de liderazgo temporal.
- Salida: boids_positions.csv (tiempo, nodo, posición, rol, fuego).

---

## 9. Experimentación y Resultados
- **Hipótesis:** El liderazgo temporal mejora la efectividad en la extinción de fuegos.
- **Escenarios:**
  - Variación de número de seguidores y líderes.
  - Medición: fuegos apagados, tiempo promedio de extinción.
- **Resultados:**
  - La condición experimental (con autoproclamación) mejora la cantidad de fuegos extinguidos y la cobertura.
  - Visualización de trayectorias y clústeres en NetAnim.

---

## 10. Conclusiones y Recomendaciones
- El liderazgo adaptativo y la movilidad Boids mejoran la resiliencia y eficiencia en MANETs.
- Es necesario ajustar parámetros de cohesión y liderazgo para optimizar resultados.
- Recomendaciones: analizar calidad de comunicaciones, probar con más nodos y escenarios, incorporar modelos de propagación de señal más realistas.

---

## 11. Referencias
- Sarkar, S. K., Basavaraju, T. G., & Puttamadappa, C. (2013). Ad Hoc Mobile Wireless Networks: Principles, Protocols, and Applications.
- Barbeau, M., & Kranakis, E. (2007). Principles of Ad Hoc Networking.

---

> **¡Gracias!**

---

### Notas para la presentación oral (10 minutos)
- Enfatizar el problema real y la motivación.
- Explicar la importancia de la jerarquía y el liderazgo temporal.
- Resumir el modelo Boids y su integración con MANET.
- Mostrar resultados experimentales y su impacto.
- Concluir con recomendaciones y posibles mejoras futuras. 