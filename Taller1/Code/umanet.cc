
#include "ns3/core-module.h"      // Time, StringValue, LogComponent, etc.
#include "ns3/network-module.h"   // Node, NodeContainer, NetDeviceContainer…
#include "ns3/wifi-module.h"      // WifiHelper, YansWifiPhyHelper, WifiMacHelper
#include "ns3/internet-module.h"   // InternetStackHelper, Ipv4*
#include "ns3/mobility-module.h"   // MobilityHelper

using namespace ns3;                // Evita escribir ns3:: en cada referencia


/*------------------------------------------------
    1. Configuración de parámetros
------------------------------------------------*/
std::pair<uint32_t,uint32_t> ParseCommandLine (int argc, char* argv[])
{
    uint32_t nClusterHeads = 2;  // Número de líderes (Cluster-Heads)
    uint32_t nFollowers = 10;     // Número de seguidores
    CommandLine cmd;              // Procesa argumentos de línea de comandos
    cmd.AddValue("nClusterHeads", "Número de líderes (Cluster-Heads)", nClusterHeads);
    cmd.AddValue("nFollowers", "Número de seguidores", nFollowers);
    cmd.Parse(argc, argv);        // Procesa los argumentos
    NS_LOG_UNCOND("Configuración: " << nClusterHeads << " líderes, " << nFollowers << " seguidores");
    return {nClusterHeads, nFollowers};
}

/*------------------------------------------------
    2. Creación de nodos
------------------------------------------------*/
auto CreateNodes (uint32_t nClusterHeads, uint32_t nFollowers)
{
    // Crea dos contenedores: uno para líderes (Cluster-Heads) y otro para seguidores
    NodeContainer chNodes;            // líderes (Cluster-Heads)
    chNodes.Create (nClusterHeads);    // crea nClusterHeads instancias
    NodeContainer memberNodes;         // seguidores
    memberNodes.Create (nFollowers);   // crea nFollowers instancias
    return std::make_pair (chNodes, memberNodes);
}

/*------------------------------------------------
    3. Configuración de Wi-Fi para los nodos
------------------------------------------------*/
auto SetupWifi (NodeContainer chNodes, NodeContainer memberNodes)
{
    //A. Configuración de Wi-Fi 
    WifiHelper wifi;    
    wifi.SetStandard (WIFI_STANDARD_80211a);          // opcional
        // Capa 802.11 física basada en el artículo Yet Another Network Simulator. Idal para MANET (YansWifiPhyHelper)
    YansWifiPhyHelper phy; 
        // Medio compartido y permite añadir modelos de retardo o pérdida (YansWifiChannelHelper)
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default(); 
    phy.SetChannel (channel.Create ());

    //B. Opciones personalizadas para el PHY
        // [No incluídas en este ejemplo, pero se pueden añadir aquí]

    // C. Configura Wifi Ad Hoc y la capa MAC (Medium Access Control) para los nodos
        // Utiliza AdhocWifiMac para permitir que los nodos se comuniquen directamente entre sí
            // sin necesidad de un punto de acceso centralizado
    WifiMacHelper mac; // Configuración de la capa MAC
    mac.SetType ("ns3::AdhocWifiMac"); // Configura la capa MAC como Ad-hoc
        // Aplicar tasa de 6 Mb/s OFDM (MCS 0 de 802.11a/g) para simplificar variabilidades en la fase inicial (calidad señal, sistema de modulación)
            // OFDM es un esquema multicarrier que divide la señal en múltiples subportadoras, mejorando la eficiencia espectral y la resistencia a interferencias.
        // Utiliza una tasa de 6 Mbps para las tramas de datos y control
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", //Aislar variables en la fase inicial
                                "DataMode", StringValue ("OfdmRate6Mbps"), 
                                "ControlMode", StringValue ("OfdmRate6Mbps")
                            ); 


    // D. Materializa interfaces de red Wi-Fi para los nodos líderes y seguidores
    NetDeviceContainer chIntf = wifi.Install (phy, mac, chNodes);
    NetDeviceContainer memIntf = wifi.Install (phy, mac, memberNodes);
    phy.EnablePcap("umanet-ch", chIntf);
    phy.EnablePcap("umanet-follower", memIntf);
  return std::make_pair (chIntf, memIntf);
}

void InstallInternet (NodeContainer chNodes, NodeContainer memberNodes)
{
    // E. Agregra a los nodos la pila de protocolos de Internet (IPv4, TCP, UDP, etc.)
        // Esto permite que los nodos se comuniquen utilizando direcciones IP y protocolos de red
    InternetStackHelper internet;
    internet.Install (chNodes);
    internet.Install (memberNodes);
}

void AssignIpv4 (NetDeviceContainer chIntf, NetDeviceContainer memberIntf)
{
    // F. Asignación de direcciones IP a las interfaces de red. Mascara de subred /24
    Ipv4AddressHelper ip;
    ip.SetBase("10.0.0.0", "255.255.255.0");
    ip.Assign(chIntf);
    ip.Assign(memberIntf);
}


/*------------------------------------------------
    Mobility
------------------------------------------------*/
void SetupMobility (NodeContainer chNodes, NodeContainer memberNodes)
{
    // Configuración de la movilidad de los nodos
        // Utiliza un modelo de movilidad constante para los líderes y seguidores
        // Esto significa que los nodos no se moverán durante la simulación, permaneciendo en una posición fija
    MobilityHelper mob;
    mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mob.Install(chNodes);
    mob.Install(memberNodes);
}

/*------------------------------------------------
    Run Simulation
------------------------------------------------*/
void RunSimulation (double simTime)
{
  Simulator::Stop (Seconds (simTime));// Establece el tiempo de parada de la simulación
  Simulator::Run (); // Ejecuta la simulación hasta que se alcance el tiempo de parada
  Simulator::Destroy (); // Libera los recursos utilizados por la simulación
  NS_LOG_UNCOND("Simulación finalizada. Recursos liberados.");
}

int
main(int argc, char* argv[])
{

    /*------------------------------------------------
        1. Configuración de parámetros
    ------------------------------------------------*/
    auto [nCH, nMem]       = ParseCommandLine (argc, argv);
    
    /*------------------------------------------------
        2. Creación de nodos
    ------------------------------------------------*/
    auto [chNodes, memberNodes] = CreateNodes (nCH, nMem);

    /*------------------------------------------------
        3. Configuración de Wi-Fi para los nodos
    ------------------------------------------------*/
    auto [chIntf, memIntf]   = SetupWifi (chNodes, memberNodes);
        // Configuración de la capa física para capturar paquetes en formato pcap
            // Esto permite registrar los paquetes transmitidos y recibidos en un archivo pcap para su posterior análisis
    InstallInternet (chNodes, memberNodes);
    AssignIpv4 (chIntf, memIntf);

    /*------------------------------------------------
        Mobility
    ------------------------------------------------*/
    SetupMobility (chNodes, memberNodes);

    /*------------------------------------------------
        Run Simulation
    ------------------------------------------------*/
    RunSimulation (30);

    return 0;
}