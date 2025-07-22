from collections import defaultdict
from datetime import datetime
import sys
import os
from typing import List, Dict, Tuple

# Añadir el directorio padre al path para poder importar los módulos
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from utils import random_utils as random
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
from classes.MecanismoNegociacionAlternadaTrivial import MecanismoNegociacionAlternadaTrivial
from classes.NegociacionAlternadaImpaciente import NegociacionAlternadaImpaciente
from classes.CondorcetStat import CondorcetStat
from classes.MatrizDeUtilidadesStat import MatrizDeUtilidadesStat


def ejecutar_pluralidad(seed: int, turistas: List[Actor], actividades: List[str], grupo: Grupo) -> Tuple[str, int]:
    """Ejecuta votación por pluralidad"""
    print("\n--- Votación por Pluralidad ---")
    metodoVotacionPluralidad = VotacionPluralidad()

    for turista in turistas:
        turista.setMetodoVotacion(metodoVotacionPluralidad)

    mecanismoVotacionPluralidad = MecanismoPluralidad()
    grupo.setMecanismoVotacion(mecanismoVotacionPluralidad)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(f"Resultado: {resultados[0]} con {resultados[1]} votos")
    return resultados


def ejecutar_borda(seed: int, turistas: List[Actor], actividades: List[str], grupo: Grupo) -> Tuple[str, int]:
    """Ejecuta votación por método Borda"""
    print("\n--- Votación por Método Borda ---")
    metodoVotacionBorda = VotacionBorda()

    for turista in turistas:
        turista.setMetodoVotacion(metodoVotacionBorda)

    mecanismoVotacionBorda = MecanismoBorda()
    grupo.setMecanismoVotacion(mecanismoVotacionBorda)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(f"Resultado: {resultados[0]} con puntuación Borda {resultados[1]}")
    return resultados


def ejecutar_aprobacion(seed: int, turistas: List[Actor], actividades: List[str], grupo: Grupo) -> Tuple[str, int]:
    """Ejecuta votación por aprobación"""
    print("\n--- Votación por Aprobación ---")
    # Los turistas aprueban sus 3 primeras preferencias
    umbral_aprobacion = 3

    metodoVotacionAprobacion = VotacionAprobacion(umbral_aprobacion)
    for turista in turistas:
        turista.setMetodoVotacion(metodoVotacionAprobacion)

    mecanismoVotacionAprobacion = MecanismoAprobacion()
    grupo.setMecanismoVotacion(mecanismoVotacionAprobacion)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(f"Resultado: {resultados[0]} con {resultados[1]} aprobaciones")
    return resultados


def ejecutar_runoff(seed: int, turistas: List[Actor], actividades: List[str], grupo: Grupo) -> Tuple[str, list]:
    """Ejecuta votación por RunOff Instantáneo"""
    print("\n--- Votación por RunOff Instantáneo ---")
    metodoVotacionRunOff = VotacionRunOffInstantaneo()

    for turista in turistas:
        turista.setMetodoVotacion(metodoVotacionRunOff)

    mecanismoVotacionRunOff = MecanismoRunOffInstantaneo()
    grupo.setMecanismoVotacion(mecanismoVotacionRunOff)

    simulador = Simulador(seed, grupo)
    resultados = simulador.simular_grupo()

    print(f"Resultado: {resultados[0]} con historial {resultados[1]}")
    return resultados


def ejecutar_negociacion(seed: int, turistas_pareja: List[Actor], actividades: List[str]) -> Dict[str, Tuple]:
    """Ejecuta diferentes tipos de negociación entre dos turistas"""
    print("\n--- Negociación entre Turistas ---")

    # Configuración para negociación
    metodoVotacionGenerico = MetodoVotacion()
    mecanismoVotacionGenerico = MecanismoVotacion()
    grupo_pareja = Grupo(turistas_pareja, actividades, mecanismoVotacionGenerico)

    # Estadísticas
    condorcetStat = CondorcetStat()
    matrizUtilidades = MatrizDeUtilidadesStat()
    grupo_pareja.registerStat(condorcetStat)
    grupo_pareja.registerStat(matrizUtilidades)

    grupo_pareja.printStats()

    print("\nPreferencias de la pareja:")
    for turista in turistas_pareja:
        print(turista)

    resultados = {}

    # Negociación alternada trivial
    print("\n-- Negociación Alternada Trivial --")
    for turista in turistas_pareja:
        turista.setMetodoVotacion(NegociacionAlternada())

    mecanismo = MecanismoNegociacionAlternadaTrivial(5)
    grupo_pareja.setMecanismoVotacion(mecanismo)

    simulador = Simulador(seed, grupo_pareja)
    resultado = simulador.simular_grupo()
    print(f"Resultado: {resultado[0]} con puntuación {resultado[1]}")
    resultados['trivial'] = resultado

    # Negociación alternada normal
    print("\n-- Negociación Alternada Normal --")
    for turista in turistas_pareja:
        turista.setMetodoVotacion(NegociacionAlternada())

    mecanismo = MecanismoNegociacionAlternada(5)
    grupo_pareja.setMecanismoVotacion(mecanismo)

    simulador = Simulador(seed, grupo_pareja)
    resultado = simulador.simular_grupo()
    print(f"Resultado: {resultado[0]} con puntuación {resultado[1]}")
    resultados['normal'] = resultado

    # Negociación alternada impaciente
    print("\n-- Negociación Alternada Impaciente --")
    coeficiente_impaciencia = 0.7
    for turista in turistas_pareja:
        turista.setMetodoVotacion(NegociacionAlternadaImpaciente(coeficiente_impaciencia))

    mecanismo = MecanismoNegociacionAlternada(5)
    grupo_pareja.setMecanismoVotacion(mecanismo)

    simulador = Simulador(seed, grupo_pareja)
    resultado = simulador.simular_grupo()
    print(f"Resultado: {resultado[0]} con puntuación {resultado[1]}")
    resultados['impaciente'] = resultado

    return resultados


