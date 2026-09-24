#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <unordered_set>

namespace SmartParking {

// Fallback shaders in case disk files are unavailable
static const char* fallbackBasicVert = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec4 aColor;
out vec4 vertexColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 overrideColor;
uniform bool useOverrideColor;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = useOverrideColor ? overrideColor : aColor;
}
)";

static const char* fallbackBasicFrag = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;
uniform float alphaMultiplier;
void main() {
    FragColor = vec4(vertexColor.rgb, vertexColor.a * (alphaMultiplier > 0.0 ? alphaMultiplier : 1.0));
}
)";

static const char* fallbackLightingVert = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec4 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 VertexColor;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    VertexColor = aColor;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    gl_Position = projection * view * worldPos;
}
)";

static const char* fallbackLightingFrag = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 VertexColor;
in vec4 FragPosLightSpace;

uniform vec3 materialColor;
uniform bool useMaterialColor;
uniform float shininess;
uniform float specularStrength;
uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;
uniform float dirLightIntensity;
uniform vec3 ambientColor;
uniform vec3 viewPos;
uniform vec3 emissiveColor;
uniform sampler2D shadowMap;
uniform bool enableShadows;
uniform float shadowBias;
uniform float shadowIntensity;

float calcShadow(vec4 lSpace, vec3 norm, vec3 lDir) {
    if (!enableShadows) return 0.0;
    vec3 proj = lSpace.xyz / lSpace.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) return 0.0;
    float bias = max(shadowBias * (1.0 - dot(norm, lDir)), shadowBias * 0.2);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcf = texture(shadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (proj.z - bias > pcf) ? 1.0 : 0.0;
        }
    }
    return (shadow / 9.0) * shadowIntensity;
}

void main() {
    vec3 baseColor = useMaterialColor ? materialColor : VertexColor.rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(-dirLightDirection);
    
    vec3 ambient = ambientColor * baseColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLightColor * dirLightIntensity * baseColor;
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess > 0.0 ? shininess : 32.0);
    vec3 specular = dirLightColor * (spec * specularStrength);
    
    float shadow = calcShadow(FragPosLightSpace, norm, lightDir);
    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular) + emissiveColor;
    FragColor = vec4(result, VertexColor.a > 0.0 ? VertexColor.a : 1.0);
}
)";

static const char* fallbackShadowVert = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;
void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

static const char* fallbackShadowFrag = R"(
#version 330 core
void main() {
}
)";

Renderer::Renderer() {
}

bool Renderer::init(const std::string& shaderDir) {
    // 1. Basic shader
    bool basicLoaded = m_basicShader.loadFromFiles(shaderDir + "/basic.vert", shaderDir + "/basic.frag");
    if (!basicLoaded) {
        std::cout << "[Renderer] Compiling fallback basic shaders." << std::endl;
        basicLoaded = m_basicShader.loadFromSource(fallbackBasicVert, fallbackBasicFrag);
    }

    // 2. Lighting shader
    bool lightingLoaded = m_lightingShader.loadFromFiles(shaderDir + "/lighting.vert", shaderDir + "/lighting.frag");
    if (!lightingLoaded) {
        std::cout << "[Renderer] Compiling fallback lighting shaders." << std::endl;
        lightingLoaded = m_lightingShader.loadFromSource(fallbackLightingVert, fallbackLightingFrag);
    }

    // 3. Shadow depth shader
    bool shadowLoaded = m_shadowShader.loadFromFiles(shaderDir + "/shadow_depth.vert", shaderDir + "/shadow_depth.frag");
    if (!shadowLoaded) {
        std::cout << "[Renderer] Compiling fallback shadow depth shaders." << std::endl;
        shadowLoaded = m_shadowShader.loadFromSource(fallbackShadowVert, fallbackShadowFrag);
    }

    if (!basicLoaded || !lightingLoaded || !shadowLoaded) {
        std::cerr << "[Renderer Error] Critical failure initializing shader pipelines." << std::endl;
        return false;
    }

    // 4. Initialize Shadow Map Depth FBO (2048x2048)
    if (!m_shadowMap.init(2048)) {
        std::cerr << "[Renderer Error] Failed to initialize shadow map depth buffer." << std::endl;
    }

    initMeshes();
    return true;
}

void Renderer::initMeshes() {
    m_cubeMesh = Mesh::createCube(glm::vec3(1.0f));
    m_planeMesh = Mesh::createPlane(1.0f, 1.0f);
    m_cylinderMesh = Mesh::createCylinder(0.5f, 1.0f, 16);
    m_sphereMesh = Mesh::createSphere(0.5f, 12, 16);
    m_coneMesh = Mesh::createCone(0.65f, 1.3f, 16);
    m_quadMesh = Mesh::create2DQuad(1.0f, 1.0f);
    m_circleMesh = Mesh::create2DCircle(1.0f, 24);
    m_arrowMesh = Mesh::createDirectionArrow(2.0f, 1.2f);
}

void Renderer::setViewport(int width, int height) {
    m_viewportWidth = width > 0 ? width : 1600;
    m_viewportHeight = height > 0 ? height : 900;
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
}

