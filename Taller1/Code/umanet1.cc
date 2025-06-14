
#include "ns3/log.h"
#include "ns3/core-module.h"     // Time, StringValue, LogComponent, etc.
#include "ns3/network-module.h"  // Node, NodeContainer, NetDeviceContainer…
#include "ns3/wifi-module.h"     // WifiHelper, YansWifiPhyHelper, WifiMacHelper
#include "ns3/internet-module.h" // InternetStackHelper, Ipv4*
#include "ns3/mobility-module.h" // MobilityHelper
// #include "ns3/animation-interface.h" // AnimationInterface
// #include "ns3/netanim-module.h" // AnimationInterface

using namespace ns3; // Evita escribir ns3:: en cada referencia

NS_LOG_COMPONENT_DEFINE("MiCmpt");

// Parámetros de la simulación

// Coeficientes para el cálculo de peso
static const double w1 = 0.7;   // calidad de la señal
static const double w2 = 0.2;   // batería
static const double w3 = 0.05;  // obstaculos
static const double w4 = 0.025; // nodos conectados
static const double w5 = 0.025; // distancia a foco incendio

static const double TX_RANGE = 3;                              // Rango de transmisión en metros
static const uint32_t N_CH = 2;                                // Número de líderes (Cluster-Heads)
static const uint32_t N_MEM = 10;                              // Número de seguidores (Miembros)
static const double SPEED_CH = 4.0;                            // Número de seguidores (Miembros)
static const double UPDATE_TIME = 6.0;                         // Tiempo de actualización de los líderes (en segundos)
static const double FOLLOWERS_UPDATE_TIME = UPDATE_TIME + 0.3; // Tiempo de actualización de los seguidores (en segundos)
static const double SMALL_DIST = 1.5;                          // Distancia pequeña para el offset aleatorio

enum Role
{
    LEADER,
    FOLLOWER
};
std::map<Ptr<Node>, Role> roles; // Mapa que asocia nodos a sus roles (líder o seguidor)

// Estructura para registrar tiempos de extinción
struct FireExtinctionRecord
{
    Vector position;
    Time detectionTime;
    Time extinctionTime;
    uint32_t extinguishingNode;
};

std::vector<FireExtinctionRecord> extinctionRecords;
std::set<uint32_t> activeFires; // Índices de incendios activos

struct FireData
{
    Ptr<ListPositionAllocator> allocator;
    std::vector<Vector> positions;
    std::map<uint32_t, bool> isActive; // Mapa de incendios activos (índice -> activo)
    uint32_t nextFireId = 0;
};

/*------------------- */

NodeContainer chNodes;     // Contenedor para los nodos líderes (Cluster-Heads)
NodeContainer memberNodes; // Contenedor para los nodos seguidores (Miembros)

std::vector<NodeContainer> clusters; // Vector para almacenar los clusters formados

NetDeviceContainer chIntf;     // Contenedor para las interfaces de red de los nodos líderes
NetDeviceContainer memberIntf; // Contenedor para las interfaces de red de los nodos seguidores

FireData fire_st; // Estructura para almacenar datos de incendios

/*------------------------------------------------
    1. Configuración de parámetros
------------------------------------------------*/
std::pair<uint32_t, uint32_t> ParseCommandLine(int argc, char *argv[])
{
    uint32_t nClusterHeads = N_CH; // Número de líderes (Cluster-Heads)
    uint32_t nFollowers = N_MEM;   // Número de seguidores
    CommandLine cmd;               // Procesa argumentos de línea de comandos
    cmd.AddValue("nClusterHeads", "Número de líderes (Cluster-Heads)", nClusterHeads);
    cmd.AddValue("nFollowers", "Número de seguidores", nFollowers);
    cmd.Parse(argc, argv); // Procesa los argumentos
    NS_LOG_UNCOND("Configuración: " << nClusterHeads << " líderes, " << nFollowers << " seguidores");
    return {nClusterHeads, nFollowers};
}

