import random
from collections import defaultdict
from typing import List, Dict, Tuple

from classes.Grupo import Grupo


class Simulador:
    def __init__(self, seed: int, grupo: Grupo):
        random.seed(seed)  # Para reproducibilidad
        self.grupo = grupo

    def simular_grupo(self):
        resultados = self.grupo.realizarVotacion()

        return resultados

        # Realizar diferentes tipos de votación
        print("\nResultados de votación:")
        pluralidad = self.grupo.votacion_pluralidad()
        print(f"Pluralidad: {pluralidad[0]} con {pluralidad[1]} votos")

        borda = self.grupo.votacion_borda()
        print(f"Borda: {borda[0]} con {borda[1]} puntos")

        aprobacion = self.grupo.votacion_aprobacion()
        print(f"Aprobación: {aprobacion[0]} con {aprobacion[1]} aprobaciones")

        # Negociación
        negociacion = self.grupo.negociacion_alternante()
        print(
            f"\nNegociación alternante: {negociacion[0]} (utilidad grupal: {negociacion[1]})"
        )

        # Ganador de Condorcet
        condorcet = self.grupo.encontrar_condorcet()
        if condorcet:
            print(f"\nGanador de Condorcet: {condorcet}")
        else:
            print("\nNo hay ganador de Condorcet (paradoja de Condorcet presente)")

    def __str__(self):
        return f"{self.nombre}: {', '.join(self.preferencias)}"
