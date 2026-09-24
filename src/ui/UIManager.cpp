#include "UIManager.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

namespace SmartParking {

UIManager::UIManager() {
}

void UIManager::init(void* glfwWindow) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    applyDarkTheme();

    ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(glfwWindow), true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void UIManager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::applyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(8.0f, 7.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.53f, 0.58f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.14f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.15f, 0.18f, 0.85f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.17f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.24f, 0.26f, 0.30f, 0.65f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.27f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.32f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.11f, 0.13f, 0.80f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.11f, 0.13f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.27f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.32f, 0.36f, 0.42f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.45f, 0.52f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.85f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.75f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.48f, 0.75f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.21f, 0.26f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.28f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.42f, 0.68f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.26f, 0.30f, 0.80f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.35f, 0.42f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.00f, 0.75f, 0.95f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.28f, 0.34f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.38f, 0.60f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 0.80f, 0.90f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.20f, 0.75f, 0.40f, 1.00f);
}

void UIManager::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::render(ParkingLot& lot,
                       Simulation& sim,
                       PathFinder& pathFinder,
                       Camera& camera,
                       Renderer& renderer,
                       Route& activeRoute,
                       float fps) {
    FacilityStats stats = lot.calculateStats();

    renderTopMenuBar(camera, sim, renderer);
    renderStatsHeader(stats, sim, fps);

    switch (m_currentPage) {
        case UIPage::DASHBOARD:
            renderDashboardPage(lot, sim, pathFinder, camera, activeRoute);
            break;
        case UIPage::PARKING_MAP_2D:
            if (camera.getMode() != CameraMode::MODE_2D_TOPDOWN) camera.setMode(CameraMode::MODE_2D_TOPDOWN);
            renderDashboardPage(lot, sim, pathFinder, camera, activeRoute);
            break;
        case UIPage::VIEW_3D:
            if (camera.getMode() != CameraMode::MODE_3D_PERSPECTIVE) camera.setMode(CameraMode::MODE_3D_PERSPECTIVE);
            renderDashboardPage(lot, sim, pathFinder, camera, activeRoute);
            break;
        case UIPage::NAVIGATION:
            renderNavigationPage(lot, pathFinder, sim, activeRoute);
            break;
        case UIPage::SIMULATION:
            renderSimulationPage(lot, sim, pathFinder, activeRoute);
            break;
        case UIPage::ANALYTICS:
            renderAnalyticsPage(lot, sim);
            break;
        case UIPage::SETTINGS:
            renderSettingsPage(camera, renderer);
            break;
        case UIPage::GRAPHICS_CONCEPTS:
            renderGraphicsConceptsPage(camera);
            break;
        case UIPage::HELP_ABOUT:
            renderHelpAboutPage();
            break;
    }

    if (m_showDebugOverlay) {
        renderDebugOverlay(camera, renderer, lot, sim, activeRoute, fps);
    }

    if (showShadowMapDebug) {
        renderShadowMapViewer(renderer);
    }

    if (showSlotLabels) {
        renderWorldOverlays(lot, camera);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::renderTopMenuBar(Camera& camera, Simulation& sim, Renderer& renderer) {
    if (ImGui::BeginMainMenuBar()) {
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), " SMART PARKING ");
        ImGui::Separator();

        if (ImGui::MenuItem("Dashboard", nullptr, m_currentPage == UIPage::DASHBOARD)) m_currentPage = UIPage::DASHBOARD;
        if (ImGui::MenuItem("2D Map", "1", m_currentPage == UIPage::PARKING_MAP_2D)) {
            m_currentPage = UIPage::PARKING_MAP_2D;
            camera.setPresetView(1);
        }
        if (ImGui::MenuItem("3D View", "2", m_currentPage == UIPage::VIEW_3D)) {
            m_currentPage = UIPage::VIEW_3D;
            camera.setPresetView(2);
        }

        // Camera viewpoint presets menu
        if (ImGui::BeginMenu("Views")) {
            if (ImGui::MenuItem("1: 2D Orthographic Map", "1")) { camera.setPresetView(1); m_currentPage = UIPage::PARKING_MAP_2D; }
            if (ImGui::MenuItem("2: 3D Perspective Digital-Twin", "2")) { camera.setPresetView(2); m_currentPage = UIPage::VIEW_3D; }
            if (ImGui::MenuItem("3: Facility Overview", "3")) { camera.setPresetView(3); m_currentPage = UIPage::VIEW_3D; }
            if (ImGui::MenuItem("4: Ground Floor / South (Rows A-B)", "4")) { camera.setPresetView(4); m_currentPage = UIPage::VIEW_3D; }
            if (ImGui::MenuItem("5: North Bays (Rows C-D)", "5")) { camera.setPresetView(5); m_currentPage = UIPage::VIEW_3D; }
            if (ImGui::MenuItem("6: Entrance Gate Corridor", "6")) { camera.setPresetView(6); m_currentPage = UIPage::VIEW_3D; }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Navigation", nullptr, m_currentPage == UIPage::NAVIGATION)) m_currentPage = UIPage::NAVIGATION;
        if (ImGui::MenuItem("Simulation", nullptr, m_currentPage == UIPage::SIMULATION)) m_currentPage = UIPage::SIMULATION;
        if (ImGui::MenuItem("Analytics", nullptr, m_currentPage == UIPage::ANALYTICS)) m_currentPage = UIPage::ANALYTICS;
        if (ImGui::MenuItem("Graphics Concepts", nullptr, m_currentPage == UIPage::GRAPHICS_CONCEPTS)) m_currentPage = UIPage::GRAPHICS_CONCEPTS;
        if (ImGui::MenuItem("Settings", nullptr, m_currentPage == UIPage::SETTINGS)) m_currentPage = UIPage::SETTINGS;
        if (ImGui::MenuItem("About", nullptr, m_currentPage == UIPage::HELP_ABOUT)) m_currentPage = UIPage::HELP_ABOUT;

        // Right side quick controls
        float rightIndent = ImGui::GetWindowWidth() - 530.0f;
        if (rightIndent > ImGui::GetCursorPosX()) {
            ImGui::SameLine(rightIndent);
        }

        // Autonomous Presentation Demo button
        if (sim.isDemoActive()) {
            if (ImGui::Button("■ Stop Demo")) sim.stopDemoMode();
        } else {
            if (ImGui::Button("▶ DEMO MODE")) sim.startDemoMode();
        }

        ImGui::SameLine();
        // Day/Night mode quick toggle
        if (renderer.getTimeOfDay() == TimeOfDay::DAY) {
            if (ImGui::Button("☀️ Day")) renderer.toggleDayNight();
        } else {
            if (ImGui::Button("🌙 Night")) renderer.toggleDayNight();
        }

        ImGui::SameLine();
        if (sim.isPlaying()) {
            if (ImGui::Button("Pause (P)")) sim.pause();
        } else {
            if (ImGui::Button("Play (Space)")) sim.play();
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset")) sim.reset();

        ImGui::SameLine();
        if (ImGui::Button(m_showDebugOverlay ? "Debug (F3) [ON]" : "Debug (F3)")) {
            toggleDebugOverlay();
        }

        ImGui::EndMainMenuBar();
    }
}

void UIManager::renderStatsHeader(const FacilityStats& stats, const Simulation& sim, float fps) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + 24.0f));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 64.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("##StatsHeader", nullptr, flags)) {
        ImGui::Columns(8, "stat_columns", false);

        // Card 1: Total Slots
        ImGui::TextDisabled("TOTAL SLOTS");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%d", stats.totalSlots);
        ImGui::NextColumn();

        // Card 2: Available Slots
        ImGui::TextDisabled("AVAILABLE");
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "%d", stats.availableSlots);
        ImGui::NextColumn();

        // Card 3: Occupied Slots
        ImGui::TextDisabled("OCCUPIED");
        ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.25f, 1.0f), "%d", stats.occupiedSlots);
        ImGui::NextColumn();

        // Card 4: Reserved Slots
        ImGui::TextDisabled("RESERVED");
        ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.2f, 1.0f), "%d", stats.reservedSlots);
        ImGui::NextColumn();

        // Card 5: EV Charging Slots
        ImGui::TextDisabled("EV CHARGING");
        ImGui::TextColored(ImVec4(0.1f, 0.75f, 1.0f, 1.0f), "%d", stats.evSlots);
        ImGui::NextColumn();

        // Card 6: Accessible Slots
        ImGui::TextDisabled("ACCESSIBLE");
        ImGui::TextColored(ImVec4(0.75f, 0.4f, 0.95f, 1.0f), "%d", stats.accessibleSlots);
        ImGui::NextColumn();

        // Card 7: Active Vehicles
        ImGui::TextDisabled("ACTIVE VEHICLES");
        ImGui::TextColored(ImVec4(0.0f, 0.85f, 1.0f, 1.0f), "%zu", sim.getActiveVehicleCount());
        ImGui::NextColumn();

        // Card 8: Occupancy Rate
        ImGui::TextDisabled("OCCUPANCY [SIM DATA]");
        char overlay[32];
        snprintf(overlay, sizeof(overlay), "%.1f%%", stats.occupancyRate);
        ImGui::ProgressBar(stats.occupancyRate / 100.0f, ImVec2(-1.0f, 16.0f), overlay);

        ImGui::Columns(1);
    }
    ImGui::End();
}

