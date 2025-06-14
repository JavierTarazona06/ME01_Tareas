
#include "ns3/core-module.h"      // Time, StringValue, LogComponent, etc.
#include "ns3/network-module.h"   // Node, NodeContainer, NetDeviceContainer…
#include "ns3/wifi-module.h"      // WifiHelper, YansWifiPhyHelper, WifiMacHelper
#include "ns3/internet-module.h"   // InternetStackHelper, Ipv4*
#include "ns3/mobility-module.h"   // MobilityHelper
//#include "ns3/animation-interface.h" // AnimationInterface
#include "ns3/netanim-module.h" // AnimationInterface

using namespace ns3;                // Evita escribir ns3:: en cada referencia

// Parámetros de la simulación

// Coeficientes para el cálculo de peso
static const double w1 = 0.7; // calidad de la señal
static const double w2 = 0.2; // batería
static const double w3 = 0.05; // obstaculos
static const double w4 = 0.025; // nodos conectados
static const double w5 = 0.025; // distancia a foco incendio

static const double TX_RANGE = 2.8; // Rango de transmisión en metros
static const uint32_t N_CH = 2; // Número de líderes (Cluster-Heads)
static const uint32_t N_MEM = 10; // Número de seguidores (Miembros)

struct FireData
{
  Ptr<ListPositionAllocator> allocator;
  std::vector<Vector>        positions;
};

/*------------------- */

NodeContainer chNodes; // Contenedor para los nodos líderes (Cluster-Heads)
NodeContainer memberNodes; // Contenedor para los nodos seguidores (Miembros)

std::vector<NodeContainer> clusters; // Vector para almacenar los clusters formados

NetDeviceContainer chIntf; // Contenedor para las interfaces de red de los nodos líderes
NetDeviceContainer memberIntf; // Contenedor para las interfaces de red de los nodos seguidores

FireData fire_st;  // Estructura para almacenar datos de incendios

/*------------------------------------------------
    1. Configuración de parámetros
------------------------------------------------*/
std::pair<uint32_t,uint32_t> ParseCommandLine (int argc, char* argv[])
{
    uint32_t nClusterHeads = N_CH;  // Número de líderes (Cluster-Heads)
    uint32_t nFollowers = N_MEM;     // Número de seguidores
    CommandLine cmd;              // Procesa argumentos de línea de comandos
    cmd.AddValue("nClusterHeads", "Número de líderes (Cluster-Heads)", nClusterHeads);
    cmd.AddValue("nFollowers", "Número de seguidores", nFollowers);
    cmd.Parse(argc, argv);        // Procesa los argumentos
    NS_LOG_UNCOND("Configuración: " << nClusterHeads << " líderes, " << nFollowers << " seguidores");
    return {nClusterHeads, nFollowers};
}

/*------------------------------------------------
    2. Creación de nodos y Clusters
------------------------------------------------*/
void CreateNodes (uint32_t nClusterHeads, uint32_t nFollowers)
{
    // Crea dos contenedores: uno para líderes (Cluster-Heads) y otro para seguidores
    chNodes.Create (nClusterHeads);    // crea nClusterHeads instancias
    memberNodes.Create (nFollowers);   // crea nFollowers instancias
}

void CreateClusters()
{
    // Distribuye los nodos seguidores en clusters basados en los nodos líderes (Cluster-Heads)
    int nMembers = memberNodes.GetN();
    NS_ABORT_MSG_IF(nMembers == 0, "El número de nodos seguidores no puede ser cero");
    int nClusters = chNodes.GetN();
    clusters.resize(nClusters);
    NS_ABORT_MSG_IF(nClusters == 0, "El número de nodos líderes no puede ser cero");

    int nMembersPerCluster = nMembers / nClusters; // Número de nodos seguidores por cluster
    int nMembersExtra = nMembers % nClusters; // Nodos seguidores extra que no se distribuyen uniformemente

    int idx = 0; // Índice para recorrer los nodos seguidores
    for (int i = 0; i < nClusters; ++i) // Recorre cada nodo líder
    {
        int size = nMembersPerCluster + (nMembersExtra > 0 ? 1 : 0); // Tamaño del cluster, incrementado si hay nodos extra
        nMembersExtra -= (nMembersExtra > 0 ? 1 : 0); // reduce el contador de extras
        for (int j = 0; j < size; ++j, ++idx)
        {
            NS_ABORT_MSG_IF(idx >= nMembers, "Índice fuera de rango");
            Ptr<Node> mNodec = memberNodes.Get(idx);
            clusters[i].Add(mNodec); // Añade el nodo seguidor al cluster correspondiente
        }
    }
}

