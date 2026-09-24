#include "Simulation.h"
#include "../graphics/Camera.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <iostream>

namespace SmartParking {

Simulation::Simulation(ParkingLot* lot, PathFinder* pathFinder)
    : m_lot(lot),
      m_pathFinder(pathFinder) {
    // Seed initial occupancy history
    for (size_t i = 0; i < AnalyticsData::MAX_HISTORY_SAMPLES; ++i) {
        m_analytics.occupancyHistory.push_back(0.0f);
    }
}

void Simulation::togglePlayPause() {
    if (m_simState == SimState::RUNNING) {
        pause();
    } else {
        play();
    }
}

void Simulation::step() {
    m_simState = SimState::STEPPING;
    update(0.05f * m_speedMultiplier);
    m_simState = SimState::PAUSED;
}

void Simulation::reset() {
    m_simState = SimState::PAUSED;
    m_simulationTime = 0.0f;
    m_vehicles.clear();
    m_nextVehicleNumber = 101;
    m_isDemoActive = false;
    m_demoStep = DemoStep::IDLE;
    m_demoVehicle = nullptr;
    m_demoTargetSlotId.clear();

    if (m_lot) {
        for (auto& slot : const_cast<std::vector<ParkingSlot>&>(m_lot->getAllSlots())) {
            slot.free();
        }
        m_lot->clearSelection();
    }

    m_analytics = AnalyticsData();
    for (size_t i = 0; i < AnalyticsData::MAX_HISTORY_SAMPLES; ++i) {
        m_analytics.occupancyHistory.push_back(0.0f);
    }
    m_totalAccumulatedParkTime = 0.0f;
    m_totalParkedVehiclesCounted = 0;
    m_totalAccumulatedNavDistance = 0.0f;
    m_totalRoutedVehiclesCounted = 0;
}

std::string Simulation::getFormattedSimulationTime() const {
    int totalSec = static_cast<int>(m_simulationTime);
    int hrs = totalSec / 3600;
    int mins = (totalSec % 3600) / 60;
    int secs = totalSec % 60;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hrs << ":"
        << std::setfill('0') << std::setw(2) << mins << ":"
        << std::setfill('0') << std::setw(2) << secs;
    return oss.str();
}

void Simulation::update(float deltaTime) {
    if (m_simState == SimState::PAUSED) {
        return;
    }

    float effectiveDt = deltaTime * m_speedMultiplier;
    m_simulationTime += effectiveDt;

    // 1. Update active vehicles
    for (auto it = m_vehicles.begin(); it != m_vehicles.end();) {
        auto& vehicle = *it;
        VehicleState prevState = vehicle->getState();
        vehicle->update(effectiveDt);

        // State change detection: Arrived at slot and parked
        if (prevState == VehicleState::NAVIGATING_TO_SLOT && vehicle->getState() == VehicleState::PARKED) {
            ParkingSlot* slot = m_lot->getSlot(vehicle->getTargetSlotId());
            if (slot) {
                slot->occupy(vehicle->getId());
            }
        }

        // State change detection: Parked timeout reached -> initiate departure
        if (vehicle->getState() == VehicleState::PARKED && 
            vehicle->getTimeParked() >= vehicle->getParkingDurationTarget()) {
            
            // Calculate route from slot bay to facility exit
            Route exitRoute = m_pathFinder->findShortestPathDijkstra(
                m_lot->getNavigationGraph().getNodeBySlotId(vehicle->getTargetSlotId())->index,
                m_lot->getNavigationGraph().getNode(m_lot->getExitNodeId())->index
            );

            if (exitRoute.isValid) {
                // Free slot as vehicle begins exiting
                ParkingSlot* slot = m_lot->getSlot(vehicle->getTargetSlotId());
                if (slot) {
                    slot->free();
                }
                vehicle->assignExitRoute(exitRoute);
            }
        }

        // Vehicle finished exit journey
        if (vehicle->isCompleted()) {
            m_analytics.totalVehiclesServed++;
            m_totalAccumulatedParkTime += vehicle->getTimeParked();
            m_totalParkedVehiclesCounted++;

            it = m_vehicles.erase(it);
        } else {
            ++it;
        }
    }

    // 2. Demo mode state machine
    if (m_isDemoActive) {
        updateDemo(effectiveDt);
    }

    // 3. Update rolling analytics
    updateAnalytics(effectiveDt);
}

void Simulation::updateAnalytics(float dt) {
    m_analyticsSampleTimer += dt;
    if (m_analyticsSampleTimer >= 1.0f) {
        m_analyticsSampleTimer = 0.0f;

        FacilityStats stats = m_lot->calculateStats();
        m_analytics.currentOccupancyRate = stats.occupancyRate;
        if (stats.occupancyRate > m_analytics.peakOccupancyRate) {
            m_analytics.peakOccupancyRate = stats.occupancyRate;
        }

        m_analytics.occupancyHistory.pop_front();
        m_analytics.occupancyHistory.push_back(stats.occupancyRate);

        int inside = 0, entering = 0, exiting = 0;
        for (const auto& v : m_vehicles) {
            if (v->getState() == VehicleState::PARKED) inside++;
            else if (v->getState() == VehicleState::NAVIGATING_TO_SLOT) entering++;
            else if (v->getState() == VehicleState::EXITING || v->getState() == VehicleState::UNPARKING) exiting++;
        }
        m_analytics.vehiclesInsideCount = inside;
        m_analytics.vehiclesEnteringCount = entering;
        m_analytics.vehiclesExitingCount = exiting;

        if (m_totalParkedVehiclesCounted > 0) {
            m_analytics.averageParkingDurationSec = m_totalAccumulatedParkTime / m_totalParkedVehiclesCounted;
        }
        if (m_totalRoutedVehiclesCounted > 0) {
            m_analytics.averageNavigationDistanceMeters = m_totalAccumulatedNavDistance / m_totalRoutedVehiclesCounted;
        }
    }
}

Vehicle* Simulation::spawnVehicle(SlotType requestedType, float parkDuration) {
    if (!m_lot || !m_pathFinder) return nullptr;

    // Find suitable parking space
    ParkingSlot* bestSlot = m_lot->findNearestAvailableSlot(m_lot->getEntrancePosition(), requestedType);
    if (!bestSlot) {
        std::cout << "[Simulation] No available parking slot found for requested type." << std::endl;
        return nullptr;
    }

    // Calculate shortest route from entrance to slot
    Route route = m_pathFinder->calculateRouteToSlot(m_lot->getEntranceNodeId(), bestSlot->getId());
    if (!route.isValid) {
        std::cout << "[Simulation] Path calculation failed for slot " << bestSlot->getId() << std::endl;
        return nullptr;
    }

    // Create vehicle
    std::string vId = "V-" + std::to_string(m_nextVehicleNumber++);
    std::string plate = generateLicensePlate();
    glm::vec3 col = getRandomVehicleColor();

    auto vehicle = std::make_unique<Vehicle>(vId, plate, col, 8.5f);
    vehicle->assignRoute(route, bestSlot->getId(), parkDuration);

    // Reserve slot temporarily during transit
    bestSlot->reserve();

    // Track analytics
    m_totalAccumulatedNavDistance += route.totalDistance;
    m_totalRoutedVehiclesCounted++;

    Vehicle* ptr = vehicle.get();
    m_vehicles.push_back(std::move(vehicle));
    return ptr;
}

void Simulation::releaseParkedVehicle(const std::string& slotId) {
    for (auto& v : m_vehicles) {
        if (v->getTargetSlotId() == slotId && v->getState() == VehicleState::PARKED) {
            Route exitRoute = m_pathFinder->findShortestPathDijkstra(
                m_lot->getNavigationGraph().getNodeBySlotId(slotId)->index,
                m_lot->getNavigationGraph().getNode(m_lot->getExitNodeId())->index
            );

            if (exitRoute.isValid) {
                ParkingSlot* slot = m_lot->getSlot(slotId);
                if (slot) {
                    slot->free();
                }
                v->assignExitRoute(exitRoute);
            }
            break;
        }
    }
}

void Simulation::startDemoMode() {
    m_isDemoActive = true;
    m_demoStep = DemoStep::OVERVIEW_FACILITY;
    m_demoTimer = 0.0f;
    m_demoVehicle = nullptr;
    m_demoTargetSlotId.clear();
    m_simState = SimState::RUNNING;
}

void Simulation::stopDemoMode() {
    m_isDemoActive = false;
    m_demoStep = DemoStep::IDLE;
    m_demoVehicle = nullptr;
    if (m_camera) {
        m_camera->setPresetView(2); // Return to standard 3D view
    }
}

void Simulation::updateDemo(float dt) {
    m_demoTimer += dt;

    switch (m_demoStep) {
        case DemoStep::OVERVIEW_FACILITY:
            // 1. Overview of parking facility
            if (m_camera) m_camera->setPresetView(3);
            if (m_demoTimer >= 2.5f) {
                m_demoStep = DemoStep::MAP_2D_VIEW;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::MAP_2D_VIEW:
            // 2. 2D map transition
            if (m_camera) m_camera->setPresetView(1);
            if (m_demoTimer >= 2.5f) {
                m_demoStep = DemoStep::TRANSITION_3D;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::TRANSITION_3D:
            // 3. 3D perspective transition
            if (m_camera) m_camera->setPresetView(2);
            if (m_demoTimer >= 2.0f) {
                m_demoStep = DemoStep::SCAN_AVAILABLE_PARKING;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::SCAN_AVAILABLE_PARKING:
            // 4. Show available parking bays
            if (m_demoTimer >= 1.5f) {
                m_demoStep = DemoStep::SELECT_DESTINATION;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::SELECT_DESTINATION:
            // 5. Select target destination bay
            {
                ParkingSlot* nearest = m_lot->findNearestAvailableSlot(m_lot->getEntrancePosition(), SlotType::REGULAR);
                if (nearest) {
                    m_demoTargetSlotId = nearest->getId();
                    m_lot->selectSlot(m_demoTargetSlotId);
                    m_demoStep = DemoStep::CALCULATE_ROUTE;
                    m_demoTimer = 0.0f;
                } else {
                    stopDemoMode();
                }
            }
            break;

        case DemoStep::CALCULATE_ROUTE:
            // 6. Calculate Dijkstra / A* route & highlight ribbon
            if (m_demoTimer >= 1.5f) {
                m_demoStep = DemoStep::SPAWN_VEHICLE;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::SPAWN_VEHICLE:
            // 7. Spawn autonomous vehicle at entrance
            m_demoVehicle = spawnVehicle(SlotType::REGULAR, 7.0f);
            if (m_demoVehicle) {
                m_demoStep = DemoStep::NAVIGATE_ROUTE;
            } else {
                stopDemoMode();
            }
            m_demoTimer = 0.0f;
            break;

        case DemoStep::NAVIGATE_ROUTE:
            // 8. Vehicle follows route along driving corridors
            if (m_demoVehicle && m_demoVehicle->getState() == VehicleState::PARKED) {
                m_demoStep = DemoStep::PARK_MANEUVER;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::PARK_MANEUVER:
            // 9. Vehicle executes parking maneuver; slot becomes OCCUPIED
            if (m_demoTimer >= 2.5f) {
                m_demoStep = DemoStep::UPDATE_DASHBOARD;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::UPDATE_DASHBOARD:
            // 10. Dashboard & occupancy telemetry update
            if (m_demoTimer >= 2.0f) {
                m_demoStep = DemoStep::TRIGGER_EXIT;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::TRIGGER_EXIT:
            // 11. Vehicle departs & triggers unpark maneuver
            if (m_demoVehicle) {
                releaseParkedVehicle(m_demoVehicle->getTargetSlotId());
                m_demoStep = DemoStep::RELEASE_SLOT;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::RELEASE_SLOT:
            // 12. Slot becomes available as vehicle approaches exit
            if (m_demoTimer >= 6.0f) {
                m_demoStep = DemoStep::SHOW_GRAPH_DEBUG;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::SHOW_GRAPH_DEBUG:
            // 13. Show navigation graph debug visualization
            if (m_camera) m_camera->setPresetView(6); // View towards Entrance/Corridor
            if (m_demoTimer >= 2.5f) {
                m_demoStep = DemoStep::RETURN_OVERVIEW;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::RETURN_OVERVIEW:
            // 14. Return to default facility overview
            if (m_camera) m_camera->setPresetView(3);
            if (m_demoTimer >= 2.5f) {
                m_demoStep = DemoStep::FINISHED;
                m_demoTimer = 0.0f;
            }
            break;

        case DemoStep::FINISHED:
            // Demonstration cycle completed cleanly; ready for restart or loop
            if (m_demoTimer >= 2.0f) {
                m_demoStep = DemoStep::OVERVIEW_FACILITY;
                m_demoTimer = 0.0f;
            }
            break;

        default:
            break;
    }
}

std::string Simulation::getDemoStepDescription() const {
    switch (m_demoStep) {
        case DemoStep::IDLE: return "Demo Idle";
        case DemoStep::OVERVIEW_FACILITY: return "1/14: Overview of Parking Facility (Elevated View)";
        case DemoStep::MAP_2D_VIEW: return "2/14: 2D Orthographic Map Transition";
        case DemoStep::TRANSITION_3D: return "3/14: 3D Perspective Digital-Twin View";
        case DemoStep::SCAN_AVAILABLE_PARKING: return "4/14: Scanning 60 bays for real-time availability";
        case DemoStep::SELECT_DESTINATION: return "5/14: Selecting nearest vacant parking bay";
        case DemoStep::CALCULATE_ROUTE: return "6/14: Computing optimal path via Dijkstra/A* graph solver";
        case DemoStep::SPAWN_VEHICLE: return "7/14: Spawning autonomous vehicle at Entrance Gate";
        case DemoStep::NAVIGATE_ROUTE: return "8/14: Vehicle navigating road corridors & obeying turns";
        case DemoStep::PARK_MANEUVER: return "9/14: Executing parking maneuver into bay; Bay marked OCCUPIED";
        case DemoStep::UPDATE_DASHBOARD: return "10/14: Real-time telemetry & occupancy metrics updated";
        case DemoStep::TRIGGER_EXIT: return "11/14: Departure requested; Vehicle unparking into aisle";
        case DemoStep::RELEASE_SLOT: return "12/14: Vehicle en route to Exit; Bay restored to AVAILABLE";
        case DemoStep::SHOW_GRAPH_DEBUG: return "13/14: Navigation graph nodes, edges & turns demonstrated";
        case DemoStep::RETURN_OVERVIEW: return "14/14: Returning to facility master perspective";
        case DemoStep::FINISHED: return "Demonstration cycle complete; restarting walkthrough";
        default: return "";
    }
}

glm::vec3 Simulation::getRandomVehicleColor() {
    static const std::vector<glm::vec3> palette = {
        {0.85f, 0.15f, 0.15f}, // Crimson Red
        {0.15f, 0.45f, 0.85f}, // Sapphire Blue
        {0.2f, 0.7f, 0.4f},   // Forest Emerald
        {0.9f, 0.8f, 0.2f},   // Solar Yellow
        {0.88f, 0.88f, 0.9f}, // Silver Pearl
        {0.18f, 0.18f, 0.2f}, // Obsidian Charcoal
        {0.75f, 0.45f, 0.15f} // Sunset Copper
    };
    static std::mt19937 rng(1337);
    std::uniform_int_distribution<size_t> dist(0, palette.size() - 1);
    return palette[dist(rng)];
}

std::string Simulation::generateLicensePlate() {
    static std::mt19937 rng(4242);
    std::uniform_int_distribution<int> numDist(1000, 9999);
    return "KA-04-SP-" + std::to_string(numDist(rng));
}

} // namespace SmartParking