void UIManager::renderDashboardPage(ParkingLot& lot, Simulation& sim, PathFinder& pathFinder, Camera& camera, Route& activeRoute) {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 92.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 580.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Smart Parking Operations", nullptr)) {
        if (ImGui::CollapsingHeader("Intelligent Slot Allocation", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Request Parking Slot:");
            const char* allocTypes[] = { "Nearest Any Available", "Regular Vehicle", "Electric Vehicle (EV)", "Accessible (Disabled)", "Reserved" };
            ImGui::Combo("Filter", &m_allocationType, allocTypes, IM_ARRAYSIZE(allocTypes));

            if (ImGui::Button("Find & Navigate to Nearest Slot (N)", ImVec2(-1.0f, 32.0f))) {
                SlotType reqType = SlotType::REGULAR;
                if (m_allocationType == 2) reqType = SlotType::EV_CHARGING;
                else if (m_allocationType == 3) reqType = SlotType::ACCESSIBLE;
                else if (m_allocationType == 4) reqType = SlotType::RESERVED;

                ParkingSlot* best = lot.findNearestAvailableSlot(lot.getEntrancePosition(), reqType);
                if (best) {
                    lot.selectSlot(best->getId());
                    activeRoute = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), best->getId(), m_routingAlgorithm == 1);
                    m_allocationStatusMessage = "Allocated nearest slot: " + best->getId() + " (" + best->getTypeString() + ")";
                    m_allocationStatusSuccess = true;
                } else {
                    lot.clearSelection();
                    activeRoute.clear();
                    m_allocationStatusMessage = "No suitable parking slot available.";
                    m_allocationStatusSuccess = false;
                }
            }

            if (!m_allocationStatusMessage.empty()) {
                ImVec4 col = m_allocationStatusSuccess ? ImVec4(0.2f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
                ImGui::TextColored(col, "%s", m_allocationStatusMessage.c_str());
            }
        }

        renderSlotDetailsCard(lot, pathFinder, sim, activeRoute);

        if (ImGui::CollapsingHeader("Quick Simulation Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (sim.isDemoActive()) {
                ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Demonstration Active:");
                ImGui::TextWrapped("%s", sim.getDemoStepDescription().c_str());
                if (ImGui::Button("Stop Demo Mode", ImVec2(-1.0f, 28.0f))) {
                    sim.stopDemoMode();
                }
            } else {
                if (ImGui::Button("Start Automatic Presentation Demo", ImVec2(-1.0f, 32.0f))) {
                    sim.startDemoMode();
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Spawn Single Vehicle", ImVec2(-1.0f, 26.0f))) {
                sim.spawnVehicle(SlotType::REGULAR, 20.0f);
            }
        }
    }
    ImGui::End();
}

