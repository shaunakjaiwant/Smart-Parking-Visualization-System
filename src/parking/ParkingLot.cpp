#include "ParkingLot.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace SmartParking {

using json = nlohmann::json;

ParkingLot::ParkingLot() {
    generateDefaultLayout();
    buildNavigationGraph();
}

void ParkingLot::addSlot(const ParkingSlot& slot) {
    m_slotIndexMap[slot.getId()] = m_slots.size();
    m_slots.push_back(slot);
}

ParkingSlot* ParkingLot::getSlot(const std::string& id) {
    auto it = m_slotIndexMap.find(id);
    if (it != m_slotIndexMap.end()) {
        return &m_slots[it->second];
    }
    return nullptr;
}

const ParkingSlot* ParkingLot::getSlot(const std::string& id) const {
    auto it = m_slotIndexMap.find(id);
    if (it != m_slotIndexMap.end()) {
        return &m_slots[it->second];
    }
    return nullptr;
}

bool ParkingLot::loadLayout(const std::string& configFilePath) {
    std::ifstream file(configFilePath);
    if (!file.is_open()) {
        std::cerr << "[ParkingLot] Warning: Could not open " << configFilePath 
                  << ", falling back to default procedural layout." << std::endl;
        generateDefaultLayout();
        buildNavigationGraph();
        return false;
    }

    try {
        json j;
        file >> j;

        m_slots.clear();
        m_rows.clear();
        m_roads.clear();
        m_slotIndexMap.clear();

        if (j.contains("facility")) {
            const auto& fac = j["facility"];
            if (fac.contains("name")) m_facilityName = fac["name"];
            if (fac.contains("dimensions")) {
                m_facilityDimensions.x = fac["dimensions"]["width"];
                m_facilityDimensions.y = fac["dimensions"]["length"];
            }
            if (fac.contains("entrance")) {
                auto ent = fac["entrance"]["position"];
                m_entrancePos = glm::vec3(ent[0], ent[1], ent[2]);
            }
            if (fac.contains("exit")) {
                auto ex = fac["exit"]["position"];
                m_exitPos = glm::vec3(ex[0], ex[1], ex[2]);
            }
        }

        if (j.contains("rows")) {
            for (const auto& rowJson : j["rows"]) {
                ParkingRow row;
                row.id = rowJson.value("id", "A");
                row.name = rowJson.value("name", "Row " + row.id);

                if (rowJson.contains("slots")) {
                    for (const auto& sJson : rowJson["slots"]) {
                        std::string sId = sJson.value("id", "S-00");
                        std::string sTypeStr = sJson.value("type", "REGULAR");
                        SlotType sType = SlotType::REGULAR;
                        if (sTypeStr == "EV_CHARGING") sType = SlotType::EV_CHARGING;
                        else if (sTypeStr == "ACCESSIBLE") sType = SlotType::ACCESSIBLE;
                        else if (sTypeStr == "RESERVED") sType = SlotType::RESERVED;

                        auto posArray = sJson["position"];
                        glm::vec3 pos(posArray[0], posArray[1], posArray[2]);

                        glm::vec2 size(2.6f, 5.2f);
                        if (sJson.contains("size")) {
                            size.x = sJson["size"][0];
                            size.y = sJson["size"][1];
                        }

                        float rot = sJson.value("rotation", 0.0f);
                        ParkingSlot slot(sId, row.id, sType, pos, size, rot);
                        slot.setDistanceFromEntrance(glm::distance(pos, m_entrancePos));

                        row.slotIds.push_back(sId);
                        addSlot(slot);
                    }
                }
                m_rows.push_back(row);
            }
        }

        if (j.contains("roads")) {
            for (const auto& rJson : j["roads"]) {
                RoadSegment r;
                r.id = rJson.value("id", "road");
                auto f = rJson["from"];
                auto t = rJson["to"];
                r.from = glm::vec3(f[0], f[1], f[2]);
                r.to = glm::vec3(t[0], t[1], t[2]);
                r.width = rJson.value("width", 6.0f);
                m_roads.push_back(r);
            }
        }

        setupLandscapeDecorations();
        buildNavigationGraph();

        std::cout << "[ParkingLot] Successfully loaded layout: " << m_slots.size() << " slots across " 
                  << m_rows.size() << " rows." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ParkingLot Error] Failed to parse JSON: " << e.what() 
                  << ", generating default layout." << std::endl;
        generateDefaultLayout();
        buildNavigationGraph();
        return false;
    }
}