void Renderer::beginFrame() {
    m_stats.drawCalls = 0;
    m_stats.renderedSlots = 0;
    m_stats.renderedVehicles = 0;
    m_stats.shadowMapResolution = m_shadowMap.getResolution();

    // Day/Night atmosphere clear color
    if (m_timeOfDay == TimeOfDay::DAY) {
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f); // Sleek modern dark charcoal
    } else {
        glClearColor(0.03f, 0.04f, 0.06f, 1.0f); // Midnight dark blue/black
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::endFrame() {
}

void Renderer::renderScene(const ParkingLot& lot,
                           const std::vector<std::unique_ptr<Vehicle>>& vehicles,
                           const Camera& camera,
                           const Route& activeRoute,
                           float animationTime,
                           bool showNavGraph,
                           bool wireframe) {
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (camera.getMode() == CameraMode::MODE_2D_TOPDOWN) {
        // 2D Mode: Fast orthographic render (Shadows bypassed to protect 2D speed & clarity)
        render2D(lot, vehicles, camera, activeRoute, animationTime, showNavGraph);
    } else {
        // 3D Mode: Two-Pass Real-Time Shadow Mapping
        if (renderShadows && m_shadowMap.isEnabled()) {
            renderShadowDepthPass(lot, vehicles);
        }
        render3D(lot, vehicles, camera, activeRoute, animationTime, showNavGraph);
    }

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Renderer::renderShadowDepthPass(const ParkingLot& lot,
                                    const std::vector<std::unique_ptr<Vehicle>>& vehicles) {
    m_shadowMap.bindForWriting();
    m_shadowShader.use();
    glm::mat4 lightSpaceMatrix = m_shadowMap.getLightSpaceMatrix();
    m_shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Front-face culling during shadow pass prevents Peter-Panning artifacts
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    // 1. Cast shadows from Vehicles
    for (const auto& vehicle : vehicles) {
        render3DVehicle(*vehicle, glm::mat4(1.0f), lightSpaceMatrix, true);
    }

    // 2. Cast shadows from Gates
    renderGate(lot.getEntrancePosition(), true, glm::mat4(1.0f), lightSpaceMatrix, true, true);
    renderGate(lot.getExitPosition(), false, glm::mat4(1.0f), lightSpaceMatrix, true, true);

    // 3. Cast shadows from Trees, Lampposts, Signposts, Buildings
    if (renderEnvironmentTrees) {
        for (const auto& item : lot.getLandscapeItems()) {
            if (item.type == LandscapeItem::Type::TREE_SPHERICAL ||
                item.type == LandscapeItem::Type::TREE_CONIFER ||
                item.type == LandscapeItem::Type::TREE_ORNAMENTAL) {
                render3DTree(item, glm::mat4(1.0f), lightSpaceMatrix, true);
            } else if (item.type == LandscapeItem::Type::LAMP_POST) {
                render3DLampPost(item.position, glm::mat4(1.0f), lightSpaceMatrix, true);
            } else if (item.type == LandscapeItem::Type::SIGN_POST) {
                render3DSignPost(item, glm::mat4(1.0f), lightSpaceMatrix, true);
            } else if (item.type == LandscapeItem::Type::BUILDING) {
                glm::mat4 bModel = glm::mat4(1.0f);
                bModel = glm::translate(bModel, item.position + glm::vec3(0.0f, item.scale.y * 0.5f, 0.0f));
                bModel = glm::scale(bModel, item.scale);
                m_shadowShader.setMat4("model", bModel);
                m_cubeMesh->draw();
            }
        }
    }

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);

    m_shadowShader.unuse();
    m_shadowMap.unbind(m_viewportWidth, m_viewportHeight);
}

void Renderer::render2D(const ParkingLot& lot,
                        const std::vector<std::unique_ptr<Vehicle>>& vehicles,
                        const Camera& camera,
                        const Route& activeRoute,
                        float animationTime,
                        bool showNavGraph) {
    float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspect);

    m_basicShader.use();
    m_basicShader.setMat4("view", view);
    m_basicShader.setMat4("projection", proj);
    m_basicShader.setBool("useOverrideColor", true);
    m_basicShader.setFloat("alphaMultiplier", 1.0f);

    // 1. Asphalt Ground
    glm::mat4 groundModel = glm::mat4(1.0f);
    groundModel = glm::scale(groundModel, glm::vec3(120.0f, 1.0f, 90.0f));
    m_basicShader.setMat4("model", groundModel);
    m_basicShader.setVec4("overrideColor", glm::vec4(0.12f, 0.13f, 0.16f, 1.0f));
    m_quadMesh->draw();
    m_stats.drawCalls++;

    // 2. Roads
    for (const auto& road : lot.getAllRoads()) {
        glm::vec3 diff = road.to - road.from;
        float length = glm::length(diff);
        glm::vec3 center = (road.from + road.to) * 0.5f;
        float angle = glm::degrees(std::atan2(diff.x, diff.z));

        glm::mat4 rModel = glm::mat4(1.0f);
        rModel = glm::translate(rModel, glm::vec3(center.x, 0.01f, center.z));
        rModel = glm::rotate(rModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        rModel = glm::scale(rModel, glm::vec3(road.width, 1.0f, length));

        m_basicShader.setMat4("model", rModel);
        m_basicShader.setVec4("overrideColor", glm::vec4(0.18f, 0.20f, 0.23f, 1.0f));
        m_quadMesh->draw();
        m_stats.drawCalls++;
    }

    // 3. Road Markings & Direction Arrows in 2D
    if (renderRoadMarkingsEnabled) {
        renderRoadMarkings(lot, activeRoute, view, proj, false);
    }

    // 4. Parking Slots
    const ParkingSlot* selectedSlot = lot.getSelectedSlot();
    for (const auto& slot : lot.getAllSlots()) {
        bool isSel = (selectedSlot && selectedSlot->getId() == slot.getId());
        glm::vec4 color = slot.getStatusColor(isSel);

        // Fill quad
        glm::mat4 sModel = glm::mat4(1.0f);
        sModel = glm::translate(sModel, glm::vec3(slot.getPosition().x, 0.02f, slot.getPosition().z));
        sModel = glm::rotate(sModel, glm::radians(slot.getRotationDeg()), glm::vec3(0.0f, 1.0f, 0.0f));
        sModel = glm::scale(sModel, glm::vec3(slot.getDimensions().x, 1.0f, slot.getDimensions().y));

        m_basicShader.setMat4("model", sModel);
        m_basicShader.setVec4("overrideColor", color);
        m_quadMesh->draw();
        m_stats.drawCalls++;
        m_stats.renderedSlots++;

        // White border outline
        glm::mat4 borderModel = glm::mat4(1.0f);
        borderModel = glm::translate(borderModel, glm::vec3(slot.getPosition().x, 0.025f, slot.getPosition().z));
        borderModel = glm::rotate(borderModel, glm::radians(slot.getRotationDeg()), glm::vec3(0.0f, 1.0f, 0.0f));
        borderModel = glm::scale(borderModel, glm::vec3(slot.getDimensions().x * 0.94f, 1.0f, slot.getDimensions().y * 0.96f));

        m_basicShader.setMat4("model", borderModel);
        m_basicShader.setVec4("overrideColor", isSel ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.35f, 0.38f, 0.42f, 0.8f));
        m_quadMesh->draw();
        m_stats.drawCalls++;
    }

    // 5. Vehicles in 2D
    for (const auto& vehicle : vehicles) {
        render2DVehicle(*vehicle, view, proj);
        m_stats.renderedVehicles++;
    }

    // 6. Entrance & Exit Gate markings in 2D
    renderGate(lot.getEntrancePosition(), true, view, proj, false);
    renderGate(lot.getExitPosition(), false, view, proj, false);

    // 7. Active Route Visualization
    if (activeRoute.isValid) {
        renderRouteLine(activeRoute, view, proj, false, animationTime);
    }

    // 8. Navigation Graph (Debug)
    if (showNavGraph) {
        renderNavigationGraph(lot.getNavigationGraph(), view, proj, false, &activeRoute);
    }

    m_basicShader.unuse();
}

