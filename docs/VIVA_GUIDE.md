# Comprehensive Viva Examination & Academic Defense Guide

**Project Title:** Interactive 2D/3D Smart Parking Visualization and Navigation System  
**Course:** Computer Graphics and Visualization (CS / CE)  
**Authoritative Implementation Reference:** Actual Codebase (`src/`, `shaders/`)

---

## 1. Computer Graphics & OpenGL Pipeline Fundamentals

### Q1: What is the OpenGL Rendering Pipeline and how does this application use it?
**Answer:** The modern OpenGL 3.3 Core Profile pipeline transforms geometric vertex data into rendered pixel fragments through programmable shader stages:
1. **Vertex Specification:** Vertex attributes (position, normals, UVs, colors) are uploaded to GPU memory via Vertex Buffer Objects (VBOs) configured by Vertex Array Objects (VAOs).
2. **Vertex Shader:** Transforms local vertex coordinates into Clip Space using the Model-View-Projection matrix ($v_{\text{clip}} = P \cdot V \cdot M \cdot v_{\text{local}}$).
3. **Primitive Assembly & Clipping:** Connects vertices into triangles/lines and clips geometry outside the canonical viewing frustum $[-1, 1]^3$.
4. **Rasterization:** Converts primitives into discrete fragment candidates, interpolating vertex attributes using barycentric coordinates across the triangle face.
5. **Fragment Shader:** Evaluates illumination (Blinn-Phong), material colors, and samples shadow maps to calculate the final color of each fragment.
6. **Per-Sample Operations:** Performs Depth Testing (`GL_DEPTH_TEST`), Stencil Testing, and Alpha Blending (`GL_BLEND`) before writing to the Framebuffer.

### Q2: What is the difference between VAO, VBO, and EBO in your implementation?
**Answer:**
- **VBO (Vertex Buffer Object):** A GPU memory buffer storing raw vertex data (e.g., float arrays containing coordinates, normals, and texture coordinates). Used in `Renderer.cpp` for parking bays, roads, vehicles, and environmental trees.
- **VAO (Vertex Array Object):** An OpenGL state object that encapsulates and remembers how vertex attributes are laid out in VBOs (`glVertexAttribPointer`, `glEnableVertexAttribArray`). Switching geometry requires only binding the single corresponding VAO.
- **EBO (Element Buffer Object) / Index Buffer:** Stores indices that define triangles, allowing vertices shared between adjacent polygons to be reused without duplicating vertex data in the VBO (indexed rendering via `glDrawElements`).

### Q3: What is the difference between Orthographic and Perspective Projection?
**Answer:**
- **Perspective Projection (`glm::perspective`):** Models human eye and camera optics where objects farther away appear smaller (perspective foreshortening). The viewing volume is a truncated pyramid (frustum). Used in the **3D View (Preset 2)** to give a realistic digital-twin view.
- **Orthographic Projection (`glm::ortho`):** Projection lines are parallel and perpendicular to the projection plane; objects retain their true dimensional scale regardless of distance from the camera (no foreshortening). Used in the **2D Top-Down View (Preset 1)** for accurate architectural blueprints, distance perception, and slot ID legibility.

### Q4: Explain the Model-View-Projection (MVP) transformation sequence mathematically.
**Answer:** A local 3D vertex $\mathbf{v}_{\text{local}} = [x, y, z, 1]^T$ passes through three affine transformation matrices:
$$\mathbf{v}_{\text{clip}} = P \cdot V \cdot M \cdot \mathbf{v}_{\text{local}}$$
1. **Model Matrix ($M = T \cdot R \cdot S$):** Transforms vertex from Local Object Space into World Space by scaling, rotating to heading $\theta$, and translating to world position $\mathbf{p}$.
2. **View Matrix ($V$):** Transforms from World Space into Camera/Eye Space using `glm::lookAt(eye, center, up)`.
3. **Projection Matrix ($P$):** Maps Camera Space into Normalized Device Coordinates (NDC) $[-1, 1]^3$ via perspective or orthographic transformation.

