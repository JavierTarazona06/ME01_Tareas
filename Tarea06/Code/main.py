
from test import *
from tools.FileManager import JSON_man

def main():
    print("!")

    # SI la base de datos no esta generada, llamar create_DB() de test.py

    docentes = JSON_man.json2dict("data/docentes.json")
    aulas = JSON_man.json2dict("data/aulas.json")
    cursos = JSON_man.json2dict("data/cursos.json")
    horarios = JSON_man.json2dict("data/horarios.json")

    create_and_save_clases(docentes, aulas, horarios, cursos)


main()