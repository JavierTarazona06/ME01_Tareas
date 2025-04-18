import math

def porcentaje_opciones(estudiantes):
    primera, segunda, tercera, todas = 0, 0, 0, 0
    total = len(estudiantes)

    for estudiante in estudiantes.values():
        preferencias = estudiante["lista_preferencias"]
        asignadas = estudiante["lista_materias_asignadas"]

        # Posiciones de las materias asignadas en la lista de preferencias
        posiciones = [preferencias.index(m) + 1 for m in asignadas if m in preferencias]

        if not posiciones:
            continue

        if 1 in posiciones:
            primera += 1
        if 2 in posiciones:
            segunda += 1
        if 3 in posiciones:
            tercera += 1
        if all(op in posiciones for op in [1, 2, 3]):
            todas += 1

    return {
        "primera_opcion": primera / total,
        "segunda_opcion": segunda / total,
        "tercera_opcion": tercera / total,
        "las_tres": todas / total
    }



def calcular_satisfaccion_normalizada(estudiante):
    preferencias = estudiante["lista_preferencias"]
    asignadas = estudiante["lista_materias_asignadas"]
    m = len(asignadas)

    if m == 0:
        return 0.0

    satisfaccion_obtenida = sum(
        1 / (preferencias.index(materia) + 1)
        for materia in asignadas if materia in preferencias
    )

    maximo_posible = sum(1 / (i + 1) for i in range(estudiante["cantidad_materias_inscribir"]))
    return satisfaccion_obtenida / maximo_posible if maximo_posible > 0 else 0


def calcular_equidad(estudiantes):
    satisfacciones = [calcular_satisfaccion_normalizada(est) for est in estudiantes.values()]
    if not satisfacciones:
        return 0.0
    media = sum(satisfacciones) / len(satisfacciones)
    varianza = sum((s - media) ** 2 for s in satisfacciones) / len(satisfacciones)
    desviacion_estandar = math.sqrt(varianza)
    return desviacion_estandar

def calcular_eficiencia_pareto(estudiantes):
    satisfacciones = {
        cedula: calcular_satisfaccion_normalizada(est)
        for cedula, est in estudiantes.items()
    }

    cedulas = list(estudiantes.keys())
    n = len(cedulas)
    no_dominados = set(cedulas)  # Comenzamos con todos como eficientes

    for i in range(n):
        for j in range(n):
            if i == j:
                continue

            si = satisfacciones[cedulas[i]]
            sj = satisfacciones[cedulas[j]]

            # si el estudiante j es estrictamente mejor que el i
            if sj > si:
                # el estudiante i está dominado
                no_dominados.discard(cedulas[i])
                break

    eficiencia = len(no_dominados) / len(estudiantes)
    return eficiencia



def imprimir_metricas(estudiantes):
    porcentajes = porcentaje_opciones(estudiantes)
    equidad = calcular_equidad(estudiantes)
    eficiencia = calcular_eficiencia_pareto(estudiantes)

    print("📊 Porcentaje de estudiantes que obtuvieron:")
    print(f" - Primera opción: {porcentajes['primera_opcion']:.2%}")
    print(f" - Segunda opción: {porcentajes['segunda_opcion']:.2%}")
    print(f" - Tercera opción: {porcentajes['tercera_opcion']:.2%}")
    print(f" - Las tres primeras opciones: {porcentajes['las_tres']:.2%}")
    print(f"📏 Equidad (desviación estándar de satisfacción): {equidad:.4f}")
    print(f"⚖️ Eficiencia de Pareto: {eficiencia:.2%}")
