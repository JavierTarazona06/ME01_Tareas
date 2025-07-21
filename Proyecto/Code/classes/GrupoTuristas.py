import random
from collections import defaultdict
from typing import List, Dict, Tuple

from classes import Turista

class GrupoTuristas:
    def __init__(self, turistas: List[Turista], destinos_disponibles: List[str]):
        self.turistas = turistas
        self.destinos = destinos_disponibles

    def votacion_pluralidad(self) -> Tuple[str, Dict[str, int]]:
        """Realiza una votación por pluralidad"""
        votos = defaultdict(int)
        for turista in self.turistas:
            voto = turista.votar_pluralidad()
            votos[voto] += 1

        ganador = max(votos.items(), key=lambda x: x[1])
        return ganador

    def votacion_borda(self) -> Tuple[str, Dict[str, int]]:
        """Realiza una votación por conteo Borda"""
        puntuaciones = defaultdict(int)
        for turista in self.turistas:
            votos = turista.votar_borda(self.destinos)
            for destino, puntos in votos.items():
                puntuaciones[destino] += puntos

        ganador = max(puntuaciones.items(), key=lambda x: x[1])
        return ganador

    def votacion_aprobacion(self, umbral: int = 2) -> Tuple[str, Dict[str, int]]:
        """Realiza una votación por aprobación"""
        aprobaciones = defaultdict(int)
        for turista in self.turistas:
            votos = turista.votar_aprobar(umbral)
            for destino in votos:
                aprobaciones[destino] += 1

        ganador = max(aprobaciones.items(), key=lambda x: x[1])
        return ganador

    def negociacion_alternante(self, rondas: int = 3) -> str:
        """Simula una negociación con ofertas alternadas"""
        # Orden aleatorio de turistas para hacer ofertas
        oferentes = random.sample(self.turistas, min(rondas, len(self.turistas)))

        mejor_oferta = None
        mejor_utilidad = -1

        for i, oferente in enumerate(oferentes):
            # El oferente propone el destino que maximiza su utilidad
            oferta = oferente.preferencias[0]
            utilidad_grupo = sum(t.utilidad[oferta] for t in self.turistas)

            if utilidad_grupo > mejor_utilidad:
                mejor_utilidad = utilidad_grupo
                mejor_oferta = oferta

        return mejor_oferta, mejor_utilidad

    def encontrar_condorcet(self) -> str:
        """Busca un ganador de Condorcet (si existe)"""
        for candidato in self.destinos:
            es_condorcet = True
            for oponente in self.destinos:
                if candidato == oponente:
                    continue

                votos_pro = 0
                for turista in self.turistas:
                    # Contar cuántos prefieren candidato sobre oponente
                    if turista.preferencias.index(
                        candidato
                    ) < turista.preferencias.index(oponente):
                        votos_pro += 1

                if votos_pro <= len(self.turistas) / 2:
                    es_condorcet = False
                    break

            if es_condorcet:
                return candidato
        return None
