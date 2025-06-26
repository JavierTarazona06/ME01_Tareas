from typing import Optional

import stochastics

COOP = 0
NOT_COOP = 1


class Agente:
    """
    Clase base para agentes del dilema del prisionero.

    Attributes:
        state (int): Acción actual (COOP o NOT_COOP).
        rounds_played (int): Número de rondas jugadas.
        prev_opponent_response (Optional[int]): Última acción del oponente.
        opponent_consec_count (int): Conteo de respuestas idénticas consecutivas del oponente.
    """
    def __init__(self) -> None:
        self.state: int = COOP
        self.rounds_played: int = 0
        self.prev_opponent_response: Optional[int] = None
        self.opponent_consec_count: int = 0

    def _validate_response(self, prev_opponent_response) -> None:
        if (prev_opponent_response != COOP
                and prev_opponent_response != NOT_COOP):
            raise ValueError(
                "Se debe enviar una respuesta "
                "válida del otro jugador")

    def start(self) -> int:
        """
        Inicia la serie de rondas y devuelve la acción inicial.

        Returns:
            int: Acción inicial (COOP o NOT_COOP).
        """
        self.rounds_played = 1
        self.prev_opponent_response = None
        self.opponent_consec_count = 0
        return self.state

    def choose(self, prev_opponent_response: int) -> int:
        """
        Decide la siguiente acción basándose en la respuesta previa del oponente.

        Args:
            prev_opponent_response (int): Acción del oponente en la ronda anterior.

        Returns:
            int: Acción elegida para esta ronda.
        """
        self._validate_response(prev_opponent_response)

        if self.opponent_consec_count == 0:
            self.prev_opponent_response = prev_opponent_response
        else:
            if self.prev_opponent_response == prev_opponent_response:
                self.opponent_consec_count += 1
            else:
                self.opponent_consec_count = 1
            self.prev_opponent_response = prev_opponent_response

        ans = None
        if self.state == COOP:
            ans = self._choose_coop()
        elif self.state == NOT_COOP:
            ans = self._choose_not_coop()

        self.rounds_played += 1
        return ans

    def _choose_coop(self) -> int:
        if self.prev_opponent_response == NOT_COOP:
            self.state = NOT_COOP
            return NOT_COOP

        return COOP

    def _choose_not_coop(self) -> int:
        if self.prev_opponent_response == COOP:
            self.state = COOP
            return COOP

        return NOT_COOP


# -------------------------
# Agente Benévolo
# -------------------------

class ElChance(Agente):
    def __init__(self):
        super().__init__()
        self.percentage_coop = 0.95
        self.percentage_not_coop = 0.9

    def _choose_coop(self):
        flag__no_coop = (
                self.prev_opponent_response == NOT_COOP and
                self.opponent_consec_count == 2
        )
        flag_chance = stochastics.bernoulli(1 - self.percentage_coop) == 1
        if flag__no_coop or flag_chance:
            self.state = NOT_COOP
            return NOT_COOP

        return COOP

    def _choose_not_coop(self):
        flag_coop = self.prev_opponent_response == COOP
        flag_chance = stochastics.bernoulli(1 - self.percentage_not_coop) == 1
        if flag_coop or flag_chance:
            self.state = COOP
            return COOP

        return NOT_COOP


# -------------------------
# Agente Malévolo
# -------------------------

class Convenceme(Agente):
    def __init__(self):
        super().__init__()
        self.state = NOT_COOP
        self.percentage_not_coop = 0.95
        self.count_coop = 0
        self.timesb4Betray = 10

    def _choose_coop(self):
        flag_coop_init = self.count_coop >= 1
        flag_op_no_coop = (
                self.prev_opponent_response == NOT_COOP
        )
        flag_coop_10 = (
                self.count_coop >= self.timesb4Betray
        )

        if (
                (flag_coop_init and flag_op_no_coop) or
                flag_coop_10
        ):
            self.count_coop = 0
            self.state = NOT_COOP
            return NOT_COOP

        self.count_coop += 1
        return COOP

    def _choose_not_coop(self):
        flag_coop = self.prev_opponent_response == COOP
        flag_chance = stochastics.bernoulli(1 - self.percentage_not_coop) == 1
        if flag_coop or flag_chance:
            self.state = COOP
            return COOP

        return NOT_COOP


# -------------------------
# Tit for tat
# -------------------------

class TitForTat(Agente):
    def __init__(self):
        super().__init__()