### Q5: How is the Camera transformation implemented?
**Answer:** The camera uses Euler angles (yaw, pitch) converted to spherical direction vectors:
$$\text{front}.x = \cos(\text{yaw}) \cdot \cos(\text{pitch}), \quad \text{front}.y = \sin(\text{pitch}), \quad \text{front}.z = \sin(\text{yaw}) \cdot \cos(\text{pitch})$$
In 3D mode, the View Matrix is computed dynamically each frame via `glm::lookAt(m_position, m_position + m_front, m_up)`. The application supports WASD translation, right-mouse drag orbit, scroll-wheel zoom, and six discrete viewpoint presets (`1` through `6`).

### Q6: What is Depth Buffering (Z-buffering) and what is Z-fighting?
**Answer:**
- **Depth Buffering:** Hardware mechanism enabled via `glEnable(GL_DEPTH_TEST)` and `glDepthFunc(GL_LESS)`. When a fragment is rasterized, its interpolated depth $z \in [0, 1]$ is compared with the current value in the 24-bit depth buffer. If $z_{\text{frag}} < z_{\text{buffer}}$, the fragment is closer to the camera, so the color and depth buffers are updated; otherwise, the fragment is discarded.
- **Z-Fighting:** Visual flickering caused by floating-point precision limits when two polygons occupy nearly identical depth planes (e.g., road markings resting directly on top of asphalt). In this application, Z-fighting is eliminated by lifting road markings and bays slightly ($Y = +0.02\text{m}$ to $+0.04\text{m}$) and applying polygon offset.

---

## 2. Lighting & Shading Models

### Q7: Explain the Blinn-Phong Illumination Model used in this project.
**Answer:** The lighting model calculates the total surface radiance $I_{\text{total}}$ as the sum of ambient, diffuse, and specular components:
$$I_{\text{total}} = I_{\text{ambient}} + I_{\text{diffuse}} + I_{\text{specular}}$$
1. **Ambient Term:** $I_{\text{ambient}} = k_a \cdot L_a$, providing constant baseline indirect environmental light.
2. **Diffuse Term (Lambertian):** $I_{\text{diffuse}} = k_d \cdot L_d \cdot \max(\mathbf{N} \cdot \mathbf{L}, 0)$, dependent on the angle between the surface normal $\mathbf{N}$ and incident light direction $\mathbf{L}$.
3. **Specular Term (Blinn-Phong):** Uses the halfway vector $\mathbf{H} = \frac{\mathbf{L} + \mathbf{V}}{\|\mathbf{L} + \mathbf{V}\|}$, computing:
   $$I_{\text{specular}} = k_s \cdot L_s \cdot \max(\mathbf{N} \cdot \mathbf{H}, 0)^{\alpha}$$
   where $\mathbf{V}$ is the view direction vector and $\alpha$ is material shininess. Blinn-Phong is computationally more efficient than standard Phong because $\mathbf{H}$ avoids computing reflection vectors $\mathbf{R} = 2(\mathbf{N} \cdot \mathbf{L})\mathbf{N} - \mathbf{L}$.

### Q8: How does the Day/Night lighting system work?
**Answer:** The system maintains a `TimeOfDay` state in `Renderer`:
- **Day Mode:** Sunlight vector $\mathbf{L}_{\text{dir}} = \text{normalize}(-0.4, -0.8, -0.3)$, clear blue sky clear color, strong ambient light (0.35), bright sunlight color $(1.0, 0.98, 0.92)$, and active directional shadows.
- **Night Mode:** Dark sky clear color, low ambient light (0.08), cool moonlight, and illuminated streetlamp posts with bright point light sources and glowing emissive lantern globes. Headlights and taillights on vehicles illuminate the driving surface.

---

## 3. Real-Time Shadow Mapping