void Renderer::render3D(const ParkingLot& lot,
                        const std::vector<std::unique_ptr<Vehicle>>& vehicles,
                        const Camera& camera,
                        const Route& activeRoute,
                        float animationTime,
                        bool showNavGraph) {
    float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspect);

    // Setup Lighting Shader with Real-Time Shadow Mapping
    m_lightingShader.use();
    m_lightingShader.setMat4("view", view);
    m_lightingShader.setMat4("projection", proj);
    m_lightingShader.setVec3("viewPos", camera.getPosition());

    // Bind Depth Texture from ShadowMap FBO to Texture Unit 1
    m_shadowMap.bindForReading(GL_TEXTURE1);
    m_lightingShader.setInt("shadowMap", 1);
    m_lightingShader.setBool("enableShadows", renderShadows && m_shadowMap.isEnabled());
    m_lightingShader.setFloat("shadowBias", m_shadowMap.getBias());
    m_lightingShader.setFloat("shadowIntensity", (m_timeOfDay == TimeOfDay::DAY) ? m_shadowMap.getIntensity() : 0.45f);
    m_lightingShader.setMat4("lightSpaceMatrix", m_shadowMap.getLightSpaceMatrix());

    // Day / Night Illumination Parameters
    if (m_timeOfDay == TimeOfDay::DAY) {
        m_lightingShader.setVec3("dirLightDirection", m_shadowMap.getLightDirection());
        m_lightingShader.setVec3("dirLightColor", glm::vec3(1.0f, 0.98f, 0.92f));
        m_lightingShader.setFloat("dirLightIntensity", renderLightingEnabled ? 1.15f : 0.0f);
        m_lightingShader.setVec3("ambientColor", glm::vec3(0.24f, 0.27f, 0.31f));
    } else {
        // Night Mode: Cool pale moonlight
        m_lightingShader.setVec3("dirLightDirection", glm::normalize(glm::vec3(0.3f, -0.9f, 0.4f)));
        m_lightingShader.setVec3("dirLightColor", glm::vec3(0.35f, 0.45f, 0.65f));
        m_lightingShader.setFloat("dirLightIntensity", renderLightingEnabled ? 0.35f : 0.0f);
        m_lightingShader.setVec3("ambientColor", glm::vec3(0.06f, 0.08f, 0.12f));
    }

    m_lightingShader.setFloat("shininess", 32.0f);
    m_lightingShader.setFloat("specularStrength", 0.5f);
    m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));

    // Dynamic Point Lights for Lampposts (Night atmosphere illumination)
    int pointLightIdx = 0;
    for (const auto& item : lot.getLandscapeItems()) {
        if (item.type == LandscapeItem::Type::LAMP_POST && pointLightIdx < 6) {
            std::string prefix = "pointLights[" + std::to_string(pointLightIdx) + "].";
            m_lightingShader.setVec3(prefix + "position", item.position + glm::vec3(0.0f, 4.5f, 0.0f));
            m_lightingShader.setVec3(prefix + "color", glm::vec3(1.0f, 0.92f, 0.78f));
            m_lightingShader.setFloat(prefix + "intensity", (m_timeOfDay == TimeOfDay::NIGHT) ? 1.4f : 0.25f);
            pointLightIdx++;
        }
    }
    m_lightingShader.setInt("numPointLights", pointLightIdx);
    m_stats.activePointLights = pointLightIdx;

    // 1. Ground Plane (Receives Shadows)
    glm::mat4 groundModel = glm::mat4(1.0f);
    groundModel = glm::translate(groundModel, glm::vec3(0.0f, -0.05f, 0.0f));
    groundModel = glm::scale(groundModel, glm::vec3(130.0f, 1.0f, 100.0f));
    m_lightingShader.setMat4("model", groundModel);
    m_lightingShader.setBool("useMaterialColor", true);
    m_lightingShader.setVec3("materialColor", (m_timeOfDay == TimeOfDay::DAY) ? glm::vec3(0.12f, 0.13f, 0.15f) : glm::vec3(0.07f, 0.08f, 0.09f));
    m_planeMesh->draw();
    m_stats.drawCalls++;

    // 2. Roads in 3D (Dark neutral asphalt material)
    for (const auto& road : lot.getAllRoads()) {
        glm::vec3 diff = road.to - road.from;
        float length = glm::length(diff);
        glm::vec3 center = (road.from + road.to) * 0.5f;
        float angle = glm::degrees(std::atan2(diff.x, diff.z));

        glm::mat4 rModel = glm::mat4(1.0f);
        rModel = glm::translate(rModel, glm::vec3(center.x, 0.01f, center.z));
        rModel = glm::rotate(rModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        rModel = glm::scale(rModel, glm::vec3(road.width, 1.0f, length));

        m_lightingShader.setMat4("model", rModel);
        m_lightingShader.setVec3("materialColor", (m_timeOfDay == TimeOfDay::DAY) ? glm::vec3(0.18f, 0.20f, 0.23f) : glm::vec3(0.11f, 0.12f, 0.14f));
        m_planeMesh->draw();
        m_stats.drawCalls++;
    }

    // 3. Road Markings & Direction Arrows in 3D
    if (renderRoadMarkingsEnabled) {
        renderRoadMarkings(lot, activeRoute, view, proj, true);
    }

    // 4. Parking Slots in 3D
    const ParkingSlot* selectedSlot = lot.getSelectedSlot();
    for (const auto& slot : lot.getAllSlots()) {
        bool isSel = (selectedSlot && selectedSlot->getId() == slot.getId());
        glm::vec4 stateCol = slot.getStatusColor(isSel);

        // Ground slot bay surface
        glm::mat4 sModel = glm::mat4(1.0f);
        sModel = glm::translate(sModel, glm::vec3(slot.getPosition().x, 0.02f, slot.getPosition().z));
        sModel = glm::rotate(sModel, glm::radians(slot.getRotationDeg()), glm::vec3(0.0f, 1.0f, 0.0f));
        sModel = glm::scale(sModel, glm::vec3(slot.getDimensions().x * 0.95f, 1.0f, slot.getDimensions().y * 0.95f));

        m_lightingShader.setMat4("model", sModel);
        m_lightingShader.setVec3("materialColor", glm::vec3(stateCol));
        if (isSel) {
            float pulse = 0.5f + 0.5f * sin(animationTime * 5.0f);
            m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f, 0.4f * pulse, 0.5f * pulse));
        } else {
            m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
        }
        m_planeMesh->draw();
        m_stats.drawCalls++;
        m_stats.renderedSlots++;

        // Curbs / Concrete dividers at slot head
        float curbZ = (slot.getPosition().z > 0.0f) ? (slot.getPosition().z + slot.getDimensions().y * 0.48f)
                                                    : (slot.getPosition().z - slot.getDimensions().y * 0.48f);
        glm::mat4 curbModel = glm::mat4(1.0f);
        curbModel = glm::translate(curbModel, glm::vec3(slot.getPosition().x, 0.08f, curbZ));
        curbModel = glm::scale(curbModel, glm::vec3(slot.getDimensions().x * 0.8f, 0.16f, 0.25f));

        m_lightingShader.setMat4("model", curbModel);
        m_lightingShader.setVec3("materialColor", glm::vec3(0.7f, 0.72f, 0.75f));
        m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
        m_cubeMesh->draw();
        m_stats.drawCalls++;
    }

    // 5. Environmental Landscape Objects
    if (renderEnvironmentTrees) {
        for (const auto& item : lot.getLandscapeItems()) {
            if (item.type == LandscapeItem::Type::TREE_SPHERICAL ||
                item.type == LandscapeItem::Type::TREE_CONIFER ||
                item.type == LandscapeItem::Type::TREE_ORNAMENTAL) {
                render3DTree(item, view, proj, false);
            } else if (item.type == LandscapeItem::Type::LAMP_POST) {
                render3DLampPost(item.position, view, proj, false);
            } else if (item.type == LandscapeItem::Type::SIGN_POST) {
                render3DSignPost(item, view, proj, false);
            } else if (item.type == LandscapeItem::Type::BUILDING) {
                glm::mat4 bModel = glm::mat4(1.0f);
                bModel = glm::translate(bModel, item.position + glm::vec3(0.0f, item.scale.y * 0.5f, 0.0f));
                bModel = glm::scale(bModel, item.scale);
                m_lightingShader.setMat4("model", bModel);
                m_lightingShader.setVec3("materialColor", glm::vec3(0.28f, 0.32f, 0.38f));
                m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
                m_cubeMesh->draw();
                m_stats.drawCalls++;
            }
        }
    }

    // 6. Entrance & Exit 3D Gate Structures
    renderGate(lot.getEntrancePosition(), true, view, proj, true, false);
    renderGate(lot.getExitPosition(), false, view, proj, true, false);

    // 7. Vehicles in 3D
    for (const auto& vehicle : vehicles) {
        render3DVehicle(*vehicle, view, proj, false);
        m_stats.renderedVehicles++;
    }

    // 8. Selection Beacon (if slot selected)
    if (selectedSlot) {
        renderSelectionBeacon(selectedSlot->getPosition(), view, proj, animationTime);
    }

    m_lightingShader.unuse();

    // 9. Route Ribbon and Navigation Graph Overlays (Basic Shader)
    m_basicShader.use();
    m_basicShader.setMat4("view", view);
    m_basicShader.setMat4("projection", proj);
    m_basicShader.setBool("useOverrideColor", true);

    if (activeRoute.isValid) {
        renderRouteLine(activeRoute, view, proj, true, animationTime);
    }

    if (showNavGraph) {
        renderNavigationGraph(lot.getNavigationGraph(), view, proj, true, &activeRoute);
    }

    m_basicShader.unuse();
}