def analizar_resultados(resultados: Dict[str, Tuple], utilidades: Dict[str, Dict[str, int]]):
    """Analiza los resultados y muestra estadísticas"""
    print("\n=== ANÁLISIS DE RESULTADOS ===")

    # Calcular utilidad grupal para cada decisión
    utilidad_total = defaultdict(int)

    for metodo, (actividad, _) in resultados.items():
        for turista, util_dict in utilidades.items():
            utilidad_total[metodo] += util_dict.get(actividad, 0)

        print(f"Método {metodo}: Actividad elegida = {actividad}, Utilidad total = {utilidad_total[metodo]}")

    # Mejor método según utilidad total
    mejor_metodo = max(utilidad_total.items(), key=lambda x: x[1])
    print(f"\nEl método que maximiza la utilidad total es: {mejor_metodo[0]} con {mejor_metodo[1]} puntos de utilidad")


if __name__ == "__main__":
    # Semilla para reproducibilidad
    seed = 42

    fecha_hora = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    # Obtener la ruta del directorio actual donde está el script
    directorio_actual = os.path.dirname(os.path.abspath(__file__))
    nombre_archivo = f"log-escenario2-{fecha_hora}.txt"
    # Crear la ruta completa para guardar el archivo en la carpeta escenario2
    ruta_completa = os.path.join(directorio_actual, nombre_archivo)

    log_file = open(ruta_completa, "w", encoding="utf-8")
    sys.stdout = log_file

    print(f"=== ESCENARIO: GRUPO DE AMIGOS EN NUEVA YORK ===")

    # Actividades disponibles
    actividades = ["Central Park", "MoMA", "Broadway", "Distrito Financiero", "Times Square"]

    # Crear turistas con preferencias predefinidas
    metodoVotacionGenerico = MetodoVotacion()

    # Preferencias de los turistas (orden de más a menos preferido)
    preferencias = {
        "Ana": ["MoMA", "Broadway", "Times Square", "Distrito Financiero", "Central Park"],
        "Bruno": ["Central Park", "Times Square", "Distrito Financiero", "MoMA", "Broadway"],
        "Carlos": ["Times Square", "Broadway", "Central Park", "MoMA", "Distrito Financiero"],
        "Diana": ["Distrito Financiero", "MoMA", "Central Park", "Broadway", "Times Square"],
        "Elena": ["Broadway", "Times Square", "Central Park", "MoMA", "Distrito Financiero"]
    }

    # Crear los actores
    turistas = []
    for nombre, prefs in preferencias.items():
        turistas.append(Actor(nombre, prefs, metodoVotacionGenerico, actividades))

    # Crear grupo y configurar estadísticas
    mecanismoVotacionGenerico = MecanismoVotacion()
    grupo = Grupo(turistas, actividades, mecanismoVotacionGenerico)

    condorcetStat = CondorcetStat()
    grupo.registerStat(condorcetStat)

    print("\n--- Información Inicial ---")
    grupo.printStats()

    print("\nPreferencias individuales:")
    for turista in turistas:
        print(turista)

    # Ejecutar todos los métodos de votación
    resultados = {}

    # Pluralidad
    actividad, votos = ejecutar_pluralidad(seed, turistas, actividades, grupo)
    resultados["pluralidad"] = (actividad, votos)

    # Borda
    actividad, puntos = ejecutar_borda(seed, turistas, actividades, grupo)
    resultados["borda"] = (actividad, puntos)

    # Aprobación
    actividad, aprobaciones = ejecutar_aprobacion(seed, turistas, actividades, grupo)
    resultados["aprobacion"] = (actividad, aprobaciones)

    # RunOff Instantáneo
    actividad, historial = ejecutar_runoff(seed, turistas, actividades, grupo)
    resultados["runoff"] = (actividad, historial)

    # Negociación entre subgrupos de 2 personas (Ana y Bruno)
    print("\n=== NEGOCIACIÓN ENTRE ANA Y BRUNO ===")
    pareja_ab = [turistas[0], turistas[1]]  # Ana y Bruno
    resultados_ab = ejecutar_negociacion(seed, pareja_ab, actividades)

    # Negociación entre subgrupos de 2 personas (Carlos y Diana)
    print("\n=== NEGOCIACIÓN ENTRE CARLOS Y DIANA ===")
    pareja_cd = [turistas[2], turistas[3]]  # Carlos y Diana
    resultados_cd = ejecutar_negociacion(seed, pareja_cd, actividades)

    print("\n=== ANÁLISIS FINAL ===")

    # Recopilar utilidades de todos los turistas
    utilidades = {}
    for turista in turistas:
        utilidades[turista.nombre] = turista.getUtilidad()

    # Analizar resultados de votación grupal
    analizar_resultados(resultados, utilidades)

    print("\nSimulación completada.")
    log_file.close()

    sys.stdout = sys.__stdout__
    print(f"Resultados guardados en {ruta_completa}")
