#include "../src/parking/ParkingSlot.h"
#include "../src/parking/ParkingLot.h"
#include "../src/navigation/Graph.h"
#include "../src/navigation/PathFinder.h"
#include "../src/simulation/Simulation.h"
#include "../src/graphics/Camera.h"
#include "../src/graphics/ShadowMap.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace SmartParking;

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << msg << std::endl; \
            g_testsPassed++; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (line " << __LINE__ << ")" << std::endl; \
            g_testsFailed++; \
        } \
    } while(0)

void testParkingSlotStates() {
    std::cout << "\n--- TEST 1: Parking Slot State Transitions ---" << std::endl;
    ParkingSlot slot("T-01", "T", SlotType::REGULAR, glm::vec3(0.0f), glm::vec2(2.6f, 5.2f));

    TEST_ASSERT(slot.getStatus() == SlotStatus::AVAILABLE, "Slot initially AVAILABLE");
    TEST_ASSERT(slot.isAvailable(), "isAvailable() returns true");

    bool occSuccess = slot.occupy("TEST-VEHICLE-1");
    TEST_ASSERT(occSuccess, "occupy() succeeds");
    TEST_ASSERT(slot.getStatus() == SlotStatus::OCCUPIED, "Slot status is OCCUPIED");
    TEST_ASSERT(!slot.isAvailable(), "Slot is not available while occupied");
    TEST_ASSERT(slot.getVehicleId() == "TEST-VEHICLE-1", "Vehicle ID matches occupant");

    bool freeSuccess = slot.free();
    TEST_ASSERT(freeSuccess, "free() succeeds");
    TEST_ASSERT(slot.getStatus() == SlotStatus::AVAILABLE, "Slot is back to AVAILABLE");
    TEST_ASSERT(slot.getVehicleId().empty(), "Vehicle ID cleared after freeing");
}

void testSlotReservation() {
    std::cout << "\n--- TEST 2: Slot Reservation ---" << std::endl;
    ParkingSlot slot("T-02", "T", SlotType::REGULAR, glm::vec3(0.0f), glm::vec2(2.6f, 5.2f));

    bool resSuccess = slot.reserve();
    TEST_ASSERT(resSuccess, "reserve() on available slot succeeds");
    TEST_ASSERT(slot.getStatus() == SlotStatus::RESERVED, "Slot status is RESERVED");
    TEST_ASSERT(!slot.isAvailable(), "Reserved slot is not freely available");

    bool unresSuccess = slot.unreserve();
    TEST_ASSERT(unresSuccess, "unreserve() succeeds");
    TEST_ASSERT(slot.getStatus() == SlotStatus::AVAILABLE, "Slot returns to AVAILABLE after unreserve");
}

void testGraphAndShortestPath() {
    std::cout << "\n--- TEST 3: Navigation Graph & Dijkstra Shortest Path ---" << std::endl;
    Graph graph;
    int n0 = graph.addNode("N0", glm::vec3(0.0f, 0.0f, 0.0f), NodeType::ENTRANCE, "Entrance");
    int n1 = graph.addNode("N1", glm::vec3(10.0f, 0.0f, 0.0f), NodeType::INTERSECTION, "Junction 1");
    int n2 = graph.addNode("N2", glm::vec3(10.0f, 0.0f, 20.0f), NodeType::ROAD_LANE, "Lane North");
    int n3 = graph.addNode("N3", glm::vec3(30.0f, 0.0f, 0.0f), NodeType::ROAD_LANE, "Lane South");
    int n4 = graph.addNode("N4", glm::vec3(30.0f, 0.0f, 20.0f), NodeType::SLOT_BAY, "Target Slot", "SLOT-99");

    // Add edges: N0->N1 (10m), N1->N2 (20m), N2->N4 (20m) -> Path A total 50m
    // Add edges: N1->N3 (20m), N3->N4 (20m) -> Path B total 50m
    graph.addEdgeByIndex(n0, n1, true);
    graph.addEdgeByIndex(n1, n2, true);
    graph.addEdgeByIndex(n2, n4, true);
    graph.addEdgeByIndex(n1, n3, true);
    graph.addEdgeByIndex(n3, n4, true);

    TEST_ASSERT(graph.getNodeCount() == 5, "Graph node count is 5");
    TEST_ASSERT(graph.getAllEdges().size() >= 5, "Graph has registered edges");

    PathFinder pathFinder(&graph);
    Route routeDijkstra = pathFinder.findShortestPathDijkstra(n0, n4);

    TEST_ASSERT(routeDijkstra.isValid, "Dijkstra finds valid path");
    TEST_ASSERT(std::abs(routeDijkstra.totalDistance - 50.0f) < 0.1f, "Path distance equals 50.0m");
    TEST_ASSERT(routeDijkstra.waypoints.size() == 4, "Path has exactly 4 waypoints");
    TEST_ASSERT(routeDijkstra.turnCount >= 1, "Path turn counter detects turns");

    Route routeAStar = pathFinder.findShortestPathAStar(n0, n4);
    TEST_ASSERT(routeAStar.isValid, "A* finds valid path");
    TEST_ASSERT(std::abs(routeAStar.totalDistance - 50.0f) < 0.1f, "A* path distance equals 50.0m");
}

