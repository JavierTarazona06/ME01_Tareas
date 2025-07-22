import math

def erlang_b_direct(lambda_rate: float, mu_rate: float, servers: int) -> float:
    """
    Calcula la probabilidad de bloqueo mediante la fórmula directa de Erlang B.

    Parámetros
    ----------
    lambda_rate : float
        Tasa de llegadas (λ > 0).
    mu_rate : float
        Tasa de servicio (μ > 0).
    servers : int
        Número de servidores (n ≥ 0).

    Retorna
    -------
    float
        Probabilidad de bloqueo B ∈ [0,1].

    Raises
    ------
    ValueError
        Si lambda_rate ≤ 0, mu_rate ≤ 0 o servers < 0.

    Notas
    -----
    - Denominador = 1 + ∑_{k=1}^{n} (A^k / k!).
    - Puede haber overflow en A**n o en math.factorial(n) si n es muy grande.
    """
    # Validaciones
    if lambda_rate <= 0:
        raise ValueError("λ debe ser > 0.")
    if mu_rate <= 0:
        raise ValueError("μ debe ser > 0.")
    if servers < 0:
        raise ValueError("Número de servidores debe ser ≥ 0.")

    # Carga de tráfico en Erlangs
    A = lambda_rate / mu_rate

    # Numerador: A^n / n!
    try:
        numerador = A**servers / math.factorial(servers)
    except OverflowError:
        raise OverflowError("Overflow al calcular A**n o n!; considere usar algoritmo recursivo o decimal.")

    # Denominador: 1 + sum_{k=1}^n A^k / k!
    denom = 1.0
    for k in range(1, servers + 1):
        try:
            denom += A**k / math.factorial(k)
        except OverflowError:
            raise OverflowError(f"Overflow al calcular término k={k}; puede usar algoritmo recursivo o aritmética de alta precisión.")

    return numerador / denom

import math

def erlang_c(lambda_rate: float, mu_rate: float, servers: int) -> float:
    """
    Calcula la probabilidad de que una llamada espere en cola (Erlang C).

    Parámetros
    ----------
    lambda_rate : float
        Tasa de llegadas (λ). Debe ser > 0.
    mu_rate : float
        Tasa de servicio por servidor (μ). Debe ser > 0.
    servers : int
        Número de servidores (m). Entero ≥ 1.

    Retorna
    -------
    float
        Probabilidad de espera en cola, P_cola ∈ [0,1].

    Raises
    ------
    ValueError
        - Si lambda_rate ≤ 0 o mu_rate ≤ 0.
        - Si servers < 1.
        - Si la utilización ρ ≥ 1 (sistema inestable).

    Notas
    -----
    - A = λ / μ ; ρ = A / m.
    - Factoriales grandes o potencias elevadas pueden causar OverflowError.
    """
    # Validaciones
    if lambda_rate <= 0:
        raise ValueError("λ debe ser > 0.")
    if mu_rate <= 0:
        raise ValueError("μ debe ser > 0.")
    if servers < 1:
        raise ValueError("El número de servidores m debe ser ≥ 1.")
    
    A = lambda_rate / mu_rate
    rho = A / servers
    if rho >= 1:
        raise ValueError(
            f"Sistema inestable: utilización ρ = {rho:.3f} ≥ 1."
        )

    # Suma de los términos desde k=0 hasta k=m-1
    try:
        sum_terms = sum(A**k / math.factorial(k) for k in range(servers))
    except OverflowError:
        raise OverflowError(
            "Overflow al calcular la suma de términos; considere usar 'decimal' o un método iterativo."
        )

    # Término de estado m con factor de espera
    try:
        term_m = (A**servers / math.factorial(servers)) * (1.0 / (1.0 - rho))
    except OverflowError:
        raise OverflowError(
            "Overflow al calcular A^m / m!; considere usar 'decimal' o un método iterativo."
        )

    # Fórmula de Erlang C
    p_cola = term_m / (sum_terms + term_m)
    return p_cola



if __name__ == "__main__":
    print(erlang_b_direct(1/5, 1/6, 2))
    print(erlang_b_direct(1/5, 1/6, 5))
    print(erlang_c(1/5, 1/6, 2))
    print(erlang_c(1/5, 1/6, 5))
    print(erlang_c(1/2, 1/15, 10))
    