void Renderer::renderRoadMarkings(const ParkingLot& lot, const Route& activeRoute, const glm::mat4& view, const glm::mat4& proj, bool is3D) {
    for (const auto& marking : lot.getAllRoadMarkings()) {
        glm::mat4 mModel = glm::mat4(1.0f);
        mModel = glm::translate(mModel, marking.position);
        mModel = glm::rotate(mModel, glm::radians(marking.rotationDeg), glm::vec3(0.0f, 1.0f, 0.0f));

        if (marking.type == RoadMarking::Type::ARROW_FORWARD ||
            marking.type == RoadMarking::Type::ARROW_LEFT ||
            marking.type == RoadMarking::Type::ARROW_RIGHT) {
            
            // Check if marking is near active route waypoint to highlight
            bool onRoute = false;
            if (activeRoute.isValid) {
                for (const auto& wp : activeRoute.waypoints) {
                    if (glm::distance(glm::vec2(marking.position.x, marking.position.z), glm::vec2(wp.x, wp.z)) < 3.2f) {
                        onRoute = true;
                        break;
                    }
                }
            }

            mModel = glm::scale(mModel, glm::vec3(marking.size.y, 1.0f, marking.size.x));
            if (is3D) {
                m_lightingShader.setMat4("model", mModel);
                m_lightingShader.setVec3("materialColor", onRoute ? glm::vec3(1.0f, 0.85f, 0.1f) : glm::vec3(0.92f, 0.94f, 0.96f));
                m_lightingShader.setVec3("emissiveColor", onRoute ? glm::vec3(0.5f, 0.4f, 0.05f) : glm::vec3(0.0f));
            } else {
                m_basicShader.setMat4("model", mModel);
                m_basicShader.setVec4("overrideColor", onRoute ? glm::vec4(1.0f, 0.85f, 0.1f, 1.0f) : glm::vec4(0.92f, 0.94f, 0.96f, 0.9f));
            }
            m_arrowMesh->draw();
            m_stats.drawCalls++;
        } else {
            // Rectangular markings: Lane dividers, Stop lines, Crosswalks
            mModel = glm::scale(mModel, glm::vec3(marking.size.x, 1.0f, marking.size.y));
            glm::vec4 markCol(0.94f, 0.94f, 0.96f, 0.92f);
            if (marking.type == RoadMarking::Type::LANE_DIVIDER) {
                markCol = glm::vec4(0.96f, 0.88f, 0.25f, 0.92f); // Traffic yellow dashes
            }

            if (is3D) {
                m_lightingShader.setMat4("model", mModel);
                m_lightingShader.setVec3("materialColor", glm::vec3(markCol));
                m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
                m_planeMesh->draw();
            } else {
                m_basicShader.setMat4("model", mModel);
                m_basicShader.setVec4("overrideColor", markCol);
                m_quadMesh->draw();
            }
            m_stats.drawCalls++;
        }
    }
}

