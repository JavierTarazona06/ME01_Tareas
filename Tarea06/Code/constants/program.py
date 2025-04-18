
CURSOS_CT = {
    "cantidad": 500,
    "num_grupos_min": 1,
    "num_grupos_max": 4,
    "num_grupos_avg": None,
    "num_grupos_dev": None
}

DOCENTES_CT = {
    "cantidad": CURSOS_CT["cantidad"]*CURSOS_CT["num_grupos_max"],
}

AULAS_CT = {
    "cantidad": CURSOS_CT["cantidad"]*CURSOS_CT["num_grupos_max"],
}

CLASES_CT = {
    "num_cupos_min": 30,
    "num_cupos_max": 60,
    "num_cupos_avg": 40,
    "num_cupos_dev": 0.6
}

ESTUDIANTES_CT = {
    "cantidad": 3000,
    "p.a.p.i_avg": 3.8,
    "p.a.p.i_dev": 0.9,
    "materias_a_inscribir": 5,
    "materias_deseadas": 9
}

CONSUMO_CT = 10000

ITERACIONES_CT = 10