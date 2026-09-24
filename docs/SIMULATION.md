# Smart Parking Simulation & Dynamic Traffic Engine

**Project Title:** Interactive 2D/3D Smart Parking Visualization and Navigation System  
**Module:** `SmartParking::Simulation`, `SmartParking::Vehicle`, `SmartParking::AnalyticsData`  
**Source Location:** `src/simulation/`

---

## 1. Vehicle Finite State Machine (FSM)

Each simulated vehicle operates as an autonomous agent executing an 8-state deterministic finite state machine (`Vehicle.h`):

```
       [ENTERING]
           │
           ▼
      [SEARCHING]  ──(No bay available)──> [EXITING]
           │
           ▼ (Bay allocated & reserved)
       [ROUTING]
           │
           ▼ (Waypoints compiled)
       [DRIVING]
           │
           ▼ (Arrived at slot boundary)
       [PARKING]
           │
           ▼ (Reaches parking center)
        [PARKED] ──(Duration expired / Release clicked)
           │
           ▼
       [EXITING]
           │
           ▼ (Reaches Exit Gate)
      [COMPLETED]
```

### State Descriptions
1. **ENTERING:** Vehicle spawns at Entrance Gate $(X = 0, Y = 0, Z = -42)$. Headlights turn on.
2. **SEARCHING:** Queries `ParkingLot::findNearestAvailableSlot` matching vehicle propulsion/accessibility requirements (`REGULAR`, `EV_CHARGING`, `ACCESSIBLE`).
3. **ROUTING:** `PathFinder` computes shortest path waypoints from Entrance to the assigned slot.
4. **DRIVING:** Vehicle travels along lane waypoints at cruising speed ($8.0\,\text{m/s}$). Heading orientation is updated via `atan2(dx, dz)`.
5. **PARKING:** Enters parking bay at reduced maneuver speed ($1.5\,\text{m/s}$), aligning with bay orientation.
6. **PARKED:** Vehicle engine cuts off; bay status updates to `OCCUPIED`. Remains parked for designated duration.
7. **EXITING:** Reverses out of bay into aisle, calculates path to Exit Gate $(X = 0, Y = 0, Z = 42)$, and drives out.
8. **COMPLETED:** Vehicle reaches Exit Gate, releases memory, and is deleted from the active fleet.

---

## 2. Animation & Motion Interpolation

### 2.1 Frame-Rate-Independent Physics
All motion integration uses delta-time $\Delta t$:
$$\mathbf{p}(t + \Delta t) = \mathbf{p}(t) + \mathbf{v} \cdot \Delta t$$
Cruising speed accelerates smoothly to target velocity using linear interpolation:
$$v_{\text{current}} = \text{lerp}(v_{\text{current}}, v_{\text{target}}, \text{acceleration} \cdot \Delta t)$$

### 2.2 Heading & Rotation Smoothness
The vehicle yaw orientation $\theta_{\text{yaw}}$ is computed from travel displacement vector $(dx, dz)$:
$$\theta_{\text{target}} = \text{atan2}(dx, dz)$$
To avoid instantaneous snapping when taking 90-degree turns at intersections, yaw angles are interpolated smoothly around the $Y$-axis:
$$\theta_{\text{yaw}} = \text{lerpAngle}(\theta_{\text{yaw}}, \theta_{\text{target}}, \omega \cdot \Delta t)$$

---

## 3. Simulation Controls & Time Warp

The simulation engine supports interactive time controls:
- **Play / Pause (`Space` / `P`):** Toggles physics update loop.
- **Single Step:** Advances simulation by exactly $\Delta t = 0.05\,\text{s}$ for fine-grained debugging.
- **Simulation Speed:** Dynamic multipliers:
  - `0.25x` (Quarter-speed slow motion for observing parking maneuvers)
  - `0.5x` (Half-speed)
  - `1.0x` (Real-time baseline)
  - `2.0x` (Double speed)
  - `4.0x` (Fast-forward traffic flow)
- **Reset:** Restores clean initial state: despawns all vehicles, frees all bays, and resets metrics to zero.

---

## 4. 14-Step Automated Presentation Demonstration Mode

Implemented to facilitate seamless, hands-free faculty viva demonstrations:
1. **Facility Overview:** High-altitude vantage point showing full facility bounds.
2. **2D Map View:** Smooth switch to 2D Orthographic blueprint.
3. **3D Digital Twin:** Perspective transition showcasing 3D depth and shadows.
4. **Scan Available Parking:** Highlights bays by type (Green = Available, Blue = EV, Purple = Accessible).
5. **Select Destination Bay:** Programmatically selects target slot.
6. **Compute Navigation Route:** Generates Dijkstra / A* route with turn breakdown.
7. **Spawn Autonomous Vehicle:** Enters through entrance gate.
8. **Follow Route:** Vehicle navigates along designated driving aisles.
9. **Execute Parking Maneuver:** Vehicle turns smoothly into the parking bay.
10. **Update Dashboard:** Bay turns Red (Occupied); occupancy rate increments.
11. **Trigger Departure:** Vehicle backs out and initiates exit route.
12. **Release Bay:** Vehicle reaches exit gate; bay status returns to Available.
13. **Show Debug Graph:** Enables F3 graph overlay displaying nodes and edges.
14. **Return to Overview:** Smooth camera interpolation back to original overview.

---

## 5. Facility Telemetry & Analytics

The `AnalyticsData` structure tracks real-time operations:
- **Total Vehicles Served:** Monotonically increasing throughput counter.
- **Current & Peak Occupancy:** Percentage of occupied slots ($0.0\%$ to $100.0\%$).
- **Rolling Occupancy History:** 60-second deque rendered as an interactive sparkline via `ImGui::PlotLines`.
- **Average Parking Duration:** Mean dwell time per vehicle in seconds.
- **Average Navigation Distance:** Metric travel distance accumulated across all completed trips.