void Renderer::render3DVehicle(const Vehicle& vehicle, const glm::mat4& view, const glm::mat4& proj, bool depthOnly) {
    glm::vec3 pos = vehicle.getPosition();
    float heading = vehicle.getHeadingDeg();
    glm::vec3 col = vehicle.getColor();

    glm::mat4 baseTransform = glm::mat4(1.0f);
    baseTransform = glm::translate(baseTransform, glm::vec3(pos.x, 0.0f, pos.z));
    baseTransform = glm::rotate(baseTransform, glm::radians(heading), glm::vec3(0.0f, 1.0f, 0.0f));

    // A. Main Chassis Body
    glm::mat4 bodyModel = glm::translate(baseTransform, glm::vec3(0.0f, 0.55f, 0.0f));
    bodyModel = glm::scale(bodyModel, glm::vec3(2.1f, 0.65f, 4.2f));

    if (depthOnly) {
        m_shadowShader.setMat4("model", bodyModel);
        m_cubeMesh->draw();
    } else {
        m_lightingShader.setMat4("model", bodyModel);
        m_lightingShader.setBool("useMaterialColor", true);
        m_lightingShader.setVec3("materialColor", col);
        m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
        m_cubeMesh->draw();
        m_stats.drawCalls++;
    }

    // B. Cabin & Tinted Windows
    glm::mat4 cabinModel = glm::translate(baseTransform, glm::vec3(0.0f, 1.15f, -0.2f));
    cabinModel = glm::scale(cabinModel, glm::vec3(1.8f, 0.65f, 2.3f));

    if (depthOnly) {
        m_shadowShader.setMat4("model", cabinModel);
        m_cubeMesh->draw();
    } else {
        m_lightingShader.setMat4("model", cabinModel);
        m_lightingShader.setVec3("materialColor", glm::vec3(0.1f, 0.15f, 0.2f));
        m_cubeMesh->draw();
        m_stats.drawCalls++;
    }

    // C. 4 Wheels (Cylinders)
    float wheelOffsetsX[2] = { -1.05f, 1.05f };
    float wheelOffsetsZ[2] = { 1.25f, -1.25f };
    for (float ox : wheelOffsetsX) {
        for (float oz : wheelOffsetsZ) {
            glm::mat4 wheelModel = glm::translate(baseTransform, glm::vec3(ox, 0.35f, oz));
            wheelModel = glm::rotate(wheelModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            wheelModel = glm::scale(wheelModel, glm::vec3(0.7f, 0.25f, 0.7f));

            if (depthOnly) {
                m_shadowShader.setMat4("model", wheelModel);
                m_cylinderMesh->draw();
            } else {
                m_lightingShader.setMat4("model", wheelModel);
                m_lightingShader.setVec3("materialColor", glm::vec3(0.05f, 0.05f, 0.06f));
                m_cylinderMesh->draw();
                m_stats.drawCalls++;
            }
        }
    }

    if (!depthOnly) {
        // D. Headlights (Bright Yellow Emissive, illuminating forward in Night Mode)
        float hlOffsetsX[2] = { -0.75f, 0.75f };
        for (float ox : hlOffsetsX) {
            glm::mat4 hlModel = glm::translate(baseTransform, glm::vec3(ox, 0.6f, 2.12f));
            hlModel = glm::scale(hlModel, glm::vec3(0.35f, 0.18f, 0.1f));
            m_lightingShader.setMat4("model", hlModel);
            m_lightingShader.setVec3("materialColor", glm::vec3(1.0f, 0.95f, 0.6f));
            float emMult = (m_timeOfDay == TimeOfDay::NIGHT) ? 2.5f : 1.2f;
            m_lightingShader.setVec3("emissiveColor", glm::vec3(1.2f, 1.1f, 0.7f) * emMult);
            m_cubeMesh->draw();
            m_stats.drawCalls++;
        }

        // E. Taillights (Red Emissive)
        for (float ox : hlOffsetsX) {
            glm::mat4 tlModel = glm::translate(baseTransform, glm::vec3(ox, 0.6f, -2.12f));
            tlModel = glm::scale(tlModel, glm::vec3(0.35f, 0.18f, 0.1f));
            m_lightingShader.setMat4("model", tlModel);
            m_lightingShader.setVec3("materialColor", glm::vec3(0.9f, 0.1f, 0.1f));
            m_lightingShader.setVec3("emissiveColor", glm::vec3(1.2f, 0.08f, 0.08f));
            m_cubeMesh->draw();
            m_stats.drawCalls++;
        }
    }
}

void Renderer::render2DVehicle(const Vehicle& vehicle, const glm::mat4& view, const glm::mat4& proj) {
    glm::vec3 pos = vehicle.getPosition();
    float heading = vehicle.getHeadingDeg();
    glm::vec3 col = vehicle.getColor();

    glm::mat4 vModel = glm::mat4(1.0f);
    vModel = glm::translate(vModel, glm::vec3(pos.x, 0.05f, pos.z));
    vModel = glm::rotate(vModel, glm::radians(heading), glm::vec3(0.0f, 1.0f, 0.0f));
    vModel = glm::scale(vModel, glm::vec3(2.1f, 1.0f, 4.2f));

    m_basicShader.setMat4("model", vModel);
    m_basicShader.setVec4("overrideColor", glm::vec4(col, 1.0f));
    m_quadMesh->draw();
    m_stats.drawCalls++;

    // Headlight dots in 2D
    glm::mat4 hl1 = glm::translate(vModel, glm::vec3(-0.35f, 0.01f, 0.45f));
    hl1 = glm::scale(hl1, glm::vec3(0.2f, 1.0f, 0.15f));
    m_basicShader.setMat4("model", hl1);
    m_basicShader.setVec4("overrideColor", glm::vec4(1.0f, 1.0f, 0.3f, 1.0f));
    m_quadMesh->draw();

    glm::mat4 hl2 = glm::translate(vModel, glm::vec3(0.35f, 0.01f, 0.45f));
    hl2 = glm::scale(hl2, glm::vec3(0.2f, 1.0f, 0.15f));
    m_basicShader.setMat4("model", hl2);
    m_basicShader.setVec4("overrideColor", glm::vec4(1.0f, 1.0f, 0.3f, 1.0f));
    m_quadMesh->draw();
    m_stats.drawCalls += 2;
}

void Renderer::render3DTree(const LandscapeItem& item, const glm::mat4& view, const glm::mat4& proj, bool depthOnly) {
    float scale = item.scale.y > 0.0f ? item.scale.y * 0.3f : 1.0f;
    glm::vec3 position = item.position;

    // A. Trunk (Common to all tree types)
    glm::mat4 trunkModel = glm::mat4(1.0f);
    trunkModel = glm::translate(trunkModel, position + glm::vec3(0.0f, 1.0f * scale, 0.0f));
    trunkModel = glm::scale(trunkModel, glm::vec3(0.42f * scale, 2.0f * scale, 0.42f * scale));

    if (depthOnly) {
        m_shadowShader.setMat4("model", trunkModel);
        m_cylinderMesh->draw();
    } else {
        m_lightingShader.setMat4("model", trunkModel);
        m_lightingShader.setVec3("materialColor", glm::vec3(0.35f, 0.22f, 0.12f));
        m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
        m_cylinderMesh->draw();
        m_stats.drawCalls++;
    }

    // B. Canopy Variants
    if (item.type == LandscapeItem::Type::TREE_CONIFER) {
        // Tree Type B: Layered Conical Evergreen / Spruce (3 stacked cones)
        float coneHeights[3] = { 1.8f * scale, 2.6f * scale, 3.4f * scale };
        float coneRadii[3] = { 2.2f * scale, 1.7f * scale, 1.2f * scale };
        glm::vec3 coniferColors[3] = {
            glm::vec3(0.08f, 0.32f, 0.14f),
            glm::vec3(0.10f, 0.38f, 0.16f),
            glm::vec3(0.12f, 0.44f, 0.19f)
        };

        for (int i = 0; i < 3; ++i) {
            glm::mat4 cModel = glm::mat4(1.0f);
            cModel = glm::translate(cModel, position + glm::vec3(0.0f, coneHeights[i], 0.0f));
            cModel = glm::scale(cModel, glm::vec3(coneRadii[i], 1.5f * scale, coneRadii[i]));

            if (depthOnly) {
                m_shadowShader.setMat4("model", cModel);
                m_coneMesh->draw();
            } else {
                m_lightingShader.setMat4("model", cModel);
                m_lightingShader.setVec3("materialColor", coniferColors[i]);
                m_coneMesh->draw();
                m_stats.drawCalls++;
            }
        }
    } else if (item.type == LandscapeItem::Type::TREE_ORNAMENTAL) {
        // Tree Type C: Ornamental flowering bush/compact tree
        glm::mat4 bushModel = glm::mat4(1.0f);
        bushModel = glm::translate(bushModel, position + glm::vec3(0.0f, 1.8f * scale, 0.0f));
        bushModel = glm::scale(bushModel, glm::vec3(1.8f * scale, 1.5f * scale, 1.8f * scale));

        if (depthOnly) {
            m_shadowShader.setMat4("model", bushModel);
            m_sphereMesh->draw();
        } else {
            m_lightingShader.setMat4("model", bushModel);
            m_lightingShader.setVec3("materialColor", glm::vec3(0.24f, 0.55f, 0.28f));
            m_sphereMesh->draw();
            m_stats.drawCalls++;
        }
    } else {
        // Tree Type A: Classic Spherical Canopy (2 overlapping spheres)
        glm::mat4 fol1 = glm::mat4(1.0f);
        fol1 = glm::translate(fol1, position + glm::vec3(0.0f, 2.6f * scale, 0.0f));
        fol1 = glm::scale(fol1, glm::vec3(2.4f * scale, 2.0f * scale, 2.4f * scale));

        glm::mat4 fol2 = glm::mat4(1.0f);
        fol2 = glm::translate(fol2, position + glm::vec3(0.0f, 3.6f * scale, 0.0f));
        fol2 = glm::scale(fol2, glm::vec3(1.8f * scale, 1.6f * scale, 1.8f * scale));

        if (depthOnly) {
            m_shadowShader.setMat4("model", fol1);
            m_sphereMesh->draw();
            m_shadowShader.setMat4("model", fol2);
            m_sphereMesh->draw();
        } else {
            m_lightingShader.setMat4("model", fol1);
            m_lightingShader.setVec3("materialColor", glm::vec3(0.12f, 0.45f, 0.18f));
            m_sphereMesh->draw();

            m_lightingShader.setMat4("model", fol2);
            m_lightingShader.setVec3("materialColor", glm::vec3(0.16f, 0.55f, 0.22f));
            m_sphereMesh->draw();
            m_stats.drawCalls += 2;
        }
    }
}

void Renderer::render3DLampPost(const glm::vec3& position, const glm::mat4& view, const glm::mat4& proj, bool depthOnly) {
    // Pole
    glm::mat4 pole = glm::mat4(1.0f);
    pole = glm::translate(pole, position + glm::vec3(0.0f, 2.25f, 0.0f));
    pole = glm::scale(pole, glm::vec3(0.15f, 4.5f, 0.15f));

    if (depthOnly) {
        m_shadowShader.setMat4("model", pole);
        m_cylinderMesh->draw();
    } else {
        m_lightingShader.setMat4("model", pole);
        m_lightingShader.setVec3("materialColor", glm::vec3(0.4f, 0.42f, 0.45f));
        m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
        m_cylinderMesh->draw();
        m_stats.drawCalls++;
    }

    // Light fixture head
    glm::mat4 fixture = glm::mat4(1.0f);
    fixture = glm::translate(fixture, position + glm::vec3(0.0f, 4.5f, 0.0f));
    fixture = glm::scale(fixture, glm::vec3(0.6f, 0.25f, 0.6f));

    if (depthOnly) {
        m_shadowShader.setMat4("model", fixture);
        m_cubeMesh->draw();
    } else {
        m_lightingShader.setMat4("model", fixture);
        m_lightingShader.setVec3("materialColor", glm::vec3(1.0f, 0.98f, 0.85f));
        float lampEmissive = (m_timeOfDay == TimeOfDay::NIGHT) ? 3.0f : 1.2f;
        m_lightingShader.setVec3("emissiveColor", glm::vec3(1.4f, 1.3f, 0.9f) * lampEmissive);
        m_cubeMesh->draw();
        m_stats.drawCalls++;
    }
}

void Renderer::render3DSignPost(const LandscapeItem& item, const glm::mat4& view, const glm::mat4& proj, bool depthOnly) {
    // Post
    glm::mat4 post = glm::mat4(1.0f);
    post = glm::translate(post, item.position + glm::vec3(0.0f, 1.1f, 0.0f));
    post = glm::scale(post, glm::vec3(0.12f, 2.2f, 0.12f));

    if (depthOnly) {
        m_shadowShader.setMat4("model", post);
        m_cylinderMesh->draw();
    } else {
        m_lightingShader.setMat4("model", post);
        m_lightingShader.setVec3("materialColor", glm::vec3(0.5f, 0.52f, 0.55f));
        m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
        m_cylinderMesh->draw();
        m_stats.drawCalls++;
    }

    // Signboard
    glm::mat4 board = glm::mat4(1.0f);
    board = glm::translate(board, item.position + glm::vec3(0.0f, 2.2f, 0.0f));
    board = glm::scale(board, glm::vec3(item.scale.x, item.scale.y * 0.45f, 0.15f));

    glm::vec3 signCol(0.2f, 0.45f, 0.85f); // Blue P parking sign
    if (item.signText == "ENTRY") signCol = glm::vec3(0.15f, 0.8f, 0.3f);
    else if (item.signText == "EXIT") signCol = glm::vec3(0.85f, 0.15f, 0.15f);
    else if (item.signText == "EV") signCol = glm::vec3(0.1f, 0.7f, 0.95f);
    else if (item.signText == "ACCESSIBLE") signCol = glm::vec3(0.65f, 0.25f, 0.85f);

    if (depthOnly) {
        m_shadowShader.setMat4("model", board);
        m_cubeMesh->draw();
    } else {
        m_lightingShader.setMat4("model", board);
        m_lightingShader.setVec3("materialColor", signCol);
        m_lightingShader.setVec3("emissiveColor", signCol * 0.35f);
        m_cubeMesh->draw();
        m_stats.drawCalls++;
    }
}

void Renderer::renderSelectionBeacon(const glm::vec3& slotPos, const glm::mat4& view, const glm::mat4& proj, float animTime) {
    float bob = sin(animTime * 3.5f) * 0.4f;
    float spin = animTime * 75.0f;

    // Inverted diamond/pyramid above slot
    glm::mat4 beaconModel = glm::mat4(1.0f);
    beaconModel = glm::translate(beaconModel, glm::vec3(slotPos.x, 3.8f + bob, slotPos.z));
    beaconModel = glm::rotate(beaconModel, glm::radians(spin), glm::vec3(0.0f, 1.0f, 0.0f));
    beaconModel = glm::rotate(beaconModel, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 1.0f));
    beaconModel = glm::scale(beaconModel, glm::vec3(0.9f));

    m_lightingShader.setMat4("model", beaconModel);
    m_lightingShader.setVec3("materialColor", glm::vec3(0.0f, 0.9f, 1.0f));
    m_lightingShader.setVec3("emissiveColor", glm::vec3(0.2f, 0.8f, 1.2f));
    m_cubeMesh->draw();
    m_stats.drawCalls++;
}

