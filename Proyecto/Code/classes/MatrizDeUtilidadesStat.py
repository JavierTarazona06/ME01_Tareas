from classes.Stat import Stat
from classes.Grupo import Grupo


class MatrizDeUtilidadesStat(Stat):
    def __init__(
        self,
    ):
        pass

    def getStat(self, grupo: Grupo):
        return self.encontrar_matriz(grupo)

    def encontrar_matriz(self, grupo: Grupo) -> str:
        """Imprime matriz de utilidades cruzadas ordenadas por preferencias"""
        if len(grupo.actores) != 2:
            raise ValueError("Este stat requiere 2 actores")

        actor1, actor2 = grupo.actores[0], grupo.actores[1]

        # Ordenar opciones según preferencias de cada actor
        opciones_actor1 = actor1.preferencias

        opciones_actor2 = actor2.preferencias

        # Encabezados de columnas (preferencias actor1)
        print("\nMatriz de Utilidades Cruzadas")
        print(f"{actor2.nombre + ' ↓ || '+actor1.nombre+ ' →':<20}", end="")
        for d1 in opciones_actor1:
            print(f"{d1:<15}", end="")
        print("\n" + "-" * (20 + 15 * len(opciones_actor1)))

        # Filas (preferencias actor2)
        for d2 in opciones_actor2:
            print(f"{d2:<20}", end="")
            for d1 in opciones_actor1:
                # Utilidad es una tupla (utilidad_actor2, utilidad_actor1)
                utilidad = (
                    actor2.utilidad.get(d2, 0),  # Actor2 elige d2 (filas)
                    actor1.utilidad.get(d1, 0),  # Actor1 elige d1 (columnas)
                )
                print(f"{str(utilidad):<15}", end="")
            print()