/*------------------------------------------------
    3. Configuración de Wi-Fi para los nodos
------------------------------------------------*/
void SetupWifi ()
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
    chIntf = wifi.Install (phy, mac, chNodes);
    memberIntf = wifi.Install (phy, mac, memberNodes);
    phy.EnablePcap("umanet-ch", chIntf);
    phy.EnablePcap("umanet-follower", memberIntf);
}

void InstallInternet ()
{
    // E. Agregra a los nodos la pila de protocolos de Internet (IPv4, TCP, UDP, etc.)
        // Esto permite que los nodos se comuniquen utilizando direcciones IP y protocolos de red
    InternetStackHelper internet;
    internet.Install (chNodes);
    internet.Install (memberNodes);
}

void AssignIpv4 ()
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
void SetupMobility ()
{
    // Configuración de la movilidad de los nodos
        // Utiliza un modelo de movilidad constante para los líderes y seguidores
        // Esto significa que los nodos no se moverán durante la simulación, permaneciendo en una posición fija
    MobilityHelper mob;
    mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mob.Install(chNodes);
    mob.Install(memberNodes);
}

void
SetUpMobilityCH (double speed)
{
  NS_ABORT_MSG_IF (speed <= 0.0, "La velocidad debe ser positiva");

  for (uint32_t i = 0; i < chNodes.GetN (); ++i)
  {
    Ptr<Node> node = chNodes.Get (i);
    NS_ABORT_MSG_IF (node == nullptr, "El nodo no puede ser nulo");

    Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
    double x, y, z; 

    if (mob == nullptr) {
        // Asignar posición inicial
        x = i * 10.0;  // Espaciado entre nodos
        y = 0.0;
        z = 0.0;
        AnimationInterface::SetConstantPosition(node, x, y, z);
    } else {
        Vector pos = mob->GetPosition();
        x = pos.x;
        y = pos.y;
        z = pos.z;
        AnimationInterface::SetConstantPosition(node, x, y, z);
    }
    
    // 1. Crear el modelo y agregarlo al nodo
    Ptr<WaypointMobilityModel> wp = node->GetObject<WaypointMobilityModel>();
    if (wp == nullptr) {
        wp = CreateObject<WaypointMobilityModel>();
        node->AggregateObject(wp);
    }

    // 2. Waypoint inicial (despegue)
    Vector start (x, y, z); // Posición inicial del nodo líder
    Time   t     = Simulator::Now(); // Tiempo de inicio del waypoint
    wp->AddWaypoint (Waypoint (t, start));

    // 3. Waypoints hacia cada incendio
    Vector last = start;
    std::vector<Vector> fires = fire_st.positions; // Lista de incendios
    for (const auto& p : fires)
    {
      double d   = CalculateDistance (last, p);
      t         += Seconds (d / speed);
      wp->AddWaypoint (Waypoint (t, p));
      last       = p;
    }
  }
}

void SetUpMobilityFollowers(int idx)
{
    Ptr<Object> chLeaderNode = chNodes.Get(idx); // Obtiene el nodo líder (Cluster Head) correspondiente al índice
    MobilityHelper mob; // Configuración de la movilidad de los seguidores
    mob.PushReferenceMobilityModel(chLeaderNode); // Utiliza el modelo de movilidad del líder como referencia

      // Crear el RandomVariable para velocidad
    Ptr<UniformRandomVariable> speedRv = CreateObject<UniformRandomVariable> ();
    speedRv->SetAttribute("Min", DoubleValue(0.0));
    speedRv->SetAttribute("Max", DoubleValue(2.0));

    mob.SetMobilityModel("ns3::RandomWalk2dMobilityModel", // Modelo de movilidad aleatoria en 2D
        "Bounds", RectangleValue(Rectangle(-10,10,-10,10)), // Límites del área de movimiento
        "Speed", PointerValue(speedRv) // Velocidad aleatoria entre 0 y 2 m/s
    );
    mob.Install(clusters[idx]); // Instala el modelo de movilidad en los nodos seguidores del cluster
}