void UIManager::renderSlotDetailsCard(ParkingLot& lot, PathFinder& pathFinder, Simulation& sim, Route& activeRoute) {
    if (ImGui::CollapsingHeader("Slot Search & Details", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Search Slot ID", m_searchQuery, IM_ARRAYSIZE(m_searchQuery));

        const char* typeItems[] = { "All Types", "Regular", "EV Charging", "Accessible", "Reserved" };
        ImGui::Combo("Type Filter", &m_typeFilterIndex, typeItems, IM_ARRAYSIZE(typeItems));

        // Live Search Results
        int filterType = m_typeFilterIndex == 0 ? -1 : (m_typeFilterIndex - 1);
        auto matchedSlots = lot.searchSlots(m_searchQuery, filterType, -1);

        if (strlen(m_searchQuery) > 0 || m_typeFilterIndex > 0) {
            ImGui::TextDisabled("Search Results (%zu found):", matchedSlots.size());
            ImGui::BeginChild("SearchResultsList", ImVec2(0.0f, 95.0f), true);
            if (matchedSlots.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No slots match criteria.");
            } else {
                for (const auto* s : matchedSlots) {
                    char label[64];
                    snprintf(label, sizeof(label), "%s [%s] - %s", s->getId().c_str(), s->getTypeString().c_str(), s->getStatusString().c_str());
                    bool isCur = (lot.getSelectedSlot() && lot.getSelectedSlot()->getId() == s->getId());
                    if (ImGui::Selectable(label, isCur)) {
                        lot.selectSlot(s->getId());
                        activeRoute = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), s->getId(), m_routingAlgorithm == 1);
                    }
                }
            }
            ImGui::EndChild();
            ImGui::Separator();
        }

        const ParkingSlot* sel = lot.getSelectedSlot();
        if (sel) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Selected Slot: %s", sel->getId().c_str());
            ImGui::Text("Floor: %s (Level %d)", sel->getFloorString().c_str(), sel->getFloor());
            ImGui::Text("Row: %s", sel->getRow().c_str());
            ImGui::Text("Type: %s", sel->getTypeString().c_str());
            ImGui::Text("Status: %s", sel->getStatusString().c_str());
            ImGui::Text("Reserved Flag: %s", sel->isReserved() ? "YES" : "NO");
            ImGui::Text("Distance to Entrance: %.1f m", sel->getDistanceFromEntrance());
            ImGui::Text("Est. Walking Distance: %.1f m", sel->getEstimatedWalkingDistance());

            if (!sel->getVehicleId().empty()) {
                ImGui::Text("Occupant Vehicle: %s", sel->getVehicleId().c_str());
            }

            if (activeRoute.isValid && lot.getSelectedSlot() && lot.getSelectedSlot()->getId() == sel->getId()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Navigation Route:");
                ImGui::BulletText("Distance: %.1f m | Est. Time: %s", activeRoute.totalDistance, activeRoute.getFormattedTime().c_str());
                ImGui::BulletText("Turns: %d | Waypoints: %zu", activeRoute.turnCount, activeRoute.waypoints.size());
            }

            ImGui::Spacing();
            if (ImGui::Button("Calculate Route", ImVec2(160.0f, 28.0f))) {
                activeRoute = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), sel->getId(), m_routingAlgorithm == 1);
            }

            ImGui::SameLine();
            if (sel->getStatus() == SlotStatus::OCCUPIED) {
                if (ImGui::Button("Release / Free", ImVec2(150.0f, 28.0f))) {
                    sim.releaseParkedVehicle(sel->getId());
                    const_cast<ParkingSlot*>(sel)->free();
                }
            } else {
                if (ImGui::Button("Toggle Occupy", ImVec2(150.0f, 28.0f))) {
                    const_cast<ParkingSlot*>(sel)->occupy("MANUAL-OCCUPANT");
                }
            }
        } else {
            ImGui::TextDisabled("Click any parking bay in 2D/3D to inspect.");
        }
    }
}

