#include "boids-mobility-model.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/vector.h"

#include <fstream>
#include <vector>

#include <map> // Añadir para std::map

NS_LOG_COMPONENT_DEFINE("BoidsMobilityModel");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(BoidsMobilityModel);

// Inicialización de variables estáticas
std::vector<Vector> BoidsMobilityModel::s_fires;
Ptr<UniformRandomVariable> BoidsMobilityModel::s_fireRng = CreateObject<UniformRandomVariable>();
Time BoidsMobilityModel::s_fireInterval = Seconds(8);
double BoidsMobilityModel::s_fireRadius = 30.0;

// Variables estáticas de metras
std::map<Vector, Time> BoidsMobilityModel::s_fireStartTimes;
uint32_t BoidsMobilityModel::s_totalFiresExtinguished = 0;
Time BoidsMobilityModel::s_totalExtinctionTime = Seconds(0);

std::vector<NodeContainer>* BoidsMobilityModel::s_clusters = nullptr;
NodeContainer* BoidsMobilityModel::s_chNodes = nullptr;

// Variable estática para el archivo de salida
std::ofstream* BoidsMobilityModel::s_outFile = nullptr;

std::vector<Vector> BoidsMobilityModel::getSpotsPoissonSpacial(
    uint32_t n, double areaX, double areaY, 
    uint32_t k, double desviacion)
{
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
        dx->SetAttribute("Mean", DoubleValue(0.0)); // Centrado en el cluster
        dx->SetAttribute("Variance", DoubleValue(desviacion * desviacion));
        dy->SetAttribute("Mean", DoubleValue(0.0));
        dy->SetAttribute("Variance", DoubleValue(desviacion * desviacion));
        
        for (uint32_t j = 0; j < promedioPorCluster; ++j) {
            double x = centros[i].x + dx->GetValue();
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

TypeId
BoidsMobilityModel::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::BoidsMobilityModel")
            .SetParent<MobilityModel>()
            .SetGroupName("Mobility")
            .AddConstructor<BoidsMobilityModel>()
            .AddAttribute("SeparationRadius",
                          "Radio de separación entre boids.",
                          DoubleValue(25.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::m_separationRadius),
                          MakeDoubleChecker<double>())
            .AddAttribute("AlignmentRadius",
                          "Radio para alineación con vecinos.",
                          DoubleValue(50.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::m_alignmentRadius),
                          MakeDoubleChecker<double>())
            .AddAttribute("CohesionRadius",
                          "Radio para cohesión con vecinos.",
                          DoubleValue(50.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::m_cohesionRadius),
                          MakeDoubleChecker<double>())
            .AddAttribute("LeaderInfluenceRadius",
                          "Radio de influencia de los líderes.",
                          DoubleValue(100.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::m_leaderInfluenceRadius),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxSpeed",
                          "Velocidad máxima del boid.",
                          DoubleValue(5.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::m_maxSpeed),
                          MakeDoubleChecker<double>())
            .AddAttribute("IsLeader",
                          "Si el nodo es un líder.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BoidsMobilityModel::m_isLeader),
                          MakeBooleanChecker())
            .AddAttribute("FireInterval",
                          "Intervalo entre aparición de nuevos fuegos.",
                          TimeValue(Seconds(10)),
                          MakeTimeAccessor(&BoidsMobilityModel::GetFireInterval,
                                           &BoidsMobilityModel::SetFireInterval),
                          MakeTimeChecker())
            .AddAttribute("FireRadius",
                          "Radio de detección de fuego por los líderes.",
                          DoubleValue(30.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::GetFireRadius,
                                             &BoidsMobilityModel::SetFireRadius),
                          MakeDoubleChecker<double>());
    return tid;
}

BoidsMobilityModel::BoidsMobilityModel()
    : m_separationRadius(25.0),
      m_alignmentRadius(50.0),
      m_cohesionRadius(50.0),
      m_leaderInfluenceRadius(100.0),
      m_maxSpeed(5.0),
      m_isLeader(false)
{
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    m_velocity.x = uv->GetValue(-1, 1);
    m_velocity.y = uv->GetValue(-1, 1);
    m_velocity.z = 0;

    if (m_isLeader)
    {
        m_target.x = uv->GetValue(0, 1000);
        m_target.y = uv->GetValue(0, 1000);
        m_target.z = 0;
    }
}

BoidsMobilityModel::~BoidsMobilityModel()
{
}

void
BoidsMobilityModel::SetOutputFile(std::ofstream* outFile)
{
    s_outFile = outFile;
}

Ptr<Node>
BoidsMobilityModel::FindNearestLeader() const
{
    Ptr<Node> nearestLeader = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    if (!s_chNodes)
        return nullptr;

    for (uint32_t i = 0; i < s_chNodes->GetN(); ++i)
    {
        Ptr<Node> leader = s_chNodes->Get(i);
        Ptr<MobilityModel> leaderMobility = leader->GetObject<MobilityModel>();

        if (leaderMobility)
        {
            double distance = CalculateDistance(m_position, leaderMobility->GetPosition());
            if (distance < m_leaderInfluenceRadius && distance < minDistance)
            {
                minDistance = distance;
                nearestLeader = leader;
            }
        }
    }
    return nearestLeader;
}

void
BoidsMobilityModel::UpdateClusterMembership()
{
    if (!s_clusters || !s_chNodes || m_isLeader)
        return;

    Ptr<Node> currentNode = GetBoidsNode();
    Ptr<Node> nearestLeader = FindNearestLeader();

    // Buscar y eliminar el nodo de cualquier cluster actual
    for (auto& cluster : *s_clusters)
    {
        for (uint32_t i = 0; i < cluster.GetN(); ++i)
        {
            if (cluster.Get(i) == currentNode)
            {
                NodeContainer newCluster;
                for (uint32_t i = 0; i < cluster.GetN(); ++i)
                {
                    if (cluster.Get(i) != currentNode)
                    {
                        newCluster.Add(cluster.Get(i));
                    }
                }
                cluster = newCluster; // Reemplazar el cluster con la nueva versión
                break;
            }
        }
    }

    // Añadir al cluster del líder más cercano si existe
    if (nearestLeader)
    {
        for (uint32_t i = 0; i < s_chNodes->GetN(); ++i)
        {
            if (s_chNodes->Get(i) == nearestLeader)
            {
                (*s_clusters)[i].Add(currentNode);
                break;
            }
        }
    }
}

Ptr<Node>
BoidsMobilityModel::GetBoidsNode() const
{
    // Usamos GetObject<Node>() en lugar de GetNode()
    return GetObject<Node>();
}

void
BoidsMobilityModel::AddRandomFire()
{
    // Generar entre 1 y 3 fuegos cada vez
    Ptr<UniformRandomVariable> countVar = CreateObject<UniformRandomVariable>();
    int fireCount = countVar->GetInteger(1, 3);
    
    // Generar los fuegos con distribución de cluster Thomas
    std::vector<Vector> newFires = getSpotsPoissonSpacial(
        fireCount, 1000.0, 1000.0, // Área de 1000x1000
        3, // 3 clusters
        50.0); // Desviación estándar de 50 metros
    
    for (const auto& fire : newFires) {
        s_fires.push_back(fire);
        // Registrar el tiempo de aparición del fuego
        s_fireStartTimes[fire] = Simulator::Now();
        NS_LOG_UNCOND("Nuevo fuego aparecido en: " << fire.x << ", " << fire.y);
    }
    
    // Programar próximo fuego
    Simulator::Schedule(s_fireInterval, &BoidsMobilityModel::AddRandomFire);
}

// Actualiza las métricas para WCA
void
BoidsMobilityModel::UpdateWcaMetrics()
{
    // 1. Actualizar energía (modelo de consumo simplificado)
    Ptr<ExponentialRandomVariable> energyConsumption = CreateObject<ExponentialRandomVariable>();
    energyConsumption->SetAttribute("Mean", DoubleValue(0.005));
    m_energy = std::max(0.0, m_energy - energyConsumption->GetValue());

    // 2. Calcular grado de conectividad (número de vecinos en rango)
    m_degree = 0.0;
    double totalDistance = 0.0;

    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<MobilityModel> otherMobility = node->GetObject<MobilityModel>();

        if (otherMobility && otherMobility != this)
        {
            double distance = CalculateDistance(GetPosition(), otherMobility->GetPosition());
            if (distance < m_leaderInfluenceRadius)
            {
                m_degree += 1.0;
                totalDistance += distance;
            }
        }
    }

    // 3. Calcular distancia a objetivos (fuegos)
    m_distanceToTargets = 0.0;
    if (!s_fires.empty())
    {
        for (const auto& fire : s_fires)
        {
            m_distanceToTargets += CalculateDistance(GetPosition(), fire);
        }
        m_distanceToTargets /= s_fires.size(); // Distancia promedio
    }

    // 4. Calcular movilidad (cambio de posición respecto al último paso)
    static Vector lastPosition;
    if (Simulator::Now().GetSeconds() > 0.1)
    { // Esperar un pequeño intervalo
        double distanceMoved = CalculateDistance(GetPosition(), lastPosition);
        m_mobility = distanceMoved / 0.1; // Velocidad instantánea
    }
    lastPosition = GetPosition();
}

