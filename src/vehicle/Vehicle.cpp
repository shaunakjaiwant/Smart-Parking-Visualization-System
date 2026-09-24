#include "Vehicle.h"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace SmartParking {

static float normalizeAngleDeg(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

Vehicle::Vehicle(const std::string& id,
                 const std::string& licensePlate,
                 const glm::vec3& color,
                 float speed)
    : m_id(id),
      m_licensePlate(licensePlate),
      m_color(color),
      m_speed(speed) {
}

void Vehicle::assignRoute(const Route& route, const std::string& targetSlotId, float parkingDuration) {
    m_currentRoute = route;
    m_targetSlotId = targetSlotId;
    m_parkingDurationTarget = parkingDuration;
    m_timeParked = 0.0f;
    m_currentWaypointIdx = 0;

    if (route.isValid && !route.waypoints.empty()) {
        m_position = route.waypoints[0];
        m_state = VehicleState::NAVIGATING_TO_SLOT;

        if (route.waypoints.size() > 1) {
            glm::vec3 dir = route.waypoints[1] - route.waypoints[0];
            m_headingDeg = glm::degrees(std::atan2(dir.x, dir.z));
            m_targetHeadingDeg = m_headingDeg;
        }
    } else {
        m_state = VehicleState::IDLE;
    }
}

void Vehicle::assignExitRoute(const Route& exitRoute) {
    m_currentRoute = exitRoute;
    m_currentWaypointIdx = 0;
    m_state = VehicleState::UNPARKING;
    m_maneuverTimer = 0.0f;

    if (exitRoute.isValid && exitRoute.waypoints.size() > 1) {
        m_maneuverStartPos = m_position;
        m_maneuverEndPos = exitRoute.waypoints[0];
    }
}

void Vehicle::update(float deltaTime) {
    if (m_state == VehicleState::IDLE || m_state == VehicleState::COMPLETED) {
        return;
    }

    // Smooth heading angle interpolation
    float diff = normalizeAngleDeg(m_targetHeadingDeg - m_headingDeg);
    m_headingDeg += diff * std::clamp(deltaTime * 8.0f, 0.0f, 1.0f);
    m_headingDeg = normalizeAngleDeg(m_headingDeg);

    switch (m_state) {
        case VehicleState::ENTERING:
        case VehicleState::NAVIGATING_TO_SLOT:
            advanceWaypointNavigation(deltaTime);
            break;

        case VehicleState::PARKING:
            // Smoothly ease into final slot position
            updateManeuver(deltaTime, VehicleState::PARKED, 1.5f);
            break;

        case VehicleState::PARKED:
            m_timeParked += deltaTime;
            break;

        case VehicleState::UNPARKING:
            // Smoothly back out or pull onto lane
            updateManeuver(deltaTime, VehicleState::EXITING, 1.5f);
            break;

        case VehicleState::EXITING:
            advanceWaypointNavigation(deltaTime);
            break;

        default:
            break;
    }
}

void Vehicle::advanceWaypointNavigation(float deltaTime) {
    if (!m_currentRoute.isValid || m_currentRoute.waypoints.empty()) {
        return;
    }

    if (m_currentWaypointIdx >= m_currentRoute.waypoints.size()) {
        // Reached end of path
        if (m_state == VehicleState::NAVIGATING_TO_SLOT) {
            m_state = VehicleState::PARKED;
        } else if (m_state == VehicleState::EXITING) {
            m_state = VehicleState::COMPLETED;
        }
        return;
    }

    const glm::vec3& targetWP = m_currentRoute.waypoints[m_currentWaypointIdx];
    glm::vec3 toTarget = targetWP - m_position;
    toTarget.y = 0.0f; // horizontal 2D plane movement

    float dist = glm::length(toTarget);
    float waypointThreshold = 0.6f;

    if (dist <= waypointThreshold) {
        // Waypoint reached
        m_currentWaypointIdx++;
        if (m_currentWaypointIdx >= m_currentRoute.waypoints.size()) {
            if (m_state == VehicleState::NAVIGATING_TO_SLOT) {
                m_state = VehicleState::PARKED;
            } else if (m_state == VehicleState::EXITING) {
                m_state = VehicleState::COMPLETED;
            }
            return;
        }
    } else {
        // Move towards waypoint
        glm::vec3 moveDir = glm::normalize(toTarget);
        m_targetHeadingDeg = glm::degrees(std::atan2(moveDir.x, moveDir.z));

        float step = m_speed * deltaTime;
        if (step > dist) {
            m_position = targetWP;
        } else {
            m_position += moveDir * step;
        }
    }
}

void Vehicle::updateManeuver(float deltaTime, VehicleState nextState, float duration) {
    m_maneuverTimer += deltaTime;
    float t = std::clamp(m_maneuverTimer / duration, 0.0f, 1.0f);

    // Smoothstep interpolation
    float smoothT = t * t * (3.0f - 2.0f * t);
    m_position = glm::mix(m_maneuverStartPos, m_maneuverEndPos, smoothT);

    if (t >= 1.0f) {
        m_state = nextState;
        m_maneuverTimer = 0.0f;
    }
}

std::string Vehicle::getStateString() const {
    switch (m_state) {
        case VehicleState::IDLE: return "Idle";
        case VehicleState::ENTERING: return "Entering";
        case VehicleState::NAVIGATING_TO_SLOT: return "Navigating to Slot";
        case VehicleState::PARKING: return "Parking Maneuver";
        case VehicleState::PARKED: return "Parked";
        case VehicleState::UNPARKING: return "Unparking";
        case VehicleState::EXITING: return "Exiting";
        case VehicleState::COMPLETED: return "Completed";
        default: return "Unknown";
    }
}

} // namespace SmartParking
