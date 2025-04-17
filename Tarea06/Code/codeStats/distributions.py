import random
import warnings
from typing import List

class Normal:
    @staticmethod
    def generar_enteros_normal_truncada(
            cantidad_elementos: int,
            minimo: int, maximo: int,
            media: float, desviacion: float,
            max_intentos: int = None
        ) -> List[int]:
        """
        Genera una lista de enteros siguiendo una distribución normal truncada
        dentro de un rango.
        """

        if not max_intentos:
            max_intentos = max(1000, cantidad_elementos*100)

        resultados = []
        intentos = 0

        while len(resultados) < cantidad_elementos and intentos < max_intentos:
            valor = random.gauss(mu=media, sigma=desviacion)
            valor_entero = int(round(valor))
            if minimo <= valor_entero <= maximo:
                resultados.append(valor_entero)
            intentos += 1

        if len(resultados) < cantidad_elementos:
            # Si no se lograron suficientes valores,
            #   completar con valores aleatorios uniformes
            warnings.warn("La distribución truncada quedó incompleta."
                          "Se va a completar uniformemente.")
            while len(resultados) < cantidad_elementos:
                resultados.append(random.randint(minimo, maximo))

        return resultados
