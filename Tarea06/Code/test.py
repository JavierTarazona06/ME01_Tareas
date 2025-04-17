from typing import Dict

from data.generator import Clase, Curso, Docente, Aula, Horario, Estudiante
from tools.FileManager import JSON_man
from constants.program import CLASES_CT, CURSOS_CT, DOCENTES_CT, AULAS_CT, ESTUDIANTES_CT

def create_DB():
    # Crea la base de datos para la simulación

    # Crear Cursos y guardarlos en .json
    cursos_dict = Curso.crear_cursos(
        CURSOS_CT["cantidad"], CURSOS_CT["num_grupos_min"],
        CURSOS_CT["num_grupos_max"], CURSOS_CT["num_grupos_avg"],
        CURSOS_CT["num_grupos_dev"]
    )
    JSON_man.dict2json(cursos_dict, "data/cursos.json")

    # Crear Docentes y guardarlos en .json
    docentes_dict = Docente.crear_docentes(
        DOCENTES_CT["cantidad"]
    )
    JSON_man.dict2json(docentes_dict, "data/docentes.json")

    # Crear Aulas y guardarlos en .json
    aulas_dict = Aula.crear_aulas(
        AULAS_CT["cantidad"]
    )
    JSON_man.dict2json(aulas_dict, "data/aulas.json")

    # Crear Horarios y guardarlos en .json
    horarios_dict = Horario.crear_horarios()
    JSON_man.dict2json(horarios_dict, "data/horarios.json")

def create_and_save_clases(docentes, aulas, horarios, cursos):
    clases_dict = Clase.crear_clases(
        docentes, aulas, horarios, cursos,
        CLASES_CT["num_cupos_min"], CLASES_CT["num_cupos_max"],
        CLASES_CT["num_cupos_avg"], CLASES_CT["num_cupos_dev"]
    )
    JSON_man.dict2json(clases_dict, "data/clases.json")
    JSON_man.dict2json(cursos, "data/cursos.json")


def create_estudiantes(docentes:Dict, cursos: Dict, clases: Dict):
    estudiantes_dict = Estudiante.crear_estudiantes(
        ESTUDIANTES_CT["cantidad"], set(docentes.keys()),
        ESTUDIANTES_CT["p.a.p.i_avg"], ESTUDIANTES_CT["p.a.p.i_dev"],
        cursos, clases
    )
    JSON_man.dict2json(estudiantes_dict, "data/estudiantes.json")