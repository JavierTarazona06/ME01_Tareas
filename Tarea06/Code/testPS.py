
import os
from data.generator import Curso, Docente, Aula, Horario, Clase, Estudiante
from tools.FileManager import JSON_man
from constants.program import CURSOS_CT, DOCENTES_CT, AULAS_CT, CLASES_CT, ESTUDIANTES_CT
from codeStats.toolsStats import PBS

# Configurar directorio 'data'
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, 'data')
os.makedirs(DATA_DIR, exist_ok=True)

def data_path(filename: str) -> str:
    return os.path.join(DATA_DIR, filename)


def main():
    # 1. Generar y guardar Cursos
    cursos = Curso.crear_cursos(
        CURSOS_CT['cantidad'], CURSOS_CT['num_grupos_min'], CURSOS_CT['num_grupos_max'],
        CURSOS_CT.get('num_grupos_avg'), CURSOS_CT.get('num_grupos_dev')
    )
    JSON_man.dict2json(cursos, data_path('cursos.json'))

    # 2. Generar y guardar Docentes
    docentes = Docente.crear_docentes(DOCENTES_CT['cantidad'])
    JSON_man.dict2json(docentes, data_path('docentes.json'))

    # 3. Generar y guardar Aulas
    aulas = Aula.crear_aulas(AULAS_CT['cantidad'])
    JSON_man.dict2json(aulas, data_path('aulas.json'))

    # 4. Generar y guardar Horarios
    horarios = Horario.crear_horarios()
    JSON_man.dict2json(horarios, data_path('horarios.json'))

    # 5. Generar y guardar Clases
    clases = Clase.crear_clases(
        docentes, aulas, horarios, cursos,
        CLASES_CT['num_cupos_min'], CLASES_CT['num_cupos_max'],
        CLASES_CT.get('num_cupos_avg'), CLASES_CT.get('num_cupos_dev')
    )
    JSON_man.dict2json(clases, data_path('clases.json'))

    # 6. Generar y guardar Estudiantes
    estudiantes = Estudiante.crear_estudiantes(
        ESTUDIANTES_CT['cantidad'], set(docentes.keys()),
        ESTUDIANTES_CT['p.a.p.i_avg'], ESTUDIANTES_CT['p.a.p.i_dev'],
        cursos, clases
    )
    JSON_man.dict2json(estudiantes, data_path('estudiantes.json'))

    # 7. Ejecutar algoritmo PS directamente con datos en memoria
    pbs = PBS(estudiantes, clases)
    consumo_fraccional = pbs.run_algo(delta_t=0.1)
    estudiantes_finales, clases_finales = pbs.round_greedy()

    # 8. Guardar resultados finales
    JSON_man.dict2json(estudiantes_finales, data_path('estudiantes_finales.json'))
    JSON_man.dict2json(clases_finales, data_path('clases_finales.json'))

    # 9. Mostrar resumen por consola
    print('--- Consumo fraccional (solo entradas > 0) ---')
    for (cedula, clase_id), amt in consumo_fraccional.items():
        if amt > 0:
            print(f'Estudiante {cedula} consumió {amt:.2f} de la clase {clase_id}')

    print('\n--- Asignaciones finales de cada estudiante ---')
    for est in estudiantes_finales.values():
        cedula = est['cedula']
        asignadas = est.get('lista_materias_asignadas', [])
        print(f'{cedula} -> {asignadas}')

if __name__ == '__main__':
    main()