// Cálculo del peso WCA
double
BoidsMobilityModel::CalculateWcaScore() const
{
    // Normalizar parámetros
    double normEnergy = m_energy; // Ya está en [0,1]

    // Normalizar grado (asumiendo máximo teórico de 10 vecinos)
    double normDegree = std::min(m_degree / 10.0, 1.0);

    // Normalizar distancia a objetivos (inversa, considerando radio de 200m)
    double normTargetDistance = 1.0 - std::min(m_distanceToTargets / 200.0, 1.0);

    // Normalizar movilidad (asumiendo velocidad máxima de 10 m/s)
    double normMobility = 1.0 - std::min(m_mobility / 10.0, 1.0);

    // Pesos configurables (suman 1.0)
    const double w1 = 0.4; // Energía
    const double w2 = 0.3; // Grado de conectividad
    const double w3 = 0.2; // Proximidad a objetivos
    const double w4 = 0.1; // Estabilidad (baja movilidad)

    // Fórmula WCA
    double wcaScore =
        (w1 * normEnergy) + (w2 * normDegree) + (w3 * normTargetDistance) + (w4 * normMobility);

    return std::max(0.0, std::min(1.0, wcaScore));
}

/*void
BoidsMobilityModel::CheckFireProximity()
{
    for (auto it = s_fires.begin(); it != s_fires.end();)
    {
        bool fireExtinguished = false;

        // Verificar si algún líder está cerca del fuego
        for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
        {
            Ptr<Node> node = *i;
            Ptr<BoidsMobilityModel> mob = node->GetObject<BoidsMobilityModel>();

            if (mob && mob->m_isLeader)
            {
                Vector leaderPos = mob->DoGetPosition();
                Vector diff = *it - leaderPos;
                double distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                if (distance < s_fireRadius)
                {
                    fireExtinguished = true;
                    SetIsLeader(false);
                    break;
                }
            }
        }

        if (fireExtinguished)
        {
            NS_LOG_UNCOND("Fuego extinguido en: " << it->x << ", " << it->y);
            it = s_fires.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Programar próxima verificación
    Simulator::Schedule(Seconds(1), &BoidsMobilityModel::CheckFireProximity);
}*/

