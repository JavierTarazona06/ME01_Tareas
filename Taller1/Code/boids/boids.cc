#include "../src/mobility/model/boids-mobility-model.h"

#include "ns3/core-module.h"
#include "ns3/internet-module.h" // InternetStackHelper, Ipv4*
#include "ns3/mobility-module.h"
#include "ns3/network-module.h" // Node, NodeContainer, NetDeviceContainer…
#include "ns3/wifi-module.h"    // WifiHelper, YansWifiPhyHelper, WifiMacHelper

#include <fstream>

using namespace ns3;

/*------------------- */

NodeContainer chNodes;     // Contenedor para los nodos líderes (Cluster-Heads)
NodeContainer memberNodes; // Contenedor para los nodos seguidores (Miembros)

NetDeviceContainer chIntf;     // Contenedor para las interfaces de red de los nodos líderes
NetDeviceContainer memberIntf; // Contenedor para las interfaces de red de los nodos seguidores

std::vector<NodeContainer> clusters; // Vector para almacenar los clusters formados

std::ofstream outFile("boids_data.csv");

static const uint32_t N_CH = 2;   // Número de líderes (Cluster-Heads)
static const uint32_t N_MEM = 10; // Número de seguidores (Miembros)

/*------------------------------------------------
    1. Configuración de parámetros
------------------------------------------------*/
std::pair<uint32_t, uint32_t>
ParseCommandLine(int argc, char* argv[])
{
    uint32_t nClusterHeads = N_CH; // Número de líderes (Cluster-Heads)
    uint32_t nFollowers = N_MEM;   // Número de seguidores
    CommandLine cmd;               // Procesa argumentos de línea de comandos
    cmd.AddValue("nClusterHeads", "Número de líderes (Cluster-Heads)", nClusterHeads);
    cmd.AddValue("nFollowers", "Número de seguidores", nFollowers);
    cmd.Parse(argc, argv); // Procesa los argumentos
    NS_LOG_UNCOND("Configuración: " << nClusterHeads << " líderes, " << nFollowers
                                    << " seguidores");
    return {nClusterHeads, nFollowers};
}

/*------------------------------------------------
    2. Inicializacion nodos
------------------------------------------------*/
void
InitializeNodes(NodeContainer nodes, bool isLeader, float maxSpeed /*, std::ofstream& outputFile*/)
{
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
                boids->SetIsLeader(isLeader);
                boids->SetMaxSpeed(maxSpeed);
                // boids->SetOutputFile(&outputFile);
            }
        }
    }
}

/*------------------------------------------------
    3. Creación de nodos y Clusters
------------------------------------------------*/
void
CreateNodes(uint32_t nClusterHeads, uint32_t nFollowers)
{
    // Crea dos contenedores: uno para líderes (Cluster-Heads) y otro para seguidores
    chNodes.Create(nClusterHeads);  // crea nClusterHeads instancias
    memberNodes.Create(nFollowers); // crea nFollowers instancias

    // InitializeNodes(chNodes, true, 8.0f /*, outFile*/);
    // InitializeNodes(memberNodes, false, 5.0f /*, outFile*/);
}

