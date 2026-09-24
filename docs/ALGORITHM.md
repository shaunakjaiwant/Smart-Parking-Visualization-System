# Algorithm Specification Document

## 1. Dijkstra's Shortest Path Algorithm

### 1.1 Mathematical Formulation
Let $G = (V, E)$ represent the directed spatial parking graph, where:
- $V$ is the set of $|V| = 74$ vertices representing entrance, exit, lane junctions, access points, and slot bays.
- $E$ is the set of directed edges representing drivable road segments.
- $w: E \to \mathbb{R}^+$ is the non-negative Euclidean distance cost function:
  $$w(u, v) = \sqrt{(x_v - x_u)^2 + (z_v - z_u)^2} \cdot \mu_{\text{cost}}$$
  where $\mu_{\text{cost}} \ge 1.0$ is a traffic multiplier.

### 1.2 Algorithm Implementation
```cpp
dist[startNode] = 0;
pq.push({0, startNode});

while (!pq.empty()) {
    auto [currentDist, u] = pq.top();
    pq.pop();

    if (visited[u]) continue;
    visited[u] = true;

    if (u == targetNode) break;

    for (const auto& edge : graph.getOutgoingEdges(u)) {
        int v = edge.toIndex;
        float newDist = currentDist + edge.cost;
        if (newDist < dist[v]) {
            dist[v] = newDist;
            prev[v] = u;
            pq.push({newDist, v});
        }
    }
}
```

### 1.3 Complexity
- **Time Complexity:** $\mathcal{O}((|V| + |E|) \log |V|)$ using a binary min-heap (`std::priority_queue`). With $|V| = 74$, the calculation completes in $< 0.05 \text{ ms}$, entirely eliminating frame drops.
- **Space Complexity:** $\mathcal{O}(|V|)$ for distance and predecessor lookup arrays.

---

## 2. A* Pathfinding Algorithm

### 2.1 Heuristic Function
A* evaluates nodes using the evaluation function:
$$f(n) = g(n) + h(n)$$
where:
- $g(n)$ is the exact accumulated path cost from the start node to node $n$.
- $h(n)$ is the admissible Euclidean distance heuristic to the destination:
  $$h(n) = \|\vec{P}_{\text{target}} - \vec{P}_n\|_2 = \sqrt{(x_t - x_n)^2 + (z_t - z_n)^2}$$

Because $h(n)$ is Euclidean distance across flat 2D ground ($Y = 0$), $h(n) \le d^*(n, \text{target})$ is strictly admissible and monotonic, guaranteeing optimality while exploring fewer vertices than standard Dijkstra.

---

## 3. Turn Detection & Travel Time Estimation

### 3.1 Turn Detection
To compute turn counts and generate navigation instructions:
For three consecutive waypoints $P_{i-1}, P_i, P_{i+1}$:
1. Compute incoming and outgoing normalized direction vectors:
   $$\vec{v}_1 = \frac{P_i - P_{i-1}}{\|P_i - P_{i-1}\|}, \quad \vec{v}_2 = \frac{P_{i+1} - P_i}{\|P_{i+1} - P_i\|}$$
2. Compute deflection angle:
   $$\theta = \arccos(\text{clamp}(\vec{v}_1 \cdot \vec{v}_2, -1.0, 1.0))$$
3. If $\theta > 35^\circ$, a turn is counted.
4. Direction is obtained via 2D cross product:
   $$\text{cross}_Y = v_{1x} v_{2z} - v_{1z} v_{2x}$$
   - If $\text{cross}_Y > 0$: **Turn Right**
   - If $\text{cross}_Y < 0$: **Turn Left**

### 3.2 Travel Time Estimation
$$\text{Time}_{\text{est}} = \frac{\text{Distance}_{\text{total}}}{v_{\text{avg}}} + N_{\text{turns}} \cdot \tau_{\text{turn}}$$
where:
- $v_{\text{avg}} = 15 \text{ km/h} \approx 4.17 \text{ m/s}$ (typical parking facility speed limit).
- $\tau_{\text{turn}} = 3.5 \text{ seconds}$ penalty for steering deceleration and intersection clearance.

---

## 4. Intelligent Slot Allocation Algorithm

When requesting the nearest slot with type constraint $T_{\text{req}}$:
1. Initialize $\text{bestSlot} = \text{null}$, $d_{\min}^2 = \infty$.
2. For each slot $S \in \text{ParkingLot}$:
   - If $S.\text{status} \ne \text{AVAILABLE}$, continue.
   - If $T_{\text{req}} \ne \text{ANY}$ and $S.\text{type} \ne T_{\text{req}}$, continue.
   - Compute squared distance on XZ plane:
     $$d^2 = (x_S - x_{\text{ref}})^2 + (z_S - z_{\text{ref}})^2$$
   - If $d^2 < d_{\min}^2$:
     $$d_{\min}^2 = d^2, \quad \text{bestSlot} = S$$
3. Reserve the slot and calculate Dijkstra route from entrance to $S$.

---

## 5. Screen-to-World Mouse Raycasting

To pick slots interactively using mouse clicks:
1. Convert screen pixel coordinates $(x_{\text{screen}}, y_{\text{screen}})$ to Normalized Device Coordinates (NDC):
   $$x_{\text{ndc}} = \frac{2 x_{\text{screen}}}{W} - 1, \quad y_{\text{ndc}} = 1 - \frac{2 y_{\text{screen}}}{H}$$
2. Invert View-Projection Matrix:
   $$M_{\text{inv}} = (P \cdot V)^{-1}$$
3. Unproject near and far plane coordinates:
   $$\vec{P}_{\text{near}} = M_{\text{inv}} \begin{pmatrix} x_{\text{ndc}} \\ y_{\text{ndc}} \\ -1 \\ 1 \end{pmatrix}, \quad \vec{P}_{\text{far}} = M_{\text{inv}} \begin{pmatrix} x_{\text{ndc}} \\ y_{\text{ndc}} \\ 1 \\ 1 \end{pmatrix}$$
   Divide by respective $W$ components.
4. Construct Ray:
   $$\vec{R}_{\text{origin}} = \vec{P}_{\text{near}}, \quad \vec{R}_{\text{dir}} = \text{normalize}(\vec{P}_{\text{far}} - \vec{P}_{\text{near}})$$
5. Intersect with ground plane ($Y = 0$):
   $$t = -\frac{R_{\text{origin}, y}}{R_{\text{dir}, y}}, \quad \vec{P}_{\text{ground}} = \vec{R}_{\text{origin}} + t \cdot \vec{R}_{\text{dir}}$$
6. Test inclusion inside slot oriented bounding boxes on the XZ plane.