void
BoidsMobilityModel::CheckFireProximity()
{
    for (auto it = s_fires.begin(); it != s_fires.end();)
    {
        bool fireExtinguished = false;
        Ptr<BoidsMobilityModel> extinguishingLeader = nullptr;

        // Verificar si algún líder está cerca del fuego
        for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
        {
            Ptr<Node> node = *i;
            Ptr<BoidsMobilityModel> mob = node->GetObject<BoidsMobilityModel>();

            if (mob && mob->m_isLeader)
            {
                Vector leaderPos = mob->DoGetPosition();
                Vector diff = *it - leaderPos;
                double distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                if (distance < s_fireRadius)
                {
                    fireExtinguished = true;

                    // Actualiza métricas SOLO UNA VEZ
                    auto fireTimeIt = s_fireStartTimes.find(*it);
                    if (fireTimeIt != s_fireStartTimes.end())
                    {
                        Time extinctionTime = Simulator::Now() - fireTimeIt->second;
                        s_totalExtinctionTime += extinctionTime;
                        s_totalFiresExtinguished += 1;
                        s_fireStartTimes.erase(fireTimeIt);
                    }

                    NS_LOG_UNCOND("Fuego extinguido en: " << it->x << ", " << it->y);
                    // Si quieres que el líder deje de ser líder:
                    mob->SetIsLeader(false);

                    // Elimina el fuego y sale del ciclo de líderes
                    it = s_fires.erase(it);
                    break; // Sale del ciclo de líderes, pasa al siguiente fuego
                }
            }


        }

        if (!fireExtinguished)
        {
            ++it;
        }
    }

    // Programar próxima verificación
    Simulator::Schedule(Seconds(1), &BoidsMobilityModel::CheckFireProximity);
}

