"""
Utilidades propias para funciones aleatorias
Reemplaza las funciones de la librería random de Python con implementaciones propias
usando el algoritmo Linear Congruential Generator (LCG)
"""

import time
from typing import List, TypeVar, Any

T = TypeVar('T')


class CustomRandom:
    """
    Generador de números pseudoaleatorios personalizado usando LCG
    """

    def __init__(self, seed: int = None):
        """
        Inicializa el generador con una semilla

        Args:
            seed: Semilla para el generador. Si es None, usa el tiempo actual
        """
        # Parámetros del LCG (basados en los usados por glibc)
        # Referencia: https://sourceware.org/git/?p=glibc.git;a=blob;f=stdlib/random_r.c

        self.a = 1103515245  # Multiplicador
        self.c = 12345       # Incremento
        self.m = 2**31       # Módulo (2^31)

        if seed is None:
            seed = int(time.time() * 1000) % self.m

        self.seed = seed % self.m
        self._current = self.seed

    def set_seed(self, seed: int) -> None:
        """
        Establece una nueva semilla para el generador

        Args:
            seed: Nueva semilla
        """
        self.seed = seed % self.m
        self._current = self.seed

    def _next_int(self) -> int:
        """
        Genera el siguiente número entero en la secuencia

        Returns:
            Siguiente número entero pseudoaleatorio
        """
        self._current = (self.a * self._current + self.c) % self.m
        return self._current

    def random(self) -> float:
        """
        Genera un número flotante aleatorio entre 0.0 y 1.0

        Returns:
            Número flotante entre 0.0 y 1.0
        """
        return self._next_int() / self.m

    def randint(self, a: int, b: int) -> int:
        """
        Genera un entero aleatorio entre a y b (ambos inclusivos)

        Args:
            a: Límite inferior (inclusivo)
            b: Límite superior (inclusivo)

        Returns:
            Entero aleatorio entre a y b
        """
        if a > b:
            raise ValueError("El límite inferior debe ser menor o igual al superior")

        range_size = b - a + 1
        return a + (self._next_int() % range_size)

    def choice(self, sequence: List[T]) -> T:
        """
        Selecciona un elemento aleatorio de una secuencia

        Args:
            sequence: Lista o secuencia de elementos

        Returns:
            Elemento seleccionado aleatoriamente

        Raises:
            IndexError: Si la secuencia está vacía
        """
        if not sequence:
            raise IndexError("No se puede elegir de una secuencia vacía")

        index = self.randint(0, len(sequence) - 1)
        return sequence[index]

    def sample(self, population, k: int):
        """
        Selecciona k elementos únicos aleatorios de una población

        Args:
            population: Secuencia de elementos para muestrear (lista, tupla, range, etc.)
            k: Número de elementos a seleccionar

        Returns:
            Lista con k elementos únicos seleccionados aleatoriamente

        Raises:
            ValueError: Si k es mayor que el tamaño de la población
        """
        if k > len(population):
            raise ValueError("El tamaño de la muestra no puede ser mayor que la población")

        if k < 0:
            raise ValueError("El tamaño de la muestra debe ser no negativo")

        # Convertir a lista si no lo es (para manejar range, tuplas, etc.)
        if hasattr(population, 'copy'):
            available = population.copy()
        else:
            available = list(population)

        result = []

        for _ in range(k):
            index = self.randint(0, len(available) - 1)
            result.append(available.pop(index))

        return result

    def shuffle(self, sequence: List[Any]) -> None:
        """
        Mezcla los elementos de una lista in-place (algoritmo Fisher-Yates)

        Args:
            sequence: Lista a mezclar (se modifica la lista original)
        """
        n = len(sequence)
        for i in range(n - 1, 0, -1):
            j = self.randint(0, i)
            sequence[i], sequence[j] = sequence[j], sequence[i]

    def uniform(self, a: float, b: float) -> float:
        """
        Genera un número flotante aleatorio uniforme entre a y b

        Args:
            a: Límite inferior
            b: Límite superior

        Returns:
            Número flotante entre a y b
        """
        return a + (b - a) * self.random()


# Instancia global del generador personalizado
_global_generator = CustomRandom()


# Funciones de conveniencia que replican la API de random
def seed(a: int = None) -> None:
    """
    Establece la semilla del generador global

    Args:
        a: Semilla para el generador
    """
    _global_generator.set_seed(a if a is not None else int(time.time() * 1000))


def random() -> float:
    """
    Genera un número flotante aleatorio entre 0.0 y 1.0

    Returns:
        Número flotante entre 0.0 y 1.0
    """
    return _global_generator.random()


def randint(a: int, b: int) -> int:
    """
    Genera un entero aleatorio entre a y b (ambos inclusivos)

    Args:
        a: Límite inferior (inclusivo)
        b: Límite superior (inclusivo)

    Returns:
        Entero aleatorio entre a y b
    """
    return _global_generator.randint(a, b)


def choice(sequence: List[T]) -> T:
    """
    Selecciona un elemento aleatorio de una secuencia

    Args:
        sequence: Lista o secuencia de elementos

    Returns:
        Elemento seleccionado aleatoriamente
    """
    return _global_generator.choice(sequence)


def sample(population: List[T], k: int) -> List[T]:
    """
    Selecciona k elementos únicos aleatorios de una población

    Args:
        population: Lista de elementos para muestrear
        k: Número de elementos a seleccionar

    Returns:
        Lista con k elementos únicos seleccionados aleatoriamente
    """
    return _global_generator.sample(population, k)


def shuffle(sequence: List[Any]) -> None:
    """
    Mezcla los elementos de una lista in-place

    Args:
        sequence: Lista a mezclar (se modifica la lista original)
    """
    _global_generator.shuffle(sequence)


def uniform(a: float, b: float) -> float:
    """
    Genera un número flotante aleatorio uniforme entre a y b

    Args:
        a: Límite inferior
        b: Límite superior

    Returns:
        Número flotante entre a y b
    """
    return _global_generator.uniform(a, b)


# Información sobre la implementación
def get_generator_info() -> dict:
    """
    Retorna información sobre el generador actual

    Returns:
        Diccionario con información del generador
    """
    return {
        "algorithm": "Linear Congruential Generator (LCG)",
        "parameters": {
            "a": _global_generator.a,
            "c": _global_generator.c,
            "m": _global_generator.m
        },
        "current_seed": _global_generator.seed,
        "current_state": _global_generator._current
    }
