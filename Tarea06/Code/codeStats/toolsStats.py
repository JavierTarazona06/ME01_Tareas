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
    def __init__(self, estudiantes: Dict, clases: Dict, sort: bool = True):

        estudiantes_IDs = list(estudiantes.keys())
        if len(estudiantes_IDs) != len(set(estudiantes_IDs)):
            raise ValueError("Las cédulas nde estudiantes no son úncias")

        clases_IDs = list(clases.keys())
        if len(clases_IDs) != len(set(clases_IDs)):
            raise ValueError("Los códigos de clases no son ´´unicos")

        self.estudiantes = copy.deepcopy(estudiantes)
        self.clases = copy.deepcopy(clases)
        self.sort = sort

        for cid, clase in self.clases.items():
            clase["cupos_fraccional"] = float(clase["cupos"])

        # Diccionario de consumo (cedula, clase) → 0.0
        self.diccionario_consumo = {
            (e, c): 0.0
            for e in self.estudiantes.keys()
            for c in self.clases.keys()
        }

    def run_algo(
            self, delta_t: float = 0.1,
            min_delta: float = 1e-6, max_iter: int = 10000
    ) -> dict[tuple[str, str], int]:

        iteracion = 0
        if (self.sort):
            estudiantes_ordenados = sorted(
                self.estudiantes.values(),
                key=lambda e: e["p.a.p.i"],
                reverse=True  # Para ordenar de mayor a menor
            )
        else:
            estudiantes_ordenados = list(self.estudiantes.values())

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
                self.diccionario_consumo[
                    (estudiante["cedula"], asignatura_actual)
                ] += consumo

                progreso = True

            if not progreso:
                break

            iteracion += 1

        return self.diccionario_consumo

    def round_greedy(self):
        """
        Tras run_algo(), asigna cupos enteros según fracciones consumidas,
        respetando orden de preferencias y límite de materias a inscribir.
        """
        # Cada estudiante solo puede inscribir hasta X materias
        cuotas = {
            est["cedula"]: est["cantidad_materias_inscribir"]
            for est in self.estudiantes.values()
        }

        # Para cada estudiante, ordenamos sus fracciones consumidas
        for est in sorted(
            self.estudiantes.values(),
            key=lambda e: e["p.a.p.i"], reverse=True
        ):
            ced = est["cedula"]
            # Obtener tuplas (clase, cantidad_fraccional) con consumo > 0
            consumos = [
                (clase, self.diccionario_consumo[(ced, clase)])
                for clase in est["lista_preferencias"]
                if self.diccionario_consumo[(ced, clase)] > 0
            ]
            # Orden descendente por fracción consumida
            consumos.sort(key=lambda x: x[1], reverse=True)

            for clase_id, frac in consumos:
                if cuotas[ced] <= 0:
                    break
                # Si aún hay cupos enteros disponibles
                if self.clases[clase_id]["cupos"] >= 1:
                    est["lista_materias_asignadas"].append(clase_id)
                    self.clases[clase_id]["cupos"] -= 1
                    cuotas[ced] -= 1

        return self.estudiantes, self.clases