/*------------------------------------------------
    Incendios
------------------------------------------------*/
//TO-DO: Añadir un modelo de propagación de incendios y hacerlos estocasticos


void CreateFireAllocator ()
{
    /* 1. Lista de incendios (const, solo se construye una vez) */
    static const std::vector<Vector> kFires = {
        {  5, 8, 0 },
        { 4,  2, 0 },
        { 8, 10, 0 }
    };

    // ListPositionAllocator es un contenedor de puntos, se reutiliza al crear waypoints.
    Ptr<ListPositionAllocator> firePA = CreateObject<ListPositionAllocator> ();
    for (const auto& v : kFires)
    firePA->Add (v);

    fire_st = FireData(); // Inicializa la estructura de datos de incendios
    fire_st.allocator = firePA; // Asigna el ListPositionAllocator a la estructura de datos
    fire_st.positions = kFires; // Asigna la lista de posiciones de incendios a la estructura de datos
}

/*------------------------------------------------
    Reelección de líderes
------------------------------------------------*/

double ComputeNodeWeight(Ptr<Node> node)
{
  double channelQ = node->GetId() % 5 + 1;
  double energy = node->GetId() % 6 + 1;
  double obstacles = node->GetId() % 7 + 1;
  double connectivity = node->GetId() % 8 + 1;
  double distanceFire = node->GetId() % 9 + 1;
  return w1 * channelQ + 
         w2 * energy + 
         w3 * obstacles + 
         w4 * connectivity + 
         w5 * distanceFire;
}


void RunWcaClustering()
{
    // Reúne todos los nodos: antiguos seguidores + líderes actuales
    NodeContainer allNodes;
    for (uint32_t i=0; i < chNodes.GetN(); ++i) {
        Ptr<Node> node = chNodes.Get(i);
        allNodes.Add(node);
    }
    for (uint32_t i=0; i < memberNodes.GetN(); ++i) {
        Ptr<Node> node = memberNodes.Get(i);
        allNodes.Add(node);
    }

    int N = allNodes.GetN();
    NS_ABORT_MSG_IF(N == 0, "No hay nodos para clustering");

    // Calcula pesos
    std::map<Ptr<Node>, double> weight;
    for (int i = 0; i < N; ++i) {
        Ptr<Node> node = allNodes.Get(i);
        weight[node] = ComputeNodeWeight(node);
    }

    // Todos los nodos inicialmente sin asignar
    std::set<Ptr<Node>> unassigned;
    for (int i = 0; i < N; ++i)
        unassigned.insert(allNodes.Get(i));

    // Reinicializa chNodes / clusters
    chNodes = NodeContainer();
    memberNodes = NodeContainer();
    clusters.clear();

    // Construye clusters iterativamente
    int it = 0;
    while (!unassigned.empty()) {
        // 5.1 Selecciona nodo con menor peso
        auto best_it = std::min_element(
            unassigned.begin(), unassigned.end(), // Ordena por peso
            [&weight](Ptr<Node> a, Ptr<Node> b){ return weight[a] < weight[b]; }); // Encuentra el nodo con menor peso
        Ptr<Node> ch = *best_it; // Nodo líder (Cluster Head) seleccionado
        chNodes.Add(ch);
        clusters.emplace_back(NodeContainer());

        // 5.2 Forma el cluster actual
        NodeContainer thisCluster;
        std::vector<Ptr<Node>> toAssign;

        for (auto node : unassigned) {
            if (node == ch) continue;
            // criterio simple: distancia ≤ 2·yourTxRange
            auto posCh   = ch->GetObject<MobilityModel>()->GetPosition();
            auto posNode = node->GetObject<MobilityModel>()->GetPosition();
            double d = CalculateDistance(posCh, posNode);
            if (d <= 2 * TX_RANGE) toAssign.push_back(node); // Añade el nodo al vector de nodos a asignar
        }

        // 5.3 Asigna esos nodos
        for (auto node : toAssign) {
            thisCluster.Add(node); // Añade el nodo al cluster actual
            memberNodes.Add(node); // Añade el nodo al contenedor de seguidores
            unassigned.erase(node); // Elimina el nodo de la lista de nodos no asignados
        }
        unassigned.erase(ch);

        // 5.4 Guarda el cluster formado
        if ((int)clusters.size() < it+1){
            clusters.resize(it+1); // Asegura que el vector de clusters tenga suficiente espacio
        }
        clusters[it] = thisCluster; // Asigna el cluster actual al vector de clusters
        it++; // Incrementa el contador de iteraciones
    }

    // Muestra resultados
    for (size_t i = 0; i < chNodes.GetN(); ++i) {
        Ptr<Node> leader = chNodes.Get(i);
        NS_LOG_UNCOND("Cluster " << i << ": CH=" << leader->GetId());
        for (uint32_t j = 0; j < clusters[i].GetN(); ++j) {
            Ptr<Node> nm = clusters[i].Get(j);
            NS_LOG_UNCOND("  Miembro " << nm->GetId() << " (Peso: " << weight[nm] << ")");
        }
    }
}


