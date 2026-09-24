# SMART PARKING: Interactive 2D/3D Smart Parking Visualization and Navigation System

**Academic Domain:** Computer Graphics and Visualization  
**Technology Stack:** C++17, Modern OpenGL (Core Profile 3.3), GLFW 3.4, GLAD, GLM, Dear ImGui, nlohmann/json  
**Target Platform:** Windows (MSVC 2019 / CMake), Linux (GCC/Clang / CMake)

---

## 1. Abstract
The **Interactive 2D/3D Smart Parking Visualization and Navigation System** is an engineering graphics simulation designed to solve the challenges of urban parking management. Urban vehicular congestion frequently stems from inefficient parking spot searches ("cruising for parking"). This application visually models a multi-row facility featuring 60 individually monitored bays (Regular, EV Charging, Accessible/Disabled, and Reserved), offers instantaneous real-time slot occupancy tracking, intelligent slot recommendations, Dijkstra/A* turn-by-turn navigation paths, and continuous autonomous vehicle movement animations in both 2D top-down orthographic and 3D perspective graphics pipelines.

---

## 2. Problem Statement
Traditional parking garages suffer from:
1. **Inefficient Navigation:** Drivers lack visibility of vacant slots upon entry, causing traffic bottlenecks in access aisles.
2. **Specialized Vehicle Bottlenecks:** Electric vehicles (EVs) and drivers requiring accessible spaces lack directional guidance to designated infrastructure.
3. **Absence of Visual Telemetry:** Operators lack unified 2D spatial layouts and immersive 3D situational awareness.
4. **Algorithmic Disconnect:** Most management software does not expose the underlying pathfinding graph and geometric transformation pipelines for educational inspection.

---

## 3. Key Objectives
- **Computer Graphics Rigor:** Demonstrate 2D/3D transformations, model-view-projection (MVP) matrices, Blinn-Phong illumination, procedural geometry generation (VBO/VAO/EBO), and screen-to-world mouse raycasting.
- **Graph & Algorithm Engineering:** Implement Dijkstra's algorithm and A* heuristic pathfinding on an adjacency-list navigation graph.
- **Smart City Automation:** Continuous state synchronization (Available, Occupied, Reserved, EV Charging, Disabled, Selected), vehicle animation with steering heading interpolation, and automated demonstration mode.
- **Professional User Experience:** Responsive Dark Charcoal UI dashboard built with Dear ImGui, delivering metrics, rolling analytics, and a developer debug HUD (F3).

---

## 4. Features

### A. Dual 2D/3D Visualization & Real-Time Shadow Mapping
- **2D Top-Down Mode:** Pure orthographic projection ($w = 1$), crisp color-coded slot bays, dashed road lane markings, pedestrian crossings, direction arrows, and 2D vehicle outlines with headlights. Shadows are bypassed for optimal framerate and administrative clarity.
- **3D Perspective Mode:** Dynamic camera flight ($FOV = 45^\circ$), two-pass real-time shadow mapping (2048x2048 Depth FBO with 3x3 PCF filter), Blinn-Phong directional sunlight, ambient and specular reflections, 6 dynamic point lights for lampposts, multi-part 3D vehicle models (chassis, cabin, 4 cylinder wheels, emissive headlights/taillights), 3D foliage trees (Spherical, Conifer, Ornamental), signposts, concrete curbs, and an elevated pulsing neon route ribbon.
- **Day / Night Lighting Modes:** Toggle between radiant daylight (sunlight + soft shadows) and atmospheric nighttime (cool moonlight + illuminated streetlamp point lights). Toggle via `L`, `Ctrl+D` (or `D` in 2D), or the UI top bar.

### B. Facility Layout & Topology
- **60 Configured Bays:**
  - **Row A:** 4 Accessible (Purple), 4 EV Charging (Cyan), 7 Regular (Emerald Green)
  - **Row B:** 4 Reserved (Yellow), 11 Regular (Green)
  - **Row C:** 15 Regular (Green)
  - **Row D:** 2 EV Charging (Cyan), 13 Regular (Green)
