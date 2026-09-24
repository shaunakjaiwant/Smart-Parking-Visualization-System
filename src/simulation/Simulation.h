#pragma once
#include "../parking/ParkingLot.h"
#include "../vehicle/Vehicle.h"
#include "../navigation/PathFinder.h"
#include <vector>
#include <memory>
#include <deque>
#include <algorithm>

/**
 * @file Simulation.h
 * @brief Discrete-event and continuous vehicle traffic simulation engine.
 * 
 * WHAT: Coordinates vehicle arrivals, slot allocation, route assignment, parking timers, and departures.
 * WHY:  Fulfills real-time smart parking operational simulation and demo mode requirements.
 * HOW:  Updates vehicle positions via deltaTime * speedMultiplier, monitors slot states,
 *       gathers rolling analytical statistics, and drives the sequential Demonstration Script.
 */
namespace SmartParking {

class Camera;

enum class SimState {
    PAUSED,
    RUNNING,
    STEPPING
};

enum class DemoStep {
    IDLE,
    OVERVIEW_FACILITY,      // 1. Overview of parking facility
    MAP_2D_VIEW,            // 2. 2D map transition
    TRANSITION_3D,          // 3. 3D perspective transition
    SCAN_AVAILABLE_PARKING, // 4. Scan & highlight available parking
    SELECT_DESTINATION,     // 5. Select target destination bay
    CALCULATE_ROUTE,        // 6. Calculate Dijkstra / A* route
    SPAWN_VEHICLE,          // 7. Spawn autonomous vehicle
    NAVIGATE_ROUTE,         // 8. Vehicle follows route along lanes
    PARK_MANEUVER,          // 9. Vehicle executes parking maneuver
    UPDATE_DASHBOARD,       // 10. Dashboard & occupancy update
    TRIGGER_EXIT,           // 11. Vehicle departs & triggers unpark
    RELEASE_SLOT,           // 12. Vehicle exits, slot becomes available
    SHOW_GRAPH_DEBUG,       // 13. Show navigation graph debug overlay
    RETURN_OVERVIEW,        // 14. Return to default facility overview
    FINISHED
};

struct AnalyticsData {
    int totalVehiclesServed = 0;
    float averageParkingDurationSec = 0.0f;
    float averageNavigationDistanceMeters = 0.0f;
    float peakOccupancyRate = 0.0f;
    float currentOccupancyRate = 0.0f;
    int vehiclesInsideCount = 0;
    int vehiclesEnteringCount = 0;
    int vehiclesExitingCount = 0;

    std::deque<float> occupancyHistory; // Last 60 data samples for graph plotting
    static constexpr size_t MAX_HISTORY_SAMPLES = 60;
};

class Simulation {
public:
    Simulation(ParkingLot* lot, PathFinder* pathFinder);
    ~Simulation() = default;

    void update(float deltaTime);

    // Simulation controls
    void play() { m_simState = SimState::RUNNING; }
    void pause() { m_simState = SimState::PAUSED; }
    void togglePlayPause();
    void step();
    void reset();
    void setSpeedMultiplier(float mult) { m_speedMultiplier = std::clamp(mult, 0.25f, 4.0f); }

    // Status queries
    SimState getState() const { return m_simState; }
    bool isPlaying() const { return m_simState == SimState::RUNNING; }
    float getSpeedMultiplier() const { return m_speedMultiplier; }
    float getSimulationTime() const { return m_simulationTime; }
    std::string getFormattedSimulationTime() const;

    // Vehicle management
    Vehicle* spawnVehicle(SlotType requestedType = SlotType::REGULAR, float parkDuration = 25.0f);
    void releaseParkedVehicle(const std::string& slotId);
    const std::vector<std::unique_ptr<Vehicle>>& getVehicles() const { return m_vehicles; }
    size_t getActiveVehicleCount() const { return m_vehicles.size(); }

    // Demonstration Mode & Camera Automation
    void setCamera(Camera* camera) { m_camera = camera; }
    void startDemoMode();
    void stopDemoMode();
    bool isDemoActive() const { return m_isDemoActive; }
    DemoStep getDemoStep() const { return m_demoStep; }
    std::string getDemoStepDescription() const;

    // Analytics
    const AnalyticsData& getAnalytics() const { return m_analytics; }

private:
    ParkingLot* m_lot = nullptr;
    PathFinder* m_pathFinder = nullptr;
    Camera* m_camera = nullptr;

    SimState m_simState = SimState::PAUSED;
    float m_speedMultiplier = 1.0f;
    float m_simulationTime = 0.0f;

    std::vector<std::unique_ptr<Vehicle>> m_vehicles;
    int m_nextVehicleNumber = 101;

    // Auto traffic generator
    bool m_autoTrafficEnabled = false;
    float m_autoSpawnTimer = 0.0f;
    float m_autoSpawnInterval = 10.0f;

    // Analytics & Metrics
    AnalyticsData m_analytics;
    float m_analyticsSampleTimer = 0.0f;
    float m_totalAccumulatedParkTime = 0.0f;
    int m_totalParkedVehiclesCounted = 0;
    float m_totalAccumulatedNavDistance = 0.0f;
    int m_totalRoutedVehiclesCounted = 0;

    // Demo Mode state machine
    bool m_isDemoActive = false;
    DemoStep m_demoStep = DemoStep::IDLE;
    float m_demoTimer = 0.0f;
    Vehicle* m_demoVehicle = nullptr;
    std::string m_demoTargetSlotId = "";

    void updateDemo(float dt);
    void updateAnalytics(float dt);
    glm::vec3 getRandomVehicleColor();
    std::string generateLicensePlate();
};

} // namespace SmartParking