void UIManager::renderNavigationPage(ParkingLot& lot, PathFinder& pathFinder, Simulation& sim, Route& activeRoute) {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 92.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(450.0f, 620.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Navigation & Pathfinding Engine", nullptr)) {
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Shortest Path Routing Engine");
        ImGui::Separator();

        ImGui::Text("Algorithm Selection:");
        bool algoChanged = false;
        if (ImGui::RadioButton("Dijkstra (Uniform Cost)", m_routingAlgorithm == 0)) {
            m_routingAlgorithm = 0;
            algoChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("A* (Euclidean Distance Heuristic)", m_routingAlgorithm == 1)) {
            m_routingAlgorithm = 1;
            algoChanged = true;
        }

        if (algoChanged && lot.getSelectedSlot()) {
            activeRoute = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), lot.getSelectedSlot()->getId(), m_routingAlgorithm == 1);
        }

        ImGui::Separator();

        if (activeRoute.isValid) {
            const ParkingSlot* sel = lot.getSelectedSlot();
            ImGui::Text("Destination Slot: %s (%s, Row %s)", 
                        sel ? sel->getId().c_str() : "Target",
                        sel ? sel->getFloorString().c_str() : "Ground Level",
                        sel ? sel->getRow().c_str() : "-");
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "Algorithm Used: %s", 
                               (m_routingAlgorithm == 1) ? "A* Search (Admissible Euclidean Heuristic)" : "Dijkstra's Algorithm (Priority Queue Min-Heap)");
            ImGui::Text("Total Path Distance: %.1f meters", activeRoute.totalDistance);
            ImGui::Text("Estimated Travel Time: %s", activeRoute.getFormattedTime().c_str());
            ImGui::Text("Intersections / Turns: %d turns (%zu waypoints)", activeRoute.turnCount, activeRoute.waypoints.size());

            ImGui::Spacing();
            if (ImGui::Button("Recalculate Path", ImVec2(180.0f, 28.0f))) {
                if (sel) {
                    activeRoute = pathFinder.calculateRouteToSlot(lot.getEntranceNodeId(), sel->getId(), m_routingAlgorithm == 1);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Dispatch Vehicle Along Route", ImVec2(220.0f, 28.0f))) {
                if (sel) {
                    sim.spawnVehicle(sel->getType(), 25.0f);
                }
            }

            if (ImGui::Button("Clear Active Route", ImVec2(-1.0f, 26.0f))) {
                activeRoute.clear();
                lot.clearSelection();
            }

            ImGui::Separator();
            ImGui::Text("Turn-by-Turn Waypoint Instructions:");
            ImGui::BeginChild("WaypointsList", ImVec2(0.0f, 200.0f), true);
            for (size_t i = 0; i < activeRoute.stepInstructions.size(); ++i) {
                ImGui::Text("%zu. %s", i + 1, activeRoute.stepInstructions[i].c_str());
            }
            ImGui::EndChild();
        } else {
            ImGui::TextWrapped("No active navigation route. Select any parking slot from the map or dashboard to compute optimal shortest path.");
        }

        ImGui::Separator();
        ImGui::Checkbox("Show Navigation Graph Overlay (F3)", &showNavGraph);
    }
    ImGui::End();
}