void ParkingLot::generateDefaultLayout() {
    m_slots.clear();
    m_rows.clear();
    m_roads.clear();
    m_slotIndexMap.clear();

    m_facilityName = "Central Plaza Smart Parking Facility";
    m_facilityDimensions = glm::vec2(110.0f, 80.0f);
    m_entrancePos = glm::vec3(-50.0f, 0.0f, 0.0f);
    m_exitPos = glm::vec3(50.0f, 0.0f, 0.0f);

    // Setup 4 rows with 15 slots each = 60 slots
    struct RowConfig {
        std::string id;
        std::string name;
        float zPos;
        SlotType primaryType;
    };

    std::vector<RowConfig> rowConfigs = {
        {"A", "Row A (North Accessible & EV)", 24.0f, SlotType::REGULAR},
        {"B", "Row B (North-Central Reserved & Regular)", 10.0f, SlotType::REGULAR},
        {"C", "Row C (South-Central Regular)", -10.0f, SlotType::REGULAR},
        {"D", "Row D (South EV & Regular)", -24.0f, SlotType::REGULAR}
    };

    for (const auto& rc : rowConfigs) {
        ParkingRow row;
        row.id = rc.id;
        row.name = rc.name;
        row.direction = glm::vec3(1.0f, 0.0f, 0.0f);

        for (int i = 1; i <= 15; ++i) {
            std::string slotId = rc.id + "-" + (i < 10 ? "0" : "") + std::to_string(i);
            SlotType type = SlotType::REGULAR;
            glm::vec2 size(2.6f, 5.2f);

            // Row A: 1-4 Accessible, 5-8 EV, 9-15 Regular
            if (rc.id == "A") {
                if (i <= 4) {
                    type = SlotType::ACCESSIBLE;
                    size = glm::vec2(3.2f, 5.8f);
                } else if (i <= 8) {
                    type = SlotType::EV_CHARGING;
                    size = glm::vec2(2.8f, 5.5f);
                }
            }
            // Row B: 1-4 Reserved, 5-15 Regular
            else if (rc.id == "B") {
                if (i <= 4) {
                    type = SlotType::RESERVED;
                }
            }
            // Row D: 1-2 EV Charging, 3-15 Regular
            else if (rc.id == "D") {
                if (i <= 2) {
                    type = SlotType::EV_CHARGING;
                    size = glm::vec2(2.8f, 5.5f);
                }
            }

            // Distribute slots horizontally from X = -35 to X = +23
            float xPos = -35.0f + (i - 1) * 3.8f;
            if (i > 8) xPos += 2.0f; // central pedestrian cross gap

            glm::vec3 pos(xPos, 0.0f, rc.zPos);
            ParkingSlot slot(slotId, rc.id, type, pos, size, 0.0f);
            slot.setDistanceFromEntrance(glm::distance(pos, m_entrancePos));

            row.slotIds.push_back(slotId);
            addSlot(slot);
        }
        m_rows.push_back(row);
    }

    // Roads
    m_roads.push_back({"road_in", {-50.0f, 0.0f, 0.0f}, {-42.0f, 0.0f, 0.0f}, 7.0f});
    m_roads.push_back({"road_west_spine", {-42.0f, 0.0f, -28.0f}, {-42.0f, 0.0f, 28.0f}, 6.5f});
    m_roads.push_back({"road_north_aisle", {-42.0f, 0.0f, 17.0f}, {34.0f, 0.0f, 17.0f}, 6.0f});
    m_roads.push_back({"road_central_cross", {-42.0f, 0.0f, 0.0f}, {34.0f, 0.0f, 0.0f}, 6.0f});
    m_roads.push_back({"road_south_aisle", {-42.0f, 0.0f, -17.0f}, {34.0f, 0.0f, -17.0f}, 6.0f});
    m_roads.push_back({"road_east_spine", {34.0f, 0.0f, -28.0f}, {34.0f, 0.0f, 28.0f}, 6.5f});
    m_roads.push_back({"road_out", {34.0f, 0.0f, 0.0f}, {50.0f, 0.0f, 0.0f}, 7.0f});

    setupLandscapeDecorations();
}