void Renderer::renderRouteLine(const Route& route, const glm::mat4& view, const glm::mat4& proj, bool is3D, float animTime) {
    if (!route.isValid || route.waypoints.size() < 2) return;

    if (m_cachedRoutePoints != route.waypoints) {
        m_cachedRoutePoints = route.waypoints;
        m_routeMesh = Mesh::createPathRibbon(route.waypoints, is3D ? 0.75f : 0.9f, glm::vec4(0.0f, 0.85f, 1.0f, 0.92f));
    }

    if (m_routeMesh) {
        m_basicShader.use();
        m_basicShader.setMat4("view", view);
        m_basicShader.setMat4("projection", proj);
        m_basicShader.setMat4("model", glm::mat4(1.0f));
        m_basicShader.setBool("useOverrideColor", true);

        float pulse = 0.8f + 0.2f * sin(animTime * 4.0f);
        m_basicShader.setVec4("overrideColor", glm::vec4(0.0f, 0.9f * pulse, 1.0f * pulse, 0.95f));
        m_routeMesh->draw();
        m_stats.drawCalls++;

        for (const auto& wp : route.waypoints) {
            glm::mat4 wpModel = glm::mat4(1.0f);
            wpModel = glm::translate(wpModel, glm::vec3(wp.x, is3D ? 0.08f : 0.06f, wp.z));
            wpModel = glm::scale(wpModel, glm::vec3(0.5f));
            m_basicShader.setMat4("model", wpModel);
            m_basicShader.setVec4("overrideColor", glm::vec4(1.0f, 0.9f, 0.1f, 1.0f));
            m_circleMesh->draw();
            m_stats.drawCalls++;
        }
    }
}

