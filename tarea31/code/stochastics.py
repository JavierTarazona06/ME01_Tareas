import random

def bernoulli(p=0.05):
    """
    p es la probabilidad de que salga el 1
    """
    # Genera un número uniforme en [0,1)
    u = random.random()
    # Si u < p devuelve 1, sino 0
    return 1 if u < p else 0