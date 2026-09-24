# Computer Graphics Pipeline & Real-Time Rendering Architecture

## Interactive 2D/3D Smart Parking System — Graphics Viva Reference Guide

This document provides a comprehensive academic and practical guide to the rendering pipeline, real-time shadow mapping, coordinate transformations, and illumination models implemented in the Smart Parking System using **OpenGL 3.3 Core Profile**, **GLSL #version 330 core**, and **C++17**.

---

## High-Level Rendering Architecture

The application implements a dual-mode graphics pipeline:
1. **2D Top-Down Mode**: Orthographic forward projection bypassing shadow computations for crisp, interactive 60+ FPS operational monitoring.
2. **3D Perspective Mode**: Multi-pass forward rendering pipeline with real-time directional shadow mapping, Blinn-Phong shading, point-light illumination, and percentage-closer filtering (PCF).

```
+-----------------------------------------------------------------------------------+
|                           PHASE 3A RENDERING PIPELINE                             |
+-----------------------------------------------------------------------------------+

   [SCENE GEOMETRY: Slots, Vehicles, Roads, Markings, Trees, Poles]
                                |
                                v
               +----------------------------------+
               |  Is 3D Perspective Mode Active?  |
               +----------------------------------+
                     /                      \
               YES  /                        \  NO (2D Orthographic)
                   v                          v
    +-----------------------------+     +-------------------------------+
    | PASS 1: Shadow Depth Pass   |     | PASS 1 & ONLY: 2D Ortho Pass  |
    | - Bind Depth FBO (2048x2048)|     | - glDisable(GL_CULL_FACE)     |
    | - Render from Sun Light POV |     | - Orthographic Projection     |
    | - Output: Depth Texture     |     | - Zero shadow overhead        |
    | - Front-face culling        |     | - Crisp vector outlines       |
    +-----------------------------+     +-------------------------------+
                   |                                    |
                   v                                    v
    +-----------------------------+              [Framebuffer Display]
    | PASS 2: Main 3D Scene Pass  |
    | - Bind Default Framebuffer  |
    | - Bind Depth Map (Unit 1)   |
    | - Blinn-Phong Lighting      |
    | - 3x3 PCF Filtered Shadows  |
    | - Day / Night Parameters    |
    | - Point Light Attenuation   |
    +-----------------------------+
                   |
                   v
    +-----------------------------+
    | PASS 3: Dear ImGui HUD Pass |
    | - Navigation & State HUD    |
    | - F3 Debug & Shadow Viewer  |
    +-----------------------------+
                   |
                   v
         [glfwSwapBuffers Presentation]
```

---

## 1. Forward Rendering

### Academic Definition
In **Forward Rendering**, scene geometry is submitted down the pipeline once per pass, and each primitive is transformed, rasterized, and shaded directly against all active light sources in the fragment shader before depth testing and presentation.

### Implementation Details
- **Pass 1 (Depth Only)**: Transforms vertices into Light Space using `LightProjection * LightView * Model` and discards color output (`glDrawBuffer(GL_NONE)` / `glReadBuffer(GL_NONE)`).
- **Pass 2 (Color & Lighting)**: Renders all visible geometry into the default color buffer. The fragment shader evaluates directional sunlight, ambient light, local lamppost point lights, and shadow occlusion factors simultaneously.
- **Complexity**: $\mathcal{O}(\text{Geometry} \times \text{Lights})$. Because the scene uses 1 directional sun light and up to 6 local point lights, forward rendering operates with near-zero overhead and minimal bandwidth compared to deferred shading.

---

## 2. Depth Buffer (Z-Buffer)

### Academic Definition
A **Depth Buffer** (or Z-Buffer) is a 2D hardware buffer storing the depth value ($z \in [0, 1]$) of the nearest fragment rendered at each pixel coordinate $(x, y)$.

### Role in Phase 3A
1. **Scene Hidden Surface Removal**: Resolves surface occlusions automatically. Incoming fragments with $z \ge z_{\text{buffer}}(x, y)$ are discarded (`GL_LESS`), preventing occluded asphalt or underground geometry from overwriting parking bays and vehicles.
2. **Shadow Map Storage**: The depth buffer attached to the offscreen Framebuffer Object acts as a spatial depth lookup table capturing distances from the light source.

---

## 3. Framebuffer Object (FBO)

### Academic Definition
A **Framebuffer Object (FBO)** is an OpenGL construct representing an offscreen render target collection containing attachments for color, depth, and stencil buffers.

### Architecture in `ShadowMap` Class
```
+-----------------------------------------------------------+
|                   ShadowMap FBO Object                    |
|                                                           |
|   +-----------------------+     +---------------------+   |
|   | GL_DEPTH_ATTACHMENT   | --> | 2D Depth Texture    |   |
|   | (GL_DEPTH_COMPONENT24)|     | (2048 x 2048 texels)|   |
|   +-----------------------+     +---------------------+   |
|                                                           |
|   glDrawBuffer(GL_NONE)         glReadBuffer(GL_NONE)     |
+-----------------------------------------------------------+
```

