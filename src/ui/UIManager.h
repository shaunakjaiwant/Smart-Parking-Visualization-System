#pragma once
#include <imgui.h>
#include "../parking/ParkingLot.h"
#include "../simulation/Simulation.h"
#include "../navigation/PathFinder.h"
#include "../graphics/Camera.h"
#include "../graphics/Renderer.h"
#include <string>

/**
 * @file UIManager.h
 * @brief High-density professional Dark UI dashboard and interactive controls using Dear ImGui.
 * 
 * WHAT: Implements complete UI pages (Dashboard, Map, 3D, Nav, Sim, Analytics, Settings, Concepts, Help).
 * WHY:  Fulfills all UI/UX specifications for a polished final-year smart parking product.
 * HOW:  Applies custom dark charcoal theme with colored status cards, search filters, route step cards,
 *       simulation sliders, analytics charts, viva graphics concept explanations, and F3 debug HUD.
 */
namespace SmartParking {

enum class UIPage {
    DASHBOARD,
    PARKING_MAP_2D,
    VIEW_3D,
    NAVIGATION,
    SIMULATION,
    ANALYTICS,
    SETTINGS,
    GRAPHICS_CONCEPTS,
    HELP_ABOUT
};

class UIManager {
public:
    UIManager();
    ~UIManager() = default;

    void init(void* glfwWindow);
    void shutdown();

    void newFrame();
    void render(ParkingLot& lot,
                Simulation& sim,
                PathFinder& pathFinder,
                Camera& camera,
                Renderer& renderer,
                Route& activeRoute,
                float fps);

    // Page navigation
    void setPage(UIPage page) { m_currentPage = page; }
    UIPage getPage() const { return m_currentPage; }

    // Debug overlay toggle
    void toggleDebugOverlay() { m_showDebugOverlay = !m_showDebugOverlay; }
    bool isDebugOverlayVisible() const { return m_showDebugOverlay; }

    // Settings
    bool showNavGraph = false;
    bool wireframeMode = false;
    bool vsyncEnabled = true;
    bool showFPS = true;
    bool showSlotLabels = true;
    bool showShadowMapDebug = false;

private:
    UIPage m_currentPage = UIPage::DASHBOARD;
    bool m_showDebugOverlay = false;

    // Search and filter buffers
    char m_searchQuery[64] = "";
    int m_typeFilterIndex = 0;   // 0: All, 1: Regular, 2: EV, 3: Accessible, 4: Reserved
    int m_statusFilterIndex = 0; // 0: All, 1: Available, 2: Occupied, 3: Reserved

    // Intelligent Allocation filter & feedback
    int m_allocationType = 0; // 0: Any, 1: Regular, 2: EV, 3: Accessible, 4: Reserved
    std::string m_allocationStatusMessage = "";
    bool m_allocationStatusSuccess = true;

    // Routing algorithm selection (0: Dijkstra, 1: A*)
    int m_routingAlgorithm = 0;

    void applyDarkTheme();

    // UI Panels
    void renderTopMenuBar(Camera& camera, Simulation& sim, Renderer& renderer);
    void renderStatsHeader(const FacilityStats& stats, const Simulation& sim, float fps);
    void renderDashboardPage(ParkingLot& lot, Simulation& sim, PathFinder& pathFinder, Camera& camera, Route& activeRoute);
    void renderNavigationPage(ParkingLot& lot, PathFinder& pathFinder, Simulation& sim, Route& activeRoute);
    void renderSimulationPage(ParkingLot& lot, Simulation& sim, PathFinder& pathFinder, Route& activeRoute);
    void renderAnalyticsPage(const ParkingLot& lot, const Simulation& sim);
    void renderGraphicsConceptsPage(const Camera& camera);
    void renderSettingsPage(Camera& camera, Renderer& renderer);
    void renderHelpAboutPage();
    void renderDebugOverlay(const Camera& camera, Renderer& renderer, const ParkingLot& lot, const Simulation& sim, const Route& activeRoute, float fps);
    void renderShadowMapViewer(Renderer& renderer);
    void renderSlotDetailsCard(ParkingLot& lot, PathFinder& pathFinder, Simulation& sim, Route& activeRoute);
    void renderWorldOverlays(const ParkingLot& lot, const Camera& camera);
};

} // namespace SmartParking
