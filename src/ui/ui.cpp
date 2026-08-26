#include "ui.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace KernelMonitor {
namespace UI {

UserInterface::UserInterface()
    : mainWin_(nullptr), systemWin_(nullptr), contentWin_(nullptr),
      statusWin_(nullptr), helpWin_(nullptr),
      viewMode_(ViewMode::PROCESS_LIST), selectedIndex_(0), scrollOffset_(0),
      statusIsError_(false), termWidth_(0), termHeight_(0) {
}

UserInterface::~UserInterface() {
    cleanup();
}

bool UserInterface::initialize() {
    // Initialize ncurses
    mainWin_ = initscr();
    if (!mainWin_) {
        return false;
    }

    // Configure ncurses
    cbreak();              // Disable line buffering
    noecho();              // Don't echo input
    keypad(stdscr, TRUE);  // Enable special keys
    nodelay(stdscr, TRUE); // Non-blocking input
    curs_set(0);           // Hide cursor

    // Enable colors if available
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLUE);
    }

    // Get terminal size
    getmaxyx(stdscr, termHeight_, termWidth_);

    // Create windows
    systemWin_ = newwin(4, termWidth_, 0, 0);
    contentWin_ = newwin(termHeight_ - 7, termWidth_, 4, 0);
    statusWin_ = newwin(1, termWidth_, termHeight_ - 3, 0);
    helpWin_ = newwin(2, termWidth_, termHeight_ - 2, 0);

    return true;
}

void UserInterface::cleanup() {
    if (systemWin_) delwin(systemWin_);
    if (contentWin_) delwin(contentWin_);
    if (statusWin_) delwin(statusWin_);
    if (helpWin_) delwin(helpWin_);

    endwin();
}

void UserInterface::draw(const System::SystemMonitor& sysMonitor,
                        const Process::ProcessMonitor& procMonitor) {
    // Check for terminal resize
    int newHeight, newWidth;
    getmaxyx(stdscr, newHeight, newWidth);

    if (newHeight != termHeight_ || newWidth != termWidth_) {
        termHeight_ = newHeight;
        termWidth_ = newWidth;

        // Recreate windows
        if (systemWin_) delwin(systemWin_);
        if (contentWin_) delwin(contentWin_);
        if (statusWin_) delwin(statusWin_);
        if (helpWin_) delwin(helpWin_);

        systemWin_ = newwin(4, termWidth_, 0, 0);
        contentWin_ = newwin(termHeight_ - 7, termWidth_, 4, 0);
        statusWin_ = newwin(1, termWidth_, termHeight_ - 3, 0);
        helpWin_ = newwin(2, termWidth_, termHeight_ - 2, 0);
    }

    // Clear screen
    clear();

    // Draw components
    drawSystemInfo(sysMonitor);

    switch (viewMode_) {
        case ViewMode::PROCESS_LIST:
            drawProcessList(procMonitor);
            break;
        case ViewMode::PROCESS_DETAIL:
            drawProcessDetail(procMonitor);
            break;
        case ViewMode::PROCESS_TREE:
            drawProcessTree(procMonitor);
            break;
        case ViewMode::EVENTS:
            drawEvents(procMonitor);
            break;
    }

    drawStatusBar();
    drawHelpBar();

    // Refresh all windows
    wrefresh(systemWin_);
    wrefresh(contentWin_);
    wrefresh(statusWin_);
    wrefresh(helpWin_);
    refresh();
}

void UserInterface::drawSystemInfo(const System::SystemMonitor& sysMonitor) {
    werase(systemWin_);
    box(systemWin_, 0, 0);

    // Title
    wattron(systemWin_, A_BOLD | COLOR_PAIR(4));
    mvwprintw(systemWin_, 0, 2, " KERNEL MONITOR ");
    wattroff(systemWin_, A_BOLD | COLOR_PAIR(4));

    // Line 1: CPU and Memory
    mvwprintw(systemWin_, 1, 2, "CPU: ");
    wattron(systemWin_, COLOR_PAIR(sysMonitor.getCpuUsagePercent() > 80 ? 3 : 1));
    wprintw(systemWin_, "%s", formatPercent(sysMonitor.getCpuUsagePercent()).c_str());
    wattroff(systemWin_, COLOR_PAIR(sysMonitor.getCpuUsagePercent() > 80 ? 3 : 1));

    wprintw(systemWin_, "     RAM: ");
    wattron(systemWin_, COLOR_PAIR(sysMonitor.getMemUsagePercent() > 80 ? 3 : 1));
    wprintw(systemWin_, "%s", formatPercent(sysMonitor.getMemUsagePercent()).c_str());
    wattroff(systemWin_, COLOR_PAIR(sysMonitor.getMemUsagePercent() > 80 ? 3 : 1));

    wprintw(systemWin_, "     SWAP: ");
    wattron(systemWin_, COLOR_PAIR(sysMonitor.getSwapUsagePercent() > 50 ? 2 : 1));
    wprintw(systemWin_, "%s", formatPercent(sysMonitor.getSwapUsagePercent()).c_str());
    wattroff(systemWin_, COLOR_PAIR(sysMonitor.getSwapUsagePercent() > 50 ? 2 : 1));

    wprintw(systemWin_, "     LOAD: %.2f", sysMonitor.getLoadAvg1());

    // Line 2: Kernel version and uptime
    mvwprintw(systemWin_, 2, 2, "Kernel: %s     Uptime: %s     CPUs: %d",
              truncate(sysMonitor.getKernelVersion(), 20).c_str(),
              sysMonitor.formatUptime().c_str(),
              sysMonitor.getCpuCount());
}