### Texture Configuration
```cpp
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
```
- **Clamp to Border**: Fragments outside the light frustum sample a depth of `1.0`, ensuring they are classified as fully lit rather than erroneously in shadow.

---

## 4. Shadow Mapping

### Academic Definition
**Shadow mapping** is a two-pass algorithm introduced by Lance Williams (1978) that determines whether a surface point is illuminated or occluded by testing if its distance to the light source is greater than the recorded depth from the light's viewpoint.

### Two-Pass Execution
1. **Depth Pass**: The virtual camera is placed at the light position looking toward the scene. Primitives are drawn, writing depth into the shadow FBO.
2. **Lighting Pass**: The virtual camera is at the viewer's position. For each fragment, its 3D world position is mapped into light space. The depth of the fragment from the light is compared to the value in the shadow map:
   $$\text{In Shadow} \iff z_{\text{frag}} > z_{\text{map}} + \text{bias}$$

### Percentage-Closer Filtering (PCF)
To eliminate hard, stair-stepped pixelated shadow edges, a **3x3 PCF filter** samples 9 surrounding texels in the shadow map and computes the average occlusion factor:
```glsl
float shadow = 0.0;
vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
for(int x = -1; x <= 1; ++x) {
    for(int y = -1; y <= 1; ++y) {
        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
        shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
    }
}
shadow /= 9.0;
```

---

## 5. Light-Space Transformation

### Mathematical Derivation
To compare world fragments against the shadow map, vertices are transformed using the **Light-Space Matrix** $\mathbf{M}_{\text{light}}$:

$$\mathbf{M}_{\text{light}} = \mathbf{P}_{\text{light}} \cdot \mathbf{V}_{\text{light}}$$

$$\mathbf{x}_{\text{light}} = \mathbf{M}_{\text{light}} \cdot \mathbf{x}_{\text{world}}$$

In the fragment shader, perspective division normalizes the coordinates to Normalized Device Coordinates (NDC) in $[-1, 1]$:
$$\mathbf{p}_{\text{ndc}} = \frac{\mathbf{x}_{\text{light}}.xyz}{\mathbf{x}_{\text{light}}.w}$$

Finally, coordinates are transformed into the texture coordinate range $[0, 1]$:
$$\mathbf{u}_{\text{tex}} = 0.5 \cdot \mathbf{p}_{\text{ndc}} + 0.5$$

---

## 6. Perspective Projection

### Mathematical Definition
Perspective projection models the human eye and camera pinhole lens where parallel lines converge at a vanishing point and distant objects appear smaller.

```
       Viewer / Eye
           (0,0)
           /   \
          /     \       Near Plane
         /-------\      (Width = w_near)
        /         \
       /           \
      /             \   Far Plane
     /---------------\  (Width = w_far >> w_near)
```

$$\mathbf{P}_{\text{persp}} = \begin{bmatrix}
\frac{1}{\text{aspect} \cdot \tan(\frac{\theta}{2})} & 0 & 0 & 0 \\
0 & \frac{1}{\tan(\frac{\theta}{2})} & 0 & 0 \\
0 & 0 & -\frac{f + n}{f - n} & -\frac{2fn}{f - n} \\
0 & 0 & -1 & 0
\end{bmatrix}$$

- $w_{\text{clip}} = -z_{\text{view}}$. During perspective division ($x/w, y/w, z/w$), distance-dependent foreshortening is achieved.

---

## 7. Orthographic Projection

### Mathematical Definition
Orthographic projection projects points along parallel lines perpendicular to the projection plane. Distance does not alter apparent object size.

```
       Parallel Rays
       ||         ||
       ||---------||    Near Plane
       ||         ||
       ||---------||    Far Plane
```

$$\mathbf{P}_{\text{ortho}} = \begin{bmatrix}
\frac{2}{r - l} & 0 & 0 & -\frac{r + l}{r - l} \\
0 & \frac{2}{t - b} & 0 & -\frac{t + b}{t - b} \\
0 & 0 & -\frac{2}{f - n} & -\frac{f + n}{f - n} \\
0 & 0 & 0 & 1
\end{bmatrix}$$

### Usage in Phase 3A
1. **Directional Sun Shadow Map**: Sunlight rays are parallel across the parking facility. Orthographic projection preserves constant shadow resolution across the entire lot.
2. **2D Top-Down Parking Map**: Orthographic projection maintains consistent scale ($1\text{m} = k\text{ pixels}$) allowing drivers and managers to measure distances accurately.

---

## 8. Model / View / Projection (MVP) Pipeline

Every vertex passes through a sequence of linear transformations:

