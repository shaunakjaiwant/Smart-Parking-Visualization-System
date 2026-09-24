#include "Application.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace SmartParking {

// Static C-style callback trampolines
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onFramebufferSize(width, height);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onKey(key, scancode, action, mods);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onMouseButton(button, action, mods);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onCursorPos(xpos, ypos);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onScroll(xoffset, yoffset);
}

Application::Application()
    : m_pathFinder(&m_parkingLot.getNavigationGraph()) {
    m_simulation = std::make_unique<Simulation>(&m_parkingLot, &m_pathFinder);
    m_simulation->setCamera(&m_camera);
}

Application::~Application() {
    shutdown();
}

bool Application::init(int width, int height, const std::string& title) {
    m_windowWidth = width;
    m_windowHeight = height;

    if (!glfwInit()) {
        std::cerr << "[Application Error] Failed to initialize GLFW." << std::endl;
        return false;
    }

    // Configure OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "[Application Error] Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);

    // Setup input callbacks
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
    glfwSetKeyCallback(m_window, key_callback);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetCursorPosCallback(m_window, cursor_position_callback);
    glfwSetScrollCallback(m_window, scroll_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[Application Error] Failed to initialize GLAD OpenGL loader." << std::endl;
        return false;
    }

    std::cout << "[Application] OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "[Application] GPU Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Load parking layout
    m_parkingLot.loadLayout("config/parking_layout.json");
    m_pathFinder.setGraph(&m_parkingLot.getNavigationGraph());

    // Initialize Renderer
    if (!m_renderer.init("shaders")) {
        std::cerr << "[Application Warning] Shaders failed to compile from disk; checking fallback shaders." << std::endl;
    }
    m_renderer.setViewport(m_windowWidth, m_windowHeight);

    // Initialize UI Manager
    m_uiManager.init(m_window);

    m_isRunning = true;
    m_lastFrameTime = static_cast<float>(glfwGetTime());

    return true;
}

void Application::run() {
    while (m_isRunning && !glfwWindowShouldClose(m_window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - m_lastFrameTime;
        m_lastFrameTime = currentFrame;

        // Cap excessive delta time spikes
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // Calculate FPS
        m_frameCount++;
        m_fpsTimer += deltaTime;
        if (m_fpsTimer >= 0.5f) {
            m_fps = static_cast<float>(m_frameCount) / m_fpsTimer;
            m_frameCount = 0;
            m_fpsTimer = 0.0f;
        }

        // Handle continuous keyboard movement (WASD)
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard) {
            if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_camera.processKeyboard('W', deltaTime);
            if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) m_camera.processKeyboard('S', deltaTime);
            if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) m_camera.processKeyboard('A', deltaTime);
            if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) m_camera.processKeyboard('D', deltaTime);
            if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) m_camera.processKeyboard('Q', deltaTime);
            if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS) m_camera.processKeyboard('E', deltaTime);
        }

        // Poll events
        glfwPollEvents();

        // Update simulation
        m_simulation->update(deltaTime);

        // Graphics Render Pass
        m_renderer.beginFrame();
        m_renderer.renderScene(m_parkingLot,
                               m_simulation->getVehicles(),
                               m_camera,
                               m_activeRoute,
                               currentFrame,
                               m_uiManager.showNavGraph,
                               m_uiManager.wireframeMode);

        // UI Render Pass
        m_uiManager.newFrame();
        m_uiManager.render(m_parkingLot,
                           *m_simulation,
                           m_pathFinder,
                           m_camera,
                           m_renderer,
                           m_activeRoute,
                           m_fps);

        m_renderer.endFrame();

        // Swap buffers
        glfwSwapInterval(m_uiManager.vsyncEnabled ? 1 : 0);
        glfwSwapBuffers(m_window);
    }
}

void Application::shutdown() {
    if (m_window) {
        m_uiManager.shutdown();
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
    }
    m_isRunning = false;
}

void Application::onFramebufferSize(int width, int height) {
    if (width > 0 && height > 0) {
        m_windowWidth = width;
        m_windowHeight = height;
        m_renderer.setViewport(width, height);
    }
}

