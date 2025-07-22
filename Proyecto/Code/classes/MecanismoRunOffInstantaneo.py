from utils import random_utils as random

from classes.MecanismoVotacion import MecanismoVotacion
from collections import defaultdict
from typing import Dict, Tuple, Optional


class MecanismoRunOffInstantaneo(MecanismoVotacion):

    def realizarVotacion(self, grupo) -> Tuple[Optional[str], Dict[str, int]]:
        """Realiza una votación por IRV"""
        ronda = 1
        opciones_activas = set(grupo.opciones)
        historial_votaciones = {}

        while True:
            # print(f"\nRonda {ronda} de IRV")
            # print(f"Destinos activos: {', '.join(opciones_activas)}")

            # Contar primeros puestos
            conteo_votos = defaultdict(int)
            for actor in grupo.actores:
                preferencias = actor.votar()
                if preferencias:  # Si todavía tiene preferencias
                    primer_opcion = preferencias[0]
                    conteo_votos[primer_opcion] += 1

            historial_votaciones[ronda] = dict(conteo_votos)
            # print("Conteo de votos:", dict(conteo_votos))

            # Verificar si hay ganador (mayoría absoluta)
            total_votos = sum(conteo_votos.values())
            for destino, votos in conteo_votos.items():
                if votos > total_votos / 2:
                    # print(
                    #    f"{destino} obtiene mayoría absoluta con {votos}/{total_votos} votos"
                    # )
                    return [destino, historial_votaciones]

            # Si no hay mayoría, eliminar el destino con menos votos
            if len(conteo_votos) <= 1:
                # print("Empate - no se puede eliminar más destinos")
                return [None, historial_votaciones]

            # Encontrar el destino con menos votos (puede haber empates)
            min_votos = min(conteo_votos.values())
            destinos_a_eliminar = [d for d, v in conteo_votos.items() if v == min_votos]

            # Si hay empate para eliminar, elegir uno aleatoriamente
            destino_eliminado = random.choice(destinos_a_eliminar)
            # print(f"Eliminando {destino_eliminado} (menos votos: {min_votos})")

            for actor in grupo.actores:
                actor.getMetodoVotacion().eliminar_destino(destino_eliminado)
            opciones_activas.remove(destino_eliminado)

            ronda += 1