```
  +------------------+
  | Object Space     | (Local vertex definitions: cube, cylinder, arrow)
  +------------------+
           |  Model Matrix (Translation, Rotation, Scale)
           v
  +------------------+
  | World Space      | (Global parking facility layout: (x, y, z) in meters)
  +------------------+
           |  View Matrix (Camera lookAt translation & orientation)
           v
  +------------------+
  | View/Eye Space   | (Coordinates relative to camera lens)
  +------------------+
           |  Projection Matrix (Perspective or Orthographic)
           v
  +------------------+
  | Clip Space       | (Homogeneous coordinates [-w, w])
  +------------------+
           |  Perspective Division (Divide by w)
           v
  +------------------+
  | NDC Space        | (Normalized Device Coordinates [-1, 1]^3)
  +------------------+
           |  Viewport Transformation (glViewport)
           v
  +------------------+
  | Screen/Window    | (Pixel raster grid coordinates)
  +------------------+
```

---

## 9. Blinn-Phong Illumination Model

### Academic Formulation
Blinn-Phong improves on classic Phong illumination by replacing the reflection vector $\mathbf{R}$ with the **halfway vector** $\mathbf{H}$:

$$\mathbf{H} = \frac{\mathbf{L} + \mathbf{V}}{\|\mathbf{L} + \mathbf{V}\|}$$

$$I_{\text{total}} = I_{\text{ambient}} + (1 - \text{shadow}) \cdot \left[ I_{\text{diffuse}} + I_{\text{specular}} \right] + I_{\text{emissive}}$$

### Component Formulations
1. **Ambient**: $I_{\text{ambient}} = k_a \cdot C_{\text{ambient}} \cdot C_{\text{diffuse}}$
2. **Diffuse (Lambert's Cosine Law)**:
   $$I_{\text{diffuse}} = k_d \cdot \max(\mathbf{N} \cdot \mathbf{L}, 0.0) \cdot C_{\text{light}} \cdot C_{\text{material}}$$
3. **Specular (Blinn Highlights)**:
   $$I_{\text{specular}} = k_s \cdot \left(\max(\mathbf{N} \cdot \mathbf{H}, 0.0)\right)^{\text{shininess}} \cdot C_{\text{light}}$$

### Point Light Attenuation (Lampposts)
For local lighting poles:
$$\text{attenuation} = \frac{1.0}{1.0 + 0.09 \cdot d + 0.032 \cdot d^2}$$

---

## 10. Depth Testing

### Purpose & Configuration
- Prevents rendering artifacts where background objects overwrite closer foreground surfaces.
- Enabled via:
  ```cpp
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  ```
- Before drawing each frame, depth values are reset to 1.0 via `glClear(GL_DEPTH_BUFFER_BIT)`.

---

## 11. Face Culling (Back-Face & Front-Face)

### Academic Concept
Closed polygonal meshes define polygons whose vertices are ordered counter-clockwise (CCW) when viewed from the outside. Polygons facing away from the camera are discarded before rasterization.

### Phase 3A Precision Culling
1. **Scene Rendering**: Discards rear-facing polygons to halve vertex shading workload:
   ```cpp
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   ```
2. **Shadow Depth Pass (Anti Peter-Panning)**:
   During shadow generation, **front faces are culled** (`glCullFace(GL_FRONT)`). This stores the depth of back surfaces in the shadow map, completely eliminating **Peter-Panning** (where objects appear to float disconnected from their shadows).

---

## Shadow Artifacts & Solutions Implemented

| Artifact | Cause | Solution Implemented |
| :--- | :--- | :--- |
| **Shadow Acne** | Limited depth precision causing surface self-shadowing | Dynamic slope-scale bias: `max(bias * (1.0 - dot(N, L)), bias * 0.2)` |
| **Peter Panning** | Excessive constant bias detaching shadow from object base | `glCullFace(GL_FRONT)` during depth pass |
| **Hard Jagged Edges** | Discrete texel resolution of the depth buffer | 3x3 Percentage-Closer Filtering (PCF) with linear texture sampling |
| **Frustum Boundary Shadows** | Sampling outside depth map returns 0.0 (shadowed) | `GL_CLAMP_TO_BORDER` with border color `vec4(1.0)` |

---

## Viva Voce Quick Revision Q&A

**Q1: Why use an orthographic projection for the directional light shadow map?**
*Answer*: Directional sunlight consists of parallel rays originating at infinite distance. Orthographic projection represents parallel light rays accurately without distance perspective distortion.

**Q2: What is the purpose of PCF in shadow mapping?**
*Answer*: Percentage-Closer Filtering samples a neighborhood of depth texels and computes an average occlusion percentage, producing smooth, soft penumbra transitions instead of aliased stair-step edges.

**Q3: Why is 2D mode insulated from shadow calculations?**
*Answer*: 2D top-down mode uses an orthographic map projection for administrative oversight. Bypassing shadow FBO generation in 2D maintains maximum framerates (60+ FPS) and provides clean, uncluttered visual contrast.