- Configurable via `config/parking_layout.json` with automatic procedural fallback.

### C. Navigation System & Shortest Path
- Spatial graph composed of 74 nodes (Entrance, Intersections, Aisle Waypoints, Slot Access points, Slot Bays, Exit).
- **Dijkstra's Algorithm:** Calculates the absolute lowest metric cost path with turn angle detection (dot product of adjacent vectors $> 35^\circ$) and estimated driving times at 15 km/h.
- Visual turn-by-turn step cards (e.g. *Start at Main Entrance -> Turn Left at West North Junction -> Access A-05 -> Arrive at Bay A-05*).
- **Asphalt Directional Arrows:** Dynamic arrows rendered on driving lanes, lighting up in glowing neon along the active route.

### D. Vehicle Simulation Engine
- Discrete and continuous simulation controller with Play, Pause, Step, Reset, and Speed multipliers ($0.25\times, 0.5\times, 1.0\times, 2.0\times, 4.0\times$).
- Vehicles smoothly traverse waypoints, calculate heading $\theta = \text{atan2}(\Delta x, \Delta z)$, execute parking entry maneuvers, stay parked for randomized durations, and navigate to the Exit Gate when departing.
- **Presentation Demo Mode:** Automatic 6-step walkthrough for viva presentations (Spawn $\rightarrow$ Detect $\rightarrow$ Route $\rightarrow$ Navigate $\rightarrow$ Park $\rightarrow$ Exit).

### E. Dashboard & Real-Time Analytics
- Dynamic statistics cards: Total Slots, Available, Occupied, Reserved, EV, Accessible, Occupancy Percentage.
- Real-time rolling graph of facility occupancy over the last 60 seconds (`ImGui::PlotLines`).
- Vehicle counters: Entering, Stationary Parked, and Exiting.
- Search panel: Filter by ID, type, and availability.
- **Shadow Map Depth Viewer:** Live debug window rendering the 2048x2048 depth texture directly from the shadow FBO.

---

## 5. Technology Stack
| Layer | Technology | Purpose |
|---|---|---|
| **Language** | C++17 | Core simulation, graph logic, memory management |
| **Graphics API** | Modern OpenGL 3.3 (Core Profile) | Hardware-accelerated 2D and 3D rendering |
| **Window & Input** | GLFW 3.4 | Window lifecycle, event dispatch, context creation |
| **OpenGL Loader** | GLAD | OpenGL function pointer initialization |
| **Mathematics** | GLM 1.0.1 | Vectors, matrices (mat4, vec3, vec4), projections |
| **UI Framework** | Dear ImGui v1.91.8 | Dark-themed dashboard, controls, analytics, HUD |
| **Serialization** | nlohmann/json | Parsing `config/parking_layout.json` |
| **Build System** | CMake 3.16+ / MSVC 2019 / GCC | Cross-platform build orchestration |

---

## 6. Directory Structure
```
SmartParking/
├── assets/
│   ├── fonts/
│   ├── models/
│   └── textures/
├── config/
│   └── parking_layout.json
├── docs/
│   ├── ARCHITECTURE.md
│   ├── ALGORITHM.md
│   ├── FINAL_IMPLEMENTATION_STATUS.md
│   ├── GRAPHICS_PIPELINE.md
│   ├── NAVIGATION.md
│   ├── SIMULATION.md
│   ├── TESTING.md
│   └── VIVA_GUIDE.md
├── external/
│   ├── glad/
│   ├── glfw/
│   ├── glm/
│   ├── imgui/
│   └── nlohmann/
├── shaders/
│   ├── basic.vert / basic.frag
│   ├── lighting.vert / lighting.frag
│   └── shadow_depth.vert / shadow_depth.frag
├── src/
│   ├── application/
│   │   ├── Application.h / Application.cpp
│   ├── graphics/
│   │   ├── Camera.h / Camera.cpp
│   │   ├── Mesh.h / Mesh.cpp
│   │   ├── Renderer.h / Renderer.cpp
│   │   ├── Shader.h / Shader.cpp
│   │   └── ShadowMap.h / ShadowMap.cpp
│   ├── navigation/
│   │   ├── Graph.h / Graph.cpp
│   │   └── PathFinder.h / PathFinder.cpp
│   ├── parking/
│   │   ├── ParkingLot.h / ParkingLot.cpp
│   │   └── ParkingSlot.h / ParkingSlot.cpp
│   ├── simulation/
│   │   ├── Simulation.h / Simulation.cpp
│   │   └── Vehicle.h / Vehicle.cpp
│   ├── ui/
│   │   ├── UIManager.h / UIManager.cpp
│   └── main.cpp
├── tests/
│   └── test_main.cpp
├── build.bat
├── run.bat
├── run_tests.bat
├── CMakeLists.txt
├── README.md
└── LICENSE
```

