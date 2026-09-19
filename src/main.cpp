#include <algorithm>
#include <clocale>
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
    (void)argc;
    (void)argv;

    // Must run before initscr(): it tells ncurses which encoding the terminal
    // uses, so multibyte characters occupy one cell instead of one per byte.
    setlocale(LC_ALL, "");

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

    // How often the /proc data is re-read, adjustable at runtime with +/-.
    // Two seconds reads comfortably; a second still looks live.
    int refreshInterval = 2000;
    const int minInterval = 500;
    const int maxInterval = 10000;
    auto lastUpdate = std::chrono::steady_clock::now();

    // Repaint only when the screen would actually differ: a data tick, a key
    // that changed something, or a resize. Redrawing on a fixed timer instead
    // rewrites identical text over itself and reads as flicker.
    bool needsRedraw = true;

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
                    lastUpdate = std::chrono::steady_clock::now();
                    ui.setStatusMessage("Refreshed");
                    break;

                case '+':
                case '=':
                    // Slower updates: easier to read
                    refreshInterval = std::min(refreshInterval + 500, maxInterval);
                    ui.setStatusMessage("Update interval: " +
                        std::to_string(refreshInterval) + " ms");
                    break;

                case '-':
                case '_':
                    // Faster updates
                    refreshInterval = std::max(refreshInterval - 500, minInterval);
                    ui.setStatusMessage("Update interval: " +
                        std::to_string(refreshInterval) + " ms");
                    break;

                case KEY_RESIZE:
                    // draw() re-creates its windows at the new size.
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

                case 'x':
                case 'X': {
                    // SIGKILL. Separate key from K deliberately: this one cannot
                    // be caught, blocked or ignored, so the process gets no
                    // chance to clean up. The confirmation names the signal.
                    if (ui.getViewMode() == UI::ViewMode::PROCESS_LIST) {
                        pid_t selectedPid = ui.getSelectedPid();
                        if (selectedPid > 0) {
                            auto procInfo = procMonitor.getProcessInfo(selectedPid);
                            if (procInfo) {
                                std::string confirmMsg = "Send SIGKILL to PID " +
                                    std::to_string(selectedPid) + " (" +
                                    procInfo->name + ")? Cannot be caught.";

                                if (ui.confirmAction(confirmMsg)) {
                                    auto result = processControl.killProcess(selectedPid);
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

            if (handled) {
                needsRedraw = true;
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
            needsRedraw = true;
        }

        // Draw UI
        if (needsRedraw) {
            ui.draw(sysMonitor, procMonitor);
            needsRedraw = false;
        }

        // No sleep here: getInput() blocks for up to one input tick, which
        // paces the loop without adding lag to a keypress.
    }

    // Cleanup
    ui.cleanup();

    std::cout << "Kernel Monitor exited cleanly." << std::endl;
    return 0;
}