void testNoRouteScenario() {
    std::cout << "\n--- TEST 4: No Route / Disconnected Node Scenario ---" << std::endl;
    Graph graph;
    int n0 = graph.addNode("N0", glm::vec3(0.0f), NodeType::ENTRANCE);
    int n1 = graph.addNode("N1", glm::vec3(10.0f), NodeType::ROAD_LANE);
    int disconnected = graph.addNode("DISCONNECTED", glm::vec3(100.0f), NodeType::SLOT_BAY);

    graph.addEdgeByIndex(n0, n1, true);
    // Node 'disconnected' has no edges

    PathFinder pathFinder(&graph);
    Route route = pathFinder.findShortestPathDijkstra(n0, disconnected);

    TEST_ASSERT(!route.isValid, "Path to disconnected node correctly marked invalid");
    TEST_ASSERT(route.totalDistance == 0.0f, "Invalid route distance is 0.0m");
    TEST_ASSERT(route.waypoints.empty(), "Invalid route waypoints are empty");
}

void testParkingLotAndSearch() {
    std::cout << "\n--- TEST 5: Parking Facility Allocation & Search ---" << std::endl;
    ParkingLot lot;
    lot.generateDefaultLayout();
    lot.buildNavigationGraph();

    TEST_ASSERT(lot.getAllSlots().size() >= 50, "Parking lot contains at least 50 slots (contains 60)");
    TEST_ASSERT(lot.getAllRows().size() == 4, "Parking lot has 4 rows (A, B, C, D)");

    // Test search
    auto resultsA = lot.searchSlots("A-", -1, -1);
    TEST_ASSERT(resultsA.size() == 15, "Search for 'A-' yields 15 slots in Row A");

    auto resultsEV = lot.searchSlots("", static_cast<int>(SlotType::EV_CHARGING), -1);
    TEST_ASSERT(!resultsEV.empty(), "Search for EV charging slots returns valid slots");

    // Test nearest allocation
    ParkingSlot* nearest = lot.findNearestAvailableSlot(lot.getEntrancePosition(), SlotType::REGULAR);
    TEST_ASSERT(nearest != nullptr, "Nearest regular slot found");
    TEST_ASSERT(nearest->isAvailable(), "Nearest slot is available");

    // Slot selection
    bool selOk = lot.selectSlot("A-01");
    TEST_ASSERT(selOk, "Slot A-01 selected successfully");
    TEST_ASSERT(lot.getSelectedSlot() != nullptr, "getSelectedSlot() is not null");
    TEST_ASSERT(lot.getSelectedSlot()->getId() == "A-01", "Selected slot ID is A-01");
    lot.clearSelection();
    TEST_ASSERT(lot.getSelectedSlot() == nullptr, "clearSelection() resets selected slot");
}

