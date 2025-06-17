#ifndef BOIDS_MOBILITY_MODEL_H
#define BOIDS_MOBILITY_MODEL_H

#include "mobility-model.h"

#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/nstime.h" // Añade esto al principio del archivo
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/vector.h"

#include <fstream>
#include <vector>

#include <map> // Añadir para std::map

namespace ns3
{

class BoidsMobilityModel : public MobilityModel
{
  public:
    static TypeId GetTypeId(void);
    static std::map<Vector, Time> s_fireStartTimes; // Tiempo de aparición de cada fuego
    static uint32_t s_totalFiresExtinguished;       // Total de fuegos extinguidos
    static Time s_totalExtinctionTime;              // Tiempo acumulado de extinción

    BoidsMobilityModel();
    virtual ~BoidsMobilityModel();
    void EvaluateLeadershipWithWCA(Ptr<const BoidsMobilityModel> otherLeader);
    // Parámetros configurables
    void SetSeparationRadius(double radius);
    void SetAlignmentRadius(double radius);
    void SetCohesionRadius(double radius);
    void SetLeaderInfluenceRadius(double radius);
    void SetMaxSpeed(double speed);
    void SetIsLeader(bool isLeader);
    void UpdateLeaderTarget();
    static void AssignFiresToLeaders(); 
    static void SetOutputFile(std::ofstream* outFile);
    static double CalculateWrappedDistance(const Vector& a, const Vector& b);
    static std::vector<Vector> getSpotsPoissonSpacial(
        uint32_t n, double areaX = 1000.0, double areaY = 1000.0,
        uint32_t k = 5, double desviacion = 10.0);
    Time GetFireInterval() const
    {
        return s_fireInterval;
    }

    void SetFireInterval(Time interval)
    {
        s_fireInterval = interval;
    }

    double GetFireRadius() const
    {
        return s_fireRadius;
    }

    void SetFireRadius(double radius)
    {
        s_fireRadius = radius;
    }

    // Métodos estáticos para manejar fuegos
    uint32_t GetClusterCount() const
    {
        return s_clusterCount;
    }

    void SetClusterCount(uint32_t count)
    {
        s_clusterCount = count;
    }

    double GetClusterDeviation() const
    {
        return s_clusterDeviation;
    }

    void SetClusterDeviation(double deviation)
    {
        s_clusterDeviation = deviation;
    }
    static void AddRandomFire();
    static void CheckFireProximity();
    // Método para generar fuegos usando Thomas cluster process
    static std::vector<Vector> GetSpotsPoissonSpacial(uint32_t n, double areaX = 1000.0, double areaY = 1000.0, uint32_t k = 5, double desviacion = 10.0);

    double CalculateWcaScore() const;

    Ptr<Node> FindNearestLeader() const;

    // Función para actualizar la pertenencia al cluster
    void UpdateClusterMembership();

    // Variable estática para acceder a los clusters
    static std::vector<NodeContainer>* s_clusters;
    static NodeContainer* s_chNodes;

  private:
    virtual Vector DoGetPosition(void) const;
    virtual void DoSetPosition(const Vector& position);
    virtual Vector DoGetVelocity(void) const;
    void Update(void);
    void DoInitialize(void);

    Ptr<Node> GetBoidsNode() const;

    static std::ofstream* s_outFile;

    // Parámetros del modelo Boids
    double m_separationRadius;
    double m_alignmentRadius;
    double m_cohesionRadius;
    double m_leaderInfluenceRadius;
    double m_maxSpeed;
    mutable bool m_isLeader;

    double m_energy;            // Energía residual (0.0 a 1.0)
    double m_degree;            // Grado de conectividad (número de vecinos)
    double m_distanceToTargets; // Distancia acumulada a objetivos
    double m_mobility;          // Medida de movilidad del nodo

    mutable Vector m_position;
    mutable Vector m_velocity;
    mutable Vector m_target; // Solo para líderes

    static std::vector<Vector> s_fires;
    static Ptr<UniformRandomVariable> s_fireRng;
    static Time s_fireInterval;
    static double s_fireRadius;
    // Parámetros para Thomas cluster process
    static uint32_t s_clusterCount;
    static double s_clusterDeviation;
    static std::vector<Vector> s_clusterCenters;
    void UpdateWcaMetrics();
    bool IsIsolated() const;
};

} // namespace ns3

#endif /* BOIDS_MOBILITY_MODEL_H */