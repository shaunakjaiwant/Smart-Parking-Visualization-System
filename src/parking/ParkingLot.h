#pragma once
#include "ParkingSlot.h"
#include "../navigation/Graph.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

/**
 * @file ParkingLot.h
 * @brief Top-level facility manager and layout container.
 * 
 * WHAT: Loads and stores all parking slots, rows, roads, entrance/exit, and landscape elements.
 * WHY:  Central data repository and smart allocation coordinator for the entire system.
 * HOW:  Parses parking_layout.json (with automatic fallback), constructs the spatial
 *       navigation graph with bidirectional and one-way lanes, and provides nearest-slot queries.
 */
namespace SmartParking {

struct ParkingRow {
    std::string id;
    std::string name;
    glm::vec3 direction;
    std::vector<std::string> slotIds;
};

struct RoadSegment {
    std::string id;
    glm::vec3 from;
    glm::vec3 to;
    float width;
};

struct LandscapeItem {
    enum class Type {
        TREE_SPHERICAL,  // Tree Type A: Straight trunk + spherical canopy
        TREE_CONIFER,    // Tree Type B: Layered conical evergreen
        TREE_ORNAMENTAL, // Tree Type C: Compact rounded decorative bush
        LAMP_POST,
        SIGN_POST,
        BARRIER,
        BUILDING
    };
    Type type;
    glm::vec3 position;
    glm::vec3 scale = glm::vec3(1.0f);
    float rotationDeg = 0.0f;
    std::string signText = ""; // "P", "ENTRY", "EXIT", "EV", "ACCESSIBLE"
};

struct RoadMarking {
    enum class Type {
        LANE_DIVIDER,
        STOP_LINE,
        CROSSWALK,
        ARROW_FORWARD,
        ARROW_LEFT,
        ARROW_RIGHT
    };
    Type type;
    glm::vec3 position;
    float rotationDeg = 0.0f;
    glm::vec2 size = glm::vec2(1.0f, 2.0f);
};

struct FacilityStats {
    int totalSlots = 0;
    int availableSlots = 0;
    int occupiedSlots = 0;
    int reservedSlots = 0;
    int evSlots = 0;
    int accessibleSlots = 0;
    float occupancyRate = 0.0f; // 0.0 to 100.0%
};

class ParkingLot {
public:
    ParkingLot();
    ~ParkingLot() = default;

    bool loadLayout(const std::string& configFilePath);
    void generateDefaultLayout();
    void buildNavigationGraph();

    // Queries
    ParkingSlot* getSlot(const std::string& id);
    const ParkingSlot* getSlot(const std::string& id) const;
    const std::vector<ParkingSlot>& getAllSlots() const { return m_slots; }
    const std::vector<ParkingRow>& getAllRows() const { return m_rows; }
    const std::vector<RoadSegment>& getAllRoads() const { return m_roads; }
    const std::vector<LandscapeItem>& getLandscapeItems() const { return m_landscapeItems; }
    const std::vector<RoadMarking>& getAllRoadMarkings() const { return m_roadMarkings; }
    const Graph& getNavigationGraph() const { return m_navGraph; }
    Graph& getNavigationGraph() { return m_navGraph; }

    const glm::vec3& getEntrancePosition() const { return m_entrancePos; }
    const glm::vec3& getExitPosition() const { return m_exitPos; }
    const std::string& getEntranceNodeId() const { return m_entranceNodeId; }
    const std::string& getExitNodeId() const { return m_exitNodeId; }

    // Smart Allocation & Search
    ParkingSlot* findNearestAvailableSlot(const glm::vec3& referencePos, SlotType requestedType = SlotType::REGULAR);
    std::vector<const ParkingSlot*> searchSlots(const std::string& query, int typeFilter = -1, int statusFilter = -1) const;

    // Slot interaction
    bool selectSlot(const std::string& id);
    void clearSelection();
    const ParkingSlot* getSelectedSlot() const { return m_selectedSlot; }

    // Real-time metrics
    FacilityStats calculateStats() const;

    // 2D Mouse picking
    ParkingSlot* pickSlot2D(float worldX, float worldZ);

private:
    std::string m_facilityName = "Smart Parking Facility";
    glm::vec2 m_facilityDimensions = glm::vec2(110.0f, 80.0f);
    glm::vec3 m_entrancePos = glm::vec3(-50.0f, 0.0f, 0.0f);
    glm::vec3 m_exitPos = glm::vec3(50.0f, 0.0f, 0.0f);
    std::string m_entranceNodeId = "node_entrance";
    std::string m_exitNodeId = "node_exit";

    std::vector<ParkingSlot> m_slots;
    std::vector<ParkingRow> m_rows;
    std::vector<RoadSegment> m_roads;
    std::vector<LandscapeItem> m_landscapeItems;
    std::vector<RoadMarking> m_roadMarkings;
    std::unordered_map<std::string, size_t> m_slotIndexMap;

    ParkingSlot* m_selectedSlot = nullptr;
    Graph m_navGraph;

    void addSlot(const ParkingSlot& slot);
    void setupLandscapeDecorations();
};

} // namespace SmartParking
