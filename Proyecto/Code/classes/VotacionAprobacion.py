from classes.MetodoVotacion import MetodoVotacion
from typing import List


# metodo de VotacionAprobacion para asignar puntuaciones de borda
class VotacionAprobacion(MetodoVotacion):
    def __init__(self, umbral: int):
        self.umbral = umbral

    def votar(self, actor) -> List[str]:
        """Aprueba los destinos que están en sus top N preferencias"""
        return actor.preferencias[: self.umbral]