void ParkingLot::setupLandscapeDecorations() {
    m_landscapeItems.clear();
    m_roadMarkings.clear();

    // 1. Boundary walls, barriers, and gatehouse building
    m_landscapeItems.push_back({LandscapeItem::Type::BARRIER, {-51.0f, 0.5f, 3.5f}, {1.0f, 1.0f, 0.2f}, 0.0f});
    m_landscapeItems.push_back({LandscapeItem::Type::BUILDING, {-48.0f, 0.0f, -6.5f}, {6.0f, 3.5f, 4.5f}, 0.0f});

    // 2. 3D Informational Signposts
    m_landscapeItems.push_back({LandscapeItem::Type::SIGN_POST, {-47.0f, 0.0f, 4.5f}, {1.2f, 2.6f, 0.3f}, 0.0f, "P"});
    m_landscapeItems.push_back({LandscapeItem::Type::SIGN_POST, {-47.0f, 0.0f, -3.5f}, {1.4f, 2.4f, 0.3f}, 0.0f, "ENTRY"});
    m_landscapeItems.push_back({LandscapeItem::Type::SIGN_POST, {46.0f, 0.0f, 3.5f}, {1.4f, 2.4f, 0.3f}, 0.0f, "EXIT"});
    m_landscapeItems.push_back({LandscapeItem::Type::SIGN_POST, {-18.0f, 0.0f, 29.5f}, {1.1f, 2.2f, 0.3f}, 0.0f, "EV"});
    m_landscapeItems.push_back({LandscapeItem::Type::SIGN_POST, {-36.0f, 0.0f, 29.5f}, {1.3f, 2.2f, 0.3f}, 0.0f, "ACCESSIBLE"});

    // 3. Varied Environmental Trees (Types A, B, C)
    // North Perimeter Trees (Z = 34.0)
    int treeVar = 0;
    for (float x = -48.0f; x <= 46.0f; x += 10.0f) {
        LandscapeItem::Type tType = LandscapeItem::Type::TREE_SPHERICAL;
        if (treeVar % 3 == 1) tType = LandscapeItem::Type::TREE_CONIFER;
        else if (treeVar % 3 == 2) tType = LandscapeItem::Type::TREE_ORNAMENTAL;
        float s = 0.9f + (treeVar % 4) * 0.15f;
        m_landscapeItems.push_back({tType, {x, 0.0f, 34.5f}, {1.6f * s, 3.4f * s, 1.6f * s}, (treeVar * 37.0f)});
        treeVar++;
    }

    // South Perimeter Trees (Z = -34.0)
    for (float x = -48.0f; x <= 46.0f; x += 10.0f) {
        LandscapeItem::Type tType = LandscapeItem::Type::TREE_SPHERICAL;
        if (treeVar % 3 == 1) tType = LandscapeItem::Type::TREE_CONIFER;
        else if (treeVar % 3 == 2) tType = LandscapeItem::Type::TREE_ORNAMENTAL;
        float s = 0.9f + (treeVar % 4) * 0.15f;
        m_landscapeItems.push_back({tType, {x, 0.0f, -34.5f}, {1.6f * s, 3.4f * s, 1.6f * s}, (treeVar * 43.0f)});
        treeVar++;
    }

    // East Perimeter Trees (X = 46.0)
    for (float z = -25.0f; z <= 25.0f; z += 12.0f) {
        if (std::abs(z) > 4.0f) { // Keep road exit clear
            m_landscapeItems.push_back({LandscapeItem::Type::TREE_CONIFER, {46.0f, 0.0f, z}, {1.5f, 3.6f, 1.5f}, z * 5.0f});
        }
    }

    // 4. Lighting Poles with Light Fixtures
    float lampX[] = { -30.0f, -10.0f, 10.0f, 25.0f };
    for (float x : lampX) {
        m_landscapeItems.push_back({LandscapeItem::Type::LAMP_POST, {x, 0.0f, 17.0f}, {0.25f, 4.6f, 0.25f}, 0.0f});
        m_landscapeItems.push_back({LandscapeItem::Type::LAMP_POST, {x, 0.0f, -17.0f}, {0.25f, 4.6f, 0.25f}, 0.0f});
    }
    m_landscapeItems.push_back({LandscapeItem::Type::LAMP_POST, {-43.0f, 0.0f, 4.5f}, {0.25f, 4.6f, 0.25f}, 0.0f});
    m_landscapeItems.push_back({LandscapeItem::Type::LAMP_POST, {35.0f, 0.0f, 4.5f}, {0.25f, 4.6f, 0.25f}, 0.0f});

    // 5. Road Markings & Direction Arrows
    // A. Center Lane Dashed Lines along Main Corridors
    for (float x = -38.0f; x <= 30.0f; x += 4.5f) {
        m_roadMarkings.push_back({RoadMarking::Type::LANE_DIVIDER, {x, 0.015f, 0.0f}, 90.0f, {0.25f, 2.5f}});
    }
    for (float x = -38.0f; x <= 30.0f; x += 4.5f) {
        m_roadMarkings.push_back({RoadMarking::Type::LANE_DIVIDER, {x, 0.015f, 17.0f}, 90.0f, {0.25f, 2.5f}});
        m_roadMarkings.push_back({RoadMarking::Type::LANE_DIVIDER, {x, 0.015f, -17.0f}, 90.0f, {0.25f, 2.5f}});
    }

    // B. Stop Lines before primary junctions
    m_roadMarkings.push_back({RoadMarking::Type::STOP_LINE, {-44.0f, 0.016f, 0.0f}, 0.0f, {5.5f, 0.45f}});
    m_roadMarkings.push_back({RoadMarking::Type::STOP_LINE, {33.0f, 0.016f, 0.0f}, 0.0f, {5.5f, 0.45f}});
    m_roadMarkings.push_back({RoadMarking::Type::STOP_LINE, {-42.0f, 0.016f, 14.5f}, 90.0f, {5.0f, 0.45f}});
    m_roadMarkings.push_back({RoadMarking::Type::STOP_LINE, {-42.0f, 0.016f, -14.5f}, 90.0f, {5.0f, 0.45f}});

    // C. Directional Arrows along lanes
    // Entrance / Exit forward arrows
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_FORWARD, {-47.0f, 0.018f, 0.0f}, 90.0f, {1.8f, 1.0f}});
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_FORWARD, {43.0f, 0.018f, 0.0f}, 90.0f, {1.8f, 1.0f}});

    // Aisle Driving Direction Arrows (Eastbound traffic along aisles)
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_FORWARD, {-26.0f, 0.018f, 17.0f}, 90.0f, {1.8f, 1.0f}});
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_FORWARD, {6.0f, 0.018f, 17.0f}, 90.0f, {1.8f, 1.0f}});
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_FORWARD, {-26.0f, 0.018f, -17.0f}, 90.0f, {1.8f, 1.0f}});
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_FORWARD, {6.0f, 0.018f, -17.0f}, 90.0f, {1.8f, 1.0f}});

    // Turn arrows at West and East Spine
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_LEFT, {-42.0f, 0.018f, 4.0f}, 0.0f, {1.8f, 1.0f}});
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_RIGHT, {-42.0f, 0.018f, -4.0f}, 180.0f, {1.8f, 1.0f}});
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_RIGHT, {34.0f, 0.018f, 13.0f}, 180.0f, {1.8f, 1.0f}});
    m_roadMarkings.push_back({RoadMarking::Type::ARROW_LEFT, {34.0f, 0.018f, -13.0f}, 0.0f, {1.8f, 1.0f}});

    // D. Pedestrian Crosswalks
    m_roadMarkings.push_back({RoadMarking::Type::CROSSWALK, {-40.0f, 0.016f, 0.0f}, 0.0f, {5.2f, 2.0f}});
}

