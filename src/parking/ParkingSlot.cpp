#include "ParkingSlot.h"
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace SmartParking {

ParkingSlot::ParkingSlot() {
}

ParkingSlot::ParkingSlot(const std::string& id,
                         const std::string& rowId,
                         SlotType type,
                         const glm::vec3& position,
                         const glm::vec2& dimensions,
                         float rotationDeg)
    : m_id(id),
      m_rowId(rowId),
      m_type(type),
      m_position(position),
      m_dimensions(dimensions),
      m_rotationDeg(rotationDeg) {
    if (m_type == SlotType::RESERVED) {
        m_status = SlotStatus::RESERVED;
    } else {
        m_status = SlotStatus::AVAILABLE;
    }
    m_savedStatusBeforeSelect = m_status;
}

void ParkingSlot::setStatus(SlotStatus status) {
    m_status = status;
}

void ParkingSlot::setDistanceFromEntrance(float dist) {
    m_distanceFromEntrance = dist;
    m_estimatedWalkingDistance = dist * 1.15f; // Pedestrian path factor
}

void ParkingSlot::setEstimatedWalkingDistance(float dist) {
    m_estimatedWalkingDistance = dist;
}

bool ParkingSlot::occupy(const std::string& vehicleId) {
    if (m_status == SlotStatus::OCCUPIED) {
        return false;
    }
    m_vehicleId = vehicleId;
    if (m_type == SlotType::EV_CHARGING) {
        m_status = SlotStatus::EV_CHARGING;
    } else {
        m_status = SlotStatus::OCCUPIED;
    }
    m_savedStatusBeforeSelect = m_status;
    return true;
}

bool ParkingSlot::free() {
    m_vehicleId.clear();
    if (m_type == SlotType::RESERVED) {
        m_status = SlotStatus::RESERVED;
    } else {
        m_status = SlotStatus::AVAILABLE;
    }
    m_savedStatusBeforeSelect = m_status;
    return true;
}

bool ParkingSlot::reserve() {
    if (m_status == SlotStatus::OCCUPIED || m_status == SlotStatus::EV_CHARGING) {
        return false;
    }
    m_status = SlotStatus::RESERVED;
    m_savedStatusBeforeSelect = m_status;
    return true;
}

bool ParkingSlot::unreserve() {
    if (m_status == SlotStatus::RESERVED) {
        m_status = SlotStatus::AVAILABLE;
        m_savedStatusBeforeSelect = m_status;
        return true;
    }
    return false;
}

void ParkingSlot::select(bool selected) {
    m_isSelected = selected;
    if (selected) {
        m_savedStatusBeforeSelect = m_status;
        m_status = SlotStatus::SELECTED;
    } else {
        if (m_status == SlotStatus::SELECTED) {
            m_status = m_savedStatusBeforeSelect;
        }
    }
}

bool ParkingSlot::isAvailable() const {
    return (m_status == SlotStatus::AVAILABLE) || 
           (m_status == SlotStatus::SELECTED && m_savedStatusBeforeSelect == SlotStatus::AVAILABLE);
}

bool ParkingSlot::isAvailableFor(SlotType requestedType) const {
    if (!isAvailable()) return false;
    if (requestedType == SlotType::EV_CHARGING) {
        return m_type == SlotType::EV_CHARGING;
    }
    if (requestedType == SlotType::ACCESSIBLE) {
        return m_type == SlotType::ACCESSIBLE;
    }
    if (requestedType == SlotType::RESERVED) {
        return m_type == SlotType::RESERVED || m_type == SlotType::REGULAR;
    }
    return (m_type == SlotType::REGULAR || m_type == SlotType::ACCESSIBLE);
}

glm::vec4 ParkingSlot::getStatusColor(bool isSelected) const {
    if (isSelected || m_status == SlotStatus::SELECTED) {
        // Bright cyan selection highlight with subtle pulsing
        return glm::vec4(0.0f, 0.95f, 1.0f, 0.95f);
    }

    switch (m_status) {
        case SlotStatus::AVAILABLE:
            // High visibility emerald green
            if (m_type == SlotType::ACCESSIBLE) {
                // Purple tint for accessible
                return glm::vec4(0.65f, 0.25f, 0.85f, 0.9f);
            }
            if (m_type == SlotType::EV_CHARGING) {
                // Electric Cyan/Blue for EV charging
                return glm::vec4(0.1f, 0.6f, 0.95f, 0.9f);
            }
            return glm::vec4(0.15f, 0.82f, 0.35f, 0.9f);

        case SlotStatus::OCCUPIED:
            // Red for occupied
            return glm::vec4(0.9f, 0.2f, 0.2f, 0.9f);

        case SlotStatus::RESERVED:
            // Yellow for reserved
            return glm::vec4(0.95f, 0.8f, 0.15f, 0.9f);

        case SlotStatus::DISABLED:
            // Purple for disabled/accessible
            return glm::vec4(0.65f, 0.25f, 0.85f, 0.9f);

        case SlotStatus::EV_CHARGING:
            // Vibrant Blue for active EV charging
            return glm::vec4(0.1f, 0.6f, 0.95f, 0.9f);

        default:
            return glm::vec4(0.5f, 0.5f, 0.5f, 0.9f);
    }
}

std::string ParkingSlot::getTypeString() const {
    switch (m_type) {
        case SlotType::REGULAR: return "Regular";
        case SlotType::EV_CHARGING: return "EV Charging";
        case SlotType::ACCESSIBLE: return "Accessible (Disabled)";
        case SlotType::RESERVED: return "Reserved";
        default: return "Unknown";
    }
}

std::string ParkingSlot::getStatusString() const {
    switch (m_status) {
        case SlotStatus::AVAILABLE: return "AVAILABLE";
        case SlotStatus::OCCUPIED: return "OCCUPIED";
        case SlotStatus::RESERVED: return "RESERVED";
        case SlotStatus::DISABLED: return "DISABLED";
        case SlotStatus::EV_CHARGING: return "EV CHARGING";
        case SlotStatus::SELECTED: return "SELECTED";
        default: return "UNKNOWN";
    }
}

bool ParkingSlot::containsPoint2D(float worldX, float worldZ) const {
    // Inverse rotate point if slot is angled
    float dx = worldX - m_position.x;
    float dz = worldZ - m_position.z;

    if (std::abs(m_rotationDeg) > 0.01f) {
        float rad = glm::radians(-m_rotationDeg);
        float rx = dx * cos(rad) - dz * sin(rad);
        float rz = dx * sin(rad) + dz * cos(rad);
        dx = rx;
        dz = rz;
    }

    float halfW = m_dimensions.x * 0.5f;
    float halfL = m_dimensions.y * 0.5f;

    return (dx >= -halfW && dx <= halfW && dz >= -halfL && dz <= halfL);
}

} // namespace SmartParking
