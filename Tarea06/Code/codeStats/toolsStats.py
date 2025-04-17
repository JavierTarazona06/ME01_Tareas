import copy
import random
from typing import Any, Dict, Tuple


class UtilsStats:
    @staticmethod
    def pick_random_element_list(lista_elegibles:list) -> Any:
        """
            In Place Change
        """
        elemento_seleccionado = lista_elegibles[
            random.randint(0, len(lista_elegibles) - 1)
        ]
        lista_elegibles.remove(elemento_seleccionado)
        return elemento_seleccionado

    @staticmethod
    def pick_prob_adaptativa_inversa(asignaciones_dict: Dict[str, Any]) -> Any:
        """

        :param asignaciones_dict: Es un diccionario que contiene como
            clave el elemento y valor el número de asignaciones que tiene
        """
        pesos = {id: 1 / (1 + count) for id, count in asignaciones_dict.items()}
        #   Normalizar
        total = sum(pesos.values())
        probabilidades = [peso / total for peso in pesos.values()]
        elementos = list(pesos.keys())
        #   Seleccionar aleatoriamente un día según la distribución adaptativa
        elemento_seleccionado = random.choices(
            elementos, weights=probabilidades, k=1
        )[0]

        asignaciones_dict[elemento_seleccionado] += 1

        return elemento_seleccionado


# Probabilistic Serial Rule
class PBS:
    """
    No hace el cambio IN-Place.
    round_greedy retorna la base de estudiantes y clases modificada
    """
    def __init__(self, estudiantes: Dict, clases: Dict):

        estudiantes_IDs = list(estudiantes.keys())
        if len(estudiantes_IDs) != len(set(estudiantes_IDs)):
            raise ValueError("Las cédulas nde estudiantes no son úncias")

        clases_IDs = list(clases.keys())
        if len(clases_IDs) != len(set(clases_IDs)):
            raise ValueError("Los códigos de clases no son ´´unicos")

        self.estudiantes = copy.deepcopy(estudiantes)
        self.clases = copy.deepcopy(clases)

        # Agregar cupos fraccional
        for clase_id in clases_IDs:
            self.clases[clase_id]["cupos_fraccional"] = (
                self.clases[clase_id]["cupos"].copy()
            )

        # Diccionario de consumo
        for cedula in estudiantes_IDs:
            self.estudiantes[cedula]["diccionario_consumo"] = {}

    def run_algo(
            self, delta_t: float = 0.1,
            min_delta: float = 1e-6, max_iter: int = 10000
    ) -> dict[str, dict]:
        """
        Corre algoritmo PS y devuelve a la lista -> objeto de estudiantes
        """

        iteracion = 0
        estudiantes_ordenados = sorted(
            self.estudiantes.values(),
            key=lambda e: e["p.a.p.i"],
            reverse=True  # Para ordenar de mayor a menor
        )

        # Continuar mientras haya al menos una asignatura con capacidad y
        #   estudiantes con asignaturas disponibles
        while iteracion < max_iter:
            progreso = False  # Indica si en la iteración actual hubo consumo
            for estudiante in estudiantes_ordenados:
                # Buscar la asignatura más preferida con cupos disponibles
                asignatura_actual = None
                for codigo in estudiante["lista_preferencias"]:
                    if self.clases[codigo]["cupos_fraccional"] > min_delta:
                        asignatura_actual = codigo
                        break

                if asignatura_actual is None:
                    # Este estudiante no tiene más asignaturas disponibles, pasa al siguiente
                    continue

                # El estudiante consume la asignatura a tasa unitaria durante delta_t
                consumo = delta_t
                # Asegurarse de no consumir más que lo disponible
                if self.clases[asignatura_actual]["cupos_fraccional"] < consumo:
                    consumo =self.clases[asignatura_actual]["cupos_fraccional"]

                # Actualizar capacidad y registro de consumo para el estudiante
                self.clases[asignatura_actual]["cupos_fraccional"] -= consumo
                self.estudiantes[estudiante["cedula"]]["diccionario_consumo"][
                    "asignatura_actual"] = \
                    self.estudiantes[estudiante["cedula"]][
                        "diccionario_consumo"].get("asignatura_actual", 0.0) + consumo

                progreso = True

            if not progreso:
                break

            iteracion += 1

        return self.estudiantes

    def round_greedy(self) -> tuple[dict[str, dict], dict[str, dict]]:
        """
        First run "run_algo"
        """
        estudiantes_ordenados = sorted(
            self.estudiantes.values(),
            key=lambda e: e["p.a.p.i"],
            reverse=True  # Para ordenar de mayor a menor
        )

        for estudiante in estudiantes_ordenados:

            # Recuperar las fracciones elegidas por el estudiante actual en todas
            #   sus clases elegidas
            asignaturas_fraccionales_elegidas: list = list(
                self.estudiantes[estudiante["cedula"]]["diccionario_consumo"].keys()
            )

            # Ordenar las clases por preferencia de mayor a menor
            asignaturas_fraccionales_elegidas: list = sorted(
                asignaturas_fraccionales_elegidas,
                key=lambda d: list(d.values())[0],
                reverse=True
            )

            for dict_elements in asignaturas_fraccionales_elegidas:
                # TODO: Revisar esto:
                codigo_clase, fraccion = dict_elements.items()
                codigo_clase, fraccion = codigo_clase[0], fraccion[0]
                if self.clases["codigo_clase"]["cupos"] >= 1:
                    self.estudiantes[estudiante["cedula"]][
                        "lista_materias_asignadas"].append(codigo_clase)
                    self.clases["codigo_clase"]["cupos"] -= 1

        return self.estudiantes, self.clases