/*------------------------------------------------
    Run Simulation
------------------------------------------------*/
void RunSimulation (double simTime)
{
    NS_LOG_UNCOND("Iniciando simulación con " << chNodes.GetN() << " líderes y " 
        << memberNodes.GetN() << " seguidores.");

    if (true){
        // Configuración de la animación
        NS_LOG_UNCOND("Configurando la interfaz de animación...");

        /*---------- Configuración Animación ----------------------*/
        AnimationInterface anim ("umanet.xml");    // trazas de movimiento/paquetes
        // anim.SetBackgroundImage("mapa.png", 0, 0, 500.0, 500.0, 0.8);
        anim.EnablePacketMetadata (true);              // dibujar flechas de paquetes
        anim.SetMobilityPollInterval (Seconds (0.1));  // suavidad 10 Hz

        /*---------------- Eventos -------------------------------*/
        Simulator::Schedule(Seconds (6.0), // Programa la reelección de líderes después de 6 segundos
            &RunWcaClustering);
        Simulator::Schedule(Seconds (6.2), // Programa la movilidad de líderes después de 6 segundos
            &SetUpMobilityCH, 10.0 // Velocidad de 10 m/s para los líderes
        );
        for (size_t i = 0; i < clusters.size(); ++i) {
            Simulator::Schedule(Seconds (6.6), // Programa la movilidad de seguidores después de 6 segundos
                &SetUpMobilityFollowers, i
            );
        }

        for (uint32_t i = 0; i < chNodes.GetN(); ++i){
            anim.UpdateNodeColor(chNodes.Get(i), 0, 0, 255);
        }
        for (uint32_t i = 0; i < memberNodes.GetN(); ++i){
            anim.UpdateNodeColor(memberNodes.Get(i), 255, 0, 0);
        }
    }


    /*------------ Simulación ------------------------------*/
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
        2. Creación de nodos y Clusters
    ------------------------------------------------*/
    CreateNodes (nCH, nMem);
    CreateClusters (); // Distribuye los nodos seguidores en clusters        

    /*------------------------------------------------
        3. Configuración de Wi-Fi para los nodos
    ------------------------------------------------*/
    SetupWifi ();
        // Configuración de la capa física para capturar paquetes en formato pcap
            // Esto permite registrar los paquetes transmitidos y recibidos en un archivo pcap para su posterior análisis
    InstallInternet ();
    AssignIpv4 ();

    /*-------------------------------------------------
        Fire Data
    --------------------------------------------------*/

    CreateFireAllocator();

    /*------------------------------------------------
        Mobility
    ------------------------------------------------*/
    SetUpMobilityCH (10.0); // Velocidad de 10 m/s para los líderes
    for (size_t i = 0; i < clusters.size(); ++i) {
        SetUpMobilityFollowers(i); // Asigna el modelo de movilidad a los seguidores
    }

    /*------------------------------------------------
        Run Simulation
    ------------------------------------------------*/
    RunSimulation (30);

    return 0;
}