void UIManager::renderSimulationPage(ParkingLot& lot, Simulation& sim, PathFinder& pathFinder, Route& activeRoute) {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 92.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380.0f, 540.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Simulation Controller", nullptr)) {
        ImGui::Text("Simulation Clock: %s", sim.getFormattedSimulationTime().c_str());
        ImGui::Text("Active Vehicles Inside: %zu", sim.getActiveVehicleCount());
        ImGui::Separator();

        // Playback Buttons
        if (sim.isPlaying()) {
            if (ImGui::Button("PAUSE (Space)", ImVec2(110.0f, 32.0f))) sim.pause();
        } else {
            if (ImGui::Button("PLAY (Space)", ImVec2(110.0f, 32.0f))) sim.play();
        }
        ImGui::SameLine();
        if (ImGui::Button("STEP", ImVec2(80.0f, 32.0f))) sim.step();
        ImGui::SameLine();
        if (ImGui::Button("RESET", ImVec2(80.0f, 32.0f))) sim.reset();

        ImGui::Spacing();
        float speed = sim.getSpeedMultiplier();
        ImGui::Text("Simulation Speed: %.2fx", speed);
        if (ImGui::SliderFloat("##SpeedSlider", &speed, 0.25f, 4.0f, "%.2fx")) {
            sim.setSpeedMultiplier(speed);
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Autonomous Presentation Mode");
        if (sim.isDemoActive()) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "Demonstration in progress:");
            ImGui::TextWrapped("%s", sim.getDemoStepDescription().c_str());
            if (ImGui::Button("Stop Demonstration", ImVec2(-1.0f, 30.0f))) {
                sim.stopDemoMode();
            }
        } else {
            if (ImGui::Button("Start Autonomous Demonstration Mode", ImVec2(-1.0f, 34.0f))) {
                sim.startDemoMode();
            }
        }

        ImGui::Separator();
        ImGui::Text("Active Vehicles in Facility:");
        ImGui::BeginChild("VehicleList", ImVec2(0.0f, 180.0f), true);
        for (const auto& v : sim.getVehicles()) {
            ImGui::Text("%s [%s] - %s (Target: %s)", 
                        v->getId().c_str(), 
                        v->getLicensePlate().c_str(), 
                        v->getStateString().c_str(),
                        v->getTargetSlotId().c_str());
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void UIManager::renderAnalyticsPage(const ParkingLot& lot, const Simulation& sim) {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 92.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(460.0f, 520.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Facility Analytics & Statistics", nullptr)) {
        const auto& analytics = sim.getAnalytics();

        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Real-Time Simulation Analytics");
        ImGui::Separator();

        ImGui::Text("Total Vehicles Served: %d", analytics.totalVehiclesServed);
        ImGui::Text("Current Facility Occupancy: %.1f%%", analytics.currentOccupancyRate);
        ImGui::Text("Peak Occupancy Recorded: %.1f%%", analytics.peakOccupancyRate);
        ImGui::Text("Average Parking Duration: %.1f seconds", analytics.averageParkingDurationSec);
        ImGui::Text("Average Navigation Distance: %.1f meters", analytics.averageNavigationDistanceMeters);

        ImGui::Separator();
        ImGui::Text("Facility Occupancy History (Last 60s):");
        std::vector<float> hist(analytics.occupancyHistory.begin(), analytics.occupancyHistory.end());
        ImGui::PlotLines("##OccupancyPlot", hist.data(), static_cast<int>(hist.size()), 0, "Occupancy (%)", 0.0f, 100.0f, ImVec2(-1.0f, 120.0f));

        ImGui::Separator();
        ImGui::Text("Traffic Distribution:");
        ImGui::Text("Entering / Navigating Vehicles: %d", analytics.vehiclesEnteringCount);
        ImGui::Text("Stationary Parked Vehicles: %d", analytics.vehiclesInsideCount);
        ImGui::Text("Exiting Vehicles: %d", analytics.vehiclesExitingCount);

        ImGui::Spacing();
        ImGui::TextDisabled("Note: All statistics generated from real-time dynamic simulation data.");
    }
    ImGui::End();
}

void UIManager::renderGraphicsConceptsPage(const Camera& camera) {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 92.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(560.0f, 640.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Educational Computer Graphics Concepts", nullptr)) {
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Computer Graphics Engineering Principles (Viva Reference)");
        ImGui::Separator();

        if (ImGui::CollapsingHeader("1. Affine Transformations (Translation, Rotation, Scaling)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Translation (T): Moves primitives in 3D space: p' = p + d.");
            ImGui::BulletText("Rotation (R): Rotates around axis using Rodrigues or trigonometric matrices: p' = R * p.");
            ImGui::BulletText("Scaling (S): Stretches or shrinks geometry: p' = S * p.");
            ImGui::BulletText("Composite Transformation: Combined via non-commutative matrix multiplication: M = T * R * S.");
        }

        if (ImGui::CollapsingHeader("2. Model, View & Projection (MVP) Pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Model Matrix (M): Converts Local Object Space -> World Space coordinates.");
            ImGui::BulletText("View Matrix (V): Converts World Space -> Camera/Eye Space via glm::lookAt(eye, center, up).");
            ImGui::BulletText("Projection Matrix (P): Converts Camera Space -> Normalized Device Coordinates (NDC) / Clip Space.");
            ImGui::BulletText("Full Pipeline Vertex Transformation: v_clip = P * V * M * v_local.");
        }

        if (ImGui::CollapsingHeader("3. Camera & Projection (Perspective vs Orthographic)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Perspective Projection: Frustum volume with foreshortening (objects farther appear smaller, FOV 45 deg).");
            ImGui::BulletText("Orthographic Projection: Cuboid volume with parallel projection rays (no foreshortening, CAD/2D maps).");
            ImGui::BulletText("Camera Viewing: Maintained via target orbit, eye position, yaw/pitch spherical coordinates.");
            glm::vec3 camPos = camera.getPosition();
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "Active Mode: %s | Pos: (%.1f, %.1f, %.1f)",
                               camera.getMode() == CameraMode::MODE_3D_PERSPECTIVE ? "3D Perspective" : "2D Orthographic Top-Down",
                               camPos.x, camPos.y, camPos.z);
        }

        if (ImGui::CollapsingHeader("4. Blinn-Phong Illumination & Lighting Models", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Ambient: Uniform baseline indirect irradiance: I_amb = k_a * L_a.");
            ImGui::BulletText("Diffuse: Lambert's cosine law: I_diff = k_d * L_d * max(dot(N, L), 0.0).");
            ImGui::BulletText("Specular: Blinn-Phong halfway vector H = normalize(L + V): I_spec = k_s * L_s * max(dot(N, H), 0.0)^shininess.");
            ImGui::BulletText("Directional Sun & Streetlights: Dual lighting model supporting Day sunlight and Night streetlight point illumination.");
        }

        if (ImGui::CollapsingHeader("5. Real-Time Shadow Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Pass 1 (Depth Generation): Render scene from light's perspective into Depth FBO (2048x2048).");
            ImGui::BulletText("Pass 2 (Shadow Comparison): Transform surface position to light-space, sample depth map.");
            ImGui::BulletText("Shadow Condition: If currentDepth - bias > closestDepthFromMap, surface is in shadow.");
            ImGui::BulletText("Shadow Acne & Bias: Slope-scaled depth bias prevents self-shadowing artifacts.");
            ImGui::BulletText("PCF (Percentage-Closer Filtering): 3x3 kernel averaging produces smooth anti-aliased penumbra.");
        }

        if (ImGui::CollapsingHeader("6. Depth Buffering (Z-Buffer) & Rasterization", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Rasterization: Converting geometric primitives into discrete pixel fragments.");
            ImGui::BulletText("Z-Buffering: Hardware depth test (GL_DEPTH_TEST, GL_LESS) eliminates hidden surfaces.");
            ImGui::BulletText("Early-Z: Depth rejection before fragment execution enhances rendering throughput.");
            ImGui::BulletText("Alpha Blending: GL_BLEND with GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA for glass and UI overlays.");
        }

        if (ImGui::CollapsingHeader("7. Vehicle & Camera Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("State-Machine Simulation: ENTERING -> ROUTING -> DRIVING -> PARKING -> PARKED -> EXITING.");
            ImGui::BulletText("Waypoints Linear Interpolation (Lerp): p(t) = (1 - t) * A + t * B.");
            ImGui::BulletText("Orientation Heading: atan2(dy, dx) smoothly interpolates car yaw to follow route curves.");
            ImGui::BulletText("Frame-Rate Independence: All positions and rotations integrated with delta-time dt.");
        }
    }
    ImGui::End();
}

void UIManager::renderSettingsPage(Camera& camera, Renderer& renderer) {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 92.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(440.0f, 540.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("System Settings", nullptr)) {
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Graphics & Visual Realism (Phase 3A)");
        ImGui::Separator();

        // Time of Day
        ImGui::Text("Time of Day:");
        bool isDay = (renderer.getTimeOfDay() == TimeOfDay::DAY);
        if (ImGui::RadioButton("Day (Sunlight)", isDay)) {
            renderer.setTimeOfDay(TimeOfDay::DAY);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Night (Streetlights)", !isDay)) {
            renderer.setTimeOfDay(TimeOfDay::NIGHT);
        }

        ImGui::Spacing();
        ImGui::Checkbox("Enable Real-Time Shadows", &renderer.renderShadows);
        ImGui::Checkbox("Show Shadow Map (Debug FBO)", &showShadowMapDebug);
        ImGui::Checkbox("Enable Lighting", &renderer.renderLightingEnabled);
        ImGui::Checkbox("Render Road Markings & Arrows", &renderer.renderRoadMarkingsEnabled);
        ImGui::Checkbox("Render Environmental Trees & Poles", &renderer.renderEnvironmentTrees);

        // Shadow parameters
        if (renderer.renderShadows) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Shadow Mapping Parameters:");
            
            // Resolution dropdown
            int currentRes = renderer.getShadowMap().getResolution();
            const char* resOptions[] = { "1024 x 1024", "2048 x 2048 (Default)", "4096 x 4096 (High)" };
            int selectedResIdx = (currentRes == 1024) ? 0 : ((currentRes == 4096) ? 2 : 1);
            if (ImGui::Combo("Shadow Map Res", &selectedResIdx, resOptions, IM_ARRAYSIZE(resOptions))) {
                int newRes = (selectedResIdx == 0) ? 1024 : ((selectedResIdx == 2) ? 4096 : 2048);
                renderer.getShadowMap().setResolution(newRes);
            }

            // Direction sliders
            glm::vec3 lDir = renderer.getShadowMap().getLightDirection();
            if (ImGui::SliderFloat3("Light Dir", &lDir.x, -1.0f, 1.0f, "%.2f")) {
                if (glm::length(lDir) > 0.001f) {
                    renderer.getShadowMap().setLightDirection(lDir);
                }
            }

            // Bias and intensity
            float bias = renderer.getShadowMap().getBias();
            if (ImGui::SliderFloat("Shadow Bias", &bias, 0.0005f, 0.010f, "%.4f")) {
                renderer.getShadowMap().setBias(bias);
            }

            float intensity = renderer.getShadowMap().getIntensity();
            if (ImGui::SliderFloat("Shadow Intensity", &intensity, 0.1f, 1.0f, "%.2f")) {
                renderer.getShadowMap().setIntensity(intensity);
            }
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Display & Overlay Settings");
        ImGui::Checkbox("Enable VSync", &vsyncEnabled);
        ImGui::Checkbox("Show FPS Counter", &showFPS);
        ImGui::Checkbox("Show Navigation Graph", &showNavGraph);
        ImGui::Checkbox("Show 2D Slot ID Labels & Gate Badges", &showSlotLabels);
        ImGui::Checkbox("Wireframe Mode", &wireframeMode);

        ImGui::Separator();
        ImGui::Text("Camera Controls:");
        ImGui::SliderFloat("Move Speed", &camera.movementSpeed, 10.0f, 60.0f, "%.1f m/s");
        ImGui::SliderFloat("Mouse Sensitivity", &camera.mouseSensitivity, 0.05f, 0.35f, "%.2f");

        if (ImGui::Button("Reset Camera Position (R)", ImVec2(-1.0f, 28.0f))) {
            camera.reset();
        }
    }
    ImGui::End();
}

void UIManager::renderHelpAboutPage() {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 92.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(440.0f, 480.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("About & User Guide", nullptr)) {
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "SMART PARKING");
        ImGui::Text("Interactive 2D/3D Visualization & Navigation System");
        ImGui::Separator();

        ImGui::Text("Keyboard Controls:");
        ImGui::BulletText("W / A / S / D : Move camera / Pan");
        ImGui::BulletText("Q / E : Move camera vertically (3D)");
        ImGui::BulletText("1 : Switch to 2D Top-Down View");
        ImGui::BulletText("2 : Switch to 3D Perspective View");
        ImGui::BulletText("R : Reset camera");
        ImGui::BulletText("Space : Play / Pause simulation");
        ImGui::BulletText("N : Find & navigate to nearest slot");
        ImGui::BulletText("F3 : Toggle Debug Overlay");
        ImGui::BulletText("Mouse Left Click : Select slot / bay");
        ImGui::BulletText("Mouse Right Click + Drag : Rotate 3D camera");
        ImGui::BulletText("Mouse Scroll : Zoom in / out");

        ImGui::Separator();
        ImGui::Text("Domain: Computer Graphics & Visualization");
        ImGui::Text("Tech: C++17, OpenGL 3.3 Core, GLFW, GLAD, GLM, Dear ImGui");
    }
    ImGui::End();
}

void UIManager::renderDebugOverlay(const Camera& camera, Renderer& renderer, const ParkingLot& lot, const Simulation& sim, const Route& activeRoute, float fps) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 300.0f, viewport->Pos.y + 92.0f));
    ImGui::SetNextWindowSize(ImVec2(290.0f, 480.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("Debug HUD (F3)", nullptr, flags)) {
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Performance Metrics:");
        ImGui::Text("FPS: %.1f (%.2f ms)", fps, 1000.0f / (fps > 0.0f ? fps : 60.0f));
        ImGui::Text("Draw Calls: %d", renderer.getStats().drawCalls);
        ImGui::Text("Shadow Map Res: %d x %d", renderer.getStats().shadowMapResolution, renderer.getStats().shadowMapResolution);
        ImGui::Text("Active Lights: 1 Sun + %d Lamps", renderer.getStats().activePointLights);
        ImGui::Text("Rendered Slots: %d", renderer.getStats().renderedSlots);
        ImGui::Text("Vehicle Count: %zu", sim.getVehicles().size());

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "System State:");
        const ParkingSlot* selSlot = lot.getSelectedSlot();
        ImGui::Text("Selected Slot: %s", selSlot ? selSlot->getId().c_str() : "None");
        ImGui::Text("Active Route: %s", activeRoute.isValid ? (std::to_string(activeRoute.totalDistance).substr(0, 5) + " m").c_str() : "None");
        ImGui::Text("Render Mode: %s", camera.getMode() == CameraMode::MODE_3D_PERSPECTIVE ? "3D Perspective" : "2D Orthographic");
        ImGui::Text("Time of Day: %s", renderer.getTimeOfDay() == TimeOfDay::DAY ? "DAY" : "NIGHT");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Camera State:");
        glm::vec3 pos = camera.getPosition();
        ImGui::Text("Pos: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
        ImGui::Text("Yaw: %.1f | Pitch: %.1f", camera.getYaw(), camera.getPitch());

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "Phase 3A Subsystem Toggles:");
        ImGui::Checkbox("Shadows", &renderer.renderShadows);
        ImGui::SameLine();
        ImGui::Checkbox("Shadow Map", &showShadowMapDebug);
        ImGui::Checkbox("Lighting", &renderer.renderLightingEnabled);
        ImGui::SameLine();
        ImGui::Checkbox("Road Markings", &renderer.renderRoadMarkingsEnabled);
        ImGui::Checkbox("Trees & Landscape", &renderer.renderEnvironmentTrees);
        ImGui::SameLine();
        ImGui::Checkbox("Debug Graph", &showNavGraph);
    }
    ImGui::End();
}

