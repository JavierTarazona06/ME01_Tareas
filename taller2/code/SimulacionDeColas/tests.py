import subprocess

# Parámetros a variar
tiempos_llegada = [8]
tiempos_atencion = [6]
num_clientes = [1000]
semillas = [1,2,3]

with open("resultados_todas_simulaciones.csv", "w") as resumen:
    resumen.write("Semilla,Llegada,Atencion,Clientes,LlegadaPromedio,AtenciónPromedio,EsperaPromedio,NumCola,ServidorUso,TiempoTotal\n")
    for semilla in semillas:
        for llegada in tiempos_llegada:
            for atencion in tiempos_atencion:
                for clientes in num_clientes:
                    # Ejecuta el simulador y captura la salida
                    resultado = subprocess.run(
                        ["SistemadeColas.exe", str(llegada), str(atencion), str(clientes), str(semilla)],
                        capture_output=True, text=True
                    ).stdout

                    # Extrae los datos relevantes de la salida
                    # (ajusta los nombres según tu formato de salida)
                    prom_llegada = None
                    prom_atencion = None
                    espera = None
                    num_cola = None
                    uso = None
                    tiempo_total = None
                    for line in resultado.splitlines():
                        if "Tiempo promedio real de llegada:" in line:
                            prom_llegada = line.split(":")[1].strip().split()[0]
                        if "Tiempo promedio real de atencion:" in line:
                            prom_atencion = line.split(":")[1].strip().split()[0]
                        if "Espera promedio en la cola:" in line:
                            espera = line.split(":")[1].strip().split()[0]
                        if "Numero promedio en cola:" in line:
                            num_cola = line.split(":")[1].strip().split()[0]
                        if "Uso del servidor:" in line:
                            uso = line.split(":")[1].strip().split()[0]
                        if "Tiempo de terminacion de la simulacion:" in line:
                            tiempo_total = line.split(":")[1].strip().split()[0]
                    resumen.write(f"{semilla},{llegada},{atencion},{clientes},{prom_llegada},{prom_atencion},{espera},{num_cola},{uso},{tiempo_total}\n")