from classes.MecanismoVotacion import MecanismoVotacion
from collections import defaultdict
from typing import Dict, Tuple


# metodo de MecanismoBorda para asignar puntuaciones de borda
class MecanismoBorda(MecanismoVotacion):
    def realizarVotacion(self, grupo) -> Tuple[str, Dict[str, int]]:
        """Realiza una votación por conteo Borda"""
        puntuaciones = defaultdict(int)
        for actor in grupo.actores:
            votos = actor.votar()
            for destino, puntos in votos.items():
                puntuaciones[destino] += puntos

        ganador = max(puntuaciones.items(), key=lambda x: x[1])
        return ganador