void
BoidsMobilityModel::DoInitialize(void)
{
    MobilityModel::DoInitialize();
    Update();
}

// Cambia la implementación para que sea const-correct:
double
BoidsMobilityModel::CalculateWrappedDistance(const Vector& a, const Vector& b) 
{
    const double mapWidth = 1000.0; // Ajusta según tu tamaño de mapa
    const double mapHeight = 1000.0;

    double dx = std::abs(a.x - b.x);
    double dy = std::abs(a.y - b.y);

    // Aplicar wrapping
    if (dx > mapWidth / 2)
    {
        dx = mapWidth - dx;
    }
    if (dy > mapHeight / 2)
    {
        dy = mapHeight - dy;
    }

    return std::sqrt(dx * dx + dy * dy);
}

/*bool
BoidsMobilityModel::IsIsolated() const
{
    if (m_isLeader)
        return false;
    // return true;
    bool hasLeaderInRange = false;

    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<BoidsMobilityModel> other = node->GetObject<BoidsMobilityModel>();

        if (other && other != this && other->m_isLeader)
        {
            double distance = CalculateDistance(m_position, other->DoGetPosition());
            if (distance <
                m_leaderInfluenceRadius * 2.5) // Aumentamos un 50% el radio de verificación
            {
                NS_LOG_DEBUG("Node " << GetBoidsNode()->GetId() << " detectó líder "
                                     << node->GetId() << " a distancia " << distance);
                hasLeaderInRange = true;
                break; // No necesitamos seguir buscando
            }
        }
    }

    NS_LOG_DEBUG("Node " << GetBoidsNode()->GetId()
                         << (hasLeaderInRange ? " tiene líder en rango" : " está aislado"));
    return !hasLeaderInRange;
}*/

