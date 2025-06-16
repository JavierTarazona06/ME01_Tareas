
#include "ns3/log.h"
#include "ns3/core-module.h"      // Time, StringValue, LogComponent, etc.
#include "ns3/network-module.h"   // Node, NodeContainer, NetDeviceContainer…
#include "ns3/wifi-module.h"      // WifiHelper, YansWifiPhyHelper, WifiMacHelper
#include "ns3/internet-module.h"   // InternetStackHelper, Ipv4*
#include "ns3/mobility-module.h"   // MobilityHelper
#include "ns3/animation-interface.h" // AnimationInterface
#include "ns3/netanim-module.h" // AnimationInterface

using namespace ns3;                // Evita escribir ns3:: en cada referencia

NS_LOG_COMPONENT_DEFINE ("MiCmpt");

// Parámetros de la simulación

// Coeficientes para el cálculo de peso
static const double w1 = 0.7; // calidad de la señal
static const double w2 = 0.2; // batería
static const double w3 = 0.05; // obstaculos
static const double w4 = 0.025; // nodos conectados
static const double w5 = 0.025; // distancia a foco incendio

static const double TX_RANGE = 3; // Rango de transmisión en metros (conexiones entre nodos directas))
static const uint32_t N_CH = 2; // Número de líderes (Cluster-Heads)
static const uint32_t N_MEM = 10; // Número de seguidores (Miembros)
static const double SPEED_CH = 4.0; // Velocidad de los líderes (Cluster-Heads) en m/s
static const double ELECT_UPDATE_TIME = 6.0; // Tiempo de actualización de la reelección de líderes (en segundos)
static const double LEADERS_UPDATE_TIME = 6.1; // Tiempo de actualización de los líderes (en segundos)
static const double FOLLOWERS_UPDATE_TIME = 6.2; // Tiempo de actualización de los seguidores (en segundos)
static const double SMALL_DIST = 1.5; // Distancia pequeña para el offset aleatorio de distancia entre líderes y seguidores
static const double EPSILON = 1e-2; // Epsilon para evitar problemas de tiempo negativo
static const double SIM_TIME = 120.0; // Tiempo total de la simulación en segundos

enum Role { LEADER, FOLLOWER };


struct FireData
{
  Ptr<ListPositionAllocator> allocator;
  std::vector<Vector>        positions;
};

/*------------------- */

std::map<Ptr<Node>, Role> roles; // Mapa que asocia nodos a sus roles (líder o seguidor)

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
    if (w1 + w2 + w3 + w4 + w5 != 1.0) {
        NS_LOG_ERROR("Los coeficientes de peso deben sumar 1.0");
        return {0, 0}; // Retorna un par vacío si la suma no es 1
    }

    uint32_t nClusterHeads = N_CH;  // Número de líderes (Cluster-Heads)
    uint32_t nFollowers = N_MEM;     // Número de seguidores
    CommandLine cmd;              // Procesa argumentos de línea de comandos
    cmd.AddValue("nClusterHeads", "Número de líderes (Cluster-Heads)", nClusterHeads);
    cmd.AddValue("nFollowers", "Número de seguidores", nFollowers);
    cmd.Parse(argc, argv);        // Procesa los argumentos
    NS_LOG_UNCOND("Configuración: " << nClusterHeads << " líderes, " << nFollowers << " seguidores");
    return {nClusterHeads, nFollowers};
}

// Crea un vector aleatorio con un desplazamiento pequeño
Vector RandomOffset(double min = -SMALL_DIST, double max = SMALL_DIST) {
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  rand->SetAttribute("Min", DoubleValue(min));
  rand->SetAttribute("Max", DoubleValue(max));

  double x = rand->GetValue();
  double y = rand->GetValue();
  double z = rand->GetValue();

  return Vector(x, y, z);
}

