#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <cstdlib>
#include "ui/ui.hpp"
#include "system/system_monitor.hpp"
#include "process/process_monitor.hpp"
#include "signals/process_control.hpp"

using namespace KernelMonitor;

// Global flag for signal handling
volatile sig_atomic_t g_running = 1;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = 0;
    }
}

void setupSignalHandlers() {
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

int main(int argc, char* argv[]) {
    // Setup signal handlers for clean exit
    setupSignalHandlers();

    // Create monitors
    System::SystemMonitor sysMonitor;
    Process::ProcessMonitor procMonitor;
    Signals::ProcessControl processControl;

    // Initialize UI
    UI::UserInterface ui;
    if (!ui.initialize()) {
        std::cerr << "Failed to initialize terminal UI" << std::endl;
        return 1;
    }

    // Initial update
    sysMonitor.update();
    procMonitor.update();

    ui.setStatusMessage("Kernel Monitor started. Press Q to quit.");

    // Refresh interval (in milliseconds)
    const int refreshInterval = 1000;
    auto lastUpdate = std::chrono::steady_clock::now();

    // Main loop
    while (g_running) {
        // Handle input
        int ch = ui.getInput();

        if (ch != ERR) {
            bool handled = true;

            switch (ch) {
                case 'q':
                case 'Q':
                    g_running = 0;
                    break;

                case 'r':
                case 'R':
                    // Force refresh
                    sysMonitor.update();
                    procMonitor.update();
                    ui.setStatusMessage("Refreshed");
                    break;

                case KEY_UP:
                    ui.scrollUp();
                    break;

                case KEY_DOWN:
                    ui.scrollDown();
                    break;

                case KEY_PPAGE:  // Page Up
                    ui.pageUp();
                    break;

                case KEY_NPAGE:  // Page Down
                    ui.pageDown();
                    break;

                case '\n':
                case KEY_ENTER:
                    // Enter detail view
                    if (ui.getViewMode() == UI::ViewMode::PROCESS_LIST) {
                        ui.setViewMode(UI::ViewMode::PROCESS_DETAIL);
                    }
                    break;

                case 27:  // ESC
                case 'b':
                case 'B':
                    // Back to process list
                    ui.setViewMode(UI::ViewMode::PROCESS_LIST);
                    ui.setStatusMessage("");
                    break;

                case 't':
                case 'T':
                    // Show process tree
                    ui.setViewMode(UI::ViewMode::PROCESS_TREE);
                    break;

                case 'e':
                case 'E':
                    // Show events
                    ui.setViewMode(UI::ViewMode::EVENTS);
                    break;

                case 'k':
                case 'K': {
                    // Kill process (SIGTERM)
                    if (ui.getViewMode() == UI::ViewMode::PROCESS_LIST) {
                        pid_t selectedPid = ui.getSelectedPid();
                        if (selectedPid > 0) {
                            auto procInfo = procMonitor.getProcessInfo(selectedPid);
                            if (procInfo) {
                                std::string confirmMsg = "Send SIGTERM to PID " +
                                    std::to_string(selectedPid) + " (" +
                                    procInfo->name + ")?";

                                if (ui.confirmAction(confirmMsg)) {
                                    auto result = processControl.terminateProcess(selectedPid);
                                    ui.setStatusMessage(
                                        "PID " + std::to_string(selectedPid) + ": " +
                                        result.errorMessage,
                                        !result.success
                                    );
                                } else {
                                    ui.setStatusMessage("Cancelled");
                                }
                            }
                        }
                    }
                    break;
                }

                case 's':
                case 'S': {
                    // Stop process (SIGSTOP)
                    if (ui.getViewMode() == UI::ViewMode::PROCESS_LIST) {
                        pid_t selectedPid = ui.getSelectedPid();
                        if (selectedPid > 0) {
                            auto procInfo = procMonitor.getProcessInfo(selectedPid);
                            if (procInfo) {
                                std::string confirmMsg = "Send SIGSTOP to PID " +
                                    std::to_string(selectedPid) + " (" +
                                    procInfo->name + ")?";

                                if (ui.confirmAction(confirmMsg)) {
                                    auto result = processControl.stopProcess(selectedPid);
                                    ui.setStatusMessage(
                                        "PID " + std::to_string(selectedPid) + ": " +
                                        result.errorMessage,
                                        !result.success
                                    );
                                }
                            }
                        }
                    }
                    break;
                }

                case 'c':
                case 'C': {
                    // Continue process (SIGCONT)
                    if (ui.getViewMode() == UI::ViewMode::PROCESS_LIST) {
                        pid_t selectedPid = ui.getSelectedPid();
                        if (selectedPid > 0) {
                            auto result = processControl.continueProcess(selectedPid);
                            ui.setStatusMessage(
                                "PID " + std::to_string(selectedPid) + ": " +
                                result.errorMessage,
                                !result.success
                            );
                        }
                    }
                    break;
                }

                default:
                    handled = false;
                    break;
            }
        }

        // Periodic refresh
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastUpdate).count();

        if (elapsed >= refreshInterval) {
            sysMonitor.update();
            procMonitor.update();
            lastUpdate = now;
        }

        // Draw UI
        ui.draw(sysMonitor, procMonitor);

        // Sleep briefly to avoid excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Cleanup
    ui.cleanup();

    std::cout << "Kernel Monitor exited cleanly." << std::endl;
    return 0;
}
