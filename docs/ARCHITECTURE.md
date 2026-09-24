# System Architecture Document

## 1. High-Level Architecture Overview

The **Smart Parking Visualization and Navigation System** is engineered using a decoupled, modular layered architecture. The system cleanly separates core data structures, algorithms, simulation state, rendering pipelines, and user interface controls.

```
+------------------------------------------------------------------------+
|                          Application (main.cpp)                        |
|   (GLFW Lifecycle, OpenGL Context, Event Dispatching, Main Game Loop)  |
+-------------------+--------------------+-------------------------------+
                    |                    |
       +------------v-----------+        |
       |     Simulation Engine  |        |
       | (Traffic, Timers, Demo)|        |
       +------------+-----------+        |
                    |                    |
       +------------v-----------+        |
       |      ParkingLot        |        |
       | (Slots, Rows, Roads)   |        |
       +------------+-----------+        |
                    |                    |
       +------------v-----------+        |
       |     Navigation Graph   |<-------+
       | (Nodes, Edges, Dijkstra|
       +------------+-----------+
                    |
+-------------------v--------------------+-------------------------------+
|         Graphics Engine (Renderer)     |     UI Layer (UIManager)      |
|  - 2D Orthographic Pipeline            |  - Dear ImGui Theme Engine    |
|  - 3D Perspective Pipeline             |  - Dashboard, Search, Nav     |
|  - Blinn-Phong Shaders                 |  - Analytics & Plots          |
|  - Procedural VBO/VAO Meshes           |  - Debug HUD (F3)             |
+----------------------------------------+-------------------------------+
```

---

## 2. Subsystem Breakdown

### 2.1 Graphics Subsystem (`src/graphics`)
- **`Shader`**: Manages GLSL shader compilation, program linking, error diagnostics, and caches uniform locations.
- **`Camera`**: Encapsulates both 2D Orthographic and 3D Perspective viewing pipelines. Handles WASDQE translation, mouse look (yaw/pitch clamping), zoom, and pan.
- **`Mesh`**: Hardware buffer encapsulation managing Vertex Array Objects (VAO), Vertex Buffer Objects (VBO), and Element Buffer Objects (EBO). Provides static procedural factories for cubes, cylinders, spheres, planes, 2D quads, circles, and dynamic route ribbons.
- **`Renderer`**: Executes separated 2D and 3D rendering passes. Configures Blinn-Phong lighting uniforms, model matrix transformations, selection beacons, vehicle 3D hierarchical models, entrance/exit gate structures with illuminated LED signboards, and complete navigation graph visualization (nodes, edges, and active path highlights).

### 2.2 Navigation Subsystem (`src/navigation`)
- **`Graph`**: Spatial road network represented as an Adjacency List. Nodes represent the entrance gate, exit gate, junctions, aisle waypoints, slot access points, and slot bays. Edges represent road lanes with metric cost weights.
- **`PathFinder`**: Implements Dijkstra's algorithm and A* heuristic search. Converts node sequences into metric `Route` objects containing waypoints, turn counts, driving duration estimates, and turn-by-turn instruction strings.

### 2.3 Parking Subsystem (`src/parking`)
- **`ParkingSlot`**: Represents an individual bay with dimensions, coordinates, rotation angle, type (Regular, EV, Accessible, Reserved), status (Available, Occupied, Reserved, Disabled, EV Charging, Selected), occupant vehicle ID, and metric distance to entrance.
- **`ParkingLot`**: Container for all bays, rows, roads, entrance, and exit. Parses `config/parking_layout.json` (with automatic programmatic fallback), builds the navigation graph, and performs nearest available slot queries with live multi-criteria search.

### 2.4 Simulation Subsystem (`src/simulation`)
- **`Vehicle`**: Represents simulated cars navigating the facility. Follows route waypoints, computes steering angle ($\text{atan2}$), transitions between states (`ENTERING`, `NAVIGATING`, `PARKING`, `PARKED`, `UNPARKING`, `EXITING`, `COMPLETED`), and handles departure routing to the Exit Gate.
- **`Simulation`**: Controls simulation time, playback states (Play, Pause, Step, Reset), speed multipliers ($0.25\times$ to $4.0\times$), vehicle spawning, rolling analytics, and the 6-step Demonstration Script.

### 2.5 UI Subsystem (`src/ui`)
- **`UIManager`**: Implements the professional Dark Charcoal UI with Dear ImGui. Coordinates top navigation tabs, metrics header, live slot search/filter results, route instruction cards, simulation controls, analytics charts, viva graphics concept explanations, F3 developer debug HUD, and screen-projected world overlays for 2D slot ID badges and gate labels.

---

## 3. Frame Execution Flow

Each frame in `Application::run()` follows a deterministic sequential cycle:
1. **Delta Time & Timing:** Computes $\Delta t = t_{\text{current}} - t_{\text{last}}$, updates FPS moving average.
2. **Input Processing:** Handles continuous WASD camera movement if ImGui does not capture keyboard.
3. **Event Polling:** `glfwPollEvents()` dispatches mouse picking, scrolling, and key clicks.
4. **Simulation Step:** `Simulation::update(\Delta t \times \text{speedMultiplier})` advances vehicles, timers, and metrics.
5. **Graphics Pass:**
   - Clears frame buffers (`GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT`).
   - Renders 2D or 3D scene (ground, roads, slots, vehicles, route ribbon, landscape, debug overlays).
6. **UI Pass:**
   - Prepares ImGui frame (`newFrame()`).
   - Renders dashboard, cards, analytics plots, and HUD.
   - Submits ImGui draw lists to OpenGL.
7. **Buffer Presentation:** Swaps front and back buffers with VSync synchronization (`glfwSwapBuffers`).

---

## 4. Memory Management & RAII Principles
- **No Raw Pointer Ownership:** Meshes, Shaders, and Vehicles are managed with `std::unique_ptr` and `std::vector`.
- **GPU Resource Cleanup:** All OpenGL objects (shaders, VAOs, VBOs, EBOs) are deallocated in destructors (`glDeleteShader`, `glDeleteVertexArrays`, `glDeleteBuffers`).
- **Const Correctness:** All query methods across `ParkingLot`, `Graph`, `Camera`, `PathFinder`, and `ParkingSlot` enforce `const` qualifiers to prevent accidental state mutation.
