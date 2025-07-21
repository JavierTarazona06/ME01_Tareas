from typing import List, Dict, Tuple
from classes.MetodoVotacion import MetodoVotacion


class Actor:
    def __init__(
        self,
        nombre: str,
        preferencias: List[str],
        metodoVotacion: MetodoVotacion,
        opcionesDisponibles: List[str],
    ):
        self.nombre = nombre
        self.preferencias = preferencias  # Lista ordenada de más a menos preferido
        self.utilidad = {
            destino: len(preferencias) - i for i, destino in enumerate(preferencias)
        }
        self.metodoVotacion = metodoVotacion
        self.opcionesDisponibles = opcionesDisponibles

    def votar(self):
        return self.metodoVotacion.votar(self)

    def getMetodoVotacion(self) -> MetodoVotacion:
        return self.metodoVotacion

    def setMetodoVotacion(self, metodoVotacion: MetodoVotacion):
        self.metodoVotacion = metodoVotacion

    def getUtilidad(self):
        return self.utilidad

    def __str__(self):
        return f"{self.nombre}: {', '.join(self.preferencias)}"