void Application::onKey(int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_1) {
            m_camera.setPresetView(1);
            m_uiManager.setPage(UIPage::PARKING_MAP_2D);
        } else if (key == GLFW_KEY_2) {
            m_camera.setPresetView(2);
            m_uiManager.setPage(UIPage::VIEW_3D);
        } else if (key == GLFW_KEY_3) {
            m_camera.setPresetView(3);
        } else if (key == GLFW_KEY_4) {
            m_camera.setPresetView(4);
        } else if (key == GLFW_KEY_5) {
            m_camera.setPresetView(5);
        } else if (key == GLFW_KEY_6) {
            m_camera.setPresetView(6);
        } else if (key == GLFW_KEY_P) {
            m_simulation->pause();
        } else if (key == GLFW_KEY_R) {
            m_camera.reset();
        } else if (key == GLFW_KEY_SPACE) {
            m_simulation->togglePlayPause();
        } else if (key == GLFW_KEY_N) {
            // Find nearest available slot
            ParkingSlot* best = m_parkingLot.findNearestAvailableSlot(m_parkingLot.getEntrancePosition(), SlotType::REGULAR);
            if (best) {
                m_parkingLot.selectSlot(best->getId());
                m_activeRoute = m_pathFinder.calculateRouteToSlot(m_parkingLot.getEntranceNodeId(), best->getId());
            }
        } else if (key == GLFW_KEY_F3) {
            m_uiManager.toggleDebugOverlay();
        } else if (key == GLFW_KEY_L || (key == GLFW_KEY_D && (mods & GLFW_MOD_CONTROL || m_camera.getMode() == CameraMode::MODE_2D_TOPDOWN))) {
            // Day / Night mode toggle shortcut (L, Ctrl+D, or D in 2D mode)
            m_renderer.toggleDayNight();
        } else if (key == GLFW_KEY_ESCAPE) {
            m_uiManager.setPage(UIPage::DASHBOARD);
        }
    }
}

void Application::onMouseButton(int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            m_rightMousePressed = true;
            glfwGetCursorPos(m_window, &m_lastMouseX, &m_lastMouseY);
        } else if (action == GLFW_RELEASE) {
            m_rightMousePressed = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);
        handleMousePicking(xpos, ypos);
    }
}

void Application::onCursorPos(double xpos, double ypos) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (m_rightMousePressed) {
        float xoffset = static_cast<float>(xpos - m_lastMouseX);
        float yoffset = static_cast<float>(m_lastMouseY - ypos); // reversed since y-coords go from bottom to top
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;

        if (m_camera.getMode() == CameraMode::MODE_3D_PERSPECTIVE) {
            m_camera.processMouseMovement(xoffset, yoffset);
        } else {
            // In 2D, right click drag pans the map
            m_camera.pan2D(-xoffset * 0.1f, -yoffset * 0.1f);
        }
    }
}

void Application::onScroll(double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    m_camera.processMouseScroll(static_cast<float>(yoffset));
}

void Application::handleMousePicking(double screenX, double screenY) {
    glm::vec3 worldGroundPos = screenToWorldGroundPlane(screenX, screenY);
    ParkingSlot* picked = m_parkingLot.pickSlot2D(worldGroundPos.x, worldGroundPos.z);

    if (picked) {
        m_parkingLot.selectSlot(picked->getId());
        m_activeRoute = m_pathFinder.calculateRouteToSlot(m_parkingLot.getEntranceNodeId(), picked->getId());
        std::cout << "[Picking] Selected Slot: " << picked->getId() 
                  << " (Distance: " << picked->getDistanceFromEntrance() << "m)" << std::endl;
    } else {
        m_parkingLot.clearSelection();
        m_activeRoute.clear();
    }
}

glm::vec3 Application::screenToWorldGroundPlane(double screenX, double screenY) {
    float aspect = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 proj = m_camera.getProjectionMatrix(aspect);
    glm::mat4 invVP = glm::inverse(proj * view);

    // Normalized Device Coordinates (NDC)
    float ndcX = (2.0f * static_cast<float>(screenX)) / m_windowWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * static_cast<float>(screenY)) / m_windowHeight;

    if (m_camera.getMode() == CameraMode::MODE_2D_TOPDOWN) {
        // Orthographic ray
        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        return glm::vec3(nearPoint.x, 0.0f, nearPoint.z);
    } else {
        // Perspective ray from near plane to far plane
        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        nearPoint /= nearPoint.w;

        glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        farPoint /= farPoint.w;

        glm::vec3 rayOrigin = glm::vec3(nearPoint);
        glm::vec3 rayDir = glm::normalize(glm::vec3(farPoint - nearPoint));

        // Intersect with ground plane Y = 0: rayOrigin.y + t * rayDir.y = 0 => t = -rayOrigin.y / rayDir.y
        if (std::abs(rayDir.y) > 0.0001f) {
            float t = -rayOrigin.y / rayDir.y;
            if (t >= 0.0f) {
                return rayOrigin + rayDir * t;
            }
        }
        return glm::vec3(0.0f);
    }
}

} // namespace SmartParking