---

## 7. Build Instructions

### Windows (Visual Studio 2019 / MSVC x64)
1. Run the automated build script:
   ```cmd
   build.bat
   ```
2. Or use CMake directly:
   ```cmd
   mkdir build
   cd build
   cmake -G "Visual Studio 16 2019" -A x64 ..
   cmake --build . --config Release
   ```

### Linux (Ubuntu / Debian / Fedora)
1. Install prerequisites:
   ```bash
   sudo apt-get update
   sudo apt-get install -y cmake build-essential libgl1-mesa-dev libx11-dev libxi-dev libxrandr-dev libxinerama-dev libxcursor-dev
   ```
2. Build:
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j$(nproc)
   ```

---

## 8. Running the Application & Tests

### Launch Application
```cmd
run.bat
```
*Or directly via executable:* `build/Release/SmartParking.exe`

### Run Automated Unit Tests
```cmd
run_tests.bat
```
*Output validates **103 automated unit tests** across slot state machines, Dijkstra and A\* navigation, floor queries, simulation FSM, camera viewpoint presets 1..6, shadow mapping matrices, and environmental placements.*

---

## 9. User Controls & Keybindings

| Key / Mouse Action | Action Description |
|---|---|
| **W / A / S / D** | Move camera (3D) or Pan view (2D) |
| **Q / E** | Move camera vertically down / up (3D) |
| **Mouse Right Click + Drag** | Look around (3D Orbit/Pitch/Yaw) or Pan (2D) |
| **Mouse Left Click** | Interactive slot picking (selects bay & computes path) |
| **Mouse Scroll Wheel** | Zoom in / Zoom out |
| **1** | Switch to Preset 1: 2D Top-Down Orthographic View |
| **2** | Switch to Preset 2: 3D Perspective Digital-Twin View |
| **3** | Switch to Preset 3: Full Facility High-Altitude Overview |
| **4** | Switch to Preset 4: Ground Floor / South Sector (Rows A-B) |
| **5** | Switch to Preset 5: North Bays Sector (Rows C-D) |
| **6** | Switch to Preset 6: Entrance Gate Corridor View |
| **R** | Reset Camera to default vantage point |
| **L / Ctrl+D / D (2D)** | Toggle Day / Night Lighting Mode |
| **Space** | Play / Pause Simulation |
| **P** | Pause Simulation |
| **N** | Find & Navigate to Nearest Available Slot |
| **F3** | Toggle Debug Overlay HUD (FPS, Shadows, Markings, Graph) |
| **ESC** | Return to Dashboard / Close Modal |

---

## 10. Computer Graphics Concepts Demonstrated

Detailed explanations suitable for viva voce are documented in [docs/GRAPHICS_PIPELINE.md](file:///c:/Builds/Rakshita/docs/GRAPHICS_PIPELINE.md).

### 1. Two-Pass Real-Time Shadow Mapping
- **Pass 1 (Light POV):** Renders scene depth from the directional sunlight into a 2048x2048 Depth Framebuffer Object (`GL_DEPTH_COMPONENT24`). Front-face culling (`glCullFace(GL_FRONT)`) is applied to eliminate Peter-Panning.
- **Pass 2 (Camera POV):** Projects world fragments into light space, applies dynamic slope-scale bias to eliminate shadow acne, and calculates soft penumbras using a 3x3 Percentage-Closer Filter (PCF).

### 2. The Viewing Pipeline & Dual Projections
- **Model Transform ($M$):** Centers, scales, and rotates procedural primitives into world coordinates.
- **View Transform ($V$):** Evaluated via `glm::lookAt(eye, center, up)`. In 2D, the eye is elevated along $+Y$ looking at $-Z$.
- **Projection Transform ($P$):**
  - **3D Perspective:** $P = \text{perspective}(\text{FOV}, \text{aspect}, z_{\text{near}}, z_{\text{far}})$, enabling foreshortening.
  - **2D Orthographic:** $P = \text{ortho}(-w/2, w/2, -h/2, h/2, z_{\text{near}}, z_{\text{far}})$, maintaining parallel scale without perspective distortion.

### 3. Blinn-Phong Illumination Model
$$\vec{I}_{\text{frag}} = \vec{I}_{\text{ambient}} + (1 - \text{shadow}) \cdot \left[ \vec{I}_{\text{diffuse}} + \vec{I}_{\text{specular}} \right] + \vec{I}_{\text{emissive}}$$
- **Halfway Vector:** $\vec{H} = \frac{\vec{L} + \vec{V}}{\|\vec{L} + \vec{V}\|}$
- **Specular Term:** $(\vec{N} \cdot \vec{H})^{\text{shininess}} \cdot \vec{k}_{\text{spec}}$
- **Point Light Attenuation:** Streetlight lampposts utilize distance attenuation $\frac{1.0}{1.0 + 0.09d + 0.032d^2}$ in night mode.

### 4. Screen-to-World Raycasting
To select slots via mouse clicks, screen pixel coordinates $(x, y)$ are converted to Normalized Device Coordinates (NDC $[-1, 1]$), multiplied by $(P \cdot V)^{-1}$ to form a ray, and intersected with the ground plane ($Y = 0$).

---

## 11. Viva Questions and Answers

**Q1: How does shadow mapping avoid self-shadowing (shadow acne)?**  
*A:* Shadow acne occurs when fragments sample their own depth due to finite shadow map precision. We apply a slope-scaled depth bias: `bias = max(shadowBias * (1.0 - dot(N, L)), shadowBias * 0.2)`. Steep surfaces receive higher bias, preventing surface artifacts.

**Q2: What is the purpose of PCF in shadow mapping?**  
*A:* Percentage-Closer Filtering samples surrounding depth texels and computes an average occlusion percentage, producing smooth, soft penumbra transitions instead of aliased stair-step edges.

**Q3: Why use an orthographic projection for the 2D mode instead of simply moving the 3D camera high up?**  
*A:* Moving a perspective camera high up still induces perspective distortion away from the center. An orthographic projection ensures $W = 1$ in homogeneous coordinates, preserving exact metric proportions and parallel parking lines across the entire facility.

**Q4: How does the application avoid Z-fighting during route and road rendering?**  
*A:* Ground is at $Y = -0.05$, road asphalt at $Y = 0.01$, markings at $Y = 0.015$, slot bays at $Y = 0.02$, vehicle bases at $Y = 0.05$, and the dynamic route ribbon at $Y = 0.08$ with depth testing enabled (`glDepthFunc(GL_LESS)`), ensuring distinct rasterization depth.

---

## 12. Limitations & Future Work
- **Multi-Level Garages:** Currently models an expansive ground facility; future extensions can add ramps and multi-floor vertical navigation layers.
- **Dynamic Obstacle Avoidance:** Vehicles follow lane graph edges; future work could integrate localized steering behaviors (Reynolds flocking/avoidance) for pedestrian crossers.
- **Automated License Plate Recognition (ALPR):** Planned for subsequent system extensions.

---

## 13. License
This project is licensed under the MIT License - see the [LICENSE](file:///c:/Builds/Rakshita/LICENSE) file for details.
