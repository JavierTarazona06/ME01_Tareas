import random
import string
from typing import Dict, Optional

from tools.FileManager import JSON_man
from codeStats.distributions import Normal
from codeStats.toolsStats import UtilsStats
from constants.program import CURSOS_CT, DOCENTES_CT, AULAS_CT, ESTUDIANTES_CT


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
        numero_grupos = Normal.generar_numeros_normal(
            cantidad_cursos, num_grupos_min, num_grupos_max,
            num_grupos_avg, num_grupos_dev
        )

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


class Horario:

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
            docentes: dict, aulas: dict, horarios: dict, cursos: Dict,
            num_cupos_min: int, num_cupos_max: int,
            num_cupos_avg: float, num_cupos_dev: float
    ) -> Dict[str, Dict]:
        """
        Los cursos se modifican
        """
        clases = {}

        # Crear base de docentes y aulas elegibles
        docentes_IDs_elegibles = [docente["cedula"] for docente in docentes.values()]
        aulas_IDs_elegibles = [aula["IDs"] for aula in aulas.values()]

        # Crear diccionarios de asignaciones por franja horaria
        asignaciones_horarios = {key: 0 for key in horarios.keys() if int(key) < 17}

        cantidad_cursos = len(cursos.items())
        conteo_cursos = 0
        codigos_clases = set()
        flag_5percent = False
        for cod_curso, curso in cursos.items():
            cursos[cod_curso]["clases_asociadas"] = []
            for grupo in range(curso["num_grupos"]):

                # Si faltan el 5% de cursos por asignar, agregue los viernes y sábados
                if (conteo_cursos / cantidad_cursos) >= 0.95 and not flag_5percent:
                    asignaciones_horarios.update(
                        {key: 0 for key in horarios.keys() if int(key) > 16}
                    )
                    flag_5percent = True

                # Seleccionar docente aleatorio
                docente_id_seleccionado = UtilsStats.pick_random_element_list(
                    docentes_IDs_elegibles
                )

                # Seleccionar aula aleatoria
                aula_id_seleccionada = UtilsStats.pick_random_element_list(
                    aulas_IDs_elegibles
                )

                # Seleccionar clase aleatoria
                horario_id_seleccionado = UtilsStats.pick_prob_adaptativa_inversa(
                    asignaciones_horarios
                )

                new_class_code = Utils.generar_codigo_unico(codigos_clases)
                codigos_clases.add(new_class_code)

                clases[new_class_code] = {
                    "codigo_clase": new_class_code,
                    "codigo_curso": cod_curso,
                    "cedula_docente": docente_id_seleccionado,
                    "horario": horario_id_seleccionado,
                    "codigo_aula": aula_id_seleccionada,
                    "grupo": grupo + 1,
                    "cupos": 0
                }

                cursos[cod_curso]["clases_asociadas"].append(new_class_code)

            conteo_cursos += 1

        # Generar lista de números de cupos para cada curso
        numero_cupos = Normal.generar_numeros_normal(
            len(clases.items()), num_cupos_min, num_cupos_max,
            num_cupos_avg, num_cupos_dev
        )

        for it, cod_clase in enumerate(clases.keys()):
            clases[cod_clase]["cupos"] = numero_cupos[it]

        return clases


class Estudiante:

    @staticmethod
    def crear_estudiantes(
            cantidad_estudiantes: int, cedulas_existentes: set,
            papi_avg: float, papi_dev: float, cursos: Dict,
            clases: Dict
    ):
        estudiantes_dict = {}

        # Generar lista de P.A.P.I's para cada estudiante
        papi_list = Normal.generar_numeros_normal(
            cantidad_elementos=cantidad_estudiantes, minimo=0, maximo=5,
            media=papi_avg, desviacion=papi_dev, to_int=False
        )

        if (ESTUDIANTES_CT["materias_a_inscribir"] >
                ESTUDIANTES_CT["materias_deseadas"]):
            raise ValueError("Las materias que un estudiante va a inscribir "
                             "tienen que ser menor o igual a las materias "
                             "que el estduainte desea")

        for it_estudiante in range(cantidad_estudiantes):

            # Creacion Cedula
            cedula = Utils.generar_codigo_unico(cedulas_existentes, letras=False)
            cedulas_existentes.add(cedula)

            # Materias a inscribir y deseadas
            while True:
                cantidad_materias = random.randint(
                    3, ESTUDIANTES_CT["materias_a_inscribir"]
                )
                cantidad_materias_deseadas = random.randint(
                    3, ESTUDIANTES_CT["materias_deseadas"]
                )

                if cantidad_materias <= cantidad_materias_deseadas:
                    break

            # Lista de deseos de materias
            lista_preferencias = []
            cursos_elegibles = list(cursos.keys())
            horarios_tomados = set()

            for i in range(cantidad_materias_deseadas):
                if len(cursos_elegibles) == 0:
                    break
                curso_seleccionado_id = cursos_elegibles[
                    random.randint(0, len(cursos_elegibles)-1)
                ]
                cursos_elegibles.remove(curso_seleccionado_id)
                curso_seleccionado:Dict = cursos[curso_seleccionado_id]

                clases_elegibles = list(curso_seleccionado["clases_asociadas"].copy())
                while True:
                    if len(clases_elegibles) == 0:
                        break
                    clase_seleccionada_id = clases_elegibles[
                        random.randint(0, len(clases_elegibles)-1)
                    ]
                    clases_elegibles.remove(clase_seleccionada_id)
                    clase_seleccionada: Dict = clases[clase_seleccionada_id]

                    if not (clase_seleccionada["horario"] in horarios_tomados):
                        horarios_tomados.add(clase_seleccionada["horario"])
                        lista_preferencias.append(clase_seleccionada_id)
                        break

            cantidad_materias_deseadas = len(lista_preferencias)
            if cantidad_materias > cantidad_materias_deseadas:
                cantidad_materias = cantidad_materias_deseadas

            # Lista de materias asignadas
            asignaciones = []

            estudiantes_dict[cedula] = {
                "cedula": cedula,
                "p.a.p.i": papi_list[it_estudiante],
                "cantidad_materias_inscribir": cantidad_materias,
                "cantidad_materias_deseadas": cantidad_materias_deseadas,
                "lista_preferencias": lista_preferencias,
                "lista_materias_asignadas": asignaciones
            }

        return estudiantes_dict