### Q9: How is Real-Time Shadow Mapping implemented in this project?
**Answer:** We implement standard **Two-Pass Depth Shadow Mapping**:
1. **Pass 1 (Depth Generation):**
   - Bind an off-screen Framebuffer Object (FBO) attached to a 2048x2048 2D depth texture (`GL_DEPTH_COMPONENT`).
   - Set viewport to $2048 \times 2048$.
   - Calculate an orthographic light-space matrix: $M_{\text{light}} = P_{\text{ortho}} \cdot V_{\text{light}}$, positioning the light camera upstream along the sunlight vector.
   - Render all shadow-casting geometry (vehicles, bays, trees, poles) using a minimal depth-only vertex shader (`shadow_depth.vert`); the fragment shader is empty (`gl_FragDepth` is written automatically).
2. **Pass 2 (Lighting & Shadow Evaluation):**
   - Bind the default screen framebuffer ($1280 \times 720$).
   - Bind the 2048x2048 depth texture to texture unit 1 (`GL_TEXTURE1`).
   - In the lighting fragment shader (`lighting.frag`), transform each fragment's world position into light-space coordinates:
     $$\mathbf{p}_{\text{light}} = M_{\text{light}} \cdot \mathbf{p}_{\text{world}}$$
   - Perform perspective division and map coordinates from $[-1, 1]$ to $[0, 1]$ texture space ($UV = 0.5 \cdot \mathbf{p}_{\text{light}} + 0.5$).
   - Compare fragment depth with the sampled depth from the shadow texture. If $\text{currentDepth} - \text{bias} > \text{closestDepth}$, the fragment is in shadow ($0.0$), otherwise fully lit ($1.0$).

### Q10: What is Shadow Acne and Peter-Panning, and how did you resolve them?
**Answer:**
- **Shadow Acne:** Moiré self-shadowing patterns caused by depth quantization when a surface fragment is compared with its own quantized depth sample from the shadow map. Solved by introducing a **slope-scaled depth bias**:
  $$\text{bias} = \max(0.005 \cdot (1.0 - \mathbf{N} \cdot \mathbf{L}), 0.0015)$$
- **Peter-Panning:** When depth bias is set too large, shadows detach and appear to float away from objects. Solved by carefully tuning our bias range ($0.0015$ to $0.004$) and applying front-face culling (`glCullFace(GL_FRONT)`) during the shadow depth generation pass.
- **Hard Edges:** Softened using **Percentage-Closer Filtering (PCF)**: a $3 \times 3$ sampling kernel that samples 9 surrounding texels in the depth map and averages the shadow factors to produce a smooth, anti-aliased penumbra.

---

## 4. Graph Theory, Navigation & Pathfinding

### Q11: How is the parking facility represented as a Navigation Graph?
**Answer:** The parking environment is represented as a directed topological graph $G = (V, E)$ in `Graph.h`:
- **Nodes ($V$):** Represent physical locations:
  - Entrance Gate (`NodeType::ENTRANCE`)
  - Exit Gate (`NodeType::EXIT`)
  - Driving Lanes & Waypoints (`NodeType::ROAD_LANE`)
  - Road Intersections & Turning Junctions (`NodeType::INTERSECTION`)
  - Individual Parking Bays (`NodeType::SLOT_BAY`), linked to their corresponding slot ID.
- **Edges ($E$):** Represent navigable road segments connecting nodes. Each directed edge contains `fromIndex`, `toIndex`, metric distance cost (Euclidean length in meters), and lane width. Roads are bidirectional with twin opposing directed edges, while parking bays feature unidirectional ingress/egress arcs.

### Q12: Explain Dijkstra's Algorithm and its implementation in PathFinder.
**Answer:** Dijkstra's Algorithm computes the single-source shortest path from a start node $s$ to target node $t$ on non-negative weighted graphs:
1. Maintain an array `dist[]` initialized to $\infty$, with $\text{dist}[s] = 0$, and a predecessor array `prev[]` initialized to $-1$.
2. Maintain a min-priority queue `std::priority_queue<std::pair<float, int>, ..., std::greater>` storing $(g, u)$.
3. Pop node $u$ with minimum tentative cost. If $u = t$, terminate early.
4. For each outgoing edge $(u, v)$ with weight $w$, relax the edge:
   $$\text{if } \text{dist}[u] + w < \text{dist}[v] \implies \text{dist}[v] = \text{dist}[u] + w, \quad \text{prev}[v] = u$$