void Renderer::renderNavigationGraph(const Graph& graph, const glm::mat4& view, const glm::mat4& proj, bool is3D, const Route* activeRoute) {
    m_basicShader.use();
    m_basicShader.setMat4("view", view);
    m_basicShader.setMat4("projection", proj);
    m_basicShader.setBool("useOverrideColor", true);

    std::unordered_set<uint64_t> activeRouteEdges;
    if (activeRoute && activeRoute->isValid && activeRoute->nodeIndices.size() >= 2) {
        for (size_t i = 0; i < activeRoute->nodeIndices.size() - 1; ++i) {
            uint64_t u = static_cast<uint64_t>(activeRoute->nodeIndices[i]);
            uint64_t v = static_cast<uint64_t>(activeRoute->nodeIndices[i + 1]);
            activeRouteEdges.insert((u << 32) | (v & 0xFFFFFFFFULL));
            activeRouteEdges.insert((v << 32) | (u & 0xFFFFFFFFULL));
        }
    }

    // 1. Draw Graph Edges
    for (const auto& edge : graph.getAllEdges()) {
        const Node* nA = graph.getNode(edge.fromIndex);
        const Node* nB = graph.getNode(edge.toIndex);
        if (!nA || !nB) continue;

        glm::vec3 diff = nB->position - nA->position;
        float len = glm::length(diff);
        if (len < 0.01f) continue;

        glm::vec3 center = (nA->position + nB->position) * 0.5f;
        float angle = glm::degrees(std::atan2(diff.x, diff.z));

        uint64_t edgeKey = (static_cast<uint64_t>(edge.fromIndex) << 32) | (static_cast<uint64_t>(edge.toIndex) & 0xFFFFFFFFULL);
        bool isPathEdge = (activeRouteEdges.find(edgeKey) != activeRouteEdges.end());

        glm::mat4 eModel = glm::mat4(1.0f);
        eModel = glm::translate(eModel, glm::vec3(center.x, is3D ? 0.22f : 0.045f, center.z));
        eModel = glm::rotate(eModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        float edgeThickness = isPathEdge ? (is3D ? 0.45f : 0.35f) : (is3D ? 0.12f : 0.08f);
        eModel = glm::scale(eModel, glm::vec3(edgeThickness, 1.0f, len));

        glm::vec4 edgeCol = isPathEdge ? glm::vec4(1.0f, 0.85f, 0.1f, 1.0f) : glm::vec4(0.35f, 0.45f, 0.55f, 0.65f);
        m_basicShader.setMat4("model", eModel);
        m_basicShader.setVec4("overrideColor", edgeCol);
        m_quadMesh->draw();
        m_stats.drawCalls++;
    }

    // 2. Draw Graph Nodes
    for (const auto& node : graph.getNodes()) {
        bool isPathNode = false;
        if (activeRoute && activeRoute->isValid) {
            for (int idx : activeRoute->nodeIndices) {
                if (idx == node.index) {
                    isPathNode = true;
                    break;
                }
            }
        }

        glm::mat4 nModel = glm::mat4(1.0f);
        nModel = glm::translate(nModel, glm::vec3(node.position.x, is3D ? 0.35f : 0.07f, node.position.z));
        float scale = isPathNode ? (is3D ? 0.55f : 0.45f) : (is3D ? 0.32f : 0.25f);
        nModel = glm::scale(nModel, glm::vec3(scale));

        glm::vec4 nodeColor(1.0f);
        if (isPathNode) {
            nodeColor = glm::vec4(1.0f, 0.9f, 0.1f, 1.0f);
        } else {
            switch (node.type) {
                case NodeType::ENTRANCE: nodeColor = glm::vec4(0.1f, 1.0f, 0.2f, 1.0f); break;
                case NodeType::EXIT: nodeColor = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f); break;
                case NodeType::INTERSECTION: nodeColor = glm::vec4(1.0f, 0.8f, 0.1f, 1.0f); break;
                case NodeType::SLOT_ACCESS: nodeColor = glm::vec4(0.2f, 0.7f, 1.0f, 1.0f); break;
                case NodeType::SLOT_BAY: nodeColor = glm::vec4(0.8f, 0.3f, 0.9f, 1.0f); break;
                default: nodeColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f); break;
            }
        }

        m_basicShader.setMat4("model", nModel);
        m_basicShader.setVec4("overrideColor", nodeColor);
        if (is3D) {
            m_sphereMesh->draw();
        } else {
            m_circleMesh->draw();
        }
        m_stats.drawCalls++;
    }
}

