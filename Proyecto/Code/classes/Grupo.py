import random
from collections import defaultdict
from typing import List, Dict, Tuple

from classes.Actor import Actor
from classes.MecanismoVotacion import MecanismoVotacion
from classes.Stat import Stat


class Grupo:
    def __init__(
        self,
        actores: List[Actor],
        opciones_disponibles: List[str],
        mecanismoVotacion: MecanismoVotacion,
    ):
        self.actores = actores
        self.opciones = opciones_disponibles
        self.mecanismoVotacion = mecanismoVotacion
        self.stats = []

    def realizarVotacion(self):
        return self.mecanismoVotacion.realizarVotacion(self)

    def setMecanismoVotacion(self, mecanismoVotacion: MecanismoVotacion):
        self.mecanismoVotacion = mecanismoVotacion

    def getOpciones(self):
        return self.opciones

    def registerStat(self, stat: Stat):
        self.stats.append(stat)

    def printStats(self):
        for stat in self.stats:
            stat.getStat(self)
