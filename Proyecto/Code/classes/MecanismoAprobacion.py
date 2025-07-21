from classes.MecanismoVotacion import MecanismoVotacion
from collections import defaultdict
from typing import Dict, Tuple


# metodo de MecanismoAprobacion para grupo
class MecanismoAprobacion(MecanismoVotacion):
    def realizarVotacion(self, grupo) -> Tuple[str, Dict[str, int]]:
        """Realiza una votación por aprobación"""
        aprobaciones = defaultdict(int)
        for actor in grupo.actores:
            votos = actor.votar()
            for destino in votos:
                aprobaciones[destino] += 1

        ganador = max(aprobaciones.items(), key=lambda x: x[1])
        return ganador