void
CreateClusters()
{
    // Distribuye los nodos seguidores en clusters basados en los nodos líderes (Cluster-Heads)
    int nMembers = memberNodes.GetN();
    NS_ABORT_MSG_IF(nMembers == 0, "El número de nodos seguidores no puede ser cero");
    int nClusters = chNodes.GetN();
    clusters.resize(nClusters);
    NS_ABORT_MSG_IF(nClusters == 0, "El número de nodos líderes no puede ser cero");

    int nMembersPerCluster = nMembers / nClusters; // Número de nodos seguidores por cluster
    int nMembersExtra =
        nMembers % nClusters; // Nodos seguidores extra que no se distribuyen uniformemente

    int idx = 0;                        // Índice para recorrer los nodos seguidores
    for (int i = 0; i < nClusters; ++i) // Recorre cada nodo líder
    {
        int size =
            nMembersPerCluster +
            (nMembersExtra > 0 ? 1 : 0); // Tamaño del cluster, incrementado si hay nodos extra
        nMembersExtra -= (nMembersExtra > 0 ? 1 : 0); // reduce el contador de extras
        for (int j = 0; j < size; ++j, ++idx)
        {
            NS_ABORT_MSG_IF(idx >= nMembers, "Índice fuera de rango");
            Ptr<Node> mNodec = memberNodes.Get(idx);
            clusters[i].Add(mNodec); // Añade el nodo seguidor al cluster correspondiente
        }
    }

    // Log minimalista
    NS_LOG_UNCOND("\n=== DISTRIBUCIÓN DE CLUSTERS ===");
    for (uint32_t i = 0; i < clusters.size(); ++i)
    {
        NS_LOG_UNCOND("Cluster " << i << " (Líder: Node " << chNodes.Get(i)->GetId() << ")");
        NS_LOG_UNCOND("  Miembros: " << clusters[i].GetN() << " nodos");

        for (uint32_t j = 0; j < clusters[i].GetN(); ++j)
        {
            NS_LOG_UNCOND("    - Node " << clusters[i].Get(j)->GetId());
        }
    }
    NS_LOG_UNCOND("===============================\n");
}

/*------------------------------------------------
    3. Configuración de Wi-Fi para los nodos
------------------------------------------------*/
void
SetupWifi()
{
    // A. Configuración de Wi-Fi
    WifiHelper wifi;
    wifi.SetStandard(
        WIFI_STANDARD_80211a); // opcional
                               // Capa 802.11 física basada en el artículo Yet Another Network
                               // Simulator. Idal para MANET (YansWifiPhyHelper)
    YansWifiPhyHelper phy;
    // Medio compartido y permite añadir modelos de retardo o pérdida (YansWifiChannelHelper)
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    // B. Opciones personalizadas para el PHY
    //  [No incluídas en este ejemplo, pero se pueden añadir aquí]

    // C. Configura Wifi Ad Hoc y la capa MAC (Medium Access Control) para los nodos
    // Utiliza AdhocWifiMac para permitir que los nodos se comuniquen directamente entre sí
    // sin necesidad de un punto de acceso centralizado
    WifiMacHelper mac; // Configuración de la capa MAC
    mac.SetType(
        "ns3::AdhocWifiMac"); // Configura la capa MAC como Ad-hoc
                              // Aplicar tasa de 6 Mb/s OFDM (MCS 0 de 802.11a/g) para simplificar
                              // variabilidades en la fase inicial (calidad señal, sistema de
                              // modulación) OFDM es un esquema multicarrier que divide la señal en
                              // múltiples subportadoras, mejorando la eficiencia espectral y la
                              // resistencia a interferencias.
    // Utiliza una tasa de 6 Mbps para las tramas de datos y control
    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager", // Aislar variables en la fase inicial
        "DataMode",
        StringValue("OfdmRate6Mbps"),
        "ControlMode",
        StringValue("OfdmRate6Mbps"));

    // D. Materializa interfaces de red Wi-Fi para los nodos líderes y seguidores
    chIntf = wifi.Install(phy, mac, chNodes);
    memberIntf = wifi.Install(phy, mac, memberNodes);
    phy.EnablePcap("umanet-ch", chIntf);
    phy.EnablePcap("umanet-follower", memberIntf);
}

void
InstallInternet()
{
    // E. Agregra a los nodos la pila de protocolos de Internet (IPv4, TCP, UDP, etc.)
    // Esto permite que los nodos se comuniquen utilizando direcciones IP y protocolos de red
    InternetStackHelper internet;
    internet.Install(chNodes);
    internet.Install(memberNodes);
}

void
AssignIpv4()
{
    // F. Asignación de direcciones IP a las interfaces de red. Mascara de subred /24
    Ipv4AddressHelper ip;
    ip.SetBase("10.0.0.0", "255.255.255.0");
    ip.Assign(chIntf);
    ip.Assign(memberIntf);
}

