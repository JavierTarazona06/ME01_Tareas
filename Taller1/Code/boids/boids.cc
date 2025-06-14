#include "../src/mobility/model/boids-mobility-model.h"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    // Al inicio de main()
    std::ofstream outFile;
    outFile.open("boids_positions.csv");
    outFile << "Time,NodeId,X,Y,IsLeader\n";
    // Crear nodos
    NodeContainer nodes;
    nodes.Create(20);

    // Configurar movilidad
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::BoidsMobilityModel");

    // Configurar posiciones iniciales
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=1000]"),
                                  "Y",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=1000]"));

    mobility.Install(nodes);

    // Configurar líderes y seguidores
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        Ptr<MobilityModel> mob = nodes.Get(i)->GetObject<MobilityModel>();
        if (mob)
        {
            // Usa DynamicCast para convertir a BoidsMobilityModel
            Ptr<BoidsMobilityModel> boids = DynamicCast<BoidsMobilityModel>(mob);
            if (boids)
            {
                if (i < 5)
                {
                    boids->SetIsLeader(true);
                    boids->SetMaxSpeed(8.0);
                    boids->SetOutputFile(&outFile);
                }
                else
                {
                    boids->SetIsLeader(false);
                    boids->SetMaxSpeed(5.0);
                    boids->SetOutputFile(&outFile);
                }
            }
        }
    }

    // Ejecutar simulación
    Simulator::Stop(Seconds(100));
    Simulator::Run();
    outFile.close();
    Simulator::Destroy();

    return 0;
}