void UIManager::renderShadowMapViewer(Renderer& renderer) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 10.0f, viewport->Pos.y + viewport->Size.y - 340.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260.0f, 320.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Shadow Map Depth Buffer", &showShadowMapDebug)) {
        GLuint depthTex = renderer.getShadowMap().getDepthTexture();
        if (depthTex != 0) {
            // Display depth texture (flipped vertically for OpenGL NDC coordinates)
            ImGui::Image(static_cast<ImTextureID>(depthTex),
                         ImVec2(220.0f, 220.0f),
                         ImVec2(0.0f, 1.0f),
                         ImVec2(1.0f, 0.0f));
        } else {
            ImGui::Text("Shadow Map FBO not initialized");
        }

        ImGui::Separator();
        ImGui::Text("Resolution: %d x %d", renderer.getShadowMap().getResolution(), renderer.getShadowMap().getResolution());
        glm::vec3 lDir = renderer.getShadowMap().getLightDirection();
        ImGui::Text("Light Dir: (%.2f, %.2f, %.2f)", lDir.x, lDir.y, lDir.z);
        ImGui::Text("Shadow Bias: %.4f", renderer.getShadowMap().getBias());
    }
    ImGui::End();
}

void UIManager::renderWorldOverlays(const ParkingLot& lot, const Camera& camera) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList) return;

    float aspect = static_cast<float>(vp->Size.x) / static_cast<float>(vp->Size.y);
    glm::mat4 viewProj = camera.getProjectionMatrix(aspect) * camera.getViewMatrix();
    bool is2D = (camera.getMode() == CameraMode::MODE_2D_TOPDOWN);

    auto worldToScreen = [&](const glm::vec3& worldPos, ImVec2& outScreen) -> bool {
        glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
        if (!is2D && clip.w <= 0.001f) return false;
        float invW = is2D ? 1.0f : (1.0f / clip.w);
        float ndcX = clip.x * invW;
        float ndcY = clip.y * invW;
        if (ndcX < -1.15f || ndcX > 1.15f || ndcY < -1.15f || ndcY > 1.15f) return false;

        outScreen.x = (ndcX * 0.5f + 0.5f) * vp->Size.x + vp->Pos.x;
        outScreen.y = ((1.0f - ndcY) * 0.5f) * vp->Size.y + vp->Pos.y;
        return true;
    };

    // 1. Entrance and Exit Badges
    ImVec2 entScreen, exitScreen;
    if (worldToScreen(lot.getEntrancePosition() + glm::vec3(0.0f, is2D ? 0.1f : 5.8f, 0.0f), entScreen)) {
        ImVec2 textSz = ImGui::CalcTextSize("ENTRANCE");
        drawList->AddRectFilled(ImVec2(entScreen.x - textSz.x * 0.5f - 6.0f, entScreen.y - textSz.y * 0.5f - 3.0f),
                                ImVec2(entScreen.x + textSz.x * 0.5f + 6.0f, entScreen.y + textSz.y * 0.5f + 3.0f),
                                IM_COL32(20, 160, 60, 230), 4.0f);
        drawList->AddText(ImVec2(entScreen.x - textSz.x * 0.5f, entScreen.y - textSz.y * 0.5f),
                          IM_COL32(255, 255, 255, 255), "ENTRANCE");
    }

    if (worldToScreen(lot.getExitPosition() + glm::vec3(0.0f, is2D ? 0.1f : 5.8f, 0.0f), exitScreen)) {
        ImVec2 textSz = ImGui::CalcTextSize("EXIT");
        drawList->AddRectFilled(ImVec2(exitScreen.x - textSz.x * 0.5f - 6.0f, exitScreen.y - textSz.y * 0.5f - 3.0f),
                                ImVec2(exitScreen.x + textSz.x * 0.5f + 6.0f, exitScreen.y + textSz.y * 0.5f + 3.0f),
                                IM_COL32(210, 35, 35, 230), 4.0f);
        drawList->AddText(ImVec2(exitScreen.x - textSz.x * 0.5f, exitScreen.y - textSz.y * 0.5f),
                          IM_COL32(255, 255, 255, 255), "EXIT");
    }

    // 2. Slot ID labels (Render in 2D mode when zoomed in appropriately)
    if (is2D && camera.getOrthoScale() <= 90.0f) {
        const ParkingSlot* selSlot = lot.getSelectedSlot();
        for (const auto& slot : lot.getAllSlots()) {
            ImVec2 screenPos;
            if (worldToScreen(slot.getPosition() + glm::vec3(0.0f, 0.1f, 0.0f), screenPos)) {
                bool isSelected = (selSlot && selSlot->getId() == slot.getId());
                std::string idStr = slot.getId();
                ImVec2 sz = ImGui::CalcTextSize(idStr.c_str());

                if (isSelected) {
                    drawList->AddRectFilled(ImVec2(screenPos.x - sz.x * 0.5f - 3.0f, screenPos.y - sz.y * 0.5f - 2.0f),
                                            ImVec2(screenPos.x + sz.x * 0.5f + 3.0f, screenPos.y + sz.y * 0.5f + 2.0f),
                                            IM_COL32(0, 200, 240, 240), 3.0f);
                    drawList->AddText(ImVec2(screenPos.x - sz.x * 0.5f, screenPos.y - sz.y * 0.5f),
                                      IM_COL32(10, 15, 20, 255), idStr.c_str());
                } else {
                    drawList->AddRectFilled(ImVec2(screenPos.x - sz.x * 0.5f - 2.0f, screenPos.y - sz.y * 0.5f - 1.0f),
                                            ImVec2(screenPos.x + sz.x * 0.5f + 2.0f, screenPos.y + sz.y * 0.5f + 1.0f),
                                            IM_COL32(20, 24, 28, 175), 2.0f);
                    drawList->AddText(ImVec2(screenPos.x - sz.x * 0.5f, screenPos.y - sz.y * 0.5f),
                                      IM_COL32(245, 245, 245, 220), idStr.c_str());
                }
            }
        }
    }
}

} // namespace SmartParking