void UserInterface::drawProcessList(const Process::ProcessMonitor& procMonitor) {
    werase(contentWin_);
    box(contentWin_, 0, 0);

    // Get process list
    auto processes = procMonitor.getProcessList();

    // Sort by CPU usage (descending)
    std::sort(processes.begin(), processes.end(),
              [](const Process::ProcessInfo& a, const Process::ProcessInfo& b) {
                  return a.cpuPercent > b.cpuPercent;
              });

    // Store PIDs for selection
    displayedPids_.clear();
    for (const auto& proc : processes) {
        displayedPids_.push_back(proc.pid);
    }

    // Header
    wattron(contentWin_, A_BOLD);
    mvwprintw(contentWin_, 1, 2, "%-7s %-20s %6s %6s %8s %5s",
              "PID", "PROCESS", "CPU%", "MEM%", "STATE", "THR");
    wattroff(contentWin_, A_BOLD);

    // Calculate visible area
    int contentHeight = termHeight_ - 9;
    size_t visibleCount = std::min(static_cast<size_t>(contentHeight),
                                   processes.size());

    // Adjust scroll offset if needed
    if (selectedIndex_ >= scrollOffset_ + visibleCount) {
        scrollOffset_ = selectedIndex_ - visibleCount + 1;
    }
    if (selectedIndex_ < scrollOffset_) {
        scrollOffset_ = selectedIndex_;
    }

    // Draw processes
    for (size_t i = 0; i < visibleCount && (scrollOffset_ + i) < processes.size(); ++i) {
        size_t procIndex = scrollOffset_ + i;
        const auto& proc = processes[procIndex];

        int row = 2 + i;

        // Highlight selected
        if (procIndex == selectedIndex_) {
            wattron(contentWin_, A_REVERSE);
        }

        double memPercent = Process::ProcessMonitor::calculateMemoryPercent(proc);

        mvwprintw(contentWin_, row, 2, "%-7d %-20s %5.1f%% %5.1f%% %8s %5ld",
                  proc.pid,
                  truncate(proc.name, 20).c_str(),
                  proc.cpuPercent,
                  memPercent,
                  truncate(Process::ProcessMonitor::getStateString(proc.state), 8).c_str(),
                  proc.numThreads);

        if (procIndex == selectedIndex_) {
            wattroff(contentWin_, A_REVERSE);
        }
    }

    // Show scroll indicator
    if (processes.size() > visibleCount) {
        mvwprintw(contentWin_, 0, termWidth_ - 20, " [%zu/%zu] ",
                  selectedIndex_ + 1, processes.size());
    }
}

