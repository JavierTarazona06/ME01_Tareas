from collections import defaultdict
from typing import Dict, Tuple, Optional
from classes.Grupo import Grupo
from classes.MecanismoVotacion import MecanismoVotacion


# metodo de MecanismoNegociacionAlternada para asignar puntuaciones de borda
class MecanismoNegociacionAlternada(MecanismoVotacion):
    def __init__(self, rondas_max: int = 5):
        self.rondas_max = rondas_max

    def realizarVotacion(self, grupo: Grupo) -> Tuple[Optional[str], Dict[str, float]]:
        """Implementa el protocolo completo de negociación alternada"""
        if len(grupo.actores) != 2:
            raise ValueError(
                "Este protocolo de negociacion requiere exactamente 2 negociadores"
            )

        # Configurar la negociación en cada actor
        A1, A2 = grupo.actores[0], grupo.actores[1]

        # Preparar negociación con las opciones disponibles
        opciones = grupo.getOpciones()
        A1.getMetodoVotacion().preparar_negociacion(A1, self.rondas_max)
        A2.getMetodoVotacion().preparar_negociacion(A2, self.rondas_max)

        # Asignar roles iniciales
        oferente, receptor = A1, A2
        neg_of, neg_re = A1.getMetodoVotacion(), A2.getMetodoVotacion()

        # Valor de conflicto
        utilidad_conflicto = {A1.nombre: 0, A2.nombre: 0}

        for ronda in range(self.rondas_max):
            # 1. Generar oferta
            oferta = neg_of.generar_oferta()
            if oferta is None:
                break  # No hay más opciones

            # 2. Evaluar oferta
            respuesta = neg_re.generar_respuesta(oferta)
            
            # 3. Criterio de aceptación sí respuesta = true significa que acepta
            if respuesta:
                # Oferta aceptada
                return oferta, {
                    oferente.nombre: oferente.getUtilidad().get(oferta, 0),
                    receptor.nombre: receptor.getUtilidad().get(oferta, 0),
                }
            else:
                # Oferta rechazada
                neg_of.registrar_rechazo(oferta)
                print(f"Ronda {ronda}: {oferente.nombre} propuso {oferta}")
                print(
                    f"{receptor.nombre} rechazó (utilidad {receptor.getUtilidad().get(oferta, 0)} < {utilidad_conflicto[receptor.nombre]})"
                )

                # Alternar roles
                oferente, receptor = receptor, oferente
                neg_of, neg_re = neg_re, neg_of

                # Avanzar ronda
                neg_of.avanzar_ronda()

        # No hubo acuerdo
        return None, utilidad_conflicto
