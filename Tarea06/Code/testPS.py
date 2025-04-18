
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
    historial = {'escenario0': [], 'escenario1': [], 'escenario2': [], 'escenario3': []}
    
    for i in range(n):
        print(f"Simulación {i+1}/{n}")
        # Generar datos base
        cursos, docentes, aulas, horarios, clases, estudiantes = generate_base_data()
        
        # Escenario 0
        sat0, use0, porcentajes0, equidad0, eficiencia0 = scenario0(estudiantes, clases)
        historial['escenario0'].append({
            'iteración': i+1,
            'satisfacción': sat0,
            'uso_cupos': use0,
            'porcentajes': porcentajes0,
            'equidad': equidad0,
            'eficiencia_pareto': eficiencia0
        })

        # Escenario 1
        sat1, use1, porcentajes1, equidad1, eficiencia1 = scenario1(estudiantes, clases)
        historial['escenario1'].append({
            'iteración': i+1,
            'satisfacción': sat1,
            'uso_cupos': use1,
            'porcentajes': porcentajes1,
            'equidad': equidad1,
            'eficiencia_pareto': eficiencia1
        })

        # Escenario 2
        sat2, use2, porcentajes2, equidad2, eficiencia2 = scenario2(estudiantes, clases)
        historial['escenario2'].append({
            'iteración': i+1,
            'satisfacción': sat2,
            'uso_cupos': use2,
            'porcentajes': porcentajes2,
            'equidad': equidad2,
            'eficiencia_pareto': eficiencia2
        })

        # Escenario 3
        sat3, use3, porcentajes3, equidad3, eficiencia3 = scenario3(estudiantes, clases)
        historial['escenario3'].append({
            'iteración': i+1,
            'satisfacción': sat3,
            'uso_cupos': use3,
            'porcentajes': porcentajes3,
            'equidad': equidad3,
            'eficiencia_pareto': eficiencia3
        })

    # Calcular promedios para cada escenario
    promedios = {}
    for escenario, datos in historial.items():
        promedios[escenario] = {
            'satisfacción_media': sum(d['satisfacción'] for d in datos) / n,
            'uso_cupos': sum(d['uso_cupos'] for d in datos) / n,
            'porcentajes': {
                'primera_opcion': sum(d['porcentajes']['primera_opcion'] for d in datos) / n,
                'segunda_opcion': sum(d['porcentajes']['segunda_opcion'] for d in datos) / n,
                'tercera_opcion': sum(d['porcentajes']['tercera_opcion'] for d in datos) / n,
                'las_tres': sum(d['porcentajes']['las_tres'] for d in datos) / n
            },
            'equidad': sum(d['equidad'] for d in datos) / n,
            'eficiencia_pareto': sum(d['eficiencia_pareto'] for d in datos) / n
        }
    
    # Crear el resumen final
    resumen = {
        'numero_iteraciones': n,
        'promedios_por_escenario': promedios,
        'iteraciones': historial
    }

    # Mostrar y guardar resultados
    for escenario, datos in promedios.items():
        print(f"\nResumen del {escenario}:")
        print("Satisfacción media:", datos['satisfacción_media'])
        print("Uso de cupos:", datos['uso_cupos'])
        print("Porcentaje de estudiantes que obtuvieron:")
        print(" - Primera opción:", datos['porcentajes']['primera_opcion'])
        print(" - Segunda opción:", datos['porcentajes']['segunda_opcion'])
        print(" - Tercera opción:", datos['porcentajes']['tercera_opcion'])
        print(" - Las tres primeras opciones:", datos['porcentajes']['las_tres'])
        print("Equidad:", datos['equidad'])
        print("Eficiencia de Pareto:", datos['eficiencia_pareto'])
    
    JSON_man.dict2json(resumen, data_path('simulation_results.json'))
    print("Resultados de la simulación guardados en:", data_path('simulation_results.json'))
    return resumen