void
BoidsMobilityModel::EvaluateLeadershipWithWCA(Ptr<const BoidsMobilityModel> otherLeader)
{
    // Solo ejecutar esta evaluación para nodos que son líderes
    if (!m_isLeader || !otherLeader->m_isLeader)
        return;

    // Calcular distancia entre los líderes (con wrapping)
    double distance = CalculateWrappedDistance(m_position, otherLeader->DoGetPosition());

    // Solo evaluar si están dentro del radio de influencia de líderes
    if (distance > m_leaderInfluenceRadius)
        return;

    // Actualizar métricas WCA para ambos líderes
    const_cast<BoidsMobilityModel*>(this)->UpdateWcaMetrics();
    const_cast<BoidsMobilityModel*>(PeekPointer(otherLeader))->UpdateWcaMetrics();

    // Obtener puntuaciones WCA
    double myScore = CalculateWcaScore();
    double otherScore = otherLeader->CalculateWcaScore();

    NS_LOG_UNCOND("Evaluación de liderazgo entre "
                  << GetBoidsNode()->GetId() << " (score: " << myScore << ") y "
                  << otherLeader->GetBoidsNode()->GetId() << " (score: " << otherScore << ")");

    // Comparar puntuaciones WCA
    if (myScore < otherScore)
    {
        // Este líder tiene peor puntuación, dejar de ser líder
        SetIsLeader(false);
        NS_LOG_UNCOND("Nodo " << GetBoidsNode()->GetId()
                              << " deja de ser líder. Mejor líder encontrado: "
                              << otherLeader->GetBoidsNode()->GetId());

        // Actualizar clusters
        UpdateClusterMembership();
    }
    else if (myScore > otherScore)
    {
        // El otro líder tiene peor puntuación, sugerir que deje de ser líder
        NS_LOG_UNCOND("Nodo " << otherLeader->GetBoidsNode()->GetId()
                              << " debería evaluar dejar de ser líder (mejor líder: "
                              << GetBoidsNode()->GetId() << ")");
    }
    // Si las puntuaciones son iguales, no hacer cambios
}

bool
BoidsMobilityModel::IsIsolated() const
{
    if (m_isLeader)
    {
        NS_LOG_DEBUG("Node " << GetBoidsNode()->GetId() << " es líder, no puede estar aislado");
        return false;
    }

    bool hasLeaderInRange = false;
    const double effectiveRadius = m_leaderInfluenceRadius * 1.2;

    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<const BoidsMobilityModel> other = node->GetObject<BoidsMobilityModel>();

        if (other && other != this && other->m_isLeader)
        {
            double distance = CalculateWrappedDistance(m_position, other->DoGetPosition());
            if (distance < effectiveRadius)
            {
                hasLeaderInRange = true;
                break;
            }
        }
    }

    return !hasLeaderInRange;
}

