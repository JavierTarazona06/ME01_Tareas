
from data.generator import Clase
from tools.FileManager import JSON_man
def main():
    print("!")

    docentes = JSON_man.json2dict("data/docentes.json")
    aulas = JSON_man.json2dict("data/aulas.json")
    cursos = JSON_man.json2dict("data/cursos.json")
    horarios = JSON_man.json2dict("data/horarios.json")
    Clase.crear_clases(docentes, aulas, cursos, horarios)

main()