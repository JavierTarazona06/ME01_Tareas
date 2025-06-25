import agents

N_ROUNDS = 200
P_COOP2 = 5
P_NCOOP1 = 10
P_NCOOP0 = 2
P_PERD = 0

scores = [0,0]

def tournament():
    chance = agents.ElChance()
    conv =  agents.Convenceme()
    tit = agents.TitForTat()

    scores = [0,0]

    g1 = conv
    g2 = chance

    s1 = g1.start()
    s2 = g2.start()

    for round in range(N_ROUNDS):
        if (round > 0):
            temp = s1
            s1 = g1.choose(s2)
            s2 = g2.choose(temp)
        if (s1 == agents.COOP and s2 == agents.COOP):
            scores[0] += P_COOP2
            scores[1] += P_COOP2
        elif (s1 == agents.NOT_COOP and s2 == agents.NOT_COOP):
            scores[0] += P_NCOOP0
            scores[1] += P_NCOOP0
        elif (s1 == agents.NOT_COOP):
            scores[0] += P_NCOOP1
            scores[1] += P_PERD
        else:
            scores[0] += P_PERD
            scores[1] += P_NCOOP1
        st1 = "COOP" if s1 == agents.COOP else "NOT_COOP"
        st2 = "COOP" if s2 == agents.COOP else "NOT_COOP"
        print(f"Round {round} Result: {scores} → {st1} - {st2}")

    print(f"Result: {scores}")
            

tournament()