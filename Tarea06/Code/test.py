

from data.generator import Clase
from tools.FileManager import JSON_man
from constants.program import CLASES_CT
from data.generator import generar_base_datos

def create_DB():
    # Crea la base de datos para la simulación
    generar_base_datos()

def create_and_save_clases(docentes, aulas, horarios, cursos):
    clases_dict = Clase.crear_clases(
        docentes, aulas, horarios, cursos,
        CLASES_CT["num_cupos_min"], CLASES_CT["num_cupos_max"],
        CLASES_CT["num_cupos_avg"], CLASES_CT["num_cupos_dev"]
    )
    JSON_man.dict2json(clases_dict, "data/clases.json")
