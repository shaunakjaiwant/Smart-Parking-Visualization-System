#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../graphics/Renderer.h"
#include "../graphics/Camera.h"
#include "../parking/ParkingLot.h"
#include "../navigation/PathFinder.h"
#include "../simulation/Simulation.h"
#include "../ui/UIManager.h"
#include <memory>
#include <string>

/**
 * @file Application.h
 * @brief Main Application lifecycle coordinator and input dispatcher.
 * 
 * WHAT: Initializes GLFW window, GLAD loader, OpenGL state, and owns the primary subsystems.
 * WHY:  Central runtime container managing the game loop, events, timing, and subsystem orchestration.
 * HOW:  Processes GLFW input events, updates simulation, triggers rendering, and presents frames.
 */
namespace SmartParking {

class Application {
public:
    Application();
    ~Application();

    bool init(int width = 1600, int height = 900, const std::string& title = "SMART PARKING - Interactive 2D/3D Visualization & Navigation System");
    void run();
    void shutdown();

    // GLFW callbacks
    void onFramebufferSize(int width, int height);
    void onKey(int key, int scancode, int action, int mods);
    void onMouseButton(int button, int action, int mods);
    void onCursorPos(double xpos, double ypos);
    void onScroll(double xoffset, double yoffset);

private:
    GLFWwindow* m_window = nullptr;
    int m_windowWidth = 1600;
    int m_windowHeight = 900;
    bool m_isRunning = false;

    // Subsystems
    Camera m_camera;
    Renderer m_renderer;
    ParkingLot m_parkingLot;
    PathFinder m_pathFinder;
    std::unique_ptr<Simulation> m_simulation;
    UIManager m_uiManager;

    Route m_activeRoute;

    // Timing and FPS
    float m_lastFrameTime = 0.0f;
    float m_fps = 60.0f;
    float m_fpsTimer = 0.0f;
    int m_frameCount = 0;

    // Mouse interaction state
    bool m_rightMousePressed = false;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;

    void handleMousePicking(double screenX, double screenY);
    glm::vec3 screenToWorldGroundPlane(double screenX, double screenY);
};

} // namespace SmartParking
