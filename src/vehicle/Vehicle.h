#pragma once
#include "../navigation/PathFinder.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

/**
 * @file Vehicle.h
 * @brief Represents an autonomous simulated vehicle navigating the parking facility.
 * 
 * WHAT: Manages vehicle transform, waypoints, navigation state, parking duration, and animation.
 * WHY:  Fulfills vehicle simulation, path following, parking maneuver, and departure requirements.
 * HOW:  Interpolates positions along Route waypoints, calculates steering heading angle (atan2),
 *       tracks parking timers, and transitions between ENTERING, NAVIGATING, PARKED, and EXITING.
 */
namespace SmartParking {

enum class VehicleState {
    IDLE,
    ENTERING,
    NAVIGATING_TO_SLOT,
    PARKING,
    PARKED,
    UNPARKING,
    EXITING,
    COMPLETED
};

class Vehicle {
public:
    Vehicle(const std::string& id,
            const std::string& licensePlate,
            const glm::vec3& color,
            float speed = 8.0f);

    // Getters
    const std::string& getId() const { return m_id; }
    const std::string& getLicensePlate() const { return m_licensePlate; }
    const glm::vec3& getColor() const { return m_color; }
    const glm::vec3& getPosition() const { return m_position; }
    float getHeadingDeg() const { return m_headingDeg; }
    float getSpeed() const { return m_speed; }
    VehicleState getState() const { return m_state; }
    const std::string& getTargetSlotId() const { return m_targetSlotId; }
    float getTimeParked() const { return m_timeParked; }
    float getParkingDurationTarget() const { return m_parkingDurationTarget; }
    const Route& getCurrentRoute() const { return m_currentRoute; }
    size_t getCurrentWaypointIndex() const { return m_currentWaypointIdx; }
    bool isCompleted() const { return m_state == VehicleState::COMPLETED; }

    // Navigation and state control
    void assignRoute(const Route& route, const std::string& targetSlotId, float parkingDuration = 30.0f);
    void assignExitRoute(const Route& exitRoute);
    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setHeadingDeg(float heading) { m_headingDeg = heading; }
    void setSpeed(float speed) { m_speed = speed; }
    void setState(VehicleState state) { m_state = state; }

    // Simulation update
    void update(float deltaTime);

    std::string getStateString() const;

private:
    std::string m_id;
    std::string m_licensePlate;
    glm::vec3 m_color;
    glm::vec3 m_position = glm::vec3(0.0f);
    float m_headingDeg = 0.0f;
    float m_targetHeadingDeg = 0.0f;
    float m_speed = 8.0f;

    VehicleState m_state = VehicleState::IDLE;
    std::string m_targetSlotId = "";
    Route m_currentRoute;
    size_t m_currentWaypointIdx = 0;

    float m_timeParked = 0.0f;
    float m_parkingDurationTarget = 30.0f;
    float m_maneuverTimer = 0.0f;
    glm::vec3 m_maneuverStartPos = glm::vec3(0.0f);
    glm::vec3 m_maneuverEndPos = glm::vec3(0.0f);

    void advanceWaypointNavigation(float deltaTime);
    void updateManeuver(float deltaTime, VehicleState nextState, float duration);
};

} // namespace SmartParking