5. Reconstruct the optimal path by backtracking from $t$ to $s$ using `prev[]` and reversing the array. Time complexity is $O((V + E)\log V)$.

### Q13: How does the A* Search Algorithm differ from Dijkstra?
**Answer:** While Dijkstra explores nodes radially in all directions (uniform cost search), A* directs search toward the destination by prioritizing nodes based on evaluation function $f(n) = g(n) + h(n)$:
- $g(n)$: The exact cost incurred from start node $s$ to current node $n$.
- $h(n)$: An admissible heuristic estimate of the remaining cost from $n$ to goal $t$.
- In `PathFinder::findShortestPathAStar`, we employ the Euclidean distance heuristic:
  $$h(n) = \|\mathbf{p}_n - \mathbf{p}_{\text{target}}\|$$
  Because Euclidean distance represents a straight line ("as the crow flies"), $h(n) \le \text{true\_cost}(n, t)$, satisfying admissibility ($A^*$ is guaranteed to return the optimal shortest path). In grid/sparse graphs, A* visits significantly fewer nodes than Dijkstra.

### Q14: How are vehicle turns detected and counted along a calculated route?
**Answer:** In `PathFinder::countTurns`, successive waypoint segments $\mathbf{v}_1 = \mathbf{p}_i - \mathbf{p}_{i-1}$ and $\mathbf{v}_2 = \mathbf{p}_{i+1} - \mathbf{p}_i$ are normalized. The angle $\theta$ between them is evaluated using the vector dot product:
$$\cos \theta = \frac{\mathbf{v}_1 \cdot \mathbf{v}_2}{\|\mathbf{v}_1\| \|\mathbf{v}_2\|}$$
If $\theta > 35^\circ$ ($\cos \theta < 0.819$), the junction is flagged as an intersection turn, incrementing `turnCount` and generating turn-by-turn navigation instructions ("Turn Left onto Row B", "Turn Right into Bay C-08").

---

## 5. Vehicle Simulation & State Machine

### Q15: Detail the Vehicle Finite State Machine (FSM).
**Answer:** Each simulated vehicle executes an autonomous 8-state machine (`Vehicle.h`):
1. `ENTERING`: Vehicle spawns at Entrance Gate coordinates $(0, 0, -42)$.
2. `SEARCHING`: System queries `ParkingLot::findNearestAvailableSlot` matching criteria (Regular, EV, Accessible).
3. `ROUTING`: `PathFinder` builds turn-by-turn waypoints from Entrance to the allocated slot bay.
4. `DRIVING`: Vehicle navigates along waypoints at cruising speed ($8.0\,\text{m/s}$ or $28.8\,\text{km/h}$), slowing at sharp turns.
5. `PARKING`: Vehicle aligns with bay centerline and executes slow entry maneuver ($1.5\,\text{m/s}$).
6. `PARKED`: Vehicle becomes stationary, turning off engine. The target bay status updates to `OCCUPIED`.
7. `EXITING`: After park duration expires or user triggers release, vehicle unparks in reverse, routes to Exit Gate, and follows egress lanes.
8. `COMPLETED`: Vehicle reaches Exit Gate, releases all resources, and is removed from the active vehicle array.

### Q16: How is Vehicle Animation and Orientation calculated?
**Answer:**
- **Position Interpolation:** Movement along waypoint segments uses frame-rate-independent delta-time linear interpolation:
  $$\mathbf{p}(t + \Delta t) = \mathbf{p}(t) + \mathbf{d} \cdot (\text{speed} \cdot \Delta t)$$
- **Heading / Orientation:** The vehicle yaw rotation $\theta_{\text{yaw}}$ is computed from the normalized velocity vector $(dx, dz)$ using the standard 2-argument arctangent:
  $$\theta_{\text{yaw}} = \text{atan2}(dx, dz)$$
  Orientation is smoothly interpolated using angular Lerp to prevent unnatural snapping when taking turns.

---

## 6. Smart Parking Concepts & Allocation Logic