void UserInterface::drawProcessDetail(const Process::ProcessMonitor& procMonitor) {
    werase(contentWin_);
    box(contentWin_, 0, 0);

    wattron(contentWin_, A_BOLD);
    mvwprintw(contentWin_, 0, 2, " PROCESS DETAILS ");
    wattroff(contentWin_, A_BOLD);

    if (displayedPids_.empty() || selectedIndex_ >= displayedPids_.size()) {
        mvwprintw(contentWin_, 2, 2, "No process selected");
        return;
    }

    pid_t selectedPid = displayedPids_[selectedIndex_];
    auto procInfo = procMonitor.getProcessInfo(selectedPid);

    if (!procInfo) {
        mvwprintw(contentWin_, 2, 2, "Process no longer exists");
        return;
    }

    const auto& proc = *procInfo;
    int row = 2;

    // Basic info
    mvwprintw(contentWin_, row++, 2, "PID: %d", proc.pid);
    mvwprintw(contentWin_, row++, 2, "PPID: %d", proc.ppid);
    mvwprintw(contentWin_, row++, 2, "Name: %s", proc.name.c_str());
    mvwprintw(contentWin_, row++, 2, "State: %s (%c)",
              Process::ProcessMonitor::getStateString(proc.state).c_str(),
              proc.state);

    row++;

    // Resource usage
    mvwprintw(contentWin_, row++, 2, "CPU Usage: %.2f%%", proc.cpuPercent);
    mvwprintw(contentWin_, row++, 2, "Memory (RSS): %s",
              System::SystemMonitor::formatBytes(proc.vmRss).c_str());
    mvwprintw(contentWin_, row++, 2, "Virtual Memory: %s",
              System::SystemMonitor::formatBytes(proc.vmSize).c_str());
    mvwprintw(contentWin_, row++, 2, "Memory Percent: %.2f%%",
              Process::ProcessMonitor::calculateMemoryPercent(proc));

    row++;

    // Process details
    mvwprintw(contentWin_, row++, 2, "Threads: %ld", proc.numThreads);
    mvwprintw(contentWin_, row++, 2, "Priority: %d", proc.priority);
    mvwprintw(contentWin_, row++, 2, "Nice: %d", proc.nice);
    mvwprintw(contentWin_, row++, 2, "File Descriptors: %d", proc.fdCount);

    row++;

    // Command line (wrap if needed)
    mvwprintw(contentWin_, row++, 2, "Command Line:");
    std::string cmdline = proc.cmdline;
    int maxWidth = termWidth_ - 6;
    size_t pos = 0;

    while (pos < cmdline.length() && row < termHeight_ - 10) {
        std::string line = cmdline.substr(pos, maxWidth);
        mvwprintw(contentWin_, row++, 4, "%s", line.c_str());
        pos += maxWidth;
    }

    if (!proc.exePath.empty()) {
        row++;
        mvwprintw(contentWin_, row++, 2, "Executable:");
        mvwprintw(contentWin_, row++, 4, "%s", truncate(proc.exePath, maxWidth).c_str());
    }
}

void UserInterface::drawProcessTree(const Process::ProcessMonitor& procMonitor) {
    werase(contentWin_);
    box(contentWin_, 0, 0);

    wattron(contentWin_, A_BOLD);
    mvwprintw(contentWin_, 0, 2, " PROCESS TREE ");
    wattroff(contentWin_, A_BOLD);

    auto tree = procMonitor.buildProcessTree();

    int row = 2;
    int maxRow = termHeight_ - 10;

    for (pid_t rootPid : tree.roots) {
        if (row >= maxRow) break;
        drawTreeRecursive(tree, rootPid, 0, row, maxRow);
    }
}

void UserInterface::drawTreeRecursive(const Process::ProcessTree& tree,
                                     pid_t pid, int depth, int& row, int maxRow) {
    if (row >= maxRow) return;

    auto it = tree.processes.find(pid);
    if (it == tree.processes.end()) return;

    const auto& proc = it->second;

    // Draw indentation
    std::string indent(depth * 2, ' ');
    if (depth > 0) {
        indent += "└─ ";
    }

    mvwprintw(contentWin_, row++, 2, "%s%d %s",
              indent.c_str(), proc.pid, truncate(proc.name, 30 - indent.length()).c_str());

    // Draw children
    auto childIt = tree.children.find(pid);
    if (childIt != tree.children.end()) {
        for (pid_t childPid : childIt->second) {
            drawTreeRecursive(tree, childPid, depth + 1, row, maxRow);
        }
    }
}

void UserInterface::drawEvents(const Process::ProcessMonitor& procMonitor) {
    werase(contentWin_);
    box(contentWin_, 0, 0);

    wattron(contentWin_, A_BOLD);
    mvwprintw(contentWin_, 0, 2, " RECENT EVENTS ");
    wattroff(contentWin_, A_BOLD);

    auto events = procMonitor.getRecentEvents(20);

    int row = 2;
    for (auto it = events.rbegin(); it != events.rend() && row < termHeight_ - 10; ++it) {
        const auto& event = *it;

        // Format timestamp
        char timeStr[32];
        struct tm* tm_info = localtime(&event.timestamp);
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);

        mvwprintw(contentWin_, row++, 2, "%s  PID %-7d %-20s %s",
                  timeStr,
                  event.pid,
                  truncate(event.processName, 20).c_str(),
                  event.description.c_str());
    }
}

