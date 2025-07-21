from classes.MetodoVotacion import MetodoVotacion

# metodo de VotacionBorda para asignar puntuaciones de borda
class VotacionBorda(MetodoVotacion):
    def votar(self, actor):
        """Asigna puntuación Borda a cada destino"""
        return {
            opcion: (
                len(actor.preferencias) - actor.preferencias.index(opcion)
                if opcion in actor.preferencias
                else 0
            )
            for opcion in actor.opcionesDisponibles
        }
