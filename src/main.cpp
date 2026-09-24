#include "application/Application.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n";
    std::cout << " SMART PARKING SYSTEM\n";
    std::cout << " Interactive 2D/3D Visualization & Navigation System\n";
    std::cout << " Computer Graphics and Visualization Academic Project\n";
    std::cout << "========================================================\n" << std::endl;

    SmartParking::Application app;
    if (!app.init(1600, 900, "SMART PARKING: Interactive 2D/3D Visualization & Navigation System")) {
        std::cerr << "Critical Error: Application failed to initialize." << std::endl;
        return -1;
    }

    app.run();
    app.shutdown();

    std::cout << "Application exited gracefully." << std::endl;
    return 0;
}
