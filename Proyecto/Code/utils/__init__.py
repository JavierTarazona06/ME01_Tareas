"""
Paquete de utilidades personalizadas
"""

from .random_utils import (
    CustomRandom,
    seed,
    random,
    randint,
    choice,
    sample,
    shuffle,
    uniform,
    get_generator_info
)

__all__ = [
    'CustomRandom',
    'seed',
    'random',
    'randint',
    'choice',
    'sample',
    'shuffle',
    'uniform',
    'get_generator_info'
]
