#pragma once
#include "Graph.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

/**
 * @file PathFinder.h
 * @brief Shortest-path navigation routing engine implementing Dijkstra and A*.
 * 
 * WHAT: Computes optimal routes from entrance/vehicle position to target parking slots.
 * WHY:  Fulfills the core algorithmic requirement of the Smart Parking project.
 * HOW:  Uses min-priority queue (std::priority_queue) Dijkstra/A* search, reconstructs
 *       waypoints, computes total metric distance, turn counts, and estimated traversal times.
 */
namespace SmartParking {

struct Route {
    bool isValid = false;
    float totalDistance = 0.0f; // meters
    float estimatedTimeSec = 0.0f; // seconds
    int turnCount = 0;
    std::vector<int> nodeIndices;
    std::vector<glm::vec3> waypoints;
    std::vector<std::string> stepInstructions;

    std::string getFormattedTime() const;
    void clear();
};

class PathFinder {
public:
    PathFinder() = default;
    explicit PathFinder(const Graph* graph);

    void setGraph(const Graph* graph) { m_graph = graph; }

    // Pathfinding algorithms
    Route findShortestPathDijkstra(int startNodeIndex, int targetNodeIndex) const;
    Route findShortestPathAStar(int startNodeIndex, int targetNodeIndex) const;

    // Helper functions for slot navigation
    Route calculateRouteToSlot(const std::string& startNodeId, const std::string& targetSlotId, bool useAStar = false) const;
    Route calculateRouteFromPosition(const glm::vec3& currentPosition, const std::string& targetSlotId, bool useAStar = false) const;

private:
    const Graph* m_graph = nullptr;

    Route buildRoute(const std::vector<int>& pathIndices) const;
    int countTurns(const std::vector<glm::vec3>& points) const;
    std::vector<std::string> generateTurnInstructions(const std::vector<int>& pathIndices) const;
};

} // namespace SmartParking
