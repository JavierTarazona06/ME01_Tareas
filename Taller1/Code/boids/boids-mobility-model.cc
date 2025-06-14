#include "boids-mobility-model.h"
#include "ns3/ptr.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/vector.h"
#include <fstream>

NS_LOG_COMPONENT_DEFINE("BoidsMobilityModel");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(BoidsMobilityModel);

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
                          MakeDoubleAccessor(&BoidsMobilityModel::SetSeparationRadius),
                          MakeDoubleChecker<double>())
            .AddAttribute("AlignmentRadius",
                          "Radio para alineación con vecinos.",
                          DoubleValue(50.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::SetAlignmentRadius),
                          MakeDoubleChecker<double>())
            .AddAttribute("CohesionRadius",
                          "Radio para cohesión con vecinos.",
                          DoubleValue(50.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::SetCohesionRadius),
                          MakeDoubleChecker<double>())
            .AddAttribute("LeaderInfluenceRadius",
                          "Radio de influencia de los líderes.",
                          DoubleValue(100.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::SetLeaderInfluenceRadius),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxSpeed",
                          "Velocidad máxima del boid.",
                          DoubleValue(5.0),
                          MakeDoubleAccessor(&BoidsMobilityModel::SetMaxSpeed),
                          MakeDoubleChecker<double>())
            .AddAttribute("IsLeader",
                          "Si el nodo es un líder.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BoidsMobilityModel::SetIsLeader),
                          MakeBooleanChecker());
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

// Variable estática para el archivo de salida
std::ofstream* BoidsMobilityModel::s_outFile = nullptr;

void
BoidsMobilityModel::SetOutputFile(std::ofstream* outFile)
{
    s_outFile = outFile;
}

Ptr<Node>
BoidsMobilityModel::GetBoidsNode() const
{
    // Usamos GetObject<Node>() en lugar de GetNode()
    return GetObject<Node>();
}

void
BoidsMobilityModel::DoInitialize(void)
{
    MobilityModel::DoInitialize();
    Update();
}

void
BoidsMobilityModel::Update(void) const
{
    // Obtener todos los nodos con movilidad Boids
    NodeList::Iterator listEnd = NodeList::End();
    Vector separation(0, 0, 0);
    Vector alignment(0, 0, 0);
    Vector cohesion(0, 0, 0);
    Vector leaderAttraction(0, 0, 0);

    int neighbors = 0;
    int leaderNeighbors = 0;

    if (m_isLeader)
    {
        // Comportamiento del líder
        Vector direction = m_target - m_position;
        double distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (distance < 10.0) // Si está cerca del objetivo, elegir uno nuevo
        {
            Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
            m_target.x = uv->GetValue(0, 1000);
            m_target.y = uv->GetValue(0, 1000);
            direction = m_target - m_position;
            distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        }

        if (distance > 0)
        {
            direction.x /= distance;
            direction.y /= distance;
            m_velocity.x += direction.x * 0.1;
            m_velocity.y += direction.y * 0.1;
        }
    }
    else
    {
        // Comportamiento de los seguidores (Boids clásico + influencia de líderes)
        for (NodeList::Iterator i = NodeList::Begin(); i != listEnd; ++i)
        {
            Ptr<Node> node = *i;
            Ptr<BoidsMobilityModel> other = node->GetObject<BoidsMobilityModel>();

            if (other == this || !other)
            {
                continue;
            }

            Vector otherPos = other->DoGetPosition();
            Vector diff = m_position - otherPos;
            double distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            // Separación
            if (distance < m_separationRadius && distance > 0)
            {
                separation.x += diff.x / distance;
                separation.y += diff.y / distance;
            }

            // Alineación y cohesión
            if (distance < m_alignmentRadius)
            {
                alignment.x += other->m_velocity.x;
                alignment.y += other->m_velocity.y;
                cohesion.x += otherPos.x;
                cohesion.y += otherPos.y;
                neighbors++;
            }

            // Influencia del líder
            if (other->m_isLeader && distance < m_leaderInfluenceRadius)
            {
                leaderAttraction.x += other->m_velocity.x;
                leaderAttraction.y += other->m_velocity.y;
                leaderNeighbors++;
            }
        }

        // Aplicar reglas
        if (neighbors > 0)
        {
            // Alineación
            alignment.x /= neighbors;
            alignment.y /= neighbors;
            m_velocity.x += (alignment.x - m_velocity.x) * 0.1;
            m_velocity.y += (alignment.y - m_velocity.y) * 0.1;

            // Cohesión
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

        // Separación
        m_velocity.x += separation.x * 0.1;
        m_velocity.y += separation.y * 0.1;

        // Influencia del líder
        if (leaderNeighbors > 0)
        {
            leaderAttraction.x /= leaderNeighbors;
            leaderAttraction.y /= leaderNeighbors;
            m_velocity.x += (leaderAttraction.x - m_velocity.x) * 0.15;
            m_velocity.y += (leaderAttraction.y - m_velocity.y) * 0.15;
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

    // Después de actualizar la posición y velocidad
    if (s_outFile && s_outFile->good())
    { // Cambiamos is_open() por good()
        Vector position = DoGetPosition();

        Ptr<Node> node = GetBoidsNode(); // Usamos nuestro nuevo método

        *s_outFile << Simulator::Now().GetSeconds() << "," << node->GetId()
                   << "," // Ahora funciona correctamente
                   << position.x << "," << position.y << "," << (m_isLeader ? 1 : 0) << "\n";
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
BoidsMobilityModel::SetIsLeader(bool isLeader)
{
    m_isLeader = isLeader;
    if (m_isLeader)
    {
        Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
        m_target.x = uv->GetValue(0, 1000);
        m_target.y = uv->GetValue(0, 1000);
    }
}

} // namespace ns3