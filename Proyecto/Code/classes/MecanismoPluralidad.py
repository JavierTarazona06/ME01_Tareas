from classes.MecanismoVotacion import MecanismoVotacion
from collections import defaultdict
from typing import Dict, Tuple


# metodo de MecanismoPluralidad para asignar puntuaciones de borda
class MecanismoPluralidad(MecanismoVotacion):
    def realizarVotacion(self, grupo) -> Tuple[str, Dict[str, int]]:
        """Realiza una votación por pluralidad"""
        votos = defaultdict(int)
        for actor in grupo.actores:
            voto = actor.votar()
            votos[voto] += 1

        ganador = max(votos.items(), key=lambda x: x[1])
        return ganador
