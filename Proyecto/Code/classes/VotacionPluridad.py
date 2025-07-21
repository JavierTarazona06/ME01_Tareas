from classes.MetodoVotacion import MetodoVotacion

# metodo de pluralidad para elegir de la lista de preferencias
class VotacionPluralidad(MetodoVotacion):
    def votar(self, actor):
        return actor.preferencias[0]  # Usa atributos del actor
