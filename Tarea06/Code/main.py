
from test import *
from tools.FileManager import JSON_man
from codeStats.metrics import imprimir_metricas
from test import create_and_save_clases, create_estudiantes
from testPS import simulate
from constants.program import ITERACIONES_CT

def main():
    print("!")

    # Sí la base de datos no está generada, llamar create_DB() de test.py
    #create_DB()

    docentes = JSON_man.json2dict(r"data/docentes.json")
    aulas = JSON_man.json2dict(r"data/aulas.json")
    cursos = JSON_man.json2dict(r"data/cursos.json")
    horarios = JSON_man.json2dict(r"data/horarios.json")

    # Crear las clases para la simulación,
    #   omitir si ya están creadas y solo cargar
    create_and_save_clases(docentes, aulas, horarios, cursos)
    clases = JSON_man.json2dict(r"data/clases.json")

    # Crear a los estudiantes para la simulación,
    #   omitir si ya están creados y solo cargar
    create_estudiantes(docentes, cursos, clases)
    estudiantes = JSON_man.json2dict(r"data/estudiantes.json")

    simulate(ITERACIONES_CT)

main()