### Q17: What is the authoritative single source of truth for parking slot states?
**Answer:** The authoritative data model is encapsulated entirely in `ParkingLot::m_slots` (`ParkingSlot.h`). Each slot maintains:
- `id` (e.g., `A-01` to `D-15`)
- `floor` (0 for Ground Level, 1 for Level 1)
- `row` (`A`, `B`, `C`, `D`)
- `type` (`REGULAR`, `EV_CHARGING`, `ACCESSIBLE`, `RESERVED`)
- `status` (`AVAILABLE`, `OCCUPIED`, `RESERVED`)
- `vehicleId` (License/ID string of parked vehicle)
- Metric position vector and physical bounding box.
No parallel or conflicting slot states exist in UI, Renderer, or Simulation.

### Q18: How does the Smart Parking Allocation algorithm work?
**Answer:** When an incoming vehicle arrives or the user clicks "Find Nearest Available" (`N` key):
1. Filters all 60 slots by requested type (`REGULAR`, `EV_CHARGING`, `ACCESSIBLE`, `RESERVED`).
2. Discards slots whose status is not `AVAILABLE`.
3. Measures Euclidean and topological graph travel distance from the reference location (typically Entrance Gate).
4. Selects the slot with minimal total travel distance.
5. Marks the slot as `RESERVED` immediately so concurrent vehicles cannot claim it, then dispatches the vehicle. If all candidate slots are occupied, it safely displays `"No suitable parking available."` without crashing or assigning occupied bays.

---

## 7. Software Architecture & Engineering Decisions

### Q19: Why C++17 and OpenGL 3.3 instead of Unity, Unreal, or WebGL?
**Answer:**
- **Pedagogical Rigor:** Computer graphics fundamentals (matrix math, projection transformations, shader writing, vertex layout, depth testing, shadow mapping) must be written from scratch, not masked by commercial game engines.
- **Deterministic High Performance:** C++ provides predictable cache-friendly memory layouts, zero garbage-collection latency, and direct hardware API bindings.
- **Portability:** OpenGL 3.3 Core Profile is universal across all modern Windows, Linux, and macOS hardware without requiring proprietary vendor extensions or heavy runtime installs.

### Q20: How are memory leaks prevented in your OpenGL application?
**Answer:** Through strict **RAII (Resource Acquisition Is Initialization)**:
- OpenGL resources (buffers, textures, shaders, FBOs) are acquired in class constructors and explicitly released via `glDeleteBuffers`, `glDeleteVertexArrays`, `glDeleteFramebuffers`, `glDeleteTextures`, and `glDeleteProgram` in class destructors.
- C++ standard library containers (`std::vector`, `std::unique_ptr`, `std::string`, `std::deque`) manage dynamic memory automatically, preventing heap leaks.

### Q21: How do you achieve 60+ FPS with 60 bays, 3D vehicles, shadows, and environment models?
**Answer:**
1. **Geometry Batching:** Static geometry (asphalt road quad, 60 parking slot lines, lane markings, curbs, trees, poles) is uploaded to the GPU once during initialization. It is rendered with batched draw calls rather than regenerating vertex buffers per frame.
2. **Minimal State Changes:** VAO and shader program bindings are minimized.
3. **Frustum & Depth Culling:** Hardware back-face culling (`glCullFace(GL_BACK)`) and depth testing (`glEnable(GL_DEPTH_TEST)`) quickly discard invisible surfaces before fragment execution.
4. **Efficient Shadow Pass:** Shadow depth generation uses an optimized minimal vertex shader with no color outputs or fragment operations.

---

## 8. User Interface & Presentation Modes

