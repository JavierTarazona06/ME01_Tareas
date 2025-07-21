from classes.MetodoVotacion import MetodoVotacion

# metodo de pluralidad para elegir de la lista de preferencias
class Pluralidad(MetodoVotacion):
    def votar(self, actor):
        return actor.preferencias[0]  # Usa atributos del actor
