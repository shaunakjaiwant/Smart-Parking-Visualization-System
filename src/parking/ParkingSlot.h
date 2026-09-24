#pragma once
#include <glm/glm.hpp>
#include <string>

/**
 * @file ParkingSlot.h
 * @brief Represents an individual parking bay within the facility.
 * 
 * WHAT: Stores slot metadata, dimensions, spatial coordinates, type, and real-time occupancy status.
 * WHY:  Core domain model for slot state tracking, visual color coding, and pathfinding destination.
 * HOW:  Provides state transition functions (occupy, free, reserve) and color-mapping helpers
 *       satisfying the 2D/3D visual state guidelines (Green=Available, Red=Occupied, etc.).
 */
namespace SmartParking {

enum class SlotType {
    REGULAR,
    EV_CHARGING,
    ACCESSIBLE,
    RESERVED
};

enum class SlotStatus {
    AVAILABLE,
    OCCUPIED,
    RESERVED,
    DISABLED,
    EV_CHARGING,
    SELECTED
};

class ParkingSlot {
public:
    ParkingSlot();
    ParkingSlot(const std::string& id,
                const std::string& rowId,
                SlotType type,
                const glm::vec3& position,
                const glm::vec2& dimensions,
                float rotationDeg = 0.0f);

    // Getters
    const std::string& getId() const { return m_id; }
    const std::string& getRowId() const { return m_rowId; }
    const std::string& getRow() const { return m_rowId; }
    int getFloor() const { return m_floor; }
    std::string getFloorString() const { return m_floor == 0 ? "Ground Level" : ("Level " + std::to_string(m_floor)); }
    SlotType getType() const { return m_type; }
    SlotStatus getStatus() const { return m_status; }
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec2& getDimensions() const { return m_dimensions; }
    float getRotationDeg() const { return m_rotationDeg; }
    const std::string& getVehicleId() const { return m_vehicleId; }
    float getDistanceFromEntrance() const { return m_distanceFromEntrance; }
    float getEstimatedWalkingDistance() const { return m_estimatedWalkingDistance; }
    bool isReserved() const { return m_status == SlotStatus::RESERVED; }

    // Setters & State transitions
    void setFloor(int floor) { m_floor = floor; }
    void setStatus(SlotStatus status);
    void setDistanceFromEntrance(float dist);
    void setEstimatedWalkingDistance(float dist);
    void setVehicleId(const std::string& vId) { m_vehicleId = vId; }

    bool occupy(const std::string& vehicleId);
    bool free();
    bool reserve();
    bool unreserve();
    void select(bool selected);

    bool isAvailable() const;
    bool isAvailableFor(SlotType requestedType) const;

    // Visual color representation
    glm::vec4 getStatusColor(bool isSelected = false) const;
    std::string getTypeString() const;
    std::string getStatusString() const;

    // Geometric intersection helper for mouse picking
    bool containsPoint2D(float worldX, float worldZ) const;

private:
    std::string m_id;
    std::string m_rowId;
    SlotType m_type = SlotType::REGULAR;
    SlotStatus m_status = SlotStatus::AVAILABLE;
    SlotStatus m_savedStatusBeforeSelect = SlotStatus::AVAILABLE;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec2 m_dimensions = glm::vec2(2.6f, 5.2f); // width, length in meters
    float m_rotationDeg = 0.0f;
    std::string m_vehicleId = "";
    float m_distanceFromEntrance = 0.0f;
    float m_estimatedWalkingDistance = 0.0f;
    int m_floor = 0; // 0 for Ground Level, 1 for Level 1
    bool m_isSelected = false;
};

} // namespace SmartParking
