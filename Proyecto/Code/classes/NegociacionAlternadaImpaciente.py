from typing import List, Set, Dict, Optional, Tuple
from classes.Actor import Actor
from classes.MetodoVotacion import MetodoVotacion
from classes.NegociacionAlternada import NegociacionAlternada


class NegociacionAlternadaImpaciente(NegociacionAlternada):
    def __init__(self, coeficiente_impaciencia):
        super().__init__()
        self.coeficiente_impaciencia = coeficiente_impaciencia

    def generar_respuesta(self, oferta) -> bool:

        utilidad_oferta = self.actor.getUtilidad().get(oferta, 0)
        posible_contraoferta = self.generar_oferta()

        return utilidad_oferta >= (
            self.actor.getUtilidad().get(posible_contraoferta, 0)
            - ((self.ronda_actual + 1) * self.coeficiente_impaciencia)
        )