/*------------------------------------------------
    2. Creación de nodos y Clusters
------------------------------------------------*/
void CreateNodes (uint32_t nClusterHeads, uint32_t nFollowers)
{
    // Crea dos contenedores: uno para líderes (Cluster-Heads) y otro para seguidores
    chNodes.Create (nClusterHeads);    // crea nClusterHeads instancias
    memberNodes.Create (nFollowers);   // crea nFollowers instancias
    NS_LOG_UNCOND("Creando containers nodos: " << nClusterHeads << " líderes, " 
        << nFollowers << " seguidores");
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

    NS_LOG_UNCOND("Creando clusters: " << nClusters << " líderes, " 
        << nMembers << " seguidores, " << nMembersPerCluster << " miembros por cluster (sin extras)");
}

/*------------------------------------------------
    3. Configuración de Wi-Fi para los nodos
------------------------------------------------*/
void SetupWifi ()
{
    //A. Configuración de Wi-Fi 
    /*
        Capa 802.11 física basada en el artículo Yet Another Network Simulator. 
            Idal para MANET (YansWifiPhyHelper)
        Medio compartido y permite añadir modelos de retardo o pérdida (YansWifiChannelHelper)
    */
    WifiHelper wifi;                                    // Crea un objeto WifiHelper para configurar Wi-Fi
    wifi.SetStandard (WIFI_STANDARD_80211a);            // Configura el estándar Wi-Fi a 802.11a
    YansWifiPhyHelper phy;                              // Configuración de la capa física (PHY) para Wi-Fi
    YansWifiChannelHelper 
        channel = YansWifiChannelHelper::Default();     // Crea un canal Wi-Fi por defecto
    phy.SetChannel (channel.Create ());                    // Configura el canal Wi-Fi para la capa física

    //B. Opciones personalizadas para el PHY
        /*[No incluídas en este ejemplo, pero se pueden añadir aquí]*/

    // C. Configura Wifi Ad Hoc y la capa MAC (Medium Access Control) para los nodos
        /* 
            Utiliza AdhocWifiMac para permitir que los nodos se comuniquen directamente entre sí
                sin necesidad de un punto de acceso centralizado
            Aplicar tasa de 6 Mb/s OFDM (MCS 0 de 802.11a/g) para simplificar variabilidades en la fase inicial (calidad señal, sistema de modulación)
                OFDM es un esquema multicarrier que divide la señal en múltiples subportadoras, mejorando la eficiencia espectral y la resistencia a interferencias.
            Utiliza una tasa de 6 Mbps para las tramas de datos y control
        */
    WifiMacHelper mac; // Configuración de la capa MAC
    mac.SetType ("ns3::AdhocWifiMac"); // Configura la capa MAC como Ad-hoc

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", //Aislar variables en la fase inicial
                                "DataMode", StringValue ("OfdmRate6Mbps"), 
                                "ControlMode", StringValue ("OfdmRate6Mbps")
                            ); 


    // D. Materializa interfaces de red Wi-Fi para los nodos líderes y seguidores
    chIntf = wifi.Install (phy, mac, chNodes); // Instala Wi-Fi en los nodos líderes (Cluster-Heads)
    memberIntf = wifi.Install (phy, mac, memberNodes); // Instala Wi-Fi en los nodos seguidores (Miembros)
    phy.EnablePcap("umanet-ch", chIntf); // Habilita la captura de paquetes en formato pcap para los nodos líderes
    phy.EnablePcap("umanet-follower", memberIntf); // Habilita la captura de paquetes en formato pcap para los nodos seguidores
    NS_LOG_UNCOND("Configuración Wi-Fi: " << chNodes.GetN() << " líderes, " 
        << memberNodes.GetN() << " seguidores");
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
    Incendios
------------------------------------------------*/
//TO-DO: Añadir un modelo de propagación de incendios y hacerlos estocasticos

