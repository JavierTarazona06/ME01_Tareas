from utils import random_utils as random  # Usar utilidades personalizadas
from collections import defaultdict
import sys
from datetime import datetime

# tipos de variables en py, no es libreria
from typing import List, Dict, Tuple

from classes.Simulator import Simulador
from classes.Actor import Actor
from classes.VotacionPluridad import VotacionPluralidad
from classes.Grupo import Grupo
from classes.MecanismoPluralidad import MecanismoPluralidad
from classes.MetodoVotacion import MetodoVotacion
from classes.VotacionBorda import VotacionBorda
from classes.MecanismoBorda import MecanismoBorda
from classes.VotacionAprobacion import VotacionAprobacion
from classes.MecanismoAprobacion import MecanismoAprobacion
from classes.MecanismoVotacion import MecanismoVotacion
from classes.MecanismoRunOffInstantaneo import MecanismoRunOffInstantaneo
from classes.VotacionRunOffInstantaneo import VotacionRunOffInstantaneo
from classes.NegociacionAlternada import NegociacionAlternada
from classes.MecanismoNegociacionAlternada import MecanismoNegociacionAlternada
from classes.MecanismoNegociacionAlternadaTrivial import (
    MecanismoNegociacionAlternadaTrivial,
)
from classes.NegociacionAlternadaImpaciente import NegociacionAlternadaImpaciente
from classes.CondorcetStat import CondorcetStat
from classes.MatrizDeUtilidadesStat import MatrizDeUtilidadesStat


def generar_preferencias(destinos: List[str], variacion: float = 0.3) -> List[str]:
    """Genera preferencias aleatorias con cierta tendencia común"""
    base = destinos.copy()
    random.shuffle(base)

    # Introducir variación individual
    for i in range(int(len(base) * variacion)):
        if random.random() > 0.5 and len(base) > 1:
            a, b = random.sample(range(len(base)), 2)
            base[a], base[b] = base[b], base[a]

    return base


def ejemploPluralidad(
    seed: int, turistas: List[Actor], destinos: List[str], grupo: Grupo
):

    metodoVotacionPluralidad = VotacionPluralidad()

    for turista in turistas:
        turista.setMetodoVotacion(metodoVotacionPluralidad)

    mecanismoVotacionPluralidad = MecanismoPluralidad()

    grupo.setMecanismoVotacion(mecanismoVotacionPluralidad)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(f"Pluralidad: {resultados[0]} con {resultados[1]} votos")


def ejemploBorda(seed: int, turistas: List[Actor], destinos: List[str], grupo: Grupo):

    metodoVotacionBorda = VotacionBorda()

    for turista in turistas:
        turista.setMetodoVotacion(metodoVotacionBorda)

    mecanismoVotacionBorda = MecanismoBorda()

    grupo.setMecanismoVotacion(mecanismoVotacionBorda)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(f"Borda: {resultados[0]} con puntuacion borda {resultados[1]}")


def ejemploAprobacion(
    seed: int, turistas: List[Actor], destinos: List[str], grupo: Grupo
):
    umbral_aprobacion = 2

    metodoVotacionAprobacion = VotacionAprobacion(umbral_aprobacion)

    for turista in turistas:
        turista.setMetodoVotacion(metodoVotacionAprobacion)

    mecanismoVotacionAprobacion = MecanismoAprobacion()

    grupo.setMecanismoVotacion(mecanismoVotacionAprobacion)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(f"Aprobacion: {resultados[0]} con puntuacion Aprobacion {resultados[1]}")


def ejemploRunOffInstantaneo(
    seed: int, turistas: List[Actor], destinos: List[str], grupo: Grupo
):

    metodoVotacionRunOffInstantaneo = VotacionRunOffInstantaneo()

    for turista in turistas:
        turista.setMetodoVotacion(metodoVotacionRunOffInstantaneo)

    mecanismoVotacionRunOffInstantaneo = MecanismoRunOffInstantaneo()

    grupo.setMecanismoVotacion(mecanismoVotacionRunOffInstantaneo)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(f"RunOffInstantaneo: {resultados[0]} con historial {resultados[1]}")