void
BoidsMobilityModel::Update(void)
{
    const_cast<BoidsMobilityModel*>(this)->UpdateWcaMetrics();

    // Calcular puntuación WCA
    double wcaScore = CalculateWcaScore();
    NS_LOG_UNCOND("Node " << GetBoidsNode()->GetId() << " WCA Score: " << wcaScore
                          << " (E: " << m_energy << ", D: " << m_degree
                          << ", T: " << m_distanceToTargets << ", M: " << m_mobility << ")");

    // Comportamiento basado en WCA (ejemplo: líderes con mejor puntuación)
    bool leaderStatusChanged = false;
    // Lógica de autopromoción cuando está aislado
    NS_LOG_UNCOND("Node " << GetBoidsNode()->GetId() << " es lider " << m_isLeader << " isolado"
                          << IsIsolated() << " score " << wcaScore);
    // Se autoproclama lider al no tener un lider cercano                          
    if (!m_isLeader && IsIsolated() && wcaScore > 0.0)
    {
        NS_LOG_UNCOND("Node se vuelve lider" << GetBoidsNode()->GetId());
        SetIsLeader(true);
        leaderStatusChanged = true;
        // Comportamiento inicial como nuevo líder
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
        m_target.x = uv->GetValue(0, 1000);
        m_target.y = uv->GetValue(0, 1000);
        // Nodo con buena puntuación podría convertirse en líder
        m_velocity.x *= 1.05;
        m_velocity.y *= 1.05;
    }
    //evalua sí tiene lideres cerca y en caso de tener un WCA score menor deja de ser lider
    if (m_isLeader)
    {
        for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
        {
            Ptr<Node> node = *i;
            Ptr<BoidsMobilityModel> other = node->GetObject<BoidsMobilityModel>();

            if (other && other != this && other->m_isLeader)
            {
                EvaluateLeadershipWithWCA(other);
                break; // Solo evaluar con un líder a la vez
            }
        }
    }

    if (m_isLeader && CalculateWcaScore() < 0.5)
    {
        // Este líder ya no es adecuado
        SetIsLeader(false);
        leaderStatusChanged = true;
    }
    if (!m_isLeader && CalculateWcaScore() > 0.8)
    {
        // Este nodo es buen candidato a líder
        SetIsLeader(true);
        leaderStatusChanged = true;
    }

    if (leaderStatusChanged || !m_isLeader)
    {
        UpdateClusterMembership();
    }
    // NS_LOG_UNCOND("entra update");
    //  Obtener todos los nodos con movilidad Boids
    // NodeList::Iterator listEnd = NodeList::End();
    Vector separation(0, 0, 0);
    Vector alignment(0, 0, 0);
    Vector cohesion(0, 0, 0);
    Vector leaderAttraction(0, 0, 0);

    int neighbors = 0;
    int leaderNeighbors = 0;

    if (m_isLeader)
    {
        // Comportamiento del líder: buscar fuegos más cercanos
        Vector closestFire;
        double minDistance = std::numeric_limits<double>::max();
        bool fireFound = false;

        for (const auto& fire : s_fires)
        {
            Vector diff = fire - m_position;
            double distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (distance < minDistance)
            {
                minDistance = distance;
                closestFire = fire;
                fireFound = true;
            }
        }

        if (fireFound)
        {
            // Moverse hacia el fuego más cercano
            Vector direction = closestFire - m_position;
            double distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            if (distance > 0)
            {
                direction.x /= distance;
                direction.y /= distance;
                m_velocity.x +=
                    direction.x * 1.5; // Mayor influencia que el comportamiento aleatorio
                m_velocity.y += direction.y * 1.5;
            }
        }
        else
        {
            // Comportamiento aleatorio si no hay fuegos
            Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
            double distance = std::sqrt((m_target.x - m_position.x) * (m_target.x - m_position.x) +
                                        (m_target.y - m_position.y) * (m_target.y - m_position.y));

            if (distance < 10.0)
            {
                m_target.x = uv->GetValue(0, 1000);
                m_target.y = uv->GetValue(0, 1000);
            }

            Vector direction = m_target - m_position;
            distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            if (distance > 0)
            {
                direction.x /= distance;
                direction.y /= distance;
                m_velocity.x += direction.x * 0.1;
                m_velocity.y += direction.y * 0.1;
            }
        }
    }
    else
    {
        // COMPORTAMIENTO DE LOS SEGUIDORES (NUEVO)
        for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
        {
            Ptr<Node> node = *i;
            Ptr<BoidsMobilityModel> other = node->GetObject<BoidsMobilityModel>();

            if (other == this || !other)
                continue;

            Vector otherPos = other->DoGetPosition();
            Vector diff = m_position - otherPos;
            double distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            // 1. Separación: evitar colisiones con vecinos cercanos
            if (distance < m_separationRadius && distance > 0)
            {
                separation.x += diff.x / distance;
                separation.y += diff.y / distance;
            }

            // 2. Alineación: ajustar velocidad a la de vecinos cercanos
            if (distance < m_alignmentRadius)
            {
                alignment.x += other->m_velocity.x;
                alignment.y += other->m_velocity.y;
                neighbors++;
            }

            // 3. Cohesión: moverse hacia el centro de masa del grupo
            if (distance < m_cohesionRadius)
            {
                cohesion.x += otherPos.x;
                cohesion.y += otherPos.y;
            }

            // 4. Atracción a líderes: seguir a los líderes dentro del radio de influencia
            if (other->m_isLeader && distance < m_leaderInfluenceRadius)
            {
                leaderAttraction.x += other->m_velocity.x;
                leaderAttraction.y += other->m_velocity.y;
                leaderNeighbors++;
            }
        }

        // Aplicar las reglas con diferentes pesos
        if (neighbors > 0)
        {
            // Normalizar alineación
            alignment.x /= neighbors;
            alignment.y /= neighbors;
            m_velocity.x += (alignment.x - m_velocity.x) * 0.1;
            m_velocity.y += (alignment.y - m_velocity.y) * 0.1;

            // Normalizar cohesión
            cohesion.x /= neighbors;
            cohesion.y /= neighbors;
            Vector cohesionForce = cohesion - m_position;
            double distance =
                std::sqrt(cohesionForce.x * cohesionForce.x + cohesionForce.y * cohesionForce.y);
            if (distance > 0)
            {
                cohesionForce.x /= distance;
                cohesionForce.y /= distance;
                m_velocity.x += cohesionForce.x * 0.05;
                m_velocity.y += cohesionForce.y * 0.05;
            }
        }

        // Aplicar separación
        m_velocity.x += separation.x * 0.15;
        m_velocity.y += separation.y * 0.15;

        // Aplicar atracción a líderes
        if (leaderNeighbors > 0)
        {
            leaderAttraction.x /= leaderNeighbors;
            leaderAttraction.y /= leaderNeighbors;
            m_velocity.x += (leaderAttraction.x - m_velocity.x) * 0.2;
            m_velocity.y += (leaderAttraction.y - m_velocity.y) * 0.2;
        }
    }

    // Limitar velocidad
    double speed = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.y * m_velocity.y);
    if (speed > m_maxSpeed)
    {
        m_velocity.x = (m_velocity.x / speed) * m_maxSpeed;
        m_velocity.y = (m_velocity.y / speed) * m_maxSpeed;
    }

    // Actualizar posición
    m_position.x += m_velocity.x;
    m_position.y += m_velocity.y;

    // Mantener dentro de límites (opcional)
    m_position.x = std::fmod(m_position.x + 1000, 1000);
    m_position.y = std::fmod(m_position.y + 1000, 1000);

    /*NS_LOG_UNCOND("ejecucion: " << Simulator::Now().GetSeconds() << "," << 0 // node->GetId()
                                << "," // Ahora funciona correctamente
                                << m_position.x << "," << m_position.y << ","
                                << (m_isLeader ? 1 : 0) << m_maxSpeed << "\n");*/

    // Después de actualizar la posición y velocidad
    if (s_outFile && s_outFile->good())
    { // Cambiamos is_open() por good()
        Vector position = DoGetPosition();

        Ptr<Node> node = GetBoidsNode(); // Usamos nuestro nuevo método

        *s_outFile << Simulator::Now().GetSeconds() << "," << node->GetId()
                   << "," // Ahora funciona correctamente
                   << position.x << "," << position.y << "," << (m_isLeader ? 1 : 0) << ",0\n";
        /*NS_LOG_UNCOND("ejecucion: " << Simulator::Now().GetSeconds() << "," << node->GetId()
                                    << "," // Ahora funciona correctamente
                                    << position.x << "," << position.y << ","
                                    << (m_isLeader ? 1 : 0) << "\n");*/
        // Si es líder y hay fuegos, escribir también los fuegos
        if (m_isLeader && !s_fires.empty())
        {
            for (const auto& fire : s_fires)
            {
                *s_outFile << Simulator::Now().GetSeconds() << "," << -1
                           << "," // ID negativo para fuegos
                           << fire.x << "," << fire.y << ","
                           << "0,1\n"; // El último 1 indica que es un fuego
            }
        }
    }

    // Programar próximo update
    Simulator::Schedule(MilliSeconds(100), &BoidsMobilityModel::Update, this);

    // Notificar cambio de posición
    NotifyCourseChange();
}

