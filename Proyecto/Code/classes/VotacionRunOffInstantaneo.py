from classes.MetodoVotacion import MetodoVotacion
from typing import List


class VotacionRunOffInstantaneo(MetodoVotacion):
    def __init__(self):
        self.eliminados = set()  # Para mantener registro de destinos eliminados

    def votar(self, actor) -> List[str]:
        """Devuelve la lista de preferencias del actor, excluyendo los destinos eliminados"""
        # Filtramos las preferencias quitando los destinos ya eliminados
        return [d for d in actor.preferencias if d not in self.eliminados]

    def eliminar_destino(self, destino: str):
        """Marca un destino como eliminado para futuras rondas"""
        self.eliminados.add(destino)

    def reset(self):
        """Reinicia el método para una nueva votación"""
        self.eliminados = set()
