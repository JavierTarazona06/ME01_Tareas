
import os
import random
import copy
from data.generator import Curso, Docente, Aula, Horario, Clase, Estudiante
from tools.FileManager import JSON_man
from constants.program import CURSOS_CT, DOCENTES_CT, AULAS_CT, CLASES_CT, ESTUDIANTES_CT
from codeStats.toolsStats import PBS
from codeStats.metrics import imprimir_metricas, compute_metrics

# Configurar directorio 'data'
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, 'data')
os.makedirs(DATA_DIR, exist_ok=True)

def data_path(filename: str) -> str:
    return os.path.join(DATA_DIR, filename)


def generate_base_data():
    cursos = Curso.crear_cursos(
        CURSOS_CT['cantidad'], CURSOS_CT['num_grupos_min'], CURSOS_CT['num_grupos_max'],
        CURSOS_CT.get('num_grupos_avg'), CURSOS_CT.get('num_grupos_dev')
    )
    JSON_man.dict2json(cursos, data_path('cursos.json'))

    docentes = Docente.crear_docentes(DOCENTES_CT['cantidad'])
    JSON_man.dict2json(docentes, data_path('docentes.json'))

    aulas = Aula.crear_aulas(AULAS_CT['cantidad'])
    JSON_man.dict2json(aulas, data_path('aulas.json'))

    horarios = Horario.crear_horarios()
    JSON_man.dict2json(horarios, data_path('horarios.json'))

    clases = Clase.crear_clases(
        docentes, aulas, horarios, cursos,
        CLASES_CT['num_cupos_min'], CLASES_CT['num_cupos_max'],
        CLASES_CT.get('num_cupos_avg'), CLASES_CT.get('num_cupos_dev')
    )
    JSON_man.dict2json(clases, data_path('clases.json'))

    estudiantes = Estudiante.crear_estudiantes(
        ESTUDIANTES_CT['cantidad'], set(docentes.keys()),
        ESTUDIANTES_CT['p.a.p.i_avg'], ESTUDIANTES_CT['p.a.p.i_dev'],
        cursos, clases
    )
    JSON_man.dict2json(estudiantes, data_path('estudiantes.json'))
    return cursos, docentes, aulas, horarios, clases, estudiantes

    # Sin modificaciones: asignación PS estándar
def scenario0(estudiantes, clases): 
    pbs = PBS(copy.deepcopy(estudiantes), copy.deepcopy(clases), sort=False)
    pbs.run_algo(delta_t=0.1)
    est_fin, cls_fin = pbs.round_greedy()
    JSON_man.dict2json(est_fin, data_path('escenario0_estudiantes.json'))
    JSON_man.dict2json(cls_fin, data_path('escenario0_clases.json'))
    print("Escenario 1 completado: resultados guardados en data/escenario0_*.json")
    porcentajes, equidad, eficiencia = imprimir_metricas(est_fin)
    sat, use = compute_metrics(est_fin, cls_fin, estudiantes, clases)
    return sat, use, porcentajes, equidad, eficiencia
    
def scenario1(estudiantes, clases): 
    pbs = PBS(copy.deepcopy(estudiantes), copy.deepcopy(clases))
    pbs.run_algo(delta_t=0.1)
    est_fin, cls_fin = pbs.round_greedy()
    JSON_man.dict2json(est_fin, data_path('escenario1_estudiantes.json'))
    JSON_man.dict2json(cls_fin, data_path('escenario1_clases.json'))
    print("Escenario 2 completado: resultados guardados en data/escenario1_*.json")
    porcentajes, equidad, eficiencia = imprimir_metricas(est_fin)
    sat, use = compute_metrics(est_fin, cls_fin, estudiantes, clases)
    return sat, use, porcentajes, equidad, eficiencia

    # Prioridad a un 20% de estudiantes
def scenario2(estudiantes, clases, prioridad_frac=0.2): 
    tot = len(estudiantes)
    num_prio = int(tot * prioridad_frac)
    lista_ids = list(estudiantes.keys())
    random.shuffle(lista_ids)
    grupo_prio = set(lista_ids[:num_prio])
    grupo_rest = set(lista_ids[num_prio:])

    # Fase prioridad
    est_prio = {e: estudiantes[e] for e in grupo_prio}
    cls_copy = copy.deepcopy(clases)
    pbs1 = PBS(est_prio, cls_copy)
    pbs1.run_algo(delta_t=0.1)
    est1, cls1 = pbs1.round_greedy()

    # Fase resto con cupos actualizados
    est_rest = {e: estudiantes[e] for e in grupo_rest}
    pbs2 = PBS(est_rest, cls1)
    pbs2.run_algo(delta_t=0.1)
    est2, cls2 = pbs2.round_greedy()

    # Combinar resultados
    resultado_est = {**est1, **est2}
    JSON_man.dict2json(resultado_est, data_path('escenario2_estudiantes.json'))
    JSON_man.dict2json(cls2, data_path('escenario2_clases.json'))
    print("Escenario 3 completado: prioridad 20%, resultados en data/escenario2_*.json")
    porcentajes, equidad, eficiencia = imprimir_metricas(resultado_est)
    sat, use = compute_metrics(resultado_est, cls2, estudiantes, clases)
    return sat, use, porcentajes, equidad, eficiencia
    

    # Cancelación temporal de 30% de clases
