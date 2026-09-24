#include "PathFinder.h"
#include <queue>
#include <limits>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace SmartParking {

std::string Route::getFormattedTime() const {
    if (!isValid || estimatedTimeSec <= 0.0f) {
        return "0s";
    }
    int totalSec = static_cast<int>(std::round(estimatedTimeSec));
    int minutes = totalSec / 60;
    int seconds = totalSec % 60;

    std::ostringstream oss;
    if (minutes > 0) {
        oss << minutes << " min " << seconds << " sec";
    } else {
        oss << seconds << " sec";
    }
    return oss.str();
}

void Route::clear() {
    isValid = false;
    totalDistance = 0.0f;
    estimatedTimeSec = 0.0f;
    turnCount = 0;
    nodeIndices.clear();
    waypoints.clear();
    stepInstructions.clear();
}

PathFinder::PathFinder(const Graph* graph)
    : m_graph(graph) {
}

Route PathFinder::findShortestPathDijkstra(int startNodeIndex, int targetNodeIndex) const {
    Route route;
    if (!m_graph) return route;

    int nodeCount = static_cast<int>(m_graph->getNodeCount());
    if (startNodeIndex < 0 || startNodeIndex >= nodeCount ||
        targetNodeIndex < 0 || targetNodeIndex >= nodeCount) {
        return route;
    }

    if (startNodeIndex == targetNodeIndex) {
        route.isValid = true;
        route.nodeIndices.push_back(startNodeIndex);
        route.waypoints.push_back(m_graph->getNode(startNodeIndex)->position);
        route.stepInstructions.push_back("Already at destination: " + m_graph->getNode(startNodeIndex)->name);
        return route;
    }

    const float INF = std::numeric_limits<float>::infinity();
    std::vector<float> dist(nodeCount, INF);
    std::vector<int> prev(nodeCount, -1);
    std::vector<bool> visited(nodeCount, false);

    // Min-heap storing pair<distance, nodeIndex>
    using QueueElement = std::pair<float, int>;
    std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> pq;

    dist[startNodeIndex] = 0.0f;
    pq.push({0.0f, startNodeIndex});

    while (!pq.empty()) {
        auto [currentDist, u] = pq.top();
        pq.pop();

        if (visited[u]) continue;
        visited[u] = true;

        if (u == targetNodeIndex) {
            break; // Found shortest path to target
        }

        const auto& edges = m_graph->getOutgoingEdges(u);
        for (const auto& edge : edges) {
            int v = edge.toIndex;
            float newDist = currentDist + edge.cost;

            if (newDist < dist[v]) {
                dist[v] = newDist;
                prev[v] = u;
                pq.push({newDist, v});
            }
        }
    }

    // Check if target was reached
    if (dist[targetNodeIndex] == INF) {
        return route; // No path found
    }

    // Reconstruct path
    std::vector<int> path;
    for (int at = targetNodeIndex; at != -1; at = prev[at]) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());

    return buildRoute(path);
}

Route PathFinder::findShortestPathAStar(int startNodeIndex, int targetNodeIndex) const {
    Route route;
    if (!m_graph) return route;

    int nodeCount = static_cast<int>(m_graph->getNodeCount());
    if (startNodeIndex < 0 || startNodeIndex >= nodeCount ||
        targetNodeIndex < 0 || targetNodeIndex >= nodeCount) {
        return route;
    }

    if (startNodeIndex == targetNodeIndex) {
        return findShortestPathDijkstra(startNodeIndex, targetNodeIndex);
    }

    const glm::vec3& targetPos = m_graph->getNode(targetNodeIndex)->position;
    auto heuristic = [&](int idx) -> float {
        return glm::distance(m_graph->getNode(idx)->position, targetPos);
    };

    const float INF = std::numeric_limits<float>::infinity();
    std::vector<float> gScore(nodeCount, INF);
    std::vector<float> fScore(nodeCount, INF);
    std::vector<int> prev(nodeCount, -1);
    std::vector<bool> closedSet(nodeCount, false);

    using AStarElem = std::pair<float, int>;
    std::priority_queue<AStarElem, std::vector<AStarElem>, std::greater<AStarElem>> openSet;

    gScore[startNodeIndex] = 0.0f;
    fScore[startNodeIndex] = heuristic(startNodeIndex);
    openSet.push({fScore[startNodeIndex], startNodeIndex});

    while (!openSet.empty()) {
        auto [currentF, u] = openSet.top();
        openSet.pop();

        if (u == targetNodeIndex) {
            std::vector<int> path;
            for (int at = targetNodeIndex; at != -1; at = prev[at]) {
                path.push_back(at);
            }
            std::reverse(path.begin(), path.end());
            return buildRoute(path);
        }

        if (closedSet[u]) continue;
        closedSet[u] = true;

        for (const auto& edge : m_graph->getOutgoingEdges(u)) {
            int v = edge.toIndex;
            if (closedSet[v]) continue;

            float tentativeG = gScore[u] + edge.cost;
            if (tentativeG < gScore[v]) {
                prev[v] = u;
                gScore[v] = tentativeG;
                fScore[v] = tentativeG + heuristic(v);
                openSet.push({fScore[v], v});
            }
        }
    }

    return route;
}