void testSimulationAndReset() {
    std::cout << "\n--- TEST 6: Vehicle Entry, Navigation & Simulation Reset ---" << std::endl;
    ParkingLot lot;
    lot.generateDefaultLayout();
    lot.buildNavigationGraph();
    PathFinder pathFinder(&lot.getNavigationGraph());
    Simulation sim(&lot, &pathFinder);

    TEST_ASSERT(sim.getActiveVehicleCount() == 0, "Initial vehicle count is 0");
    TEST_ASSERT(sim.getState() == SimState::PAUSED, "Simulation initially paused");

    sim.play();
    TEST_ASSERT(sim.isPlaying(), "sim.play() sets running state");

    Vehicle* v = sim.spawnVehicle(SlotType::REGULAR, 15.0f);
    TEST_ASSERT(v != nullptr, "Vehicle spawned successfully");
    TEST_ASSERT(sim.getActiveVehicleCount() == 1, "Active vehicle count is now 1");
    TEST_ASSERT(v->getState() == VehicleState::NAVIGATING_TO_SLOT, "Spawned vehicle is navigating");

    // Step simulation forward
    sim.update(2.0f);
    TEST_ASSERT(sim.getSimulationTime() > 0.0f, "Simulation clock advanced");

    // Reset simulation
    sim.reset();
    TEST_ASSERT(sim.getActiveVehicleCount() == 0, "Vehicle count is 0 after reset");
    TEST_ASSERT(sim.getSimulationTime() == 0.0f, "Simulation time reset to 0.0s");
    TEST_ASSERT(!sim.isPlaying(), "Simulation paused after reset");
}

void testCameraControls() {
    std::cout << "\n--- TEST 7: Dual-Mode Camera Controls & Projections ---" << std::endl;
    Camera camera;

    TEST_ASSERT(camera.getMode() == CameraMode::MODE_3D_PERSPECTIVE, "Default camera mode is 3D");
    glm::mat4 proj3D = camera.getProjectionMatrix(16.0f / 9.0f);
    TEST_ASSERT(proj3D[3][3] == 0.0f, "3D projection has perspective divide (w != 1)");

    camera.setMode(CameraMode::MODE_2D_TOPDOWN);
    TEST_ASSERT(camera.getMode() == CameraMode::MODE_2D_TOPDOWN, "Camera mode set to 2D");
    glm::mat4 proj2D = camera.getProjectionMatrix(16.0f / 9.0f);
    TEST_ASSERT(proj2D[3][3] == 1.0f, "2D projection is orthographic (w == 1)");

    camera.processKeyboard('W', 0.1f);
    camera.processMouseScroll(2.0f);
    camera.reset();
    TEST_ASSERT(camera.getZoom() == 45.0f, "Camera reset restores default zoom");
}