def scenario3(estudiantes, clases, cancel_frac=0.3):
    clase_ids = list(clases.keys())
    random.shuffle(clase_ids)
    num_cancel = int(len(clase_ids) * cancel_frac)
    canceladas = set(clase_ids[:num_cancel])

    # Primera fase: clases canceladas con cupos 0
    clases1 = copy.deepcopy(clases)
    for cid in canceladas:
        clases1[cid]['cupos'] = 0
        clases1[cid]['cupos_fraccional'] = 0.0

    pbs1 = PBS(estudiantes, clases1)
    consumo1 = pbs1.run_algo(delta_t=0.1)
    # No redondeo aún

    # Segunda fase: restaurar cupos y redondeo final
    clases2 = copy.deepcopy(clases)
    pbs2 = PBS(estudiantes, clases2)
    pbs2.diccionario_consumo = consumo1  # mantenemos consumo previo
    est_fin, cls_fin = pbs2.round_greedy()

    JSON_man.dict2json(est_fin, data_path('escenario3_estudiantes.json'))
    JSON_man.dict2json(cls_fin, data_path('escenario3_clases.json'))
    print("Escenario 4 completado: cancelación 30%, resultados en data/escenario3_*.json")
    porcentajes, equidad, eficiencia = imprimir_metricas(est_fin)
    sat, use = compute_metrics(est_fin, cls_fin, estudiantes, clases)
    return sat, use, porcentajes, equidad, eficiencia


def simulate(n=1):
    historial = []
    for i in range(n):
        print(f"Simulación {i+1}/{n}")
        cursos, docentes, aulas, horarios, clases, estudiantes = generate_base_data()
        sat, use, porcentajes, equidad, eficiencia = scenario0(estudiantes, clases)
        historial.append({
            'iteración': i+1,
            'satisfacción': sat,
            'uso_cupos': use,
            'porcentajes': porcentajes,
            'equidad': equidad,
            'eficiencia_pareto': eficiencia
        })

    # Calcular promedios
    avg_satisfaccion = sum(r['satisfacción'] for r in historial) / n
    avg_uso         = sum(r['uso_cupos']    for r in historial) / n
    avg_porcentajes = {
        'primera_opcion': sum(r['porcentajes']['primera_opcion'] for r in historial) / n,
        'segunda_opcion': sum(r['porcentajes']['segunda_opcion'] for r in historial) / n,
        'tercera_opcion': sum(r['porcentajes']['tercera_opcion'] for r in historial) / n,
        'las_tres':       sum(r['porcentajes']['las_tres']          for r in historial) / n
    }
    avg_equidad     = sum(r['equidad']     for r in historial) / n
    avg_eficiencia  = sum(r['eficiencia_pareto'] for r in historial) / n

    resumen = {
        'numero_iteraciones': n,
        'promedios': {
            'satisfacción_media': avg_satisfaccion,
            'uso_cupos':          avg_uso,
            'porcentajes':        avg_porcentajes,
            'equidad':            avg_equidad,
            'eficiencia_pareto':  avg_eficiencia
        },
        'iteraciones': historial
    }
    print("Resumen de la simulación:")
    print("Satisfacción media:", avg_satisfaccion)
    print("Uso de cupos:", avg_uso)
    print("Porcentaje de estudiantes que obtuvieron:")
    print(" - Primera opción:", avg_porcentajes['primera_opcion'])
    print(" - Segunda opción:", avg_porcentajes['segunda_opcion'])
    print(" - Tercera opción:", avg_porcentajes['tercera_opcion'])
    print(" - Las tres primeras opciones:", avg_porcentajes['las_tres'])
    print("Equidad:", avg_equidad)
    print("Eficiencia de Pareto:", avg_eficiencia)
    
    JSON_man.dict2json(resumen, data_path('simulation_results.json'))
    print("Resultados de la simulación guardados en:", data_path('simulation_results.json'))
    return resumen