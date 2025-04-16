import random
import string
from typing import Dict, Optional

from tools.FileManager import JSON_man
from codeStats.distributions import Normal
from constants.program import CURSOS_CT, DOCENTES_CT, AULAS_CT


class Utils:
    @staticmethod
    def generar_codigo_unico(
            existentes: set, longitud: int = 10,
            numeros: bool = True, letras: bool = True
    ) -> str:
        """
        Genera un código único, según parámetros de entrada,
        no presente en el conjunto 'existentes'.
        """
        if not numeros and not letras:
            raise ValueError("Debe permitir al menos letras o "
                             "números para generar el código.")

        caracteres = ''
        if numeros:
            caracteres += string.digits
        if letras:
            caracteres += string.ascii_uppercase

        while True:
            codigo = ''.join(random.choices(caracteres, k=longitud))
            if codigo not in existentes:
                return codigo

    @staticmethod
    def generar_codigos_unicos(
            cantidad, longitud_codigo: int = 10,
            numeros: bool = True, letras: bool = True
    ) -> set:
        """
        Genera un set de código unicos, según parámetros de entrada,
        """

        codigos_generados = set()

        if not numeros and not letras:
            raise ValueError("Debe permitir al menos letras o "
                             "números para generar el código.")

        caracteres = ''
        if numeros:
            caracteres += string.digits
        if letras:
            caracteres += string.ascii_uppercase

        cc = 0
        while cc < cantidad:
            codigo = ''.join(random.choices(caracteres, k=longitud_codigo))
            if codigo not in codigos_generados:
                codigos_generados.add(codigo)
                cc += 1

        return codigos_generados

    @staticmethod
    def generar_codigos_unicos_pro(
            cantidad, longitud_codigo: int = 10,
            numeros: bool = True, long_num: int = 0,
            letras: bool = True, order: int = 0
    ) -> set:
        """
        Genera un set de código unicos, según parámetros de entrada,

        order:
            - 0 Not order
            - 1 number first, then str
            - 2 number second, first str
        """

        codigos_generados = set()

        if not numeros and not letras:
            raise ValueError("Debe permitir al menos letras o "
                             "números para generar el código.")

        caracteres = ''
        if numeros:
            caracteres += string.digits
        if letras:
            caracteres += string.ascii_uppercase

        cc = 0
        while cc < cantidad:
            if order == 1 and numeros and letras:
                codigo = ''.join(random.choices(string.digits, k=long_num))
                codigo += ''.join(random.choices(string.ascii_uppercase, k=longitud_codigo - long_num))
            elif order == 2 and numeros and letras:
                codigo = ''.join(random.choices(string.ascii_uppercase, k=longitud_codigo - long_num))
                codigo += ''.join(random.choices(string.digits, k=long_num))
            else:
                codigo = ''.join(random.choices(caracteres, k=longitud_codigo))
            if codigo not in codigos_generados:
                codigos_generados.add(codigo)
                cc += 1

        return codigos_generados


class Curso:

    @staticmethod
    def crear_cursos(
            cantidad_cursos: int,
            num_grupos_min: int, num_grupos_max: int,
            num_grupos_avg: Optional[float] = None,
            num_grupos_dev: Optional[float] = None
    ) -> Dict[str, Dict]:

        cursos = {}

        # Generar lista de número de grupos para cada curso
        if num_grupos_avg is not None and num_grupos_dev is not None:
            # Distribución normal truncada para los grupos
            numero_grupos = Normal.generar_enteros_normal_truncada(
                cantidad_cursos, num_grupos_min, num_grupos_max,
                num_grupos_avg, num_grupos_dev
            )
        else:
            # Distribución uniforme entre num_grupos_min y num_grupos_max
            numero_grupos = [random.randint(num_grupos_min, num_grupos_max)
                             for _ in range(cantidad_cursos)]

        # Crear los codigos de los cursos
        codigos_unicos = list(Utils.generar_codigos_unicos_pro(
            cantidad_cursos, long_num=7, order=1
        ))

        # Crear los cursos con sus códigos y números de grupos
        for i in range(cantidad_cursos):
            codigo = codigos_unicos[i]
            cursos[codigo] = {
                'codigo': codigo,
                'programado': False,
                'num_grupos': numero_grupos[i]
            }

        return cursos