void ParkingLot::buildNavigationGraph() {
    m_navGraph.clear();

    // 1. Entrance & Exit Nodes
    m_navGraph.addNode(m_entranceNodeId, m_entrancePos, NodeType::ENTRANCE, "Main Entrance");
    m_navGraph.addNode(m_exitNodeId, m_exitPos, NodeType::EXIT, "Main Exit");

    // 2. West Spine Nodes
    m_navGraph.addNode("west_in", {-42.0f, 0.0f, 0.0f}, NodeType::INTERSECTION, "West Entrance Junction");
    m_navGraph.addNode("west_north", {-42.0f, 0.0f, 17.0f}, NodeType::INTERSECTION, "West North Junction");
    m_navGraph.addNode("west_south", {-42.0f, 0.0f, -17.0f}, NodeType::INTERSECTION, "West South Junction");
    m_navGraph.addNode("west_far_north", {-42.0f, 0.0f, 28.0f}, NodeType::ROAD_LANE, "West Far North Turn");
    m_navGraph.addNode("west_far_south", {-42.0f, 0.0f, -28.0f}, NodeType::ROAD_LANE, "West Far South Turn");

    // 3. East Spine Nodes
    m_navGraph.addNode("east_out", {34.0f, 0.0f, 0.0f}, NodeType::INTERSECTION, "East Exit Junction");
    m_navGraph.addNode("east_north", {34.0f, 0.0f, 17.0f}, NodeType::INTERSECTION, "East North Junction");
    m_navGraph.addNode("east_south", {34.0f, 0.0f, -17.0f}, NodeType::INTERSECTION, "East South Junction");
    m_navGraph.addNode("east_far_north", {34.0f, 0.0f, 28.0f}, NodeType::ROAD_LANE, "East Far North Turn");
    m_navGraph.addNode("east_far_south", {34.0f, 0.0f, -28.0f}, NodeType::ROAD_LANE, "East Far South Turn");

    // Connect Primary Arteries
    m_navGraph.addEdge(m_entranceNodeId, "west_in", false); // One-way in
    m_navGraph.addEdge("west_in", "west_north", true);
    m_navGraph.addEdge("west_in", "west_south", true);
    m_navGraph.addEdge("west_north", "west_far_north", true);
    m_navGraph.addEdge("west_south", "west_far_south", true);

    m_navGraph.addEdge("east_north", "east_out", true);
    m_navGraph.addEdge("east_south", "east_out", true);
    m_navGraph.addEdge("east_far_north", "east_north", true);
    m_navGraph.addEdge("east_far_south", "east_south", true);
    m_navGraph.addEdge("east_out", m_exitNodeId, false); // One-way out

    // Central crossroad
    m_navGraph.addEdge("west_in", "east_out", true);

    // 4. North Aisle & South Aisle Waypoints and Slot Connectors
    // North Aisle connects Row A (Z=24) and Row B (Z=10) from aisle centerline at Z=17
    // South Aisle connects Row C (Z=-10) and Row D (Z=-24) from aisle centerline at Z=-17

    std::string prevNorthLaneNode = "west_north";
    std::string prevSouthLaneNode = "west_south";

    for (const auto& slot : m_slots) {
        float aisleZ = (slot.getPosition().z > 0.0f) ? 17.0f : -17.0f;
        std::string accessNodeId = "access_" + slot.getId();
        std::string bayNodeId = "bay_" + slot.getId();

        glm::vec3 accessPos(slot.getPosition().x, 0.0f, aisleZ);
        glm::vec3 bayPos = slot.getPosition();

        m_navGraph.addNode(accessNodeId, accessPos, NodeType::SLOT_ACCESS, "Access " + slot.getId());
        m_navGraph.addNode(bayNodeId, bayPos, NodeType::SLOT_BAY, "Bay " + slot.getId(), slot.getId());

        // Connect access node to bay
        m_navGraph.addEdge(accessNodeId, bayNodeId, true);

        // Chain access nodes along the aisle
        if (aisleZ > 0.0f) {
            m_navGraph.addEdge(prevNorthLaneNode, accessNodeId, true);
            prevNorthLaneNode = accessNodeId;
        } else {
            m_navGraph.addEdge(prevSouthLaneNode, accessNodeId, true);
            prevSouthLaneNode = accessNodeId;
        }
    }

    // Connect ends of aisles to East Spine
    m_navGraph.addEdge(prevNorthLaneNode, "east_north", true);
    m_navGraph.addEdge(prevSouthLaneNode, "east_south", true);
}

