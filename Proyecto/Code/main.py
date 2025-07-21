import random
from collections import defaultdict
from typing import List, Dict, Tuple

from classes.Turista import Turista
from classes.GrupoTuristas import GrupoTuristas
from classes.Simulator import Simulador


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


if __name__ == "__main__":
    seed = 42
    n_turistas = 3

    destinos = None

    if destinos is None:
        destinos = ["París", "Roma", "Barcelona", "Berlín", "Londres"]

    # Crear turistas con preferencias variadas
    turistas = []

    for i in range(n_turistas):
        nombre = f"Turista-{i+1}"
        preferencias = generar_preferencias(destinos)
        turistas.append(Turista(nombre, preferencias))
    grupo = GrupoTuristas(turistas, destinos)

    # Mostrar preferencias
    print("\nPreferencias individuales:")
    for turista in turistas:
        print(turista)

    simulador = Simulador(seed, grupo)
    simulador.simular_grupo()