void testFacilityRoutingAndNavigation() {
    std::cout << "\n--- TEST 8: Full Facility Routing & Lifecycle Verification ---" << std::endl;
    ParkingLot lot;
    lot.generateDefaultLayout();
    lot.buildNavigationGraph();

    PathFinder pathFinder(&lot.getNavigationGraph());

    // 1. Entrance to Nearest Slot
    ParkingSlot* nearest = lot.findNearestAvailableSlot(lot.getEntrancePosition(), SlotType::REGULAR);
    TEST_ASSERT(nearest != nullptr, "Nearest regular slot identified");
    Route routeNearest = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), nearest->getId());
    TEST_ASSERT(routeNearest.isValid, "Route from entrance to nearest slot is valid");
    TEST_ASSERT(routeNearest.totalDistance > 0.0f, "Nearest route has positive metric distance");
    TEST_ASSERT(routeNearest.waypoints.size() >= 3, "Nearest route has multiple waypoints");

    // 2. Entrance to Farthest Slot (D-15)
    Route routeFar = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), "D-15");
    TEST_ASSERT(routeFar.isValid, "Route from entrance to farthest slot (D-15) is valid");
    TEST_ASSERT(routeFar.totalDistance > routeNearest.totalDistance, "Farthest slot distance exceeds nearest slot distance");
    TEST_ASSERT(routeFar.turnCount >= 2, "Cross-facility route contains multiple turns");

    // 3. Multi-Row Traversal (Row A to Row D)
    const Node* nodeA = lot.getNavigationGraph().getNodeBySlotId("A-01");
    const Node* nodeD = lot.getNavigationGraph().getNodeBySlotId("D-15");
    TEST_ASSERT(nodeA != nullptr && nodeD != nullptr, "Row A and Row D nodes resolved in graph");
    Route routeAtoD = pathFinder.findShortestPathDijkstra(nodeA->index, nodeD->index);
    TEST_ASSERT(routeAtoD.isValid, "Route connecting Row A to Row D is valid");
    TEST_ASSERT(routeAtoD.turnCount >= 2, "Row A to Row D path executes required turns");

    // 4. Vehicle Full State Machine Lifecycle
    Simulation sim(&lot, &pathFinder);
    sim.play();
    Vehicle* v = sim.spawnVehicle(SlotType::REGULAR, 3.0f);
    TEST_ASSERT(v != nullptr, "Vehicle successfully spawned");
    TEST_ASSERT(v->getState() == VehicleState::NAVIGATING_TO_SLOT, "Vehicle starts in NAVIGATING_TO_SLOT state");

    // Advance simulation until parked
    for (int i = 0; i < 60 && v->getState() == VehicleState::NAVIGATING_TO_SLOT; ++i) {
        sim.update(0.5f);
    }
    TEST_ASSERT(v->getState() == VehicleState::PARKED, "Vehicle transitioned to PARKED at target bay");
    ParkingSlot* targetSlot = lot.getSlot(v->getTargetSlotId());
    TEST_ASSERT(targetSlot && targetSlot->getStatus() == SlotStatus::OCCUPIED, "Target bay status is OCCUPIED");

    // Advance simulation until departure initiated
    for (int i = 0; i < 40 && v->getState() == VehicleState::PARKED; ++i) {
        sim.update(0.5f);
    }
    TEST_ASSERT(v->getState() == VehicleState::UNPARKING || v->getState() == VehicleState::EXITING, "Vehicle transitioned to UNPARKING/EXITING");
}