Vector
BoidsMobilityModel::DoGetPosition(void) const
{
    return m_position;
}

void
BoidsMobilityModel::DoSetPosition(const Vector& position)
{
    m_position = position;
}

Vector
BoidsMobilityModel::DoGetVelocity(void) const
{
    return m_velocity;
}

void
BoidsMobilityModel::SetSeparationRadius(double radius)
{
    m_separationRadius = radius;
}

void
BoidsMobilityModel::SetAlignmentRadius(double radius)
{
    m_alignmentRadius = radius;
}

void
BoidsMobilityModel::SetCohesionRadius(double radius)
{
    m_cohesionRadius = radius;
}

void
BoidsMobilityModel::SetLeaderInfluenceRadius(double radius)
{
    m_leaderInfluenceRadius = radius;
}

void
BoidsMobilityModel::SetMaxSpeed(double speed)
{
    m_maxSpeed = speed;
}

void
BoidsMobilityModel::AssignFiresToLeaders()
{
    // 1. Recolecta todos los líderes activos
    std::vector<Ptr<BoidsMobilityModel>> leaders;
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<BoidsMobilityModel> mob = node->GetObject<BoidsMobilityModel>();
        if (mob && mob->m_isLeader)
        {
            leaders.push_back(mob);
        }
    }

    // 2. Lleva registro de fuegos ya asignados
    std::vector<Vector> assignedFires;

    // 3. Para cada líder, asigna el fuego más cercano no asignado
    for (auto& leader : leaders)
    {
        if (!s_fires.empty())
        {
            Vector myPos = leader->DoGetPosition();
            double minDist = std::numeric_limits<double>::max();
            Vector closestFire;
            bool found = false;
            for (const auto& fire : s_fires)
            {
                // Si ya fue asignado, ignóralo
                if (std::find(assignedFires.begin(), assignedFires.end(), fire) != assignedFires.end())
                    continue;

                double dist = CalculateWrappedDistance(myPos, fire);
                if (dist < minDist)
                {
                    minDist = dist;
                    closestFire = fire;
                    found = true;
                }
            }
            if (found)
            {
                leader->m_target = closestFire;
                assignedFires.push_back(closestFire);
            }
            else
            {
                // Si todos los fuegos ya están asignados, elige el más cercano (puede repetirse)
                minDist = std::numeric_limits<double>::max();
                for (const auto& fire : s_fires)
                {
                    double dist = CalculateWrappedDistance(myPos, fire);
                    if (dist < minDist)
                    {
                        minDist = dist;
                        closestFire = fire;
                    }
                }
                leader->m_target = closestFire;
            }
        }
    }

    // Programa la próxima actualización periódica
    Simulator::Schedule(Seconds(1), &BoidsMobilityModel::AssignFiresToLeaders);
}

