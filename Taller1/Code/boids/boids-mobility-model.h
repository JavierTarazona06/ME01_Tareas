#ifndef BOIDS_MOBILITY_MODEL_H
#define BOIDS_MOBILITY_MODEL_H

#include "mobility-model.h"
#include "ns3/vector.h"
#include <fstream>
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/node.h"

namespace ns3 {

class BoidsMobilityModel : public MobilityModel
{
public:
    static TypeId GetTypeId (void);
    BoidsMobilityModel();
    virtual ~BoidsMobilityModel();

    // Parámetros configurables
    void SetSeparationRadius (double radius);
    void SetAlignmentRadius (double radius);
    void SetCohesionRadius (double radius);
    void SetLeaderInfluenceRadius (double radius);
    void SetMaxSpeed (double speed);
    void SetIsLeader (bool isLeader);
    static void SetOutputFile(std::ofstream* outFile);

private:
    virtual Vector DoGetPosition (void) const;
    virtual void DoSetPosition (const Vector &position);
    virtual Vector DoGetVelocity (void) const;
    void Update (void) const;
    void DoInitialize (void);

    Ptr<Node> GetBoidsNode() const;

    static std::ofstream* s_outFile;

    // Parámetros del modelo Boids
    double m_separationRadius;
    double m_alignmentRadius;
    double m_cohesionRadius;
    double m_leaderInfluenceRadius;
    double m_maxSpeed;
    bool m_isLeader;

    mutable Vector m_position;
    mutable Vector m_velocity;
    mutable Vector m_target; // Solo para líderes
};

} // namespace ns3

#endif /* BOIDS_MOBILITY_MODEL_H */