void CreateFireAllocator ()
{
    /* 1. Lista de incendios (const, solo se construye una vez) */
    static const std::vector<Vector> kFires = {
        { 15, 10, 0 },
        { 20,  10, 0 },
        { 30, 10, 0 }
    };

    // ListPositionAllocator es un contenedor de puntos, se reutiliza al crear waypoints.
    Ptr<ListPositionAllocator> firePA = CreateObject<ListPositionAllocator> ();
    for (const auto& v : kFires)
    firePA->Add (v);

    fire_st = FireData(); // Inicializa la estructura de datos de incendios
    fire_st.allocator = firePA; // Asigna el ListPositionAllocator a la estructura de datos
    fire_st.positions = kFires; // Asigna la lista de posiciones de incendios a la estructura de datos
}

std::vector<Vector> firesDistOrdered(Vector position){
    std::vector<Vector> sortedFires = fire_st.positions;  // crea copia para no alterar el original

    // Ordenamos por distancia ascendente utilizando lambda optimizado
    std::sort(sortedFires.begin(), sortedFires.end(),
        [&position](const Vector &a, const Vector &b) {
            double da = CalculateDistance(position, a);
            double db = CalculateDistance(position, b);
            return da < db;
        }
    );
    return sortedFires; // devuelve el vector ordenado de incendios
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

// Inicializa modelo de movilidad para los líderes, antes de iniciar la simulación
void SetUpMobilityCH (double speed){
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::WaypointMobilityModel");
    mobility.Install(chNodes);

    for (uint32_t i = 0; i < chNodes.GetN (); ++i){
        Ptr<Node> node = chNodes.Get (i);
        NS_ABORT_MSG_IF (node == nullptr, "El nodo no puede ser nulo");
        roles[node] = LEADER; // Asigna el rol de líder al nodo

       
        // Get Model
        Ptr<WaypointMobilityModel> wp = node->GetObject<WaypointMobilityModel>();
        if (wp == nullptr) {
            NS_ABORT_MSG_IF (true, "WaypointMobilityModel no encontrado en el nodo líder "
                << node->GetId() << ". Asegúrate de que el modelo de movilidad se ha instalado correctamente.");
        }

        // 2. Waypoint inicial (despegue)
        double x, y, z;
        // Asignar posición inicial
        x = i * 10.0;  // Espaciado entre nodos
        y = 0.0;
        z = 0.0;
        Vector start (x, y, z); // Posición inicial del nodo líder
        double time_base = 0.0; // Tiempo base para los waypoints

        NS_LOG_UNCOND("Nodo " << node->GetId() << " -> Waypoint en " << time_base);
        wp->AddWaypoint (Waypoint (Seconds(time_base), start)); // Añade el waypoint inicial al modelo de movilidad

        std::vector<Vector> firesSorted = firesDistOrdered(start); // Ordena los incendios por distancia al nodo
        NS_ABORT_MSG_IF (firesSorted.empty(), "No hay incendios disponibles");
        
        for (const auto& p : firesSorted){
            double d = CalculateDistance(start, p);
            if (d == 0.0) {
                time_base += EPSILON;  // tiny epsilon to ensure increasing time
            } else {
                time_base += d / speed;
            }
            NS_LOG_UNCOND("Nodo " << node->GetId() << " -> Waypoint en " << time_base);
            wp->AddWaypoint (Waypoint (Seconds(time_base), p));
            start       = p;
        }

        NS_LOG_UNCOND("Nodo " << node->GetId() << " -> Waypoint en " << SIM_TIME+1);
        wp->AddWaypoint (Waypoint (Seconds(SIM_TIME+1), start)); // Añade el waypoint inicial al modelo de movilidad

        NS_LOG_UNCOND("Líder " << node->GetId() << " creado");
    }
}

// Actualiza la movilidad de los líderes
void UpdateMobilityCH(double speed){
    NS_ABORT_MSG_IF (speed <= 0.0, "La velocidad debe ser positiva");
    NS_LOG_UNCOND("Actualizando movilidad de líderes con velocidad " << speed << " m/s" << "Time: " << Simulator::Now().GetSeconds());

    for (uint32_t i = 0; i < chNodes.GetN (); ++i){
        Ptr<Node> node = chNodes.Get (i);
        NS_ABORT_MSG_IF (node == nullptr, "El nodo no puede ser nulo");
        Ptr<WaypointMobilityModel> wp = node->GetObject<WaypointMobilityModel>();
        NS_ABORT_MSG_IF(wp == nullptr,
            "WaypointMobilityModel no encontrado en el nodo líder "
            << node->GetId());

        // Obtener el siguiente waypoint (el que se está procesando actualmente)
        Vector pos_base = wp->GetNextWaypoint().position; // posición del último waypoint
        double time_base = Simulator::Now().GetSeconds(); // tiempo actual de la simulación    

        wp->EndMobility(); // Termina la movilidad para limpiar waypoints previos

        NS_LOG_UNCOND("Nodo " << node->GetId() << " -> Waypoint en " << time_base);
        wp->AddWaypoint(Waypoint(Seconds(time_base), pos_base)); // Añade el waypoint actual para evitar problemas de tiempo negativo
        
        time_base += EPSILON;
        NS_LOG_UNCOND("Nodo " << node->GetId() << " -> Waypoint en " << time_base);
        wp->AddWaypoint(Waypoint(Seconds(time_base), pos_base)); // Añade el waypoint actual para evitar problemas de tiempo negativo
        
        Vector pos = wp->GetPosition();
        std::vector<Vector> firesSorted = firesDistOrdered(pos); // Ordena los incendios por distancia al nodo
        NS_ABORT_MSG_IF (firesSorted.empty(), "No hay incendios disponibles");
        
        for (const auto& p : firesSorted)
        {
            double d = CalculateDistance(pos_base, p);
            if (d == 0.0) {
                time_base += EPSILON;  // tiny epsilon to ensure increasing time
            } else {
                time_base += d / speed;
            }
            NS_LOG_UNCOND("Nodo " << node->GetId() << " -> Waypoint en " << time_base);
            wp->AddWaypoint (Waypoint (Seconds(time_base), p));
            pos_base       = p;
        }
    }

    Simulator::Schedule(Seconds (LEADERS_UPDATE_TIME), // Programa la actualización de movilidad de líderes después de 6 segundos
        &UpdateMobilityCH, SPEED_CH);
}

// Inicializa modelo de movilidad para los seguidores
void SetUpMobilityFollowers()
{
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::WaypointMobilityModel");
    mobility.Install(memberNodes);

    for (uint32_t i = 0; i < memberNodes.GetN(); ++i) {
        Ptr<Node> node = memberNodes.Get(i);
        NS_ABORT_MSG_IF(node == nullptr, "El nodo no puede ser nulo");
        roles[node] = FOLLOWER; // Asigna el rol de seguidor al nodo
        NS_LOG_UNCOND("Seguidor " << node->GetId() << " creado");

        Ptr<WaypointMobilityModel> wp = node->GetObject<WaypointMobilityModel>();
        NS_ABORT_MSG_IF(wp == nullptr,
            "WaypointMobilityModel no encontrado en el nodo seguidor "
            << node->GetId());

        double x, y, z;
        // Asignar posición inicial
        x = i * 1.0;  // Espaciado entre nodos
        y = 4.0;
        z = 0.0;
        Vector start (x, y, z); // Posición inicial del nodo líder
        double time_base = 0.0;

        NS_LOG_UNCOND("Nodo " << node->GetId() << " -> Waypoint en " << time_base);
        wp->AddWaypoint (Waypoint (Seconds(time_base), start));

        NS_LOG_UNCOND("Nodo " << node->GetId() << " -> Waypoint en " << SIM_TIME+1);
        wp->AddWaypoint (Waypoint (Seconds(SIM_TIME+1), start));
    }
}

// Actualiza la movilidad de los seguidores en función del líder
void UpdateMobilityFollowersCl(int cluster_idx, double deltaTime){
    // 1. Obtiene el nodo líder y su modelo Waypoint
    Ptr<Node> leaderNode = chNodes.Get(cluster_idx);
    if (leaderNode == nullptr) {
        NS_LOG_ERROR("Líder no encontrado para el índice de cluster: " << cluster_idx);
        return; // Abortamos si no hay líder
    }
    Ptr<WaypointMobilityModel> wpLeader = leaderNode->GetObject<WaypointMobilityModel>();
    if (!wpLeader) {
        NS_LOG_ERROR("Líder " << leaderNode->GetId() << " sin WaypointMobilityModel");
        return;
    }
    Vector leaderPos = wpLeader->GetPosition();

    // 2. Recorre el cluster y actualiza cada seguidor
    for (uint32_t j = 0; j < clusters[cluster_idx].GetN(); ++j) {
        Ptr<Node> follower = clusters[cluster_idx].Get(j);

        Ptr<WaypointMobilityModel> wpFollower = follower->GetObject<WaypointMobilityModel>();
        if (wpFollower == nullptr) {
            NS_LOG_ERROR("Seguidor " << follower->GetId() << " sin WaypointMobilityModel");
            continue;  // Mejor omitir este nodo en lugar de abortar
        }

        double time_base = Simulator::Now().GetSeconds(); // tiempo actual de la simulación
        Vector pos_base = wpFollower->GetPosition(); // posición del último waypoint

        wpFollower->EndMobility(); // Termina la movilidad para limpiar waypoints previos
        
        NS_LOG_UNCOND("Nodo " << follower->GetId() << " -> Waypoint en " << time_base);
        wpFollower->AddWaypoint(Waypoint(Seconds(time_base), pos_base));

        time_base += EPSILON; // Asegura que el tiempo no sea negativo
        NS_LOG_UNCOND("Nodo " << follower->GetId() << " -> Waypoint en " << time_base);
        wpFollower->AddWaypoint(Waypoint(Seconds(time_base), pos_base));

        // 4. Añade nuevo waypoint cerca del líder
        Vector offset = RandomOffset();  // función que genera vector pequeño
        double t = time_base + deltaTime; // Tiempo futuro para el waypoint
        if (t < Simulator::Now().GetSeconds()) {
            NS_LOG_ERROR("El tiempo del waypoint actual es menor que el tiempo de simulación actual");
            t = Simulator::Now().GetSeconds(); // Asegura que el tiempo no sea negativo
        }
        NS_LOG_UNCOND("Nodo " << follower->GetId() << " -> Waypoint en " << t);
        wpFollower->AddWaypoint(Waypoint(Seconds(t), leaderPos + offset));
    }
}

void UpdateMobilityFollowers(){
    for (int cluster_idx = 0; cluster_idx < (int)clusters.size(); ++cluster_idx) {
        NS_LOG_UNCOND("Actualizando movilidad de seguidores en el cluster " << cluster_idx << "Time: " << Simulator::Now().GetSeconds());
        UpdateMobilityFollowersCl(cluster_idx, FOLLOWERS_UPDATE_TIME); // Actualiza la movilidad de los seguidores en el cluster
    }

    Simulator::Schedule(Seconds (FOLLOWERS_UPDATE_TIME), // Programa la movilidad de seguidores después de x segundos
        &UpdateMobilityFollowers); // Re-ejecuta la actualización de movilidad de seguidores
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


void RunWCAClustering(){
    // Reúne todos los nodos: antiguos seguidores + líderes actuales
    NS_LOG_UNCOND("Ejecutando reelección de líderes y formación de clusters..." << "Time: " << Simulator::Now().GetSeconds());
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
        roles[ch] = LEADER; // Asigna el rol de líder al nodo

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
            roles[node] = FOLLOWER; // Asigna el rol de seguidor al nodo
            thisCluster.Add(node); // Añade el nodo al cluster actual
            memberNodes.Add(node); // Añade el nodo al contenedor de seguidores
            unassigned.erase(node); // Elimina el nodo de la lista de nodos no asignados
        }
        chNodes.Add(ch); // Añade el líder al contenedor de líderes
        unassigned.erase(ch); // Elimina el líder del conjunto de nodos no asignados

        // 5.4 Guarda el cluster formado
        if ((int)clusters.size() < it+1){
            clusters.resize(it+1); // Asegura que el vector de clusters tenga suficiente espacio
        }
        clusters[it] = thisCluster; // Asigna el cluster actual al vector de clusters
        it++; // Incrementa el contador de iteraciones
    }
    clusters.resize(it);

    // Muestra resultados
    for (size_t i = 0; i < chNodes.GetN(); ++i) {
        Ptr<Node> leader = chNodes.Get(i);
        NS_LOG_UNCOND("Cluster " << i << ": CH=" << leader->GetId());
        for (uint32_t j = 0; j < clusters[i].GetN(); ++j) {
            Ptr<Node> nm = clusters[i].Get(j);
            NS_LOG_UNCOND("  Miembro " << nm->GetId() << " (Peso: " << weight[nm] << ")");
        }
    }

    Simulator::Schedule(Seconds (ELECT_UPDATE_TIME), // Programa la reelección de líderes después de 6 segundos
        &RunWCAClustering); // Re-ejecuta el algoritmo de clustering
}