void
BoidsMobilityModel::UpdateLeaderTarget()
{
    if (m_isLeader)
    {
        if (!s_fires.empty())
        {
            Vector myPos = DoGetPosition();
            double minDist = std::numeric_limits<double>::max();
            Vector closestFire = s_fires[0];
            for (const auto& fire : s_fires)
            {
                double dist = CalculateWrappedDistance(myPos, fire);
                if (dist < minDist)
                {
                    minDist = dist;
                    closestFire = fire;
                }
            }
            m_target = closestFire;
        }
        // Si no hay fuegos, puedes mantener el target actual o asignar uno aleatorio si lo prefieres
    }
    Simulator::Schedule(Seconds(1), &BoidsMobilityModel::UpdateLeaderTarget, this);
}

void
BoidsMobilityModel::SetIsLeader(bool isLeader)
{
    m_isLeader = isLeader;
    if (m_isLeader)
    {
        // Inicializa el target al fuego más cercano (o aleatorio si no hay fuegos)
        if (!s_fires.empty())
        {
            Vector myPos = DoGetPosition();
            double minDist = std::numeric_limits<double>::max();
            Vector closestFire = s_fires[0];
            for (const auto& fire : s_fires)
            {
                double dist = CalculateWrappedDistance(myPos, fire);
                if (dist < minDist)
                {
                    minDist = dist;
                    closestFire = fire;
                }
            }
            m_target = closestFire;
        }
        else
        {
            Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
            m_target.x = uv->GetValue(0, 1000);
            m_target.y = uv->GetValue(0, 1000);
        }
    }
}

} // namespace ns3