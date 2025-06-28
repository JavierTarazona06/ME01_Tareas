#include <random>
#include "ns3/log.h"
#include "ns3/core-module.h"      // Time, StringValue, LogComponent, etc.
#include "ns3/network-module.h"   // Node, NodeContainer, NetDeviceContainer…
#include "ns3/wifi-module.h"      // WifiHelper, YansWifiPhyHelper, WifiMacHelper
#include "ns3/internet-module.h"   // InternetStackHelper, Ipv4*
#include "ns3/mobility-module.h"   // MobilityHelper
#include "ns3/animation-interface.h" // AnimationInterface
#include "ns3/netanim-module.h" // AnimationInterface
#include "ns3/rng-seed-manager.h"

using namespace ns3;                // Evita escribir ns3:: en cada referencia

NS_LOG_COMPONENT_DEFINE ("MiCmpt");

// Parámetros de la simulación

// Coeficientes para el cálculo de peso de acoplamiento
static const double w1 = 0.7; // calidad de la señal
static const double w2 = 0.2; // batería
static const double w3 = 0.05; // obstaculos (-)
static const double w4 = 0.025; // nodos conectados (-)
static const double w5 = 0.025; // distancia a foco incendio

static const double AREAX = 30;
static const double AREAY = 20;

static const double TX_RANGE = 3; // Rango de transmisión en metros (conexiones entre nodos directas))

static const uint32_t N_CH = 2; // Número de líderes (Cluster-Heads)
static const uint32_t N_MEM = 10; // Número de seguidores (Miembros)
static const double SPEED_CH = 1.5; // Velocidad de los líderes (Cluster-Heads) en m/s
static const double SPEED_FL = 1.0; // Velocidad de los líderes (Cluster-Heads) en m/s
static const double ELECT_UPDATE_TIME = 8.0; // Tiempo de actualización de la reelección de líderes (en segundos)
static const double LEADERS_UPDATE_TIME = 2.0; // Tiempo de actualización de los líderes (en segundos)
static const double FOLLOWERS_UPDATE_TIME = 0.4; // Tiempo de actualización de los seguidores (en segundos)
static const double SMALL_DIST = 1; // Distancia pequeña para el offset aleatorio de distancia entre líderes y seguidores
static const double EPSILON = 1e-3; // Epsilon para evitar problemas de tiempo negativo
static const double SIM_TIME = 50.0; // Tiempo total de la simulación en segundos
static const double DELTA_TIME = 0.1; // Tiempo entre movimientos
static const double NFIRES = 3; // Número de incendios
static const double NFIRESSPOTS = 3; // Número de focos de incendio
static const uint32_t FIRERESISTANCE = 250; // Resistencia del fuego
static const uint32_t SHOOTPOWER = 5; // Valor que le resta a la resistencia del fuego
static const uint32_t TXSHOOT = 1.5; // Rango de ataque

static const Vector LEADERSTARTPOS(10.0, 0, 0);
static const Vector FOLLOWERSTARTPOS(1.0, 2, 0);

enum Role { LEADER, FOLLOWER };

struct FireData
{
  Ptr<ListPositionAllocator> allocator;
  std::vector<Vector>        positions;
};

/*------------------- */

NodeContainer chNodes; // Contenedor para los nodos líderes (Cluster-Heads)
NodeContainer memberNodes; // Contenedor para los nodos seguidores (Miembros)
NodeContainer fireNodes; // Contenedor para los focos de incendio

std::vector<NodeContainer> clusters; // Vector para almacenar los clusters formados

NetDeviceContainer chIntf; // Contenedor para las interfaces de red de los nodos líderes
NetDeviceContainer memberIntf; // Contenedor para las interfaces de red de los nodos seguidores

FireData fire_st;  // Estructura para almacenar datos de incendios
std::map<Vector, Ptr<Node>> getFireNode;
std::map<Ptr<Node>, int> fireResistance;
std::queue<Ptr<Node>> firesKilled;


