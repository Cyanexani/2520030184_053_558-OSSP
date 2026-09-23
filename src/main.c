#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "process/process_monitor.h"
#include "signals/process_control.h"
#include "system/system_monitor.h"
#include "ui/ui.h"
#include "utils/parser.h"

/* Global flag for signal handling
   [CO-3] Signal handlers and POSIX signal programming.
   A handler can interrupt the main flow at any machine instruction, so a flag
   shared with it must be written atomically and never cached in a register.
   That is exactly what volatile sig_atomic_t guarantees, and why an ordinary
   bool or int is not safe here. */
static volatile sig_atomic_t g_running = 1;

static void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = 0;
    }
}

/* [CO-3] Installing handlers with sigaction() rather than signal(). sigaction
   has defined, portable semantics; signal() varies between systems in whether
   the handler is reset after firing. The handler itself does no work: it sets a
   flag and returns, so the main loop exits on its next pass. Doing real work
   inside a handler is unsafe, because most library functions are not
   async-signal-safe. */
static void setup_signal_handlers(void) {
    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Prompt and signal the highlighted process, reporting the outcome on the
   status bar. Shared by the SIGTERM, SIGKILL and SIGSTOP keys, which differ
   only in wording and in which call they make. */
static void signal_selected(km_ui *ui, const km_process_monitor *monitor,
                            km_signal_result (*action)(pid_t),
                            const char *signal_name, const char *caveat) {
    pid_t selected_pid;
    const km_process_info *info;
    char confirm_msg[256];
    char status[320];
    km_signal_result result;

    if (km_ui_get_view_mode(ui) != KM_VIEW_PROCESS_LIST) {
        return;
    }

    selected_pid = km_ui_selected_pid(ui);
    if (selected_pid <= 0) {
        return;
    }

    info = km_process_monitor_find(monitor, selected_pid);
    if (!info) {
        return;
    }

    snprintf(confirm_msg, sizeof(confirm_msg), "Send %s to PID %d (%s)?%s",
             signal_name, (int)selected_pid, info->name, caveat);

    if (!km_ui_confirm(ui, confirm_msg)) {
        km_ui_set_status(ui, "Cancelled", false);
        return;
    }

    result = action(selected_pid);
    snprintf(status, sizeof(status), "PID %d: %s",
             (int)selected_pid, result.error_message);
    km_ui_set_status(ui, status, !result.success);
}

int main(int argc, char *argv[]) {
    km_system_monitor sys_monitor;
    km_process_monitor proc_monitor;
    km_ui ui;

    /* How often the /proc data is re-read, adjustable at runtime with +/-.
       Two seconds reads comfortably; a second still looks live. */
    int refresh_interval = 2000;
    const int min_interval = 500;
    const int max_interval = 10000;
    struct timespec last_update;

    /* Repaint only when the screen would actually differ: a data tick, a key
       that changed something, or a resize. Redrawing on a fixed timer instead
       rewrites identical text over itself and reads as flicker. */
    bool needs_redraw = true;

    (void)argc;
    (void)argv;

    /* Refuse to start without a terminal on both ends. ncurses cannot draw to a
       pipe, and worse, getch() stops honouring its timeout on a non-tty and
       returns ERR immediately, turning the main loop into a spin that pegs a
       core. Redirecting output is the common way to hit this by accident.
       [CO-1] The shell as a user-space program: stdin and stdout are inherited
       from whatever launched us, and may be a terminal, a pipe or a file. A
       full-screen UI is only meaningful on a terminal, so we ask the kernel what
       we were actually given before assuming. */
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr,
                "kernel-monitor: needs an interactive terminal.\n"
                "Standard input or output is not a tty, so the UI cannot be "
                "drawn and input cannot be read.\n"
                "Run it directly in a terminal rather than through a pipe or a "
                "redirect.\n");
        return 1;
    }

    /* Must run before initscr(): it tells ncurses which encoding the terminal
       uses, so multibyte characters occupy one cell instead of one per byte. */
    setlocale(LC_ALL, "");

    setup_signal_handlers();

    km_system_monitor_init(&sys_monitor);
    km_process_monitor_init(&proc_monitor);

    km_ui_init_state(&ui);
    if (!km_ui_initialize(&ui)) {
        fprintf(stderr, "Failed to initialize terminal UI\n");
        km_system_monitor_destroy(&sys_monitor);
        km_process_monitor_destroy(&proc_monitor);
        return 1;
    }

    km_system_monitor_update(&sys_monitor);
    km_process_monitor_update(&proc_monitor);

    km_ui_set_status(&ui, "Kernel Monitor started. Press Q to quit.", false);

    km_clock_now(&last_update);

    while (g_running) {
        int ch = km_ui_get_input();
        struct timespec now;

        if (ch != ERR) {
            bool handled = true;
            char status[64];

            switch (ch) {
                case 'q':
                case 'Q':
                    g_running = 0;
                    break;

                case 'r':
                case 'R':
                    km_system_monitor_update(&sys_monitor);
                    km_process_monitor_update(&proc_monitor);
                    km_clock_now(&last_update);
                    km_ui_set_status(&ui, "Refreshed", false);
                    break;

                case '+':
                case '=':
                    /* Slower updates: easier to read. */
                    refresh_interval += 500;
                    if (refresh_interval > max_interval) {
                        refresh_interval = max_interval;
                    }
                    snprintf(status, sizeof(status), "Update interval: %d ms",
                             refresh_interval);
                    km_ui_set_status(&ui, status, false);
                    break;

                case '-':
                case '_':
                    /* Faster updates. */
                    refresh_interval -= 500;
                    if (refresh_interval < min_interval) {
                        refresh_interval = min_interval;
                    }
                    snprintf(status, sizeof(status), "Update interval: %d ms",
                             refresh_interval);
                    km_ui_set_status(&ui, status, false);
                    break;

                case KEY_RESIZE:
                    /* km_ui_draw() re-creates its windows at the new size. */
                    break;

                case KEY_UP:
                    km_ui_scroll_up(&ui);
                    break;

                case KEY_DOWN:
                    km_ui_scroll_down(&ui);
                    break;

                case KEY_PPAGE:
                    km_ui_page_up(&ui);
                    break;

                case KEY_NPAGE:
                    km_ui_page_down(&ui);
                    break;

                case '\n':
                case KEY_ENTER:
                    if (km_ui_get_view_mode(&ui) == KM_VIEW_PROCESS_LIST) {
                        km_ui_set_view_mode(&ui, KM_VIEW_PROCESS_DETAIL);
                    }
                    break;

                case 27: /* ESC */
                case 'b':
                case 'B':
                    km_ui_set_view_mode(&ui, KM_VIEW_PROCESS_LIST);
                    km_ui_set_status(&ui, "", false);
                    break;

                case 't':
                case 'T':
                    km_ui_set_view_mode(&ui, KM_VIEW_PROCESS_TREE);
                    break;

                case 'e':
                case 'E':
                    km_ui_set_view_mode(&ui, KM_VIEW_EVENTS);
                    break;

                case 'k':
                case 'K':
                    signal_selected(&ui, &proc_monitor, km_terminate_process,
                                    "SIGTERM", "");
                    break;

                case 'x':
                case 'X':
                    /* SIGKILL. Separate key from K deliberately: this one cannot
                       be caught, blocked or ignored, so the process gets no
                       chance to clean up. The confirmation names the signal. */
                    signal_selected(&ui, &proc_monitor, km_kill_process,
                                    "SIGKILL", " Cannot be caught.");
                    break;

                case 's':
                case 'S':
                    signal_selected(&ui, &proc_monitor, km_stop_process,
                                    "SIGSTOP", "");
                    break;

                case 'c':
                case 'C': {
                    /* SIGCONT resumes a stopped process and is not destructive,
                       so it skips the confirmation the other three require. */
                    pid_t selected_pid = km_ui_selected_pid(&ui);

                    if (km_ui_get_view_mode(&ui) == KM_VIEW_PROCESS_LIST &&
                        selected_pid > 0) {
                        km_signal_result result = km_continue_process(selected_pid);
                        char msg[320];

                        snprintf(msg, sizeof(msg), "PID %d: %s",
                                 (int)selected_pid, result.error_message);
                        km_ui_set_status(&ui, msg, !result.success);
                    }
                    break;
                }

                default:
                    handled = false;
                    break;
            }

            if (handled) {
                needs_redraw = true;
            }
        }

        /* Periodic refresh. */
        km_clock_now(&now);
        if (km_clock_diff_ms(&last_update, &now) >= refresh_interval) {
            km_system_monitor_update(&sys_monitor);
            km_process_monitor_update(&proc_monitor);
            last_update = now;
            needs_redraw = true;
        }

        if (needs_redraw) {
            km_ui_draw(&ui, &sys_monitor, &proc_monitor);
            needs_redraw = false;
        }

        /* No sleep here: km_ui_get_input() blocks for up to one input tick,
           which paces the loop without adding lag to a keypress. */
    }

    km_ui_cleanup(&ui);
    km_process_monitor_destroy(&proc_monitor);
    km_system_monitor_destroy(&sys_monitor);

    printf("Kernel Monitor exited cleanly.\n");
    return 0;
}
