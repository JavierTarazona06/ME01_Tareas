# Boids Mobility Model Simulation

## Overview

This repository implements a **Boids-based mobility model** for simulating the movement and coordination of nodes (agents) in a wireless ad hoc network, using the ns-3 network simulator. The simulation models both **leader** and **follower** nodes, their clustering, wireless communication, and their collective response to dynamic events (such as the appearance and extinguishing of "fires") in a 2D environment.

The project is designed for research and educational purposes, particularly in the study of **self-organizing networks**, **clustering algorithms**, and **bio-inspired mobility models**.

---

## Key Features

- **Boids Model**: Implements the classic Boids rules (separation, alignment, cohesion) for node movement, with extensions for leader-follower dynamics.
- **Clustering**: Nodes are organized into clusters, each with a leader (Cluster-Head) and several followers.
- **Dynamic Leadership**: Leadership can change dynamically based on node metrics (energy, connectivity, proximity to targets, mobility) using a Weighted Clustering Algorithm (WCA).
- **Wireless Communication**: Nodes are equipped with Wi-Fi interfaces and communicate in ad hoc mode.
- **Event Simulation**: Random "fires" appear in the environment, and leaders coordinate the response to extinguish them.
- **Metrics Logging**: The simulation logs node positions, leadership status, and fire events to CSV files for later analysis and visualization.
- **Visualization**: A Python script (`simulate/showNodes.py`) animates the simulation results, showing node movement, leadership zones, and fire locations.

---

## Main Components

### 1. **boids.cc** (Simulation Entry Point)

- **Node Creation**: Initializes leader and follower nodes, assigns them to clusters.
- **Wi-Fi Setup**: Configures ad hoc Wi-Fi communication for all nodes.
- **Mobility Assignment**: Applies the BoidsMobilityModel to all nodes, with random initial positions.
- **Cluster Distribution**: Distributes followers among leaders, forming clusters.
- **Event Scheduling**: Fires are generated at random intervals and locations using a spatial Poisson process.
- **Simulation Loop**: Runs the simulation, updating node positions, leadership, and fire response.
- **Metrics Output**: Writes node positions and fire events to `boids_positions.csv`, and summary metrics to `boids_summary.csv`.

### 2. **boids-mobility-model.h / .cc** (Boids Mobility Model)

- **Boids Rules**: Implements separation, alignment, and cohesion for followers; leaders are attracted to fire locations.
- **Leadership Evaluation**: Uses a Weighted Clustering Algorithm (WCA) to dynamically assign or revoke leadership based on:
  - Residual energy
  - Node connectivity (number of neighbors)
  - Proximity to fires (targets)
  - Node mobility (stability)
- **Fire Handling**: Fires are generated and assigned to leaders, who coordinate their extinguishing.
- **Metrics Calculation**: Tracks and logs metrics such as the number of fires extinguished and average extinction time.

### 3. **simulate/showNodes.py** (Visualization)

- **Reads**: `boids_positions.csv` generated by the simulation.
- **Animates**: Node movement, leadership zones, and fire events over time.
- **Interactive Controls**: Toggle visibility of fires and leader zones.

---

## How It Works

1. **Initialization**: The simulation starts by creating a specified number of leader and follower nodes, assigning them to clusters.
2. **Mobility Model**: Each node is assigned the BoidsMobilityModel, which governs its movement based on local rules and leadership status.
3. **Wireless Setup**: Nodes are configured to communicate over Wi-Fi in ad hoc mode, enabling direct peer-to-peer communication.
4. **Fire Events**: Fires appear at random locations and times. Leaders detect and move towards fires to extinguish them, coordinating their followers.
5. **Dynamic Leadership**: If a leader becomes isolated or another node has a better WCA score, leadership can change dynamically.
6. **Logging and Visualization**: All relevant events and positions are logged for post-simulation analysis and visualization.

---

## Output Files

- **boids_positions.csv**: Contains time-stamped positions of all nodes, their leadership status, and fire events.
- **boids_summary.csv**: Summarizes key metrics such as the total number of fires extinguished and average extinction time.

---

## Visualization

To visualize the simulation:

```bash
cd simulate
python showNodes.py
```

This will animate the movement of nodes, display leader zones, and show fire events as they occur.

---

## Use Cases

- **Research in Mobile Ad Hoc Networks (MANETs)**
- **Study of Bio-inspired Algorithms**
- **Clustering and Leadership Dynamics**
- **Disaster Response and Swarm Robotics Simulations**

---

## Requirements

- **ns-3** (for running the simulation)
- **Python 3** with `pandas`, `matplotlib`, and `numpy` (for visualization)

---

## File Structure

- `boids.cc` — Main simulation script
- `boids-mobility-model.h/.cc` — Boids mobility model implementation
- `simulate/showNodes.py` — Visualization script
- `simulate/boids_positions.csv` — Output data (generated)
- `simulate/boids_summary.csv` — Output summary (generated)
- `CMakeLists.txt` — Build configuration

---

## References

- Reynolds, C. W. (1987). "Flocks, herds and schools: A distributed behavioral model." *ACM SIGGRAPH Computer Graphics*.
- Chatterjee, M., Das, S. K., & Turgut, D. (2002). "WCA: A Weighted Clustering Algorithm for Mobile Ad Hoc Networks." *Cluster Computing*. 