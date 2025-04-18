
from test import *
from tools.FileManager import JSON_man
from codeStats.metrics import imprimir_metricas

def main():
    print("!")

    # Sí la base de datos no está generada, llamar create_DB() de test.py
    #create_DB()

    docentes = JSON_man.json2dict("data/docentes.json")
    aulas = JSON_man.json2dict("data/aulas.json")
    cursos = JSON_man.json2dict("data/cursos.json")
    horarios = JSON_man.json2dict("data/horarios.json")

    # Crear las clases para la simulación,
    #   omitir si ya están creadas y solo cargar
    create_and_save_clases(docentes, aulas, horarios, cursos)
    clases = JSON_man.json2dict("data/clases.json")

    # Crear a los estudiantes para la simulación,
    #   omitir si ya están creados y solo cargar
    create_estudiantes(docentes, cursos, clases)
    estudiantes = JSON_man.json2dict("data/estudiantes.json")

    imprimir_metricas(estudiantes)


main()