Route PathFinder::calculateRouteToSlot(const std::string& startNodeId, const std::string& targetSlotId, bool useAStar) const {
    if (!m_graph) return Route();

    const Node* startNode = m_graph->getNode(startNodeId);
    const Node* targetNode = m_graph->getNodeBySlotId(targetSlotId);

    if (!startNode || !targetNode) {
        return Route();
    }

    if (useAStar) {
        return findShortestPathAStar(startNode->index, targetNode->index);
    }
    return findShortestPathDijkstra(startNode->index, targetNode->index);
}

Route PathFinder::calculateRouteFromPosition(const glm::vec3& currentPosition, const std::string& targetSlotId, bool useAStar) const {
    if (!m_graph) return Route();

    int startIdx = m_graph->findNearestNode(currentPosition, NodeType::ROAD_LANE, true);
    const Node* targetNode = m_graph->getNodeBySlotId(targetSlotId);

    if (startIdx < 0 || !targetNode) {
        return Route();
    }

    if (useAStar) {
        return findShortestPathAStar(startIdx, targetNode->index);
    }
    return findShortestPathDijkstra(startIdx, targetNode->index);
}

Route PathFinder::buildRoute(const std::vector<int>& pathIndices) const {
    Route route;
    if (pathIndices.empty() || !m_graph) return route;

    route.isValid = true;
    route.nodeIndices = pathIndices;
    route.waypoints.reserve(pathIndices.size());

    float distSum = 0.0f;
    for (size_t i = 0; i < pathIndices.size(); ++i) {
        const Node* node = m_graph->getNode(pathIndices[i]);
        if (node) {
            route.waypoints.push_back(node->position);
        }
        if (i > 0) {
            distSum += glm::distance(route.waypoints[i], route.waypoints[i - 1]);
        }
    }

    route.totalDistance = distSum;
    route.turnCount = countTurns(route.waypoints);

    // Speed calculation: Average driving speed in parking lot ~ 15 km/h = 4.17 m/s
    // Plus 3.5 seconds penalty per 90-degree turn/intersection maneuver
    float drivingTime = route.totalDistance / 4.17f;
    float turnPenaltyTime = static_cast<float>(route.turnCount) * 3.5f;
    route.estimatedTimeSec = drivingTime + turnPenaltyTime;

    route.stepInstructions = generateTurnInstructions(pathIndices);

    return route;
}

int PathFinder::countTurns(const std::vector<glm::vec3>& points) const {
    if (points.size() < 3) return 0;

    int turns = 0;
    for (size_t i = 1; i < points.size() - 1; ++i) {
        glm::vec3 v1 = glm::normalize(points[i] - points[i - 1]);
        glm::vec3 v2 = glm::normalize(points[i + 1] - points[i]);

        float dot = glm::clamp(glm::dot(v1, v2), -1.0f, 1.0f);
        float angleDeg = glm::degrees(std::acos(dot));

        if (angleDeg > 35.0f) {
            turns++;
        }
    }
    return turns;
}

std::vector<std::string> PathFinder::generateTurnInstructions(const std::vector<int>& pathIndices) const {
    std::vector<std::string> instructions;
    if (pathIndices.empty() || !m_graph) return instructions;

    for (size_t i = 0; i < pathIndices.size(); ++i) {
        const Node* node = m_graph->getNode(pathIndices[i]);
        if (!node) continue;

        if (i == 0) {
            instructions.push_back("Start at: " + node->name);
        } else if (i == pathIndices.size() - 1) {
            instructions.push_back("Arrive at destination: " + node->name);
        } else {
            // Check for turn between i-1, i, i+1
            const Node* prev = m_graph->getNode(pathIndices[i - 1]);
            const Node* next = m_graph->getNode(pathIndices[i + 1]);

            if (prev && next) {
                glm::vec3 v1 = glm::normalize(node->position - prev->position);
                glm::vec3 v2 = glm::normalize(next->position - node->position);
                float crossY = v1.x * v2.z - v1.z * v2.x;
                float dot = glm::clamp(glm::dot(v1, v2), -1.0f, 1.0f);
                float angle = glm::degrees(std::acos(dot));

                if (angle > 35.0f) {
                    if (crossY > 0.0f) {
                        instructions.push_back("Turn Right at " + node->name);
                    } else {
                        instructions.push_back("Turn Left at " + node->name);
                    }
                } else {
                    instructions.push_back("Continue through " + node->name);
                }
            }
        }
    }

    return instructions;
}

} // namespace SmartParking