std::map<Ptr<Node>, Role> roles; // Mapa que asocia nodos a sus roles (líder o seguidor)
std::map<Ptr<Node>, std::queue<Vector>> destinationsQ; // Fila de destinos de nodo


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

/*------------------------------------------------
    Funciones estocásticas
------------------------------------------------*/

uint32_t RandomUniform(uint32_t minVal, uint32_t maxVal)
{
    std::uniform_int_distribution<uint32_t> dist(minVal, maxVal);
    static std::mt19937 rng(std::random_device{}());  // motor Mersenne Twister
    return dist(rng);
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


uint32_t IntNormalNum(
    uint32_t n, int32_t min=0, double mean=-1.0, double deviation = -1.0)
{
    // Valor medio en la mitad del rango
    if (mean <= 0.0){
        mean = static_cast<double>(n) / 2.0;
    }

    // Desviación por defecto: un sexto del rango (para que el 99.7% caiga dentro)
    if (deviation <= 0.0) {
        deviation = static_cast<double>(n) / 6.0;
    }

    Ptr<NormalRandomVariable> rv = CreateObject<NormalRandomVariable>();
    rv->SetAttribute("Mean", DoubleValue(mean));
    rv->SetAttribute("Variance", DoubleValue(deviation * deviation));

    // Truncar al rango [min, n]
    int valor = static_cast<int>(std::round(rv->GetValue()));
    valor = std::max(min, std::min(static_cast<int>(n), valor));

    return static_cast<uint32_t>(valor);
}


std::vector<Vector> getVectoresInAreaC(
    const std::vector<Vector>& vectIn, const Vector& center, 
    double tx){

    std::vector<Vector> inRange;
    double tx2 = tx * tx; // comparar con distancia² para evitar sqrt innecesario

    for (const auto& v : vectIn) {
        double dx = v.x - center.x;
        double dy = v.y - center.y;
        double dist2 = dx * dx + dy * dy;

        if (dist2 <= tx2) {
            inRange.push_back(v);
        }
    }

    return inRange;
}

/*------------------------------------------------
    2. Creación de nodos y Clusters
------------------------------------------------*/
void CreateNodes (uint32_t nClusterHeads, uint32_t nFollowers)
{
    // Crea dos contenedores: uno para líderes (Cluster-Heads) y otro para seguidores
    chNodes.Create (nClusterHeads);    // crea nClusterHeads instancias
    memberNodes.Create (nFollowers);   // crea nFollowers instancias
    fireNodes.Create(NFIRES);          // crea NFIRES instancias
    NS_LOG_UNCOND("Creando containers nodos: " << nClusterHeads << " líderes, " 
        << nFollowers << " seguidores\n " << NFIRES << " instancias de focos de incendio.");
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

// Poisson espacial uniforme > Thomas cluster process con distribución fija
std::vector<Vector> getSpotsPoissonSpacial(
    uint32_t n, double areaX=AREAX, double areaY=AREAY, 
    uint32_t k = 5, double desviacion = 10.0){
    // k es focos de incedio
    // La desviación de 10 permite un agrupamiento natural.
        // Los nodos suelen ubicarse a 2sigma del centro en una distribución normal bidimensional

    std::vector<Vector> focos;

    // 1. Generar k centros de clúster (uniformemente en el área)
    Ptr<UniformRandomVariable> ux = CreateObject<UniformRandomVariable>();
    Ptr<UniformRandomVariable> uy = CreateObject<UniformRandomVariable>();
    ux->SetAttribute("Min", DoubleValue(0.0));
    ux->SetAttribute("Max", DoubleValue(areaX));
    uy->SetAttribute("Min", DoubleValue(0.0));
    uy->SetAttribute("Max", DoubleValue(areaY));


    std::vector<Vector> centros;
    for (uint32_t i = 0; i < k; ++i) {
        centros.push_back(Vector(ux->GetValue(), uy->GetValue(), 0.0));
    }

    // 2. Repartir n focos en los clústeres
    uint32_t promedioPorCluster = n / k;

    for (uint32_t i = 0; i < k; ++i) {
        Ptr<NormalRandomVariable> dx = CreateObject<NormalRandomVariable>();
        Ptr<NormalRandomVariable> dy = CreateObject<NormalRandomVariable>();
        dx->SetAttribute("Mean", DoubleValue(0.0)); // Para que quede en el centro del Cluster
        dx->SetAttribute("Variance", DoubleValue(desviacion * desviacion));
        dy->SetAttribute("Mean", DoubleValue(0.0)); // Para que quede en el centro del Cluster
        dy->SetAttribute("Variance", DoubleValue(desviacion * desviacion));

        for (uint32_t j = 0; j < promedioPorCluster; ++j) {
            double x = centros[i].x + dx->GetValue(); // Por esto se marco promedio 0 antes
            double y = centros[i].y + dy->GetValue();

            // Recortar si se sale del área
            x = std::max(0.0, std::min(x, areaX));
            y = std::max(0.0, std::min(y, areaY));

            focos.push_back(Vector(x, y, 0.0));
        }
    }

    // 3. Si falta alguno por redondeo, añade desde clúster 0
    while (focos.size() < n) {
        Ptr<NormalRandomVariable> dx = CreateObject<NormalRandomVariable>();
        Ptr<NormalRandomVariable> dy = CreateObject<NormalRandomVariable>();
        dx->SetAttribute("Mean", DoubleValue(0.0));
        dx->SetAttribute("Variance", DoubleValue(desviacion * desviacion));
        dy->SetAttribute("Mean", DoubleValue(0.0));
        dy->SetAttribute("Variance", DoubleValue(desviacion * desviacion));

        double x = centros[0].x + dx->GetValue();
        double y = centros[0].y + dy->GetValue();
        x = std::max(0.0, std::min(x, areaX));
        y = std::max(0.0, std::min(y, areaY));
        focos.push_back(Vector(x, y, 0.0));
    }

    return focos;
}

void CreateFireAllocator ()
{
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(fireNodes);

    std::vector<Vector> kFires = getSpotsPoissonSpacial(
        NFIRES, AREAX, AREAY, NFIRESSPOTS, 10.0);

    // ListPositionAllocator es un contenedor de puntos, se reutiliza al crear waypoints.
    Ptr<ListPositionAllocator> firePA = CreateObject<ListPositionAllocator> ();

    uint32_t n_min = FIRERESISTANCE * 0.6; // Valor para limitar el rango de resistencia
    double mean = 
        ((FIRERESISTANCE - n_min) / 2) + n_min; // Centrado en la mitad
    double deviation = (FIRERESISTANCE - n_min) / 6.0;

    for (uint32_t i = 0; i < fireNodes.GetN(); ++i){
        Ptr<Node> fire = fireNodes.Get(i);
        NS_ABORT_MSG_IF(fire == nullptr, "El nodo fuego no puede ser nulo");

        Ptr<ConstantPositionMobilityModel> model = 
            fire->GetObject<ConstantPositionMobilityModel>();
        NS_ABORT_MSG_IF(model == nullptr, 
            "ConstantPositionMobilityModel no encontrado en el nodo fuego " << fire->GetId());

        model->SetPosition(kFires[i]);

        getFireNode[kFires[i]] = fire;
        fireResistance[fire] = IntNormalNum(
            FIRERESISTANCE, n_min, mean, deviation);

        firePA->Add (kFires[i]);
    }    

    fire_st = FireData(); // Inicializa la estructura de datos de incendios
    fire_st.allocator = firePA; // Asigna el ListPositionAllocator a la estructura de datos
    fire_st.positions = kFires; // Asigna la lista de posiciones de incendios a la estructura de datos
}

void updateFire(){
    /* Si el nodo de fuego ya no tiene resistencia, se borra de
        fire_st positiom, de getFireNode, fireResistance y se añade a la cola de 
        firesKilled.
    */
    for (uint32_t i = 0; i < fireNodes.GetN(); ++i){
        Ptr<Node> fire = fireNodes.Get(i);
        NS_ABORT_MSG_IF(fire == nullptr, "El nodo fuego no puede ser nulo");

        Ptr<ConstantPositionMobilityModel> model = 
            fire->GetObject<ConstantPositionMobilityModel>();
        NS_ABORT_MSG_IF(model == nullptr, 
            "ConstantPositionMobilityModel no encontrado en el nodo fuego " << fire->GetId());

        if (fireResistance[fire] < 0){
            firesKilled.push(fire);
            
            Vector pos = model->GetPosition();

            auto it = getFireNode.find(pos);
            if (it != getFireNode.end()) {
                getFireNode.erase(it);
            } else {
                NS_LOG_UNCOND("No hay nodo en esa posición para eliminar.");
            }

            auto it2 = std::find(fire_st.positions.begin(), fire_st.positions.end(), pos);
            if (it2 != fire_st.positions.end()) {
                fire_st.positions.erase(it2);
            }
        }
    }

    Simulator::Schedule(Seconds (DELTA_TIME), // Programa la actualización de los focos de incendio
        &updateFire);
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

void moveNode2PosLeader(
    Ptr<Node> node, Vector toPos, 
    double speed, double interval=DELTA_TIME
){
    Ptr<ConstantPositionMobilityModel> model = node->GetObject<ConstantPositionMobilityModel>();
    Vector pos = model->GetPosition();
    Vector delta = toPos - pos;

    // Normaliza para avanzar x por paso
    double distancia = std::hypot(delta.x, delta.y);
    double step = speed * interval; // Avance en metros por intervalo de tiempo

    if (distancia > 0.1) {
        delta.x = step * delta.x / distancia;
        delta.y = step * delta.y / distancia;

        pos.x += delta.x;
        pos.y += delta.y;

        model->SetPosition(pos);

        std::vector<Vector> firesIn = getVectoresInAreaC(fire_st.positions, pos, TXSHOOT);
        for (const Vector& firePos : firesIn) {
            auto it = getFireNode.find(firePos);
            NS_ABORT_MSG_IF(it == getFireNode.end(), "No existe un nodo de fuego en la posición dada.");
            Ptr<Node> fire = it->second;
            fireResistance[fire] -= SHOOTPOWER;
        }
        
    } else {
        NS_LOG_UNCOND("Nodo " << node->GetId() << " ha alcanzado la posición objetivo: " << pos);

        auto it = getFireNode.find(toPos);
        if (it == getFireNode.end()){ // El nodo fire ya no existe
            destinationsQ[node].pop(); // Elimina la posición actual de la cola
            return;
        }
        
        Ptr<Node> fire = it->second;
        if (fireResistance[fire] < 0){
            destinationsQ[node].pop(); // Elimina la posición actual de la cola
        } else {
            fireResistance[fire] -= SHOOTPOWER;
        }
    }    
}

void moveNode2PosFollower(Ptr<Node> node, Vector toPos, double speed, double interval=DELTA_TIME){
    Ptr<ConstantPositionMobilityModel> model = node->GetObject<ConstantPositionMobilityModel>();
    Vector pos = model->GetPosition();
    Vector delta = toPos - pos;

    // Normaliza para avanzar x por paso
    double distancia = std::hypot(delta.x, delta.y);
    double step = speed * interval; // Avance en metros por intervalo de tiempo

    if (distancia > 0.1) {
        delta.x = step * delta.x / distancia;
        delta.y = step * delta.y / distancia;

        pos.x += delta.x;
        pos.y += delta.y;

        model->SetPosition(pos);        
    } else {
        NS_LOG_UNCOND("Nodo " << node->GetId() << " ha alcanzado la posición objetivo: " << pos);
        destinationsQ[node].pop();
    }   

    std::vector<Vector> firesIn = getVectoresInAreaC(fire_st.positions, pos, TXSHOOT);
    for (const Vector& firePos : firesIn) {
        auto it = getFireNode.find(firePos);
        NS_ABORT_MSG_IF(it == getFireNode.end(), "No existe un nodo de fuego en la posición dada.");
        Ptr<Node> fire = it->second;
        fireResistance[fire] -= SHOOTPOWER;
    }
}


void moveNode(Ptr<Node> node, double interval=DELTA_TIME){
    Role nodeRole = roles[node]; // Obtiene el rol del nodo (líder o seguidor)
    double speed = (nodeRole == LEADER) ? SPEED_CH : SPEED_FL; // Define la velocidad según el rol
    auto& nodeQueue = destinationsQ[node];

    if (nodeQueue.empty()){
        NS_LOG_UNCOND("Nodo " << node->GetId() << " no tiene más trayectorias");
    } else {
        Vector toPos = nodeQueue.front();
        if (nodeRole == LEADER){
            moveNode2PosLeader(node, toPos, speed, interval);
        } else if (nodeRole == FOLLOWER){
            moveNode2PosFollower(node, toPos, speed, interval);
        }
    }

    Simulator::Schedule(Seconds(interval), &moveNode, node, DELTA_TIME);
}

void setUpMobilityCH(){
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(chNodes);

    for (uint32_t i = 0; i < chNodes.GetN(); ++i) {
        Ptr<Node> node = chNodes.Get(i);
        NS_ABORT_MSG_IF(node == nullptr, "El nodo líder no puede ser nulo");

        Ptr<ConstantPositionMobilityModel> cpm = node->GetObject<ConstantPositionMobilityModel>();
        NS_ABORT_MSG_IF(cpm == nullptr, "ConstantPositionMobilityModel no encontrado en el nodo líder " << node->GetId());

        roles[node] = LEADER; // Asigna el rol de líder al nodo
        destinationsQ[node] = std::queue<Vector>(); // Inicializa la cola de movimiento del líder

        //Vector startPos(i * 10.0, 0, 0); // Posición inicial del líder
        Vector startPos = LEADERSTARTPOS;
        startPos.x = (double) i * startPos.x;
        cpm->SetPosition(startPos); // Establece la posición inicial del líder

        std::vector<Vector> firesSorted = firesDistOrdered(startPos); // Ordena los incendios por distancia al líder
        for (int j=0; j < (int)firesSorted.size(); ++j) {
            destinationsQ[node].push(firesSorted[j]); // Añade la posición inicial a la cola de movimiento del líder
        }

        NS_LOG_UNCOND("Líder " << node->GetId() << " creado en posición " << startPos);

        Simulator::Schedule(Seconds(DELTA_TIME), &moveNode, node, DELTA_TIME);
    }
}

void updateMobilityCH(){
    NS_LOG_UNCOND("Actualizando movilidad de líderes...");
    for (uint32_t i = 0; i < chNodes.GetN(); ++i) {
        Ptr<Node> node = chNodes.Get(i);
        NS_ABORT_MSG_IF(node == nullptr, "El nodo líder no puede ser nulo");

        Ptr<ConstantPositionMobilityModel> cpm = node->GetObject<ConstantPositionMobilityModel>();
        NS_ABORT_MSG_IF(cpm == nullptr, "ConstantPositionMobilityModel no encontrado en el nodo líder " << node->GetId());

        Vector startPos = cpm->GetPosition();

        destinationsQ[node] = std::queue<Vector>(); // Limpia la cola de movimiento del nodo
        
        std::vector<Vector> firesSorted = firesDistOrdered(startPos); // Ordena los incendios por distancia al líder
        for (int j=0; j < (int)firesSorted.size(); ++j) {
            destinationsQ[node].push(firesSorted[j]); // Añade la posición inicial a la cola de movimiento del líder
        }
    }

    Simulator::Schedule(Seconds(LEADERS_UPDATE_TIME), &updateMobilityCH);
}

void setUpMobilityFollower(){
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(memberNodes);

    int all_idx = 0;
    for (int cluster_idx = 0; cluster_idx < (int)clusters.size(); ++cluster_idx) {
        Ptr<Node> leader = chNodes.Get(cluster_idx);
        if (leader == nullptr) {
            NS_LOG_ERROR("Líder no encontrado para el índice de cluster: " << cluster_idx);
            return; // Abortamos si no hay líder
        }
        Ptr<ConstantPositionMobilityModel> cpLeader = leader->GetObject<ConstantPositionMobilityModel>();
        if (!cpLeader) {
            NS_LOG_ERROR("Líder " << leader->GetId() << " sin ConstantPositionMobilityModel");
            return;
        }
        Vector leaderPos = cpLeader->GetPosition();

        for (uint32_t j = 0; j < clusters[cluster_idx].GetN(); ++j, ++all_idx) {
            Ptr<Node> follower = clusters[cluster_idx].Get(j);
            if (follower == nullptr) {
                NS_LOG_ERROR("Seguidor no encontrado para el índice de cluster: " << cluster_idx);
                return; // Abortamos si no hay líder
            }

            Ptr<ConstantPositionMobilityModel> cpm = follower->GetObject<ConstantPositionMobilityModel>();
            if (cpm == nullptr) {
                NS_LOG_ERROR("Seguidor " << follower->GetId() << " sin ConstantPositionMobilityMode");
                continue;  // Mejor omitir este nodo en lugar de abortar
            }

            roles[follower] = FOLLOWER; // Asigna el rol de líder al nodo
            destinationsQ[follower] = std::queue<Vector>(); // Inicializa la cola de movimiento del líder

            Vector startPos = FOLLOWERSTARTPOS;
            startPos.x = (double)all_idx *  startPos.x;
            cpm->SetPosition(startPos); // Establece la posición inicial del seguidor

            Vector offset = RandomOffset();  // función que genera vector pequeño
            Vector toPos = leaderPos + offset;
            destinationsQ[follower].push(toPos);

            NS_LOG_UNCOND("Seguidor " << follower->GetId() << " creado en posición " << startPos);

            Simulator::Schedule(Seconds(DELTA_TIME), &moveNode, follower, DELTA_TIME);
        }
    }
}


void updateMobilityFollowersCl(int cluster_idx){
    Ptr<Node> leader = chNodes.Get(cluster_idx);
    if (leader == nullptr) {
        NS_LOG_ERROR("Líder no encontrado para el índice de cluster: " << cluster_idx);
        return; // Abortamos si no hay líder
    }
    Ptr<ConstantPositionMobilityModel> cpLeader = leader->GetObject<ConstantPositionMobilityModel>();
    if (!cpLeader) {
        NS_LOG_ERROR("Líder " << leader->GetId() << " sin ConstantPositionMobilityModel");
        return;
    }
    Vector leaderPos = cpLeader->GetPosition();

    for (uint32_t j = 0; j < clusters[cluster_idx].GetN(); ++j) {
        Ptr<Node> follower = clusters[cluster_idx].Get(j);
        if (follower == nullptr) {
            NS_LOG_ERROR("Seguidor no encontrado para el índice de cluster: " << cluster_idx);
            return; // Abortamos si no hay líder
        }

        destinationsQ[follower] = std::queue<Vector>(); // Limpia la cola de movimiento del nodo

        Vector offset = RandomOffset();  // función que genera vector pequeño
        Vector toPos = leaderPos + offset;
        destinationsQ[follower].push(toPos);
    }
}

void updateMobilityFollowers(){
    for (int cluster_idx = 0; cluster_idx < (int)clusters.size(); ++cluster_idx) {
        NS_LOG_UNCOND("Actualizando movilidad de seguidores en el cluster " << cluster_idx << "Time: " << Simulator::Now().GetSeconds());
        updateMobilityFollowersCl(cluster_idx); // Actualiza la movilidad de los seguidores en el cluster
    }

    Simulator::Schedule(Seconds (FOLLOWERS_UPDATE_TIME), // Programa la movilidad de seguidores después de x segundos
        &updateMobilityFollowers); // Re-ejecuta la actualización de movilidad de seguidores
}


/*------------------------------------------------
    Reelección de líderes
------------------------------------------------*/
 
double ComputeNodeWeight(Ptr<Node> node)
{
  double channelQ = node->GetId() % 5 + 1;
  double energy = node->GetId() % 6 + 1;
  double obstacles = 0;
  double connectivity = 0;
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
    Animation
------------------------------------------------*/

void prepareAnimation(AnimationInterface& anim){
    NS_LOG_UNCOND("Configurando la interfaz de animación...");

    // anim.SetBackgroundImage("mapa.png", 0, 0, 500.0, 500.0, 0.8);
    anim.EnablePacketMetadata(true);
    anim.SetMobilityPollInterval(Seconds(0.1));

    for (uint32_t i = 0; i < chNodes.GetN(); ++i) {
        anim.UpdateNodeColor(chNodes.Get(i), 0, 0, 255); // Blue leaders
    }
    for (uint32_t i = 0; i < memberNodes.GetN(); ++i) {
        anim.UpdateNodeColor(memberNodes.Get(i), 255, 0, 0); // Red Followers
    }
    for (uint32_t i = 0; i < fireNodes.GetN(); ++i) {
        anim.UpdateNodeColor(fireNodes.Get(i), 255, 165, 0); // Orange Fires
    }
}

void changeNodesColor(AnimationInterface& anim) {
    for (uint32_t i = 0; i < chNodes.GetN(); ++i) {
        anim.UpdateNodeColor(chNodes.Get(i), 0, 0, 255); // Blue leaders
    }
    for (uint32_t i = 0; i < memberNodes.GetN(); ++i) {
        anim.UpdateNodeColor(memberNodes.Get(i), 255, 0, 0);// Red Followers
    }
    while(!firesKilled.empty()){
        Ptr<Node> fire = firesKilled.front();
        anim.UpdateNodeColor(fire, 255, 255, 255); // Black fires, killed
        firesKilled.pop();
    }
    NS_LOG_UNCOND("Colores de nodos actualizados");
    Simulator::Schedule(Seconds (DELTA_TIME), // Programa la actualización de los focos de incendio
        &changeNodesColor, std::ref(anim));
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
    //RandomUniform(0, 9999999)
    ns3::RngSeedManager::SetSeed(
        3  
    );    // Ejemplo: semilla = 3
    ns3::RngSeedManager::SetRun(
        7
    );     // y corrida = 7

    LogComponentEnable ("MiCmpt", LOG_LEVEL_ERROR);

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
    Simulator::Schedule(Seconds (DELTA_TIME), // Programa la actualización de los focos de incendio
        &updateFire);

    /*------------------------------------------------
        Set Up Mobility e inicia movimiento
    ------------------------------------------------*/
    setUpMobilityCH();
    setUpMobilityFollower();


    /*------------------------------------------------
        Actualizar modelo de movilidad de lider y nodo
    ------------------------------------------------*/
    Simulator::Schedule(Seconds (LEADERS_UPDATE_TIME), // Programa la actualización de movilidad de líderes después de 6 segundos
        &updateMobilityCH);
    Simulator::Schedule(Seconds (FOLLOWERS_UPDATE_TIME), // Programa la movilidad de seguidores después de x segundos
        &updateMobilityFollowers);


    /*------------------------------------------------
        Reelección de líderes
    ------------------------------------------------*/
    Simulator::Schedule(Seconds (ELECT_UPDATE_TIME), // Programa la reelección de líderes
        &RunWCAClustering);

    /*------------------------------------------------
        Animación
    ------------------------------------------------*/
    AnimationInterface anim("umanet.xml"); // Crea una interfaz de animación para visualizar la simulación
    prepareAnimation(anim); // Configura la interfaz de animación    
    changeNodesColor(anim); // Cambia el color de los nodos en la animación

    /*------------------------------------------------
        Run Simulation
    ------------------------------------------------*/
    RunSimulation (SIM_TIME);

    return 0;
}