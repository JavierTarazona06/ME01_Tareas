from typing import List, Set, Dict, Optional, Tuple
from classes.Actor import Actor
from classes.MetodoVotacion import MetodoVotacion


class NegociacionAlternada(MetodoVotacion):
    def __init__(self):
        self.ronda_actual = 0
        self.destinos_rechazados: Set[str] = set()

    def preparar_negociacion(self, actor: Actor, rondas_max: int):
        """Inicializa el estado para una nueva negociación"""
        self.ronda_actual = 0
        self.destinos_rechazados = set()
        self.actor = actor
        self.opciones = actor.opcionesDisponibles.copy()
        self.rondas_max = rondas_max

    def generar_oferta(self) -> Optional[str]:
        """Genera la mejor oferta disponible para el actor"""
        if not self.opciones:
            return None

        # Filtrar destinos no rechazados
        destinos_disponibles = [
            d for d in self.opciones if d not in self.destinos_rechazados
        ]

        if not destinos_disponibles:
            return None

        # Seleccionar el destino de mayor utilidad
        return max(
            destinos_disponibles, key=lambda d: self.actor.getUtilidad().get(d, 0)
        )

    def generar_respuesta(self, oferta) -> bool:

        utilidad_oferta = self.actor.getUtilidad().get(oferta, 0)
        posible_contraoferta = self.generar_oferta()

        return utilidad_oferta >= self.actor.getUtilidad().get(posible_contraoferta, 0)

    def registrar_rechazo(self, destino: str):
        """Registra un destino rechazado para no proponerlo nuevamente"""
        self.destinos_rechazados.add(destino)

    def avanzar_ronda(self):
        """Actualiza el estado para la siguiente ronda"""
        self.ronda_actual += 1

    def votar(self, actor: "Actor") -> Optional[str]:
        """Implementación requerida por MetodoVotacion"""
        return self.generar_oferta(actor)