void testPhase3AGraphicsAndEnvironment() {
    std::cout << "\n--- TEST 9: Phase 3A Graphics, Shadows & Environment ---" << std::endl;
    ParkingLot lot; // Automatically generates default layout & decorations

    // 1. Environmental Landscape Items
    const auto& landscape = lot.getLandscapeItems();
    TEST_ASSERT(!landscape.empty(), "Landscape items populated in facility");

    bool hasTreeSpherical = false;
    bool hasTreeConifer = false;
    bool hasTreeOrnamental = false;
    bool hasLampPost = false;
    bool hasSignPost = false;

    for (const auto& item : landscape) {
        if (item.type == LandscapeItem::Type::TREE_SPHERICAL) hasTreeSpherical = true;
        if (item.type == LandscapeItem::Type::TREE_CONIFER) hasTreeConifer = true;
        if (item.type == LandscapeItem::Type::TREE_ORNAMENTAL) hasTreeOrnamental = true;
        if (item.type == LandscapeItem::Type::LAMP_POST) hasLampPost = true;
        if (item.type == LandscapeItem::Type::SIGN_POST) hasSignPost = true;
    }

    TEST_ASSERT(hasTreeSpherical, "Environmental System includes Tree Type A (Spherical)");
    TEST_ASSERT(hasTreeConifer, "Environmental System includes Tree Type B (Conifer / Layered)");
    TEST_ASSERT(hasTreeOrnamental, "Environmental System includes Tree Type C (Ornamental)");
    TEST_ASSERT(hasLampPost, "Facility contains lighting poles with point lights");
    TEST_ASSERT(hasSignPost, "Facility contains parking signs (P, ENTRY, EXIT, EV, ACCESSIBLE)");

    // 2. Verify Trees do NOT obstruct parking slots (Part 7 safety)
    bool treeObstructsSlot = false;
    for (const auto& item : landscape) {
        if (item.type == LandscapeItem::Type::TREE_SPHERICAL ||
            item.type == LandscapeItem::Type::TREE_CONIFER ||
            item.type == LandscapeItem::Type::TREE_ORNAMENTAL) {
            for (const auto& slot : lot.getAllSlots()) {
                float dist = glm::distance(glm::vec2(item.position.x, item.position.z),
                                           glm::vec2(slot.getPosition().x, slot.getPosition().z));
                if (dist < 2.0f) { // Within slot clearance
                    treeObstructsSlot = true;
                    break;
                }
            }
        }
    }
    TEST_ASSERT(!treeObstructsSlot, "Trees are placed in valid safety zones (no slot obstruction)");

    // 3. Road Markings & Direction Arrows
    const auto& markings = lot.getAllRoadMarkings();
    TEST_ASSERT(!markings.empty(), "Facility includes generated road markings");

    bool hasCenterLines = false;
    bool hasStopLines = false;
    bool hasDirectionArrows = false;

    for (const auto& m : markings) {
        if (m.type == RoadMarking::Type::LANE_DIVIDER) hasCenterLines = true;
        if (m.type == RoadMarking::Type::STOP_LINE) hasStopLines = true;
        if (m.type == RoadMarking::Type::ARROW_FORWARD ||
            m.type == RoadMarking::Type::ARROW_LEFT ||
            m.type == RoadMarking::Type::ARROW_RIGHT) {
            hasDirectionArrows = true;
        }
    }

    TEST_ASSERT(hasCenterLines, "Road markings include center dashed dividing lines");
    TEST_ASSERT(hasStopLines, "Road markings include intersection stop lines");
    TEST_ASSERT(hasDirectionArrows, "Road markings include asphalt directional arrows");

    // 4. Shadow Map Configuration & Light-Space Math
    ShadowMap shadowMap;
    TEST_ASSERT(shadowMap.getResolution() == 2048, "Default shadow map resolution is 2048x2048");
    TEST_ASSERT(shadowMap.isEnabled(), "Shadow map enabled by default");

    glm::mat4 lightSpaceMat = shadowMap.getLightSpaceMatrix();
    // Validate that light-space matrix is non-zero and invertible
    float det = glm::determinant(lightSpaceMat);
    TEST_ASSERT(std::abs(det) > 1e-8f, "Light-space matrix is valid and invertible");
    TEST_ASSERT(glm::length(shadowMap.getLightDirection()) > 0.0f, "Sunlight has valid non-zero direction");
}

