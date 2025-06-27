import numpy as np

import agents
import graphics

N_ROUNDS = 200          # Número de rondas
P_COOP2 = 5             # Ganancia cuando los dos cooperan
P_NCOOP1 = 10           # Ganancia del ganador que traicionó (no coopero)
P_PERD = 0              # Ganancia del que traicionaron (cooperó)
P_NCOOP0 = 2            # Ganancia cuando los dos no cooperan

players = {
    1: agents.ElChance,
    2: agents.Convenceme,
    3: agents.TitForTat,
}

def show_menu():
    print(f"Seleccione jugador:")
    for i, cls in players.items():
        print(f"{i} - {cls.__name__}")

def choose_player(prompt):
    while True:
        try:
            choice = int(input(prompt))
            cls = players[choice]
            return cls()  # instanciamos
        except (KeyError, ValueError):
            print("Opción inválida, intente de nuevo.")


def tournament(g1, g2, input_path, store_path):
    header = ""
    history = ""
    scores = [0,0]
    coop_rate = [0,0]
    rel_win = [0,0]

    s1 = g1.start()
    s2 = g2.start()
    series_round = np.arange(0, N_ROUNDS)
    series_score1 = []
    series_score2 = []

    for round in range(N_ROUNDS):
        if (round > 0):
            temp = s1
            s1 = g1.choose(s2)
            s2 = g2.choose(temp)
        if (s1 == agents.COOP and s2 == agents.COOP):
            scores[0] += P_COOP2
            scores[1] += P_COOP2
            coop_rate[0] += 1
            coop_rate[1] += 1
        elif (s1 == agents.NOT_COOP and s2 == agents.NOT_COOP):
            scores[0] += P_NCOOP0
            scores[1] += P_NCOOP0
        elif (s1 == agents.NOT_COOP):
            scores[0] += P_NCOOP1
            rel_win[0] += 1
            scores[1] += P_PERD
            coop_rate[1] += 1
        else:
            scores[0] += P_PERD
            coop_rate[0] += 1
            scores[1] += P_NCOOP1
            rel_win[1] += 1

        series_score1.append(scores[0])
        series_score2.append(scores[1])
        st1 = "COOP" if s1 == agents.COOP else "NOT_COOP"
        st2 = "COOP" if s2 == agents.COOP else "NOT_COOP"
        history += f"Round {round} Result: {scores} → {st1} - {st2}\n"

    coop_rate = [r/N_ROUNDS for r in coop_rate]
    header += (f"¡Torneo terminado en {N_ROUNDS}!\n"
               f"1: {g1.__class__.__name__} vs 2: {g2.__class__.__name__}\n"
               f"\t Resultados: {scores}\n"
               f"\t\t Tasa de Cooperacion: {coop_rate[0]}% - {coop_rate[1]}%\n"
               f"\t\t Victorias Relativas: {rel_win[0]} - {rel_win[1]}\n")

    save_results(header+'\n\n'+history, input_path)
    series_score1 = np.array(series_score1)
    series_score2 = np.array(series_score2)
    series_score = [series_score1, series_score2]
    graphics.graph_line(
        series_round, series_score,
        labelx ="Rounds", labely= "Score",
        title = "Scores", store_path=store_path
    )



def save_results(texto: str, ruta_archivo: str) -> None:
    """
    Guarda el string `texto` en un archivo de texto en la ruta `ruta_archivo`.
    Si el archivo no existe, se crea. Si existe, se sobrescribe.

    Args:
        texto (str): Contenido que se desea escribir en el archivo.
        ruta_archivo (str): Ruta (o solo nombre) del archivo .txt donde guardar el texto.

    Returns:
        None
    """
    with open(ruta_archivo, 'w', encoding='utf-8') as f:
        f.write(texto)

# Ejemplo de uso:
if __name__ == "__main__":
    print("Ingrese el path donde quiere guardar el archivo:")
    input_path = input()
    print("Ingrese el path donde quiere guardar la imagen:")
    store_path = input()
    show_menu()
    g1 = choose_player("Jugador 1 → ")
    g2 = choose_player("Jugador 2 → ")

    tournament(g1, g2, input_path, store_path)