/*------------------------------------------------
    Run Simulation
------------------------------------------------*/
void RunSimulation (double simTime)
{
    NS_LOG_UNCOND("Iniciando simulación con " << chNodes.GetN() << " líderes y " 
        << memberNodes.GetN() << " seguidores.");

    /*------------ Simulación ------------------------------*/
    Simulator::Stop (Seconds (simTime));// Establece el tiempo de parada de la simulación
    Simulator::Run (); // Ejecuta la simulación hasta que se alcance el tiempo de parada
    Simulator::Destroy (); // Libera los recursos utilizados por la simulación
    NS_LOG_UNCOND("Simulación finalizada. Recursos liberados.");
}

int main(int argc, char* argv[]){
    LogComponentEnable ("MiCmpt", LOG_LEVEL_ERROR);
    /*------------------------------------------------
        1. Configuración de parámetros
    ------------------------------------------------*/
    auto [nCH, nMem]       = ParseCommandLine (argc, argv);

    Config::SetDefault (
        "ns3::WaypointMobilityModel::InitialPositionIsWaypoint",
        BooleanValue (true)
    );
    
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
    SetUpMobilityCH (SPEED_CH);
    SetUpMobilityFollowers();

    /*------------------------------------------------
        Iniciar movimiento de líderes y seguidores
    ------------------------------------------------*/
    /*Simulator::Schedule(Seconds (0.2), // Programa la movilidad de seguidores después de x segundos
        &UpdateMobilityFollowers);
    Simulator::Schedule(Seconds (LEADERS_UPDATE_TIME), // Programa la actualización de movilidad de líderes después de 6 segundos
        &UpdateMobilityCH, SPEED_CH);*/

    /*------------------------------------------------
        Reelección de líderes
    ------------------------------------------------*/
    /*Simulator::Schedule(Seconds (ELECT_UPDATE_TIME), // Programa la reelección de líderes después de 6 segundos
        &RunWCAClustering); // Ejecuta el algoritmo de clustering*/


    /*------------------------------------------------
        Animación
    ------------------------------------------------*/
    Simulator::Schedule(Seconds(1e-9), []() {
        NS_LOG_UNCOND("Configurando la interfaz de animación...");
        AnimationInterface anim("umanet.xml");
        // anim.SetBackgroundImage("mapa.png", 0, 0, 500.0, 500.0, 0.8);
        anim.EnablePacketMetadata(true);
        anim.SetMobilityPollInterval(Seconds(0.1));

        for (uint32_t i = 0; i < chNodes.GetN(); ++i) {
            anim.UpdateNodeColor(chNodes.Get(i), 0, 0, 255);
        }
        for (uint32_t i = 0; i < memberNodes.GetN(); ++i) {
            anim.UpdateNodeColor(memberNodes.Get(i), 255, 0, 0);
        }
    });


    /*------------------------------------------------
        Run Simulation
    ------------------------------------------------*/
    RunSimulation (SIM_TIME);

    return 0;
}