### Q22: How does the Automated 14-Step Presentation Demo Mode work?
**Answer:** Designed specifically for seamless faculty viva presentations, clicking `"DEMO MODE"` triggers an autonomous 14-stage scripted sequence managed by `Simulation::updateDemoMode`:
1. Overview of facility from high vantage point.
2. Transition to 2D Orthographic Map.
3. Transition to 3D Perspective Digital Twin.
4. Scan and highlight available bays.
5. Select destination slot.
6. Calculate Dijkstra / A* route.
7. Spawn autonomous test vehicle.
8. Follow route along designated aisles.
9. Execute parking bay entry maneuver.
10. Update occupancy metrics and telemetry header.
11. Vehicle signals departure and unparks.
12. Vehicle exits facility, freeing the bay.
13. Enable navigation graph overlay to showcase nodes and edges.
14. Smoothly restore default overview camera.
Each step displays on-screen explanatory captions and requires zero manual interaction.

### Q23: What does the F3 Debug Mode display?
**Answer:** Pressing `F3` toggles the Computer Graphics Debug HUD:
- Real-time FPS and Frame Time ($\Delta t$ in milliseconds).
- Camera Coordinates $(x, y, z)$, Yaw, Pitch, and Projection Mode.
- Active Vehicle Count and Facility Occupancy Rate.
- Active Route Distance (meters), Waypoint Count, and Turn Count.
- Navigational Graph Wireframe visualization (nodes rendered as color-coded spheres and edges as line strips).
- Shadow Map Debug FBO viewer showing raw depth buffer textures.

---

## 9. Academic Self-Assessment & Edge Cases

### Q24: How does your system handle saturated parking (0 available slots)?
**Answer:** When all 60 bays are full, `findNearestAvailableSlot` returns `nullptr`. The allocation engine reports `"No suitable parking available"` in the dashboard, spawning is safely throttled, and incoming vehicles are denied entry rather than clipping through existing cars or overwriting slot pointers.

### Q25: What happens if a destination slot becomes occupied while a vehicle is in transit?
**Answer:** Slots are placed in the `RESERVED` state the instant a vehicle is assigned to them. A reserved slot cannot be selected by another vehicle. In the rare event of manual admin override, the vehicle detects slot unavailability, enters the `SEARCHING` state, recalculates a route to the next nearest bay, and resumes driving.

### Q26: What are the main limitations of the current implementation?
**Answer:**
1. Single-level ground layout: While multi-floor data structures (`getFloor()`, `setFloor()`, `Level 1`) and viewpoint presets are fully operational, physical multi-tier helical ramp geometry is modeled via ground sectors rather than a vertical spiraling deck.
2. Collision physics: Vehicles follow precise graph topological lanes and maintain safe separation distances based on path lookahead; rigid-body wheel physics and friction slip are abstracted for performance.

### Q27: How would you extend this project in the future?
**Answer:**
1. Physical multi-deck vertical car park with procedural helical ramp geometry.
2. Dynamic occupancy heatmaps using Gaussian kernel density estimation rendered into a secondary texture.
3. Mobile MQTT / REST telemetry streaming for live smart city sensor integration.
4. Cascaded Shadow Maps (CSM) for extended rendering distance outdoors.

---

## 10. Rapid-Fire Technical Definitions

| Term | Technical Viva Definition |
| :--- | :--- |
| **Normalized Device Coordinates (NDC)** | The 3D canonical cube $[-1, 1] \times [-1, 1] \times [-1, 1]$ produced after the perspective divide ($x/w, y/w, z/w$). |
| **Homogeneous Coordinates** | 4D coordinate representation $(x, y, z, w)$ enabling affine translation, rotation, and scaling to be combined into single $4 \times 4$ matrix multiplications. |
| **Rasterization** | The fixed-function hardware stage converting vector primitive lines and triangles into discrete pixel fragments. |
| **Percentage-Closer Filtering (PCF)**| Technique for rendering anti-aliased soft shadows by sampling multiple depth texels around the shadow coordinate and averaging the test results. |
| **Admissible Heuristic** | A heuristic $h(n)$ that never overestimates the true minimal cost to reach the goal, ensuring $A^*$ pathfinding optimality. |
| **Euler Angles** | Yaw (heading around $Y$-axis), Pitch (elevation around $X$-axis), and Roll (tilt around $Z$-axis) used to orient the 3D camera. |
| **Dear ImGui** | Immediate-mode graphical user interface library where UI elements are declared and rendered in-line within the game loop without stateful object persistence. |