void testFloorAndPresentationDemo() {
    std::cout << "\n--- TEST 10: Multi-Floor Metadata, Camera Presets & Demo Mode ---" << std::endl;
    ParkingLot lot;
    lot.generateDefaultLayout();
    lot.buildNavigationGraph();

    // 1. Floor metadata
    const auto& slots = lot.getAllSlots();
    TEST_ASSERT(!slots.empty(), "Parking lot has slots");
    const ParkingSlot& s0 = slots[0];
    TEST_ASSERT(s0.getFloor() == 0, "Initial slot floor is 0 (Ground Level)");
    TEST_ASSERT(s0.getFloorString() == "Ground Level", "Initial slot floor string is Ground Level");
    TEST_ASSERT(!s0.isReserved(), "Regular slot is not reserved by default");

    ParkingSlot modifiableSlot = s0;
    modifiableSlot.setFloor(1);
    TEST_ASSERT(modifiableSlot.getFloor() == 1, "Slot floor can be set to 1");
    TEST_ASSERT(modifiableSlot.getFloorString() == "Level 1", "Slot floor string reports Level 1");

    // 2. Camera Viewpoint Presets
    Camera camera;
    camera.setPresetView(1);
    TEST_ASSERT(camera.getMode() == CameraMode::MODE_2D_TOPDOWN, "Preset 1 activates 2D Orthographic mode");

    camera.setPresetView(2);
    TEST_ASSERT(camera.getMode() == CameraMode::MODE_3D_PERSPECTIVE, "Preset 2 activates 3D Perspective mode");

    camera.setPresetView(3);
    TEST_ASSERT(camera.getPosition().y > 40.0f, "Preset 3 provides high-altitude facility overview");

    camera.setPresetView(4);
    TEST_ASSERT(camera.getMode() == CameraMode::MODE_3D_PERSPECTIVE, "Preset 4 provides Ground Floor / South view");

    camera.setPresetView(5);
    TEST_ASSERT(camera.getMode() == CameraMode::MODE_3D_PERSPECTIVE, "Preset 5 provides North Bays view");

    camera.setPresetView(6);
    TEST_ASSERT(camera.getMode() == CameraMode::MODE_3D_PERSPECTIVE, "Preset 6 provides Gate Corridor view");

    // 3. A* vs Dijkstra Route Equivalence
    PathFinder pathFinder(&lot.getNavigationGraph());
    Route dijkstraRoute = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), "C-08", false);
    Route aStarRoute = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), "C-08", true);
    TEST_ASSERT(dijkstraRoute.isValid, "Dijkstra finds valid route to C-08");
    TEST_ASSERT(aStarRoute.isValid, "A* finds valid route to C-08");
    TEST_ASSERT(std::abs(dijkstraRoute.totalDistance - aStarRoute.totalDistance) < 0.1f,
                "Dijkstra and A* yield identical optimal shortest path distance");

    // 4. Automated 14-Step Presentation Demonstration Mode
    Simulation sim(&lot, &pathFinder);
    sim.setCamera(&camera);

    TEST_ASSERT(!sim.isDemoActive(), "Demo is inactive initially");
    sim.startDemoMode();
    TEST_ASSERT(sim.isDemoActive(), "Demo mode is active after startDemoMode");
    TEST_ASSERT(sim.getDemoStep() == DemoStep::OVERVIEW_FACILITY, "Demo starts at Step: OVERVIEW_FACILITY");
    TEST_ASSERT(!sim.getDemoStepDescription().empty(), "Demo provides descriptive step text for viva presentation");

    // Advance simulation time to progress steps
    sim.update(3.5f);
    TEST_ASSERT(sim.getDemoStep() != DemoStep::OVERVIEW_FACILITY, "Demo automatically transitions to next step over time");

    sim.stopDemoMode();
    TEST_ASSERT(!sim.isDemoActive(), "Demo stops cleanly when requested");

    // 5. Edge cases: Search when all slots are occupied
    ParkingLot smallLot;
    smallLot.generateDefaultLayout();
    for (auto& s : smallLot.getAllSlots()) {
        const_cast<ParkingSlot&>(s).occupy("FILL");
    }
    ParkingSlot* noSlot = smallLot.findNearestAvailableSlot(smallLot.getEntrancePosition(), SlotType::REGULAR);
    TEST_ASSERT(noSlot == nullptr, "findNearestAvailableSlot gracefully returns nullptr when lot is 100% full");

    ParkingSlot* nonExistent = smallLot.getSlot("NON-EXISTENT-ID");
    TEST_ASSERT(nonExistent == nullptr, "getSlot returns nullptr for non-existent slot ID");
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " SMART PARKING SYSTEM - AUTOMATED UNIT TEST SUITE\n";
    std::cout << " Validating Algorithms, States, Graph, Simulation & Math\n";
    std::cout << "========================================================\n";

    testParkingSlotStates();
    testSlotReservation();
    testGraphAndShortestPath();
    testNoRouteScenario();
    testParkingLotAndSearch();
    testSimulationAndReset();
    testCameraControls();
    testFacilityRoutingAndNavigation();
    testPhase3AGraphicsAndEnvironment();
    testFloorAndPresentationDemo();

    std::cout << "\n========================================================" << std::endl;
    std::cout << " TEST SUMMARY: " << g_testsPassed << " PASSED, " 
              << g_testsFailed << " FAILED." << std::endl;
    std::cout << "========================================================" << std::endl;

    return g_testsFailed == 0 ? 0 : 1;
}
