#include "Graph.h"
#include <glm/gtc/epsilon.hpp>
#include <limits>
#include <cmath>

namespace SmartParking {

Graph::Graph() {
}

void Graph::clear() {
    m_nodes.clear();
    m_allEdges.clear();
    m_idToIndex.clear();
    m_slotToNodeIndex.clear();
    m_adjacencyList.clear();
}

int Graph::addNode(const std::string& id, const glm::vec3& position, NodeType type, const std::string& name, const std::string& slotId) {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        return it->second;
    }

    int newIndex = static_cast<int>(m_nodes.size());
    m_nodes.emplace_back(newIndex, id, position, type, name, slotId);
    m_idToIndex[id] = newIndex;
    m_adjacencyList.emplace_back();

    if (!slotId.empty()) {
        m_slotToNodeIndex[slotId] = newIndex;
    }

    return newIndex;
}

bool Graph::addEdge(const std::string& fromId, const std::string& toId, bool bidirectional, float costMultiplier) {
    auto itFrom = m_idToIndex.find(fromId);
    auto itTo = m_idToIndex.find(toId);

    if (itFrom == m_idToIndex.end() || itTo == m_idToIndex.end()) {
        return false;
    }

    return addEdgeByIndex(itFrom->second, itTo->second, bidirectional, costMultiplier);
}

bool Graph::addEdgeByIndex(int fromIndex, int toIndex, bool bidirectional, float costMultiplier) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_nodes.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(m_nodes.size())) {
        return false;
    }

    float dist = glm::distance(m_nodes[fromIndex].position, m_nodes[toIndex].position) * costMultiplier;

    Edge edgeForward(fromIndex, toIndex, dist, !bidirectional);
    m_adjacencyList[fromIndex].push_back(edgeForward);
    m_allEdges.push_back(edgeForward);

    if (bidirectional) {
        Edge edgeBackward(toIndex, fromIndex, dist, false);
        m_adjacencyList[toIndex].push_back(edgeBackward);
    }

    return true;
}

const Node* Graph::getNode(const std::string& id) const {
    auto it = m_idToIndex.find(id);
    if (it != m_idToIndex.end()) {
        return &m_nodes[it->second];
    }
    return nullptr;
}

const Node* Graph::getNode(int index) const {
    if (index >= 0 && index < static_cast<int>(m_nodes.size())) {
        return &m_nodes[index];
    }
    return nullptr;
}

const Node* Graph::getNodeBySlotId(const std::string& slotId) const {
    auto it = m_slotToNodeIndex.find(slotId);
    if (it != m_slotToNodeIndex.end()) {
        return &m_nodes[it->second];
    }
    return nullptr;
}

int Graph::findNearestNode(const glm::vec3& position, NodeType filterType, bool ignoreTypeFilter) const {
    int bestIndex = -1;
    float minDistSq = std::numeric_limits<float>::max();

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (!ignoreTypeFilter && m_nodes[i].type != filterType) {
            continue;
        }

        glm::vec3 diff = m_nodes[i].position - position;
        diff.y = 0.0f; // Calculate distance on ground plane
        float distSq = glm::dot(diff, diff);

        if (distSq < minDistSq) {
            minDistSq = distSq;
            bestIndex = static_cast<int>(i);
        }
    }

    return bestIndex;
}

const std::vector<Edge>& Graph::getOutgoingEdges(int nodeIndex) const {
    static const std::vector<Edge> emptyList;
    if (nodeIndex >= 0 && nodeIndex < static_cast<int>(m_adjacencyList.size())) {
        return m_adjacencyList[nodeIndex];
    }
    return emptyList;
}

} // namespace SmartParking