def ejemploNegociacionAlternadaTrivial(
    seed: int, turistas: List[Actor], destinos: List[str], grupo: Grupo
):

    for turista in turistas:
        metodoNegociacionAlternada = NegociacionAlternada()
        turista.setMetodoVotacion(metodoNegociacionAlternada)

    mecanismoNegociacionAlternada = MecanismoNegociacionAlternadaTrivial(5)

    grupo.setMecanismoVotacion(mecanismoNegociacionAlternada)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(
        f"NegociacionAlternadaTrivuial (cualquier opcion es mejor que 0): {resultados[0]} con puntuacion NegociacionAlternada {resultados[1]}"
    )


def ejemploNegociacionAlternada(
    seed: int, turistas: List[Actor], destinos: List[str], grupo: Grupo
):

    for turista in turistas:
        metodoNegociacionAlternada = NegociacionAlternada()
        turista.setMetodoVotacion(metodoNegociacionAlternada)

    mecanismoNegociacionAlternada = MecanismoNegociacionAlternada(5)

    grupo.setMecanismoVotacion(mecanismoNegociacionAlternada)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(
        f"NegociacionAlternada: {resultados[0]} con puntuacion NegociacionAlternada {resultados[1]}"
    )


def ejemploNegociacionAlternadaImpaciente(
    seed: int, turistas: List[Actor], destinos: List[str], grupo: Grupo
):
    coeficiente_impaciencia = 0.7
    for turista in turistas:
        metodoNegociacionAlternada = NegociacionAlternadaImpaciente(
            coeficiente_impaciencia
        )
        turista.setMetodoVotacion(metodoNegociacionAlternada)

    mecanismoNegociacionAlternada = MecanismoNegociacionAlternada(5)

    grupo.setMecanismoVotacion(mecanismoNegociacionAlternada)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(
        f"NegociacionAlternadaImpaciente: {resultados[0]} con puntuacion NegociacionAlternadaImpaciente {resultados[1]}"
    )


if __name__ == "__main__":
    seed = 42
    n_turistas = 10
    destinos = ["París", "Roma", "Barcelona", "Berlín", "Londres"]

    # configuracion log del programa
    fecha_hora = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    nombre_archivo = f"log-{fecha_hora}.txt"
    log_file = open(nombre_archivo, "w", encoding="utf-8")
    sys.stdout = log_file  # Redirige print() al archivo

    # Crear turistas con preferencias variadas
    turistas = []
    metodoVotacionGenerico = MetodoVotacion()

    metodoVotacionPluralidad = VotacionPluralidad()

    for i in range(n_turistas):
        nombre = f"Turista-{i+1}"
        preferencias = generar_preferencias(destinos)
        turistas.append(Actor(nombre, preferencias, metodoVotacionGenerico, destinos))

    mecanismoVotacionGenerico = MecanismoVotacion()

    grupo = Grupo(turistas, destinos, mecanismoVotacionGenerico)
    condorcetStat = CondorcetStat()
    grupo.registerStat(condorcetStat)

    grupo.printStats()

    # Mostrar preferencias
    print("\nPreferencias individuales:")
    for turista in turistas:
        print(turista)

    ejemploPluralidad(seed, turistas, destinos, grupo)
    ejemploBorda(seed, turistas, destinos, grupo)
    ejemploAprobacion(seed, turistas, destinos, grupo)
    ejemploRunOffInstantaneo(seed, turistas, destinos, grupo)

    print("Ejemplos negociacion")
    print()

    ## se cambian los turistas a 2 para hacer ejemplos de negociaciones
    n_turistas = 2

    # Crear turistas con preferencias variadas
    turistas = []
    metodoVotacionGenerico = MetodoVotacion()

    metodoVotacionPluralidad = VotacionPluralidad()

    for i in range(n_turistas):
        nombre = f"Turista-{i+1}"
        preferencias = generar_preferencias(destinos)
        turistas.append(Actor(nombre, preferencias, metodoVotacionGenerico, destinos))

    mecanismoVotacionGenerico = MecanismoVotacion()

    grupo = Grupo(turistas, destinos, mecanismoVotacionGenerico)

    condorcetStat = CondorcetStat()
    matrizUtilidades = MatrizDeUtilidadesStat()
    grupo.registerStat(condorcetStat)
    grupo.registerStat(matrizUtilidades)

    grupo.printStats()

    # Mostrar preferencias
    print("\nPreferencias individuales:")
    for turista in turistas:
        print(turista)

    ejemploNegociacionAlternadaTrivial(seed, turistas, destinos, grupo)
    ejemploNegociacionAlternada(seed, turistas, destinos, grupo)
    ejemploNegociacionAlternadaImpaciente(seed, turistas, destinos, grupo)
