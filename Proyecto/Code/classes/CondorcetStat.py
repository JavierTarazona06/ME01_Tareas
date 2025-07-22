from classes.Stat import Stat
from classes.Grupo import Grupo


class CondorcetStat(Stat):
    def __init__(
        self,
    ):
        pass

    def getStat(self, grupo: Grupo):
        return self.encontrar_condorcet(grupo)

    def encontrar_condorcet(self, grupo: Grupo) -> str:
        """Busca un ganador de Condorcet (si existe)"""
        for candidato in grupo.opciones:
            es_condorcet = True
            for oponente in grupo.opciones:
                if candidato == oponente:
                    continue

                votos_pro = 0
                for turista in grupo.actores:
                    # Contar cuántos prefieren candidato sobre oponente
                    if turista.preferencias.index(
                        candidato
                    ) < turista.preferencias.index(oponente):
                        votos_pro += 1

                if votos_pro <= len(grupo.actores) / 2:
                    es_condorcet = False
                    break

            if es_condorcet:
                print(f"candidato condorcet: {candidato}")
                return candidato
        print(f"no hay candidato condorcet")
        return None