Vector RandomOffset(double min = -SMALL_DIST, double max = SMALL_DIST)
{
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
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
void CreateNodes(uint32_t nClusterHeads, uint32_t nFollowers)
{
    // Crea dos contenedores: uno para líderes (Cluster-Heads) y otro para seguidores
    chNodes.Create(nClusterHeads);  // crea nClusterHeads instancias
    memberNodes.Create(nFollowers); // crea nFollowers instancias
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
    int nMembersExtra = nMembers % nClusters;      // Nodos seguidores extra que no se distribuyen uniformemente

    int idx = 0;                        // Índice para recorrer los nodos seguidores
    for (int i = 0; i < nClusters; ++i) // Recorre cada nodo líder
    {
        int size = nMembersPerCluster + (nMembersExtra > 0 ? 1 : 0); // Tamaño del cluster, incrementado si hay nodos extra
        nMembersExtra -= (nMembersExtra > 0 ? 1 : 0);                // reduce el contador de extras
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
void SetupWifi()
{
    // A. Configuración de Wi-Fi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a); // opcional
                                            // Capa 802.11 física basada en el artículo Yet Another Network Simulator. Idal para MANET (YansWifiPhyHelper)
    YansWifiPhyHelper phy;
    // Medio compartido y permite añadir modelos de retardo o pérdida (YansWifiChannelHelper)
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    // B. Opciones personalizadas para el PHY
    //  [No incluídas en este ejemplo, pero se pueden añadir aquí]

    // C. Configura Wifi Ad Hoc y la capa MAC (Medium Access Control) para los nodos
    // Utiliza AdhocWifiMac para permitir que los nodos se comuniquen directamente entre sí
    // sin necesidad de un punto de acceso centralizado
    WifiMacHelper mac;                // Configuración de la capa MAC
    mac.SetType("ns3::AdhocWifiMac"); // Configura la capa MAC como Ad-hoc
                                      // Aplicar tasa de 6 Mb/s OFDM (MCS 0 de 802.11a/g) para simplificar variabilidades en la fase inicial (calidad señal, sistema de modulación)
                                      // OFDM es un esquema multicarrier que divide la señal en múltiples subportadoras, mejorando la eficiencia espectral y la resistencia a interferencias.
    // Utiliza una tasa de 6 Mbps para las tramas de datos y control
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", // Aislar variables en la fase inicial
                                 "DataMode", StringValue("OfdmRate6Mbps"),
                                 "ControlMode", StringValue("OfdmRate6Mbps"));

    // D. Materializa interfaces de red Wi-Fi para los nodos líderes y seguidores
    chIntf = wifi.Install(phy, mac, chNodes);
    memberIntf = wifi.Install(phy, mac, memberNodes);
    phy.EnablePcap("umanet-ch", chIntf);
    phy.EnablePcap("umanet-follower", memberIntf);
}

void CheckFireExtinction()
{
    for (uint32_t i = 0; i < chNodes.GetN(); ++i)
    {
        Ptr<Node> node = chNodes.Get(i);
        Ptr<WaypointMobilityModel> wp = node->GetObject<WaypointMobilityModel>();
        if (!wp)
            continue;

        Vector nodePos = wp->GetPosition();

        // Verifica cada incendio activo
        for (auto &fire : fire_st.positions)
        {
            double distance = CalculateDistance(nodePos, fire);
            if (distance < 1.0)
            { // Radio de extinción (1 metro)
                // Marca incendio como extinguido
                fire_st.isActive[&fire - &fire_st.positions[0]] = false;

                // Registra el tiempo de extinción
                FireExtinctionRecord record;
                record.position = fire;
                record.extinguishingNode = node->GetId();
                record.extinctionTime = Simulator::Now();
                extinctionRecords.push_back(record);

                NS_LOG_UNCOND("INCENDIO EXTINGUIDO! Nodo " << node->GetId()
                                                           << " ha apagado incendio en (" << fire.x << ", " << fire.y
                                                           << ") a los " << record.extinctionTime.GetSeconds() << " segundos");
            }
        }
    }

    // Programa próxima verificación
    Simulator::Schedule(Seconds(0.5), &CheckFireExtinction);
}

void InstallInternet()
{
    // E. Agregra a los nodos la pila de protocolos de Internet (IPv4, TCP, UDP, etc.)
    // Esto permite que los nodos se comuniquen utilizando direcciones IP y protocolos de red
    InternetStackHelper internet;
    internet.Install(chNodes);
    internet.Install(memberNodes);
}

void AssignIpv4()
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
// TO-DO: Añadir un modelo de propagación de incendios y hacerlos estocasticos

void CreateFireAllocator()
{
    Ptr<UniformRandomVariable> randX = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> randY = CreateObject<UniformRandomVariable>();

    randX->SetAttribute("Min", DoubleValue(0.0));
    randX->SetAttribute("Max", DoubleValue(50.0));
    randY->SetAttribute("Min", DoubleValue(0.0));
    randY->SetAttribute("Max", DoubleValue(30.0));

    Ptr<UniformRandomVariable> randCount = CreateObject<UniformRandomVariable>();
    randCount->SetAttribute("Min", DoubleValue(3));
    randCount->SetAttribute("Max", DoubleValue(6));
    uint32_t fireCount = randCount->GetInteger();

    Ptr<ListPositionAllocator> firePA = CreateObject<ListPositionAllocator>();
    fire_st.positions.clear();
    fire_st.isActive.clear();

    for (uint32_t i = 0; i < fireCount; ++i)
    {
        double x = randX->GetValue();
        double y = randY->GetValue();
        Vector firePos(x, y, 0);

        firePA->Add(firePos);
        fire_st.positions.push_back(firePos);
        fire_st.isActive[i] = true;

        // Registra tiempo de detección
        FireExtinctionRecord record;
        record.position = firePos;
        record.detectionTime = Simulator::Now();
        extinctionRecords.push_back(record);

        NS_LOG_UNCOND("NUEVO INCENDIO " << i << " en posición: ("
                                        << x << ", " << y << ")");
    }

    fire_st.allocator = firePA;
}

std::vector<Vector> firesDistOrdered(Ptr<Node> node)
{
    std::vector<Vector> sortedFires = fire_st.positions; // crea copia para no alterar el original

    // Obtiene posición actual del nodo
    Ptr<WaypointMobilityModel> wp = node->GetObject<WaypointMobilityModel>();
    if (wp == nullptr)
    {
        NS_LOG_ERROR("WaypointMobilityModel no encontrado en el nodo " << node->GetId());
        return sortedFires; // Devuelve la copia sin ordenar si no hay modelo
    }
    Vector pos = wp->GetPosition();

    // Ordenamos por distancia ascendente utilizando lambda optimizado
    std::sort(sortedFires.begin(), sortedFires.end(),
              [&pos](const Vector &a, const Vector &b)
              {
                  double da = CalculateDistance(pos, a);
                  double db = CalculateDistance(pos, b);
                  return da < db;
              });
    return sortedFires; // devuelve el vector ordenado de incendios
}

/*------------------------------------------------
    Mobility
------------------------------------------------*/
void SetupMobility()
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
void SetUpMobilityCH()
{
    for (uint32_t i = 0; i < chNodes.GetN(); ++i)
    {
        Ptr<Node> node = chNodes.Get(i);
        NS_ABORT_MSG_IF(node == nullptr, "El nodo no puede ser nulo");
        roles[node] = LEADER; // Asigna el rol de líder al nodo

        double x, y, z;
        // Asignar posición inicial
        x = i * 10.0; // Espaciado entre nodos
        y = 0.0;
        z = 0.0;
        // AnimationInterface::SetConstantPosition(node, x, y, z);

        // 1. Crear el modelo y agregarlo al nodo
        Ptr<WaypointMobilityModel> wp = CreateObject<WaypointMobilityModel>();
        node->AggregateObject(wp);

        // 2. Waypoint inicial (despegue)
        Vector start(x, y, z); // Posición inicial del nodo líder
        Time t = Seconds(0.5); // Tiempo de inicio del waypoint
        wp->AddWaypoint(Waypoint(t, start));

        NS_LOG_UNCOND("Líder " << node->GetId() << " creado");
    }
}

void UpdateMobilityCH(double speed)
{
    NS_ABORT_MSG_IF(speed <= 0.0, "La velocidad debe ser positiva");

    for (uint32_t i = 0; i < chNodes.GetN(); ++i)
    {
        Ptr<Node> node = chNodes.Get(i);
        Ptr<WaypointMobilityModel> wp = node->GetObject<WaypointMobilityModel>();
        if (!wp)
            continue;

        Vector lastPos = wp->GetPosition();
        wp->EndMobility();

        // Busca solo incendios activos
        std::vector<Vector> activeFirePositions;
        for (uint32_t j = 0; j < fire_st.positions.size(); ++j)
        {
            if (fire_st.isActive[j])
            {
                activeFirePositions.push_back(fire_st.positions[j]);
            }
        }

        if (activeFirePositions.empty())
            continue;

        // Ordena incendios activos por distancia
        std::sort(activeFirePositions.begin(), activeFirePositions.end(),
                  [lastPos](const Vector &a, const Vector &b)
                  {
                      return CalculateDistance(lastPos, a) < CalculateDistance(lastPos, b);
                  });

        Vector targetFire = activeFirePositions[0];
        double d = CalculateDistance(lastPos, targetFire);
        double t = Simulator::Now().GetSeconds() + d / speed;

        wp->AddWaypoint(Waypoint(Seconds(t), targetFire));
    }
}

void PrintExtinctionReport()
{
    NS_LOG_UNCOND("\n===== REPORTE DE EXTINCIÓN DE INCENDIOS =====");
    NS_LOG_UNCOND("Total de incendios: " << extinctionRecords.size());

    double totalTime = 0.0;
    uint32_t extinguishedCount = 0;

    for (auto &record : extinctionRecords)
    {
        if (record.extinctionTime != Time(0))
        {
            double extinctionDuration = (record.extinctionTime - record.detectionTime).GetSeconds();
            totalTime += extinctionDuration;
            extinguishedCount++;

            NS_LOG_UNCOND("Incendio en (" << record.position.x << ", " << record.position.y
                                          << ") - Tiempo de extinción: " << extinctionDuration << "s"
                                          << " - Extinguido por nodo " << record.extinguishingNode);
        }
    }

    if (extinguishedCount > 0)
    {
        NS_LOG_UNCOND("Tiempo promedio de extinción: " << (totalTime / extinguishedCount) << "s");
    }

    NS_LOG_UNCOND("Incendios activos restantes: " << (extinctionRecords.size() - extinguishedCount));
    NS_LOG_UNCOND("========================================\n");
}

// Inicializa modelo de movilidad para los seguidores
void SetUpMobilityFollowers()
{
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::WaypointMobilityModel");
    mobility.Install(memberNodes);

    for (uint32_t i = 0; i < memberNodes.GetN(); ++i)
    {
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
        x = i * 1.0; // Espaciado entre nodos
        y = 4.0;
        z = 0.0;
        AnimationInterface::SetConstantPosition(node, x, y, z);
        Vector start(x, y, z); // Posición inicial del nodo líder
        Time t = Seconds(0.5); // Tiempo de inicio del waypoint
        wp->AddWaypoint(Waypoint(t, start));
    }
}

// Actualiza la movilidad de los seguidores en función del líder
void UpdateMobilityFollowers(int cluster_idx, double deltaTime)
{
    // 1. Obtiene el nodo líder y su modelo Waypoint
    Ptr<Node> leaderNode = chNodes.Get(cluster_idx);
    Ptr<WaypointMobilityModel> wpLeader = leaderNode->GetObject<WaypointMobilityModel>();
    if (!wpLeader)
    {
        NS_LOG_ERROR("Líder " << leaderNode->GetId() << " sin WaypointMobilityModel");
        return;
    }
    Vector leaderPos = wpLeader->GetPosition();

    // 2. Recorre el cluster y actualiza cada seguidor
    for (uint32_t j = 0; j < clusters[cluster_idx].GetN(); ++j)
    {
        Ptr<Node> follower = clusters[cluster_idx].Get(j);
        Ptr<WaypointMobilityModel> wpFollower = follower->GetObject<WaypointMobilityModel>();
        if (wpFollower == nullptr)
        {
            NS_LOG_ERROR("Seguidor " << follower->GetId() << " sin WaypointMobilityModel");
            continue; // Mejor omitir este nodo en lugar de abortar
        }

        // 3. Limpia waypoints previos
        wpFollower->EndMobility();

        // 4. Añade nuevo waypoint cerca del líder
        Vector offset = RandomOffset();                 // función que genera vector pequeño
        Time t = Simulator::Now() + Seconds(deltaTime); // Tiempo futuro para el waypoint
        wpFollower->AddWaypoint(Waypoint(t, leaderPos + offset));
    }
}

/*------------------------------------------------
    Reelección de líderes
------------------------------------------------*/
// TODO: Modificar para segpún parámetros del notion
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
    for (uint32_t i = 0; i < chNodes.GetN(); ++i)
    {
        Ptr<Node> node = chNodes.Get(i);
        allNodes.Add(node);
    }
    for (uint32_t i = 0; i < memberNodes.GetN(); ++i)
    {
        Ptr<Node> node = memberNodes.Get(i);
        allNodes.Add(node);
    }

    int N = allNodes.GetN();
    NS_ABORT_MSG_IF(N == 0, "No hay nodos para clustering");

    // Calcula pesos
    std::map<Ptr<Node>, double> weight;
    for (int i = 0; i < N; ++i)
    {
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
    while (!unassigned.empty())
    {
        // 5.1 Selecciona nodo con menor peso
        auto best_it = std::min_element(
            unassigned.begin(), unassigned.end(), // Ordena por peso
            [&weight](Ptr<Node> a, Ptr<Node> b)
            { return weight[a] < weight[b]; }); // Encuentra el nodo con menor peso

        Ptr<Node> ch = *best_it; // Nodo líder (Cluster Head) seleccionado
        roles[ch] = LEADER;      // Asigna el rol de líder al nodo

        // 5.2 Forma el cluster actual
        NodeContainer thisCluster;
        std::vector<Ptr<Node>> toAssign;

        for (auto node : unassigned)
        {
            if (node == ch)
                continue;
            // criterio simple: distancia ≤ 2·yourTxRange
            auto posCh = ch->GetObject<MobilityModel>()->GetPosition();
            auto posNode = node->GetObject<MobilityModel>()->GetPosition();
            double d = CalculateDistance(posCh, posNode);
            if (d <= 2 * TX_RANGE)
                toAssign.push_back(node); // Añade el nodo al vector de nodos a asignar
        }

        // 5.3 Asigna esos nodos
        for (auto node : toAssign)
        {
            roles[node] = FOLLOWER; // Asigna el rol de seguidor al nodo
            thisCluster.Add(node);  // Añade el nodo al cluster actual
            memberNodes.Add(node);  // Añade el nodo al contenedor de seguidores
            unassigned.erase(node); // Elimina el nodo de la lista de nodos no asignados
        }
        chNodes.Add(ch);      // Añade el líder al contenedor de líderes
        unassigned.erase(ch); // Elimina el líder del conjunto de nodos no asignados

        // 5.4 Guarda el cluster formado
        if ((int)clusters.size() < it + 1)
        {
            clusters.resize(it + 1); // Asegura que el vector de clusters tenga suficiente espacio
        }
        clusters[it] = thisCluster; // Asigna el cluster actual al vector de clusters
        it++;                       // Incrementa el contador de iteraciones
    }
    clusters.resize(it);

    // Muestra resultados
    for (size_t i = 0; i < chNodes.GetN(); ++i)
    {
        Ptr<Node> leader = chNodes.Get(i);
        NS_LOG_UNCOND("Cluster " << i << ": CH=" << leader->GetId());
        for (uint32_t j = 0; j < clusters[i].GetN(); ++j)
        {
            Ptr<Node> nm = clusters[i].Get(j);
            NS_LOG_UNCOND("  Miembro " << nm->GetId() << " (Peso: " << weight[nm] << ")");
        }
    }
}

/*------------------------------------------------
    Run Simulation
------------------------------------------------*/
void RunSimulation(double simTime)
{
    NS_LOG_UNCOND("Iniciando simulación con " << chNodes.GetN() << " líderes y "
                                              << memberNodes.GetN() << " seguidores.");

    if (true)
    {
        // Configuración de la animación
        NS_LOG_UNCOND("Configurando la interfaz de animación...");

        /*---------- Configuración Animación ----------------------*/
        AnimationInterface anim("umanet.xml"); // trazas de movimiento/paquetes
        // anim.SetBackgroundImage("mapa.png", 0, 0, 500.0, 500.0, 0.8);
        // anim.EnablePacketMetadata (true);              // dibujar flechas de paquetes
        // anim.SetMobilityPollInterval (Seconds (0.1));  // suavidad 10 Hz

        /*---------------- Eventos -------------------------------*/
        Simulator::Schedule(Seconds(UPDATE_TIME), // Programa la reelección de líderes después de 6 segundos
                            &UpdateMobilityCH, SPEED_CH);
        Simulator::Schedule(Seconds(UPDATE_TIME + 0.2), // Programa la movilidad de líderes después de 6 segundos
                            &UpdateMobilityCH, 10.0     // Velocidad de 10 m/s para los líderes
        );
        for (size_t i = 0; i < clusters.size(); ++i)
        {
            Simulator::Schedule(Seconds(FOLLOWERS_UPDATE_TIME), // Programa la movilidad de seguidores después de 6 segundos
                                &UpdateMobilityFollowers, i, FOLLOWERS_UPDATE_TIME);
        }

        for (uint32_t i = 0; i < chNodes.GetN(); ++i)
        {
            anim.UpdateNodeColor(chNodes.Get(i), 0, 0, 255);
        }
        for (uint32_t i = 0; i < memberNodes.GetN(); ++i)
        {
            anim.UpdateNodeColor(memberNodes.Get(i), 255, 0, 0);
        }
    }

    /*------------ Simulación ------------------------------*/
    Simulator::Stop(Seconds(simTime)); // Establece el tiempo de parada de la simulación
    Simulator::Run();                  // Ejecuta la simulación hasta que se alcance el tiempo de parada
    Simulator::Destroy();              // Libera los recursos utilizados por la simulación
    NS_LOG_UNCOND("Simulación finalizada. Recursos liberados.");
}

int main(int argc, char *argv[])
{
    LogComponentEnable("MiCmpt", LOG_LEVEL_ERROR);

    auto [nCH, nMem] = ParseCommandLine(argc, argv);

    CreateNodes(nCH, nMem);
    CreateClusters();

    SetupWifi();
    InstallInternet();
    AssignIpv4();

    CreateFireAllocator();

    SetUpMobilityCH();
    SetUpMobilityFollowers();

    // Inicia verificación periódica de extinción
    Simulator::Schedule(Seconds(1.0), &CheckFireExtinction);

    // Programa actualización periódica de incendios
    Simulator::Schedule(Seconds(30.0), &UpdateFires, 30.0);

    // Programa reporte final
    Simulator::Schedule(Seconds(119.0), &PrintExtinctionReport);

    UpdateMobilityCH(SPEED_CH);
    for (size_t i = 0; i < clusters.size(); ++i)
    {
        UpdateMobilityFollowers(i, FOLLOWERS_UPDATE_TIME);
    }

    RunSimulation(120);

    return 0;
}