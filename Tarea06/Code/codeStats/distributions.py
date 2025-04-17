import random
import warnings
from typing import List


class Normal:
    @staticmethod
    def generar_numeros_normal_truncada(
            cantidad_elementos: int,
            minimo: int, maximo: int,
            media: float, desviacion: float,
            max_intentos: int = None, to_int: bool = True
    ) -> List[int | float]:
        """
        Genera una lista de enteros siguiendo una distribución normal truncada
        dentro de un rango.
        """

        if not max_intentos:
            max_intentos = max(1000, cantidad_elementos * 100)

        resultados = []
        intentos = 0

        while len(resultados) < cantidad_elementos and intentos < max_intentos:
            valor = random.gauss(mu=media, sigma=desviacion)
            valor = round(valor, 2)
            if to_int:
                valor = int(valor)
            if minimo <= valor <= maximo:
                resultados.append(valor)
            intentos += 1

        if len(resultados) < cantidad_elementos:
            # Si no se lograron suficientes valores,
            #   completar con valores aleatorios uniformes
            warnings.warn("La distribución truncada quedó incompleta."
                          "Se va a completar uniformemente.")
            while len(resultados) < cantidad_elementos:
                if to_int:
                    resultados.append(random.randint(minimo, maximo))
                else:
                    resultados.append(round(random.uniform(minimo, maximo), 2))

        return resultados

    @staticmethod
    def generar_numeros_normal(
            cantidad_elementos: int,
            minimo: int, maximo: int,
            media: float = None, desviacion: float = None, to_int: bool = True
    ) -> List[int | float]:

        if media is not None and desviacion is not None:
            # Distribución normal truncada para los cupos
            lista_final = Normal.generar_numeros_normal_truncada(
                cantidad_elementos, minimo, maximo,
                media, desviacion, to_int=to_int
            )
        else:
            # Distribución uniforme
            if to_int:
                lista_final = [random.randint(minimo, maximo)
                               for _ in range(cantidad_elementos)]
            else:
                lista_final = [round(random.uniform(minimo, maximo), 2)
                               for _ in range(cantidad_elementos)]

        return lista_final
