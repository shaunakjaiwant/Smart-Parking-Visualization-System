#pragma once
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "ShadowMap.h"
#include "../parking/ParkingLot.h"
#include "../vehicle/Vehicle.h"
#include "../navigation/PathFinder.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

/**
 * @file Renderer.h
 * @brief High-performance dual 2D/3D graphics rendering engine with real-time shadow mapping.
 * 
 * WHAT: Renders parking slots, 3D vehicles, roads, landscape, real-time shadows, and navigation paths.
 * WHY:  Central computer graphics visualization component fulfilling Phase 3A graphics rigor.
 * HOW:  Executes shadow map depth pass (FBO), 2D/3D forward rendering passes with Blinn-Phong lighting,
 *       percentage-closer filtering (PCF), road markings, and day/night environment cycling.
 */
namespace SmartParking {

enum class TimeOfDay {
    DAY,
    NIGHT
};

struct RenderStats {
    int drawCalls = 0;
    int renderedSlots = 0;
    int renderedVehicles = 0;
    float frameTimeMs = 0.0f;
    int shadowMapResolution = 2048;
    int activePointLights = 0;
};

class Renderer {
public:
    Renderer();
    ~Renderer() = default;

    bool init(const std::string& shaderDir = "shaders");
    void setViewport(int width, int height);

    void beginFrame();
    void renderScene(const ParkingLot& lot,
                     const std::vector<std::unique_ptr<Vehicle>>& vehicles,
                     const Camera& camera,
                     const Route& activeRoute,
                     float animationTime,
                     bool showNavGraph = false,
                     bool wireframe = false);
    void endFrame();

    const RenderStats& getStats() const { return m_stats; }

    // Shaders & Shadow Mapping
    Shader& getBasicShader() { return m_basicShader; }
    Shader& getLightingShader() { return m_lightingShader; }
    Shader& getShadowShader() { return m_shadowShader; }
    ShadowMap& getShadowMap() { return m_shadowMap; }
    const ShadowMap& getShadowMap() const { return m_shadowMap; }

    // Day / Night Mode
    void setTimeOfDay(TimeOfDay tod) { m_timeOfDay = tod; }
    TimeOfDay getTimeOfDay() const { return m_timeOfDay; }
    void toggleDayNight() { m_timeOfDay = (m_timeOfDay == TimeOfDay::DAY) ? TimeOfDay::NIGHT : TimeOfDay::DAY; }

    // Toggles for Phase 3A Debug HUD
    bool renderShadows = true;
    bool renderRoadMarkingsEnabled = true;
    bool renderEnvironmentTrees = true;
    bool renderLightingEnabled = true;

private:
    int m_viewportWidth = 1600;
    int m_viewportHeight = 900;
    RenderStats m_stats;
    TimeOfDay m_timeOfDay = TimeOfDay::DAY;

    Shader m_basicShader;
    Shader m_lightingShader;
    Shader m_shadowShader;
    ShadowMap m_shadowMap;

    // Procedural 3D and 2D reusable meshes
    std::unique_ptr<Mesh> m_cubeMesh;
    std::unique_ptr<Mesh> m_planeMesh;
    std::unique_ptr<Mesh> m_cylinderMesh;
    std::unique_ptr<Mesh> m_sphereMesh;
    std::unique_ptr<Mesh> m_coneMesh;
    std::unique_ptr<Mesh> m_quadMesh;
    std::unique_ptr<Mesh> m_circleMesh;
    std::unique_ptr<Mesh> m_arrowMesh;

    // Route ribbon dynamic mesh
    std::unique_ptr<Mesh> m_routeMesh;
    std::vector<glm::vec3> m_cachedRoutePoints;

    void initMeshes();

    // Shadow Mapping Pass
    void renderShadowDepthPass(const ParkingLot& lot,
                               const std::vector<std::unique_ptr<Vehicle>>& vehicles);

    // 2D Rendering Pipeline
    void render2D(const ParkingLot& lot,
                  const std::vector<std::unique_ptr<Vehicle>>& vehicles,
                  const Camera& camera,
                  const Route& activeRoute,
                  float animationTime,
                  bool showNavGraph);

    // 3D Rendering Pipeline
    void render3D(const ParkingLot& lot,
                  const std::vector<std::unique_ptr<Vehicle>>& vehicles,
                  const Camera& camera,
                  const Route& activeRoute,
                  float animationTime,
                  bool showNavGraph);

    // Sub-renderers
    void render3DVehicle(const Vehicle& vehicle, const glm::mat4& view, const glm::mat4& proj, bool depthOnly = false);
    void render2DVehicle(const Vehicle& vehicle, const glm::mat4& view, const glm::mat4& proj);
    void render3DTree(const LandscapeItem& item, const glm::mat4& view, const glm::mat4& proj, bool depthOnly = false);
    void render3DLampPost(const glm::vec3& position, const glm::mat4& view, const glm::mat4& proj, bool depthOnly = false);
    void render3DSignPost(const LandscapeItem& item, const glm::mat4& view, const glm::mat4& proj, bool depthOnly = false);
    void renderNavigationGraph(const Graph& graph, const glm::mat4& view, const glm::mat4& proj, bool is3D, const Route* activeRoute = nullptr);
    void renderRouteLine(const Route& route, const glm::mat4& view, const glm::mat4& proj, bool is3D, float animTime);
    void renderSelectionBeacon(const glm::vec3& slotPos, const glm::mat4& view, const glm::mat4& proj, float animTime);
    void renderGate(const glm::vec3& position, bool isEntrance, const glm::mat4& view, const glm::mat4& proj, bool is3D, bool depthOnly = false);
    void renderRoadMarkings(const ParkingLot& lot, const Route& activeRoute, const glm::mat4& view, const glm::mat4& proj, bool is3D);
};

} // namespace SmartParking
