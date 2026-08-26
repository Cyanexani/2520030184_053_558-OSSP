#ifndef UI_HPP
#define UI_HPP

#include <ncurses.h>
#include <string>
#include <vector>
#include "../system/system_monitor.hpp"
#include "../process/process_monitor.hpp"

namespace KernelMonitor {
namespace UI {

enum class ViewMode {
    PROCESS_LIST,
    PROCESS_DETAIL,
    PROCESS_TREE,
    EVENTS
};

class UserInterface {
public:
    UserInterface();
    ~UserInterface();

    // Initialize ncurses
    bool initialize();

    // Cleanup ncurses
    void cleanup();

    // Main draw function
    void draw(const System::SystemMonitor& sysMonitor,
              const Process::ProcessMonitor& procMonitor);

    // Handle user input (non-blocking)
    int getInput();

    // View mode management
    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const;

    // Process selection
    void selectNextProcess();
    void selectPreviousProcess();
    void selectProcess(size_t index);
    pid_t getSelectedPid() const;

    // Scrolling
    void scrollUp();
    void scrollDown();
    void pageUp();
    void pageDown();

    // Status message
    void setStatusMessage(const std::string& message, bool isError = false);

    // Confirmation dialog
    bool confirmAction(const std::string& message);

private:
    // Drawing functions
    void drawSystemInfo(const System::SystemMonitor& sysMonitor);
    void drawProcessList(const Process::ProcessMonitor& procMonitor);
    void drawProcessDetail(const Process::ProcessMonitor& procMonitor);
    void drawProcessTree(const Process::ProcessMonitor& procMonitor);
    void drawEvents(const Process::ProcessMonitor& procMonitor);
    void drawStatusBar();
    void drawHelpBar();

    // Helper functions
    void drawTreeRecursive(const Process::ProcessTree& tree, pid_t pid,
                          int depth, int& row, int maxRow);
    std::string formatPercent(double percent);
    std::string truncate(const std::string& str, size_t width);
    void clearArea(int startRow, int endRow);

    // ncurses windows
    WINDOW* mainWin_;
    WINDOW* systemWin_;
    WINDOW* contentWin_;
    WINDOW* statusWin_;
    WINDOW* helpWin_;

    // UI state
    ViewMode viewMode_;
    size_t selectedIndex_;
    size_t scrollOffset_;
    std::vector<pid_t> displayedPids_;  // PIDs in current view
    std::string statusMessage_;
    bool statusIsError_;
    int termWidth_;
    int termHeight_;
};

} // namespace UI
} // namespace KernelMonitor

#endif // UI_HPP
