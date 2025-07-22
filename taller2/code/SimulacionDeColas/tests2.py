import subprocess

# Parámetros a variar
tiempos_llegada = [8,10, 20] 
tiempos_atencion = [5,7]
num_clientes = [1000]
semillas = [1]  # Semillas para la aleatoriedad
n_repeticiones = 5  # Número de veces que se repite cada simulación

with open("resultados_todas_simulaciones2.csv", "w") as resumen:
    resumen.write("Semilla,Llegada,Atencion,Clientes,LlegadaPromedio,AtenciónPromedio,EsperaPromedio,NumCola,ServidorUso,TiempoTotal,Wq_teorico,Lq_teorico,Uso_teorico,Error_Wq,Error_Lq,Error_Uso\n")
    
    for semilla in semillas:
        for llegada in tiempos_llegada:
            for atencion in tiempos_atencion:
                for clientes in num_clientes:

                    # Variables acumuladoras
                    suma_llegada = 0.0
                    suma_atencion = 0.0
                    suma_espera = 0.0
                    suma_cola = 0.0
                    suma_uso = 0.0
                    suma_tiempo_total = 0.0

                    for _ in range(n_repeticiones):
                        resultado = subprocess.run(
                            ["SistemadeColas.exe", str(llegada), str(atencion), str(clientes), str(semilla)],
                            capture_output=True, text=True
                        ).stdout

                        # Extrae los datos relevantes
                        for line in resultado.splitlines():
                            if "Tiempo promedio real de llegada:" in line:
                                suma_llegada += float(line.split(":")[1].strip().split()[0])
                            if "Tiempo promedio real de atencion:" in line:
                                suma_atencion += float(line.split(":")[1].strip().split()[0])
                            if "Espera promedio en la cola:" in line:
                                suma_espera += float(line.split(":")[1].strip().split()[0])
                            if "Numero promedio en cola:" in line:
                                suma_cola += float(line.split(":")[1].strip().split()[0])
                            if "Uso del servidor:" in line:
                                suma_uso += float(line.split(":")[1].strip().split()[0])
                            if "Tiempo de terminacion de la simulacion:" in line:
                                suma_tiempo_total += float(line.split(":")[1].strip().split()[0])


                    # Promedios simulados
                    prom_llegada = suma_llegada / n_repeticiones
                    prom_atencion = suma_atencion / n_repeticiones
                    espera = suma_espera / n_repeticiones
                    num_cola = suma_cola / n_repeticiones
                    uso = suma_uso / n_repeticiones
                    tiempo_total = suma_tiempo_total / n_repeticiones

                    # Parámetros teóricos
                    λ = 1 / llegada
                    μ = 1 / atencion
                    rho = λ / μ
                    Wq_teorico = rho / (μ * (1 - rho))
                    Lq_teorico = (rho**2) / (1 - rho)
                    uso_teorico = rho

                    # Errores absolutos
                    error_Wq = abs(espera - Wq_teorico)
                    error_Lq = abs(num_cola - Lq_teorico)
                    error_uso = abs(uso - uso_teorico)

                    resumen.write(f"{semilla},{llegada},{atencion},{clientes},{prom_llegada:.4f},{prom_atencion:.4f},{espera:.4f},{num_cola:.4f},{uso:.4f},{tiempo_total:.4f},{Wq_teorico:.4f},{Lq_teorico:.4f},{uso_teorico:.4f},{error_Wq:.4f},{error_Lq:.4f},{error_uso:.4f}\n")
