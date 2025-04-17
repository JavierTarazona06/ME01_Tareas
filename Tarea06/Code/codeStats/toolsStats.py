import random
from typing import Any, Dict


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