ParkingSlot* ParkingLot::findNearestAvailableSlot(const glm::vec3& referencePos, SlotType requestedType) {
    ParkingSlot* bestSlot = nullptr;
    float minDistSq = std::numeric_limits<float>::max();

    for (auto& slot : m_slots) {
        if (!slot.isAvailableFor(requestedType)) {
            continue;
        }

        glm::vec3 diff = slot.getPosition() - referencePos;
        diff.y = 0.0f;
        float distSq = glm::dot(diff, diff);

        if (distSq < minDistSq) {
            minDistSq = distSq;
            bestSlot = &slot;
        }
    }

    return bestSlot;
}

std::vector<const ParkingSlot*> ParkingLot::searchSlots(const std::string& query, int typeFilter, int statusFilter) const {
    std::vector<const ParkingSlot*> results;

    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for (const auto& slot : m_slots) {
        // ID search
        if (!lowerQuery.empty()) {
            std::string slotIdLower = slot.getId();
            std::transform(slotIdLower.begin(), slotIdLower.end(), slotIdLower.begin(), ::tolower);
            if (slotIdLower.find(lowerQuery) == std::string::npos) {
                continue;
            }
        }

        // Type filter
        if (typeFilter >= 0 && static_cast<int>(slot.getType()) != typeFilter) {
            continue;
        }

        // Status filter
        if (statusFilter >= 0 && static_cast<int>(slot.getStatus()) != statusFilter) {
            continue;
        }

        results.push_back(&slot);
    }

    return results;
}