void Renderer::renderGate(const glm::vec3& position, bool isEntrance, const glm::mat4& view, const glm::mat4& proj, bool is3D, bool depthOnly) {
    if (is3D) {
        glm::vec3 signColor = isEntrance ? glm::vec3(0.1f, 0.9f, 0.3f) : glm::vec3(0.95f, 0.15f, 0.15f);
        glm::vec3 pillarColor = glm::vec3(0.35f, 0.38f, 0.42f);

        // Left Pillar
        glm::mat4 pLeft = glm::mat4(1.0f);
        pLeft = glm::translate(pLeft, position + glm::vec3(0.0f, 2.5f, -3.8f));
        pLeft = glm::scale(pLeft, glm::vec3(0.8f, 5.0f, 0.8f));

        // Right Pillar
        glm::mat4 pRight = glm::mat4(1.0f);
        pRight = glm::translate(pRight, position + glm::vec3(0.0f, 2.5f, 3.8f));
        pRight = glm::scale(pRight, glm::vec3(0.8f, 5.0f, 0.8f));

        // Overhead Crossbeam
        glm::mat4 crossbeam = glm::mat4(1.0f);
        crossbeam = glm::translate(crossbeam, position + glm::vec3(0.0f, 4.9f, 0.0f));
        crossbeam = glm::scale(crossbeam, glm::vec3(1.1f, 0.6f, 8.4f));

        // Signboard
        glm::mat4 sign = glm::mat4(1.0f);
        sign = glm::translate(sign, position + glm::vec3(0.0f, 4.9f, 0.0f));
        sign = glm::scale(sign, glm::vec3(1.15f, 0.45f, 5.6f));

        if (depthOnly) {
            m_shadowShader.setMat4("model", pLeft); m_cubeMesh->draw();
            m_shadowShader.setMat4("model", pRight); m_cubeMesh->draw();
            m_shadowShader.setMat4("model", crossbeam); m_cubeMesh->draw();
            m_shadowShader.setMat4("model", sign); m_cubeMesh->draw();
        } else {
            m_lightingShader.setMat4("model", pLeft);
            m_lightingShader.setVec3("materialColor", pillarColor);
            m_lightingShader.setVec3("emissiveColor", glm::vec3(0.0f));
            m_cubeMesh->draw();

            m_lightingShader.setMat4("model", pRight);
            m_lightingShader.setVec3("materialColor", pillarColor);
            m_cubeMesh->draw();

            m_lightingShader.setMat4("model", crossbeam);
            m_lightingShader.setVec3("materialColor", glm::vec3(0.25f, 0.28f, 0.32f));
            m_cubeMesh->draw();

            m_lightingShader.setMat4("model", sign);
            m_lightingShader.setVec3("materialColor", signColor);
            m_lightingShader.setVec3("emissiveColor", signColor * 1.4f);
            m_cubeMesh->draw();

            // Barrier arm
            glm::mat4 arm = glm::mat4(1.0f);
            arm = glm::translate(arm, position + glm::vec3(0.0f, 1.0f, 0.0f));
            arm = glm::scale(arm, glm::vec3(0.18f, 0.22f, 6.5f));
            m_lightingShader.setMat4("model", arm);
            m_lightingShader.setVec3("materialColor", glm::vec3(1.0f, 0.95f, 0.2f));
            m_lightingShader.setVec3("emissiveColor", glm::vec3(0.3f, 0.28f, 0.05f));
            m_cubeMesh->draw();

            m_stats.drawCalls += 5;
        }
    } else {
        // 2D Gate road threshold and boundary line
        glm::vec4 gateColor = isEntrance ? glm::vec4(0.15f, 0.85f, 0.35f, 0.95f) : glm::vec4(0.95f, 0.2f, 0.2f, 0.95f);

        glm::mat4 threshold = glm::mat4(1.0f);
        threshold = glm::translate(threshold, glm::vec3(position.x, 0.025f, position.z));
        threshold = glm::scale(threshold, glm::vec3(2.5f, 1.0f, 7.5f));
        m_basicShader.setMat4("model", threshold);
        m_basicShader.setVec4("overrideColor", gateColor);
        m_quadMesh->draw();

        glm::mat4 postL = glm::mat4(1.0f);
        postL = glm::translate(postL, glm::vec3(position.x, 0.035f, position.z - 4.0f));
        postL = glm::scale(postL, glm::vec3(1.0f, 1.0f, 1.0f));
        m_basicShader.setMat4("model", postL);
        m_basicShader.setVec4("overrideColor", glm::vec4(0.9f, 0.9f, 0.9f, 1.0f));
        m_quadMesh->draw();

        glm::mat4 postR = glm::mat4(1.0f);
        postR = glm::translate(postR, glm::vec3(position.x, 0.035f, position.z + 4.0f));
        postR = glm::scale(postR, glm::vec3(1.0f, 1.0f, 1.0f));
        m_basicShader.setMat4("model", postR);
        m_basicShader.setVec4("overrideColor", glm::vec4(0.9f, 0.9f, 0.9f, 1.0f));
        m_quadMesh->draw();

        m_stats.drawCalls += 3;
    }
}

} // namespace SmartParking