int
main(int argc, char* argv[])
{
    // Al inicio de main()
    std::ofstream outFile("boids_positions.csv");
    BoidsMobilityModel::SetOutputFile(&outFile);
    if (!outFile.is_open())
    {
        NS_LOG_UNCOND("No se pudo abrir el archivo de salida!");
        return 1;
    }

    // outFile.open("boids_positions.csv");
    // outFile << "Time,NodeId,X,Y,IsLeader\n";
    outFile << "Time,NodeId,X,Y,IsLeader,IsFire\n";
    /*------------------------------------------------
       1. Configuración de parámetros
   ------------------------------------------------*/
    auto [nCH, nMem] = ParseCommandLine(argc, argv);

    /*------------------------------------------------
        2. Creación de nodos y Clusters
    ------------------------------------------------*/
    // Crear nodos
    // NodeContainer nodes;
    // nodes.Create(20);
    CreateNodes(nCH, nMem);
    CreateClusters();

    /*------------------------------------------------
        3. Configuración de Wi-Fi para los nodos
    ------------------------------------------------*/
    SetupWifi();
    // Configuración de la capa física para capturar paquetes en formato pcap
    // Esto permite registrar los paquetes transmitidos y recibidos en un archivo pcap para su
    // posterior análisis
    InstallInternet();
    AssignIpv4();

    BoidsMobilityModel::s_clusters = &clusters;
    BoidsMobilityModel::s_chNodes = &chNodes;

    // Configurar movilidad
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::BoidsMobilityModel");

    // Configurar posiciones iniciales
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=1000]"),
                                  "Y",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=1000]"));

    // mobility.Install(nodes);
    mobility.Install(chNodes);
    mobility.Install(memberNodes);

    Ptr<UniformRandomVariable> clusterRng = CreateObject<UniformRandomVariable>();
    clusterRng->SetAttribute("Min", DoubleValue(200.0));
    clusterRng->SetAttribute("Max", DoubleValue(800.0));

    // Distribución de clusters
    for (uint32_t i = 0; i < clusters.size(); ++i)
    {
        // Posición central del cluster
        double centerX = clusterRng->GetValue();
        double centerY = clusterRng->GetValue();

        // Posicionar el líder del cluster
        Ptr<Node> leaderNode = chNodes.Get(i);
        Ptr<MobilityModel> leaderMobility = leaderNode->GetObject<MobilityModel>();
        if (!leaderMobility)
        {
            NS_FATAL_ERROR("No se pudo obtener el modelo de movilidad para el líder " << i);
        }
        leaderMobility->SetPosition(Vector(centerX, centerY, 0));

        // Posicionar los seguidores alrededor del líder
        Ptr<NormalRandomVariable> offsetRng = CreateObject<NormalRandomVariable>();
        offsetRng->SetAttribute("Mean", DoubleValue(0.0));
        offsetRng->SetAttribute("Variance", DoubleValue(400.0)); // Más dispersión

        for (uint32_t j = 0; j < clusters[i].GetN(); ++j)
        {
            Ptr<Node> followerNode = clusters[i].Get(j);
            Ptr<MobilityModel> followerMobility = followerNode->GetObject<MobilityModel>();
            if (!followerMobility)
            {
                NS_FATAL_ERROR("No se pudo obtener el modelo de movilidad para el seguidor "
                               << j << " del cluster " << i);
            }

            double offsetX = offsetRng->GetValue();
            double offsetY = offsetRng->GetValue();
            followerMobility->SetPosition(Vector(centerX + offsetX, centerY + offsetY, 0));
        }
    }

    InitializeNodes(chNodes, true, 6.5f /*, outFile*/);
    InitializeNodes(memberNodes, false, 6.0f /*, outFile*/);

    BoidsMobilityModel::AddRandomFire();      // Primer fuego
    BoidsMobilityModel::CheckFireProximity(); // Iniciar verificaciones
    // Ejecutar simulación
    Simulator::Stop(Seconds(100));
    Simulator::Run();
    outFile.close();
    Simulator::Destroy();

    return 0;
}