bool ParkingLot::selectSlot(const std::string& id) {
    if (m_selectedSlot) {
        m_selectedSlot->select(false);
        m_selectedSlot = nullptr;
    }

    ParkingSlot* slot = getSlot(id);
    if (slot) {
        slot->select(true);
        m_selectedSlot = slot;
        return true;
    }
    return false;
}

void ParkingLot::clearSelection() {
    if (m_selectedSlot) {
        m_selectedSlot->select(false);
        m_selectedSlot = nullptr;
    }
}

FacilityStats ParkingLot::calculateStats() const {
    FacilityStats stats;
    stats.totalSlots = static_cast<int>(m_slots.size());

    for (const auto& slot : m_slots) {
        if (slot.getType() == SlotType::EV_CHARGING) stats.evSlots++;
        if (slot.getType() == SlotType::ACCESSIBLE) stats.accessibleSlots++;

        if (slot.getStatus() == SlotStatus::OCCUPIED || slot.getStatus() == SlotStatus::EV_CHARGING) {
            stats.occupiedSlots++;
        } else if (slot.getStatus() == SlotStatus::RESERVED) {
            stats.reservedSlots++;
        } else {
            stats.availableSlots++;
        }
    }

    if (stats.totalSlots > 0) {
        stats.occupancyRate = (static_cast<float>(stats.occupiedSlots) / static_cast<float>(stats.totalSlots)) * 100.0f;
    }

    return stats;
}

ParkingSlot* ParkingLot::pickSlot2D(float worldX, float worldZ) {
    for (auto& slot : m_slots) {
        if (slot.containsPoint2D(worldX, worldZ)) {
            return &slot;
        }
    }
    return nullptr;
}

} // namespace SmartParking