class Docente:

    @staticmethod
    def crear_docentes(
            cantidad_docentes: int,
    ) -> Dict[str, Dict]:
        docentes = {}

        # Crear las cedulas
        cedulas = list(
            Utils.generar_codigos_unicos(
                cantidad_docentes, longitud_codigo=10,
                numeros=True, letras=False
            )
        )

        # Crear los docentes
        for i in range(cantidad_docentes):
            cedula = cedulas[i]
            docentes[cedula] = {
                'cedula': cedula,
                'programado': False
            }

        return docentes


class Aula:

    @staticmethod
    def crear_aulas(
            cantidad_aulas: int,
    ) -> Dict[str, Dict]:
        aulas = {}

        # Crear los id
        IDs = list(
            Utils.generar_codigos_unicos(
                cantidad_aulas, longitud_codigo=6,
                numeros=True, letras=False
            )
        )

        # Crear las aulas
        for i in range(cantidad_aulas):
            edificio = IDs[i][:3]
            aula_id = IDs[i][3:]
            aulas[IDs[i]] = {
                'edificio': edificio,
                'aula_id': aula_id,
                "IDs": IDs[i],
                'programado': False
            }

        return aulas

class Horaio:

    @staticmethod
    def crear_horarios():
        dias = ["1-3", "2-4", "3-5", "6"]
        franjas = [
            "7-9", "9-11", "11-13", "14-16",
            "16-18", "18-20", "7-13", "14-18"
        ]

        # Construcción del diccionario
        horarios = {}
        id_counter = 1

        for dia in dias:
            for franja in franjas:
                horarios[str(id_counter)] = {
                    "id": id_counter,
                    "dias": dia,
                    "franja": franja
                }
                id_counter += 1

        return horarios


class Clase:
    @staticmethod
    def crear_clases(
            docentes:dict, aulas:dict, horarios:dict, cursos: Dict
    ) -> Dict[str, Dict]:
        clases = {}

        docentes_IDs_elegibles = [docente["cedula"] for docente in docentes.values()]
        aulas_IDs_elegibles = [aula["IDs"] for aula in aulas.values()]

        # Seleccionar docente aleatorio
        docente_id_seleccionado = docentes_IDs_elegibles[
            random.randint(0, len(docentes_IDs_elegibles)-1)
        ]
        docentes_IDs_elegibles.remove(docente_id_seleccionado)

        # Seleccionar aula aleatoria
        aula_id_seleccionada = aulas_IDs_elegibles[
            random.randint(0, len(aulas_IDs_elegibles)-1)
        ]
        aulas_IDs_elegibles.remove(aula_id_seleccionada)



        return clases


def generar_base_datos():
    cursos_dict = Curso.crear_cursos(
        CURSOS_CT["cantidad"], CURSOS_CT["num_grupos_min"],
        CURSOS_CT["num_grupos_max"], CURSOS_CT["num_grupos_avg"],
        CURSOS_CT["num_grupos_dev"]
    )
    JSON_man.dict2json(cursos_dict, "data/cursos.json")

    docentes_dict = Docente.crear_docentes(
        DOCENTES_CT["cantidad"]
    )
    JSON_man.dict2json(docentes_dict, "data/docentes.json")

    aulas_dict = Aula.crear_aulas(
        AULAS_CT["cantidad"]
    )
    JSON_man.dict2json(aulas_dict, "data/aulas.json")

    horarios_dict = Horaio.crear_horarios()
    JSON_man.dict2json(horarios_dict, "data/horarios.json")