void UserInterface::drawStatusBar() {
    werase(statusWin_);

    if (!statusMessage_.empty()) {
        if (statusIsError_) {
            wattron(statusWin_, COLOR_PAIR(3) | A_BOLD);
        } else {
            wattron(statusWin_, COLOR_PAIR(1));
        }

        mvwprintw(statusWin_, 0, 1, "%s", statusMessage_.c_str());

        if (statusIsError_) {
            wattroff(statusWin_, COLOR_PAIR(3) | A_BOLD);
        } else {
            wattroff(statusWin_, COLOR_PAIR(1));
        }
    }
}

void UserInterface::drawHelpBar() {
    werase(helpWin_);

    std::string helpText;

    switch (viewMode_) {
        case ViewMode::PROCESS_LIST:
            helpText = "[↑↓] Select  [ENTER] Details  [T] Tree  [E] Events  "
                      "[K] Kill  [S] Stop  [C] Continue  [R] Refresh  [Q] Quit";
            break;
        case ViewMode::PROCESS_DETAIL:
            helpText = "[ESC/B] Back  [Q] Quit";
            break;
        case ViewMode::PROCESS_TREE:
            helpText = "[↑↓] Scroll  [B] Back  [Q] Quit";
            break;
        case ViewMode::EVENTS:
            helpText = "[B] Back  [Q] Quit";
            break;
    }

    mvwprintw(helpWin_, 0, 1, "%s", truncate(helpText, termWidth_ - 2).c_str());
}

int UserInterface::getInput() {
    return getch();
}

void UserInterface::setViewMode(ViewMode mode) {
    viewMode_ = mode;
    scrollOffset_ = 0;
}

ViewMode UserInterface::getViewMode() const {
    return viewMode_;
}

void UserInterface::selectNextProcess() {
    if (!displayedPids_.empty() && selectedIndex_ < displayedPids_.size() - 1) {
        selectedIndex_++;
    }
}

void UserInterface::selectPreviousProcess() {
    if (selectedIndex_ > 0) {
        selectedIndex_--;
    }
}

void UserInterface::selectProcess(size_t index) {
    if (index < displayedPids_.size()) {
        selectedIndex_ = index;
    }
}

pid_t UserInterface::getSelectedPid() const {
    if (displayedPids_.empty() || selectedIndex_ >= displayedPids_.size()) {
        return -1;
    }
    return displayedPids_[selectedIndex_];
}

void UserInterface::scrollUp() {
    selectPreviousProcess();
}

void UserInterface::scrollDown() {
    selectNextProcess();
}

void UserInterface::pageUp() {
    if (selectedIndex_ >= 10) {
        selectedIndex_ -= 10;
    } else {
        selectedIndex_ = 0;
    }
}

void UserInterface::pageDown() {
    if (!displayedPids_.empty()) {
        selectedIndex_ = std::min(selectedIndex_ + 10, displayedPids_.size() - 1);
    }
}

void UserInterface::setStatusMessage(const std::string& message, bool isError) {
    statusMessage_ = message;
    statusIsError_ = isError;
}

bool UserInterface::confirmAction(const std::string& message) {
    // Create a confirmation window
    int confirmHeight = 7;
    int confirmWidth = std::min(60, termWidth_ - 4);
    int startY = (termHeight_ - confirmHeight) / 2;
    int startX = (termWidth_ - confirmWidth) / 2;

    WINDOW* confirmWin = newwin(confirmHeight, confirmWidth, startY, startX);
    box(confirmWin, 0, 0);

    wattron(confirmWin, A_BOLD | COLOR_PAIR(2));
    mvwprintw(confirmWin, 0, 2, " CONFIRMATION ");
    wattroff(confirmWin, A_BOLD | COLOR_PAIR(2));

    // Word wrap the message
    mvwprintw(confirmWin, 2, 2, "%s", truncate(message, confirmWidth - 4).c_str());

    mvwprintw(confirmWin, 4, 2, "Press [Y] to confirm, [N] to cancel");

    wrefresh(confirmWin);

    // Wait for input (blocking)
    nodelay(stdscr, FALSE);
    int ch = getch();
    nodelay(stdscr, TRUE);

    delwin(confirmWin);

    // Redraw screen
    touchwin(stdscr);
    refresh();

    return (ch == 'y' || ch == 'Y');
}

std::string UserInterface::formatPercent(double percent) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%5.1f%%", percent);
    return std::string(buffer);
}

std::string UserInterface::truncate(const std::string& str, size_t width) {
    if (str.length() <= width) {
        return str;
    }
    return str.substr(0, width - 3) + "...";
}

void UserInterface::clearArea(int startRow, int endRow) {
    for (int row = startRow; row <= endRow; ++row) {
        move(row, 0);
        clrtoeol();
    }
}

} // namespace UI
} // namespace KernelMonitor
