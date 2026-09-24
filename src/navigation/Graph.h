#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

/**
 * @file Graph.h
 * @brief Navigation Graph data structure for parking facility routing.
 * 
 * WHAT: Spatial graph representing roads, lanes, intersections, entrance, exit, and slots.
 * WHY:  Required for pathfinding algorithms (Dijkstra and A*) to compute exact turn-by-turn routes.
 * HOW:  Maintains indexed Node list and Adjacency List of weighted directional edges.
 */
namespace SmartParking {

enum class NodeType {
    ENTRANCE,
    EXIT,
    INTERSECTION,
    ROAD_LANE,
    SLOT_ACCESS,
    SLOT_BAY
};

struct Node {
    int index = -1;
    std::string id;
    std::string name;
    glm::vec3 position;
    NodeType type = NodeType::ROAD_LANE;
    std::string slotId = ""; // Associated slot if this node connects to a bay

    Node() = default;
    Node(int idx, const std::string& nodeId, const glm::vec3& pos, NodeType t, const std::string& n = "", const std::string& sId = "")
        : index(idx), id(nodeId), name(n.empty() ? nodeId : n), position(pos), type(t), slotId(sId) {}
};

struct Edge {
    int fromIndex;
    int toIndex;
    float cost;
    bool isOneWay;

    Edge(int from, int to, float c, bool oneWay = false)
        : fromIndex(from), toIndex(to), cost(c), isOneWay(oneWay) {}
};

class Graph {
public:
    Graph();
    ~Graph() = default;

    void clear();

    int addNode(const std::string& id, const glm::vec3& position, NodeType type, const std::string& name = "", const std::string& slotId = "");
    bool addEdge(const std::string& fromId, const std::string& toId, bool bidirectional = true, float costMultiplier = 1.0f);
    bool addEdgeByIndex(int fromIndex, int toIndex, bool bidirectional = true, float costMultiplier = 1.0f);

    const Node* getNode(const std::string& id) const;
    const Node* getNode(int index) const;
    const Node* getNodeBySlotId(const std::string& slotId) const;
    int findNearestNode(const glm::vec3& position, NodeType filterType = NodeType::ROAD_LANE, bool ignoreTypeFilter = true) const;

    const std::vector<Edge>& getOutgoingEdges(int nodeIndex) const;
    const std::vector<Node>& getNodes() const { return m_nodes; }
    const std::vector<Edge>& getAllEdges() const { return m_allEdges; }

    size_t getNodeCount() const { return m_nodes.size(); }
    size_t getEdgeCount() const { return m_allEdges.size(); }

private:
    std::vector<Node> m_nodes;
    std::vector<Edge> m_allEdges;
    std::unordered_map<std::string, int> m_idToIndex;
    std::unordered_map<std::string, int> m_slotToNodeIndex;
    std::vector<std::vector<Edge>> m_adjacencyList;
};

} // namespace SmartParking
