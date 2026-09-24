# Smart Parking Navigation & Pathfinding Architecture

**Project Title:** Interactive 2D/3D Smart Parking Visualization and Navigation System  
**Module:** `SmartParking::Graph`, `SmartParking::PathFinder`, `SmartParking::Route`  
**Source Location:** `src/navigation/`

---

## 1. Topological Graph Representation

The physical parking layout is represented as a directed graph $G = (V, E)$ constructed programmatically in `ParkingLot::buildNavigationGraph()`:

### Node Structure (`Node`)
```cpp
enum class NodeType {
    ROAD_LANE,     // Navigable roadway waypoint along driving aisles
    INTERSECTION,  // Road junction / turning point
    ENTRANCE,      // Main facility vehicular ingress gate
    EXIT,          // Main facility vehicular egress gate
    SLOT_BAY,      // Individual parking slot terminal parking bay
    RAMP_CONNECTOR // Intermediate connector node
};

struct Node {
    int index;
    std::string id;
    glm::vec3 position;  // Metric coordinates (X, Y, Z) in meters
    NodeType type;
    std::string name;
    std::string slotId;  // Associated slot ID if type == SLOT_BAY
};
```

### Edge Structure (`Edge`)
```cpp
struct Edge {
    int fromIndex;
    int toIndex;
    float cost;        // Euclidean length in meters
    float laneWidth;   // Roadway lane clearance (default 3.5m)
    bool isOneWay;     // Unidirectional flag
};
```

---

## 2. Pathfinding Algorithms

### 2.1 Dijkstra's Shortest Path Algorithm
Dijkstra's algorithm finds the optimal shortest path between a start node $s$ and destination node $t$:

```cpp
Route PathFinder::findShortestPathDijkstra(int startNodeIndex, int targetNodeIndex) const;
```
- **Data Structures:** Min-priority queue `std::priority_queue<std::pair<float, int>, ..., std::greater>` storing pair of `(distance, nodeIndex)`.
- **Predecessor Array:** Tracks optimal parent nodes `prev[nodeCount]` for path reconstruction.
- **Complexity:** $\mathcal{O}((V + E) \log V)$ using binary min-heap.
- **Guarantee:** Optimal shortest path on graphs with non-negative edge costs.

### 2.2 A* (A-Star) Pathfinding Algorithm
A* enhances search efficiency toward a specific goal node using a goal-directed heuristic:

```cpp
Route PathFinder::findShortestPathAStar(int startNodeIndex, int targetNodeIndex) const;
```
- **Evaluation Function:**
  $$f(n) = g(n) + h(n)$$
  where:
  - $g(n)$: Exact accumulated cost from start node $s$ to node $n$.
  - $h(n)$: Euclidean distance heuristic estimate from $n$ to goal $t$:
    $$h(n) = \|\mathbf{p}_n - \mathbf{p}_{\text{target}}\| = \sqrt{(x_n - x_t)^2 + (y_n - y_t)^2 + (z_n - z_t)^2}$$
- **Admissibility:** Since Euclidean distance is the straight-line distance, $h(n) \le \text{cost}^*(n, t)$ for all $n$. This guarantees that A* never overestimates true travel distance and returns an optimal shortest path.
- **UI Toggle:** Users can switch between Dijkstra and A* using the radio buttons in the Navigation tab.

---

## 3. Route Calculation & Waypoint Interpolation

### 3.1 Turn Detection & Geometry
To provide turn-by-turn guidance, turns are detected by analyzing consecutive direction vectors:
```cpp
glm::vec3 dir1 = glm::normalize(points[i] - points[i - 1]);
glm::vec3 dir2 = glm::normalize(points[i + 1] - points[i]);
float cosAngle = glm::dot(dir1, dir2);
if (cosAngle < 0.819f) { // Angle > 35 degrees
    turnCount++;
}
```

### 3.2 Metric Distance & Estimated Time
- **Total Distance:** $\sum_{i=1}^{k} \|\mathbf{p}_i - \mathbf{p}_{i-1}\|$ meters.
- **Estimated Travel Time:**
  $$T = \frac{D_{\text{straight}}}{v_{\text{cruise}}} + \sum T_{\text{turn}} + T_{\text{park}}$$
  assuming $v_{\text{cruise}} = 8.0\,\text{m/s}$ and $T_{\text{turn}} = 2.0\,\text{s}$ per turn.

---

## 4. Visual Rendering of Routes

- **2D Mode:** Rendered as a glowing neon cyan-to-green polyline directly over the orthographic map with directional markers.
- **3D Mode:** Rendered as an elevated ribbon ($Y = +0.06\text{m}$) above the asphalt surface with dynamic directional chevron arrows pulsing in the direction of vehicular travel.
- **Reactivity:** Changing destination bays immediately clears the existing ribbon and renders the newly calculated path in real time.
