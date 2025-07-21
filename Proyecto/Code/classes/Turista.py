from typing import List, Dict, Tuple

from classes.Actor import Actor

class Turista(Actor):
    def __init__(self, nombre: str, preferencias: List[str]):
        super().__init__(nombre, preferencias)

    def votar_pluralidad(self) -> str:
        """Vota por su destino más preferido (sistema de pluralidad)"""
        return self.preferencias[0]

    def votar_borda(self, destinos: List[str]) -> Dict[str, int]:
        """Asigna puntuación Borda a cada destino"""
        return {
            destino: (
                len(self.preferencias) - self.preferencias.index(destino)
                if destino in self.preferencias
                else 0
            )
            for destino in destinos
        }

    def votar_aprobar(self, umbral: int = 2) -> List[str]:
        """Aprueba los destinos que están en sus top N preferencias"""
        return self.preferencias[:umbral]
