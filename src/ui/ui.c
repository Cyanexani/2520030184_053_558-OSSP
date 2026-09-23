#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* How long km_ui_get_input() waits for a key before reporting ERR. Bounds how
   long a resize or a data tick can sit unpainted, without spinning the CPU. */
#define KM_INPUT_TIMEOUT_MS 120

/* Room for the ancestry bars drawn to the left of a tree node. Each level adds
   at most three bytes of ASCII or five of UTF-8. */
#define KM_TREE_PREFIX_MAX 512

static size_t size_min(size_t a, size_t b) {
    return a < b ? a : b;
}

/* Shorten str to width columns, marking the cut with an ellipsis. */
static const char *truncate_to(char *buf, size_t bufsz, const char *str, size_t width) {
    size_t len = strlen(str);

    if (bufsz == 0) {
        return "";
    }

    if (width >= bufsz) {
        width = bufsz - 1;
    }

    if (len <= width) {
        memcpy(buf, str, len + 1);
        return buf;
    }

    /* Too narrow to spend three columns on "...", so cut hard instead. */
    if (width < 4) {
        memcpy(buf, str, width);
        buf[width] = '\0';
        return buf;
    }

    memcpy(buf, str, width - 3);
    memcpy(buf + width - 3, "...", 4);
    return buf;
}

/* Clamp a width computed from the terminal size, which can go negative on a
   very narrow terminal. */
static size_t clamp_width(int width) {
    return width > 0 ? (size_t)width : 0;
}

void km_ui_init_state(km_ui *ui) {
    memset(ui, 0, sizeof(*ui));
    ui->view_mode = KM_VIEW_PROCESS_LIST;
}

static void create_windows(km_ui *ui) {
    ui->system_win = newwin(4, ui->term_width, 0, 0);
    ui->content_win = newwin(ui->term_height - 7, ui->term_width, 4, 0);
    ui->status_win = newwin(1, ui->term_width, ui->term_height - 3, 0);
    ui->help_win = newwin(2, ui->term_width, ui->term_height - 2, 0);
}

static void destroy_windows(km_ui *ui) {
    if (ui->system_win) delwin(ui->system_win);
    if (ui->content_win) delwin(ui->content_win);
    if (ui->status_win) delwin(ui->status_win);
    if (ui->help_win) delwin(ui->help_win);

    ui->system_win = NULL;
    ui->content_win = NULL;
    ui->status_win = NULL;
    ui->help_win = NULL;
}

bool km_ui_initialize(km_ui *ui) {
    ui->main_win = initscr();
    if (!ui->main_win) {
        return false;
    }

    cbreak();                       /* Disable line buffering.               */
    noecho();                       /* Don't echo input.                     */
    keypad(stdscr, TRUE);           /* Enable special keys.                  */
    timeout(KM_INPUT_TIMEOUT_MS);   /* Block for at most one tick.           */
    curs_set(0);                    /* Hide cursor.                          */

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLUE);
    }

    getmaxyx(stdscr, ui->term_height, ui->term_width);
    create_windows(ui);

    return true;
}

void km_ui_cleanup(km_ui *ui) {
    destroy_windows(ui);

    free(ui->displayed_pids);
    ui->displayed_pids = NULL;
    ui->displayed_count = 0;
    ui->displayed_capacity = 0;

    free(ui->sorted);
    ui->sorted = NULL;
    ui->sorted_capacity = 0;

    endwin();
}

static bool reserve_displayed(km_ui *ui, size_t needed) {
    if (needed > ui->displayed_capacity) {
        size_t new_cap = ui->displayed_capacity ? ui->displayed_capacity : 512;
        pid_t *grown;

        while (new_cap < needed) {
            new_cap *= 2;
        }

        grown = realloc(ui->displayed_pids, new_cap * sizeof(*grown));
        if (!grown) {
            return false;
        }
        ui->displayed_pids = grown;
        ui->displayed_capacity = new_cap;
    }
    return true;
}

static bool reserve_sorted(km_ui *ui, size_t needed) {
    if (needed > ui->sorted_capacity) {
        size_t new_cap = ui->sorted_capacity ? ui->sorted_capacity : 512;
        const km_process_info **grown;

        while (new_cap < needed) {
            new_cap *= 2;
        }

        grown = realloc(ui->sorted, new_cap * sizeof(*grown));
        if (!grown) {
            return false;
        }
        ui->sorted = grown;
        ui->sorted_capacity = new_cap;
    }
    return true;
}

static void draw_system_info(km_ui *ui, const km_system_monitor *sys) {
    char buf[64];
    char uptime[32];

    werase(ui->system_win);
    box(ui->system_win, 0, 0);

    wattron(ui->system_win, A_BOLD | COLOR_PAIR(4));
    mvwprintw(ui->system_win, 0, 2, " KERNEL MONITOR ");
    wattroff(ui->system_win, A_BOLD | COLOR_PAIR(4));

    mvwprintw(ui->system_win, 1, 2, "CPU: ");
    wattron(ui->system_win, COLOR_PAIR(sys->cpu_usage_percent > 80 ? 3 : 1));
    wprintw(ui->system_win, "%5.1f%%", sys->cpu_usage_percent);
    wattroff(ui->system_win, COLOR_PAIR(sys->cpu_usage_percent > 80 ? 3 : 1));

    wprintw(ui->system_win, "     RAM: ");
    wattron(ui->system_win, COLOR_PAIR(sys->mem_usage_percent > 80 ? 3 : 1));
    wprintw(ui->system_win, "%5.1f%%", sys->mem_usage_percent);
    wattroff(ui->system_win, COLOR_PAIR(sys->mem_usage_percent > 80 ? 3 : 1));

    wprintw(ui->system_win, "     SWAP: ");
    wattron(ui->system_win, COLOR_PAIR(sys->swap_usage_percent > 50 ? 2 : 1));
    wprintw(ui->system_win, "%5.1f%%", sys->swap_usage_percent);
    wattroff(ui->system_win, COLOR_PAIR(sys->swap_usage_percent > 50 ? 2 : 1));

    wprintw(ui->system_win, "     LOAD: %.2f", sys->load_avg_1);

    km_format_uptime(uptime, sizeof(uptime), sys->uptime);
    mvwprintw(ui->system_win, 2, 2, "Kernel: %s     Uptime: %s     CPUs: %d",
              truncate_to(buf, sizeof(buf), sys->kernel_version, 20),
              uptime,
              sys->cpu_count);
}

/* CPU descending, ties broken by PID. The tie-break is what keeps the table
   still: without it the dozens of rows sitting at 0.0% would swap places on
   every tick, since qsort is not stable. Rows that hold position let ncurses
   rewrite just the few numbers that moved. */
static int compare_by_cpu(const void *a, const void *b) {
    const km_process_info *pa = *(const km_process_info *const *)a;
    const km_process_info *pb = *(const km_process_info *const *)b;

    if (pa->cpu_percent > pb->cpu_percent) return -1;
    if (pa->cpu_percent < pb->cpu_percent) return 1;
    if (pa->pid < pb->pid) return -1;
    return pa->pid > pb->pid;
}

static void draw_process_list(km_ui *ui, const km_process_monitor *proc) {
    size_t count = proc->count;
    pid_t previously_selected;
    size_t i;
    int content_height;
    size_t visible_count;

    werase(ui->content_win);
    box(ui->content_win, 0, 0);

    if (!reserve_sorted(ui, count) || !reserve_displayed(ui, count)) {
        mvwprintw(ui->content_win, 2, 2, "Out of memory");
        return;
    }

    /* Remember which process the highlight is on before the list is rebuilt. */
    previously_selected = km_ui_selected_pid(ui);

    for (i = 0; i < count; i++) {
        ui->sorted[i] = &proc->processes[i];
    }
    qsort(ui->sorted, count, sizeof(*ui->sorted), compare_by_cpu);

    ui->displayed_count = count;
    for (i = 0; i < count; i++) {
        ui->displayed_pids[i] = ui->sorted[i]->pid;
    }

    /* Follow the selected process to its new row rather than keeping the row
       index, so K/S/C always signal the process that is highlighted. A busy
       process climbing the list would otherwise slide out from under the
       highlight between the keypress and the confirmation. */
    if (previously_selected > 0) {
        for (i = 0; i < ui->displayed_count; i++) {
            if (ui->displayed_pids[i] == previously_selected) {
                ui->selected_index = i;
                break;
            }
        }
    }
    if (ui->displayed_count > 0 && ui->selected_index >= ui->displayed_count) {
        ui->selected_index = ui->displayed_count - 1;
    }

    wattron(ui->content_win, A_BOLD);
    mvwprintw(ui->content_win, 1, 2, "%-7s %-20s %6s %6s %12s %5s",
              "PID", "PROCESS", "CPU%", "MEM%", "STATE", "THR");
    wattroff(ui->content_win, A_BOLD);

    content_height = ui->term_height - 9;
    visible_count = size_min(clamp_width(content_height), count);

    if (ui->selected_index >= ui->scroll_offset + visible_count) {
        ui->scroll_offset = ui->selected_index - visible_count + 1;
    }
    if (ui->selected_index < ui->scroll_offset) {
        ui->scroll_offset = ui->selected_index;
    }

    for (i = 0; i < visible_count && (ui->scroll_offset + i) < count; ++i) {
        size_t proc_index = ui->scroll_offset + i;
        const km_process_info *p = ui->sorted[proc_index];
        int row = 2 + (int)i;
        char name_buf[32];
        char state_buf[24];
        double mem_percent;

        if (proc_index == ui->selected_index) {
            wattron(ui->content_win, A_REVERSE);
        }

        mem_percent = km_process_memory_percent(p);

        /* 12 columns, not 8: "Disk Sleep" and "Tracing Stop" do not fit in 8 and
           were being cut to "Disk ..." and "Traci...". D is the state you look
           for when a process is wedged on uninterruptible I/O, so mangling it
           defeats the point of the column. */
        mvwprintw(ui->content_win, row, 2, "%-7d %-20s %5.1f%% %5.1f%% %12s %5ld",
                  (int)p->pid,
                  truncate_to(name_buf, sizeof(name_buf), p->name, 20),
                  p->cpu_percent,
                  mem_percent,
                  truncate_to(state_buf, sizeof(state_buf),
                              km_process_state_string(p->state), 12),
                  p->num_threads);

        if (proc_index == ui->selected_index) {
            wattroff(ui->content_win, A_REVERSE);
        }
    }

    if (count > visible_count) {
        mvwprintw(ui->content_win, 0, ui->term_width - 20, " [%zu/%zu] ",
                  ui->selected_index + 1, count);
    }
}

static void draw_process_detail(km_ui *ui, const km_process_monitor *proc) {
    pid_t selected_pid;
    const km_process_info *p;
    int row = 2;
    int max_width;
    size_t pos;
    size_t cmdline_len;
    char bytes[64];

    werase(ui->content_win);
    box(ui->content_win, 0, 0);

    wattron(ui->content_win, A_BOLD);
    mvwprintw(ui->content_win, 0, 2, " PROCESS DETAILS ");
    wattroff(ui->content_win, A_BOLD);

    if (ui->displayed_count == 0 || ui->selected_index >= ui->displayed_count) {
        mvwprintw(ui->content_win, 2, 2, "No process selected");
        return;
    }

    selected_pid = ui->displayed_pids[ui->selected_index];
    p = km_process_monitor_find(proc, selected_pid);

    if (!p) {
        mvwprintw(ui->content_win, 2, 2, "Process no longer exists");
        return;
    }

    mvwprintw(ui->content_win, row++, 2, "PID: %d", (int)p->pid);
    mvwprintw(ui->content_win, row++, 2, "PPID: %d", (int)p->ppid);
    mvwprintw(ui->content_win, row++, 2, "Name: %s", p->name);
    mvwprintw(ui->content_win, row++, 2, "State: %s (%c)",
              km_process_state_string(p->state), p->state);

    row++;

    mvwprintw(ui->content_win, row++, 2, "CPU Usage: %.2f%%", p->cpu_percent);

    km_format_bytes(bytes, sizeof(bytes), p->vm_rss);
    mvwprintw(ui->content_win, row++, 2, "Memory (RSS): %s", bytes);

    km_format_bytes(bytes, sizeof(bytes), p->vm_size);
    mvwprintw(ui->content_win, row++, 2, "Virtual Memory: %s", bytes);

    mvwprintw(ui->content_win, row++, 2, "Memory Percent: %.2f%%",
              km_process_memory_percent(p));

    row++;

    mvwprintw(ui->content_win, row++, 2, "Threads: %ld", p->num_threads);
    mvwprintw(ui->content_win, row++, 2, "Priority: %d", p->priority);
    mvwprintw(ui->content_win, row++, 2, "Nice: %d", p->nice);
    mvwprintw(ui->content_win, row++, 2, "File Descriptors: %d", p->fd_count);

    row++;

    mvwprintw(ui->content_win, row++, 2, "Command Line:");

    max_width = ui->term_width - 6;
    if (max_width < 1) {
        max_width = 1;
    }

    cmdline_len = strlen(p->cmdline);
    pos = 0;
    while (pos < cmdline_len && row < ui->term_height - 10) {
        mvwprintw(ui->content_win, row++, 4, "%.*s", max_width, p->cmdline + pos);
        pos += (size_t)max_width;
    }

    if (p->exe_path[0] != '\0') {
        char path_buf[KM_EXE_PATH_MAX];
        row++;
        mvwprintw(ui->content_win, row++, 2, "Executable:");
        mvwprintw(ui->content_win, row++, 4, "%s",
                  truncate_to(path_buf, sizeof(path_buf), p->exe_path,
                              clamp_width(max_width)));
    }
}

static void draw_tree_recursive(km_ui *ui, const km_process_tree *tree, pid_t pid,
                                int depth, int *row, int max_row, bool is_last,
                                char *prefix, size_t prefix_len) {
    const km_process_info *p;
    const pid_t *kids;
    size_t kid_count;
    char indent[KM_TREE_PREFIX_MAX + 8];
    char name_buf[KM_COMM_MAX];
    int used;
    int name_room;

    if (*row >= max_row) {
        return;
    }

    p = km_process_tree_find(tree, pid);
    if (!p) {
        return;
    }

    /* prefix carries the ancestry: one vertical bar for every ancestor that
       still has siblings below it, blanks for the ones that do not. Without it
       a deep child floats free and you cannot trace which branch it hangs off.
       The tee and corner forms then mark whether this node ends its own level. */
    snprintf(indent, sizeof(indent), "%s%s", prefix,
             depth > 0 ? (is_last ? "└─ " : "├─ ") : "");

    /* indent holds multibyte glyphs, so its byte length overstates the columns
       used. Every level contributes exactly three columns, so derive it from
       depth. */
    used = depth * 3;
    name_room = ui->term_width - 12 - used;
    if (name_room < 8) {
        name_room = 8;
    }

    mvwprintw(ui->content_win, (*row)++, 2, "%s%d %s",
              indent, (int)p->pid,
              truncate_to(name_buf, sizeof(name_buf), p->name, (size_t)name_room));

    /* Draw children. A child's prefix extends ours: a bar if this node still
       has siblings coming, blanks if it was the last one. */
    kids = km_process_tree_children(tree, pid, &kid_count);
    if (kids) {
        size_t child_len = prefix_len;
        size_t i;

        if (depth > 0) {
            const char *segment = is_last ? "   " : "│  ";
            size_t segment_len = strlen(segment);

            if (child_len + segment_len < KM_TREE_PREFIX_MAX) {
                memcpy(prefix + child_len, segment, segment_len);
                child_len += segment_len;
                prefix[child_len] = '\0';
            }
        }

        for (i = 0; i < kid_count; ++i) {
            draw_tree_recursive(ui, tree, kids[i], depth + 1, row, max_row,
                                i + 1 == kid_count, prefix, child_len);
        }

        /* Hand the buffer back to the caller as we found it. */
        prefix[prefix_len] = '\0';
    }
}

static void draw_process_tree(km_ui *ui, const km_process_monitor *proc) {
    km_process_tree tree;
    char prefix[KM_TREE_PREFIX_MAX];
    int row = 2;
    int max_row;
    size_t i;

    werase(ui->content_win);
    box(ui->content_win, 0, 0);

    wattron(ui->content_win, A_BOLD);
    mvwprintw(ui->content_win, 0, 2, " PROCESS TREE ");
    wattroff(ui->content_win, A_BOLD);

    if (!km_process_tree_build(proc, &tree)) {
        mvwprintw(ui->content_win, 2, 2, "Out of memory");
        return;
    }

    max_row = ui->term_height - 10;
    prefix[0] = '\0';

    for (i = 0; i < tree.root_count; ++i) {
        if (row >= max_row) {
            break;
        }
        draw_tree_recursive(ui, &tree, tree.roots[i], 0, &row, max_row,
                            i + 1 == tree.root_count, prefix, 0);
    }

    km_process_tree_destroy(&tree);
}

static void draw_events(km_ui *ui, const km_process_monitor *proc) {
    const km_event *events;
    size_t count;
    size_t i;
    int row = 2;

    werase(ui->content_win);
    box(ui->content_win, 0, 0);

    wattron(ui->content_win, A_BOLD);
    mvwprintw(ui->content_win, 0, 2, " RECENT EVENTS ");
    wattroff(ui->content_win, A_BOLD);

    events = km_process_monitor_recent_events(proc, 20, &count);

    /* Newest first. */
    for (i = count; i > 0; --i) {
        const km_event *event = &events[i - 1];
        char time_str[32];
        char name_buf[32];
        struct tm tm_buf;
        struct tm *tm_info;

        if (row >= ui->term_height - 10) {
            break;
        }

        tm_info = localtime_r(&event->timestamp, &tm_buf);
        if (tm_info) {
            strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
        } else {
            snprintf(time_str, sizeof(time_str), "--:--:--");
        }

        mvwprintw(ui->content_win, row++, 2, "%s  PID %-7d %-20s %s",
                  time_str,
                  (int)event->pid,
                  truncate_to(name_buf, sizeof(name_buf), event->process_name, 20),
                  event->description);
    }
}

static void draw_status_bar(km_ui *ui) {
    werase(ui->status_win);

    if (ui->status_message[0] == '\0') {
        return;
    }

    if (ui->status_is_error) {
        wattron(ui->status_win, COLOR_PAIR(3) | A_BOLD);
    } else {
        wattron(ui->status_win, COLOR_PAIR(1));
    }

    mvwprintw(ui->status_win, 0, 1, "%s", ui->status_message);

    if (ui->status_is_error) {
        wattroff(ui->status_win, COLOR_PAIR(3) | A_BOLD);
    } else {
        wattroff(ui->status_win, COLOR_PAIR(1));
    }
}

static void draw_help_bar(km_ui *ui) {
    const char *help_text = "";
    char buf[256];

    werase(ui->help_win);

    switch (ui->view_mode) {
        case KM_VIEW_PROCESS_LIST:
            help_text = "[↑↓] Select  [ENTER] Details  [T] Tree  [E] Events  "
                        "[K] Term  [X] Kill  [S] Stop  [C] Cont  [R] Refresh  "
                        "[+/-] Rate  [Q] Quit";
            break;
        case KM_VIEW_PROCESS_DETAIL:
            help_text = "[ESC/B] Back  [Q] Quit";
            break;
        case KM_VIEW_PROCESS_TREE:
            help_text = "[↑↓] Scroll  [B] Back  [Q] Quit";
            break;
        case KM_VIEW_EVENTS:
            help_text = "[B] Back  [Q] Quit";
            break;
    }

    mvwprintw(ui->help_win, 0, 1, "%s",
              truncate_to(buf, sizeof(buf), help_text, clamp_width(ui->term_width - 2)));
}

void km_ui_draw(km_ui *ui, const km_system_monitor *sys, const km_process_monitor *proc) {
    int new_height, new_width;

    getmaxyx(stdscr, new_height, new_width);

    if (new_height != ui->term_height || new_width != ui->term_width) {
        ui->term_height = new_height;
        ui->term_width = new_width;

        destroy_windows(ui);
        create_windows(ui);

        /* Geometry moved, so the physical screen holds text at coordinates no
           window owns any more. This is the one case that needs a full wipe. */
        clear();
        wnoutrefresh(stdscr);
    }

    draw_system_info(ui, sys);

    switch (ui->view_mode) {
        case KM_VIEW_PROCESS_LIST:
            draw_process_list(ui, proc);
            break;
        case KM_VIEW_PROCESS_DETAIL:
            draw_process_detail(ui, proc);
            break;
        case KM_VIEW_PROCESS_TREE:
            draw_process_tree(ui, proc);
            break;
        case KM_VIEW_EVENTS:
            draw_events(ui, proc);
            break;
    }

    draw_status_bar(ui);
    draw_help_bar(ui);

    /* Stage all four windows, then push a single update. ncurses diffs its
       virtual screen against the physical one and rewrites only the cells that
       changed, so a tick repaints the numbers that moved and leaves the labels,
       borders and unchanged rows untouched. Two things would defeat that: a
       clear() above (it sets clearok, forcing a full repaint) and a refresh()
       on stdscr here, which would paint stdscr's blank contents back over the
       windows we just drew. Neither belongs in this path. */
    wnoutrefresh(ui->system_win);
    wnoutrefresh(ui->content_win);
    wnoutrefresh(ui->status_win);
    wnoutrefresh(ui->help_win);
    doupdate();
}

int km_ui_get_input(void) {
    return getch();
}

void km_ui_set_view_mode(km_ui *ui, km_view_mode mode) {
    ui->view_mode = mode;
    ui->scroll_offset = 0;
}

km_view_mode km_ui_get_view_mode(const km_ui *ui) {
    return ui->view_mode;
}

void km_ui_select_next(km_ui *ui) {
    if (ui->displayed_count > 0 && ui->selected_index < ui->displayed_count - 1) {
        ui->selected_index++;
    }
}

void km_ui_select_previous(km_ui *ui) {
    if (ui->selected_index > 0) {
        ui->selected_index--;
    }
}

pid_t km_ui_selected_pid(const km_ui *ui) {
    if (ui->displayed_count == 0 || ui->selected_index >= ui->displayed_count) {
        return -1;
    }
    return ui->displayed_pids[ui->selected_index];
}

void km_ui_scroll_up(km_ui *ui) {
    km_ui_select_previous(ui);
}

void km_ui_scroll_down(km_ui *ui) {
    km_ui_select_next(ui);
}

void km_ui_page_up(km_ui *ui) {
    if (ui->selected_index >= 10) {
        ui->selected_index -= 10;
    } else {
        ui->selected_index = 0;
    }
}

void km_ui_page_down(km_ui *ui) {
    if (ui->displayed_count > 0) {
        ui->selected_index = size_min(ui->selected_index + 10, ui->displayed_count - 1);
    }
}

void km_ui_set_status(km_ui *ui, const char *message, bool is_error) {
    snprintf(ui->status_message, sizeof(ui->status_message), "%s", message);
    ui->status_is_error = is_error;
}

bool km_ui_confirm(km_ui *ui, const char *message) {
    int confirm_height = 7;
    int confirm_width = ui->term_width - 4 < 60 ? ui->term_width - 4 : 60;
    int start_y;
    int start_x;
    WINDOW *confirm_win;
    char buf[128];
    int ch;

    /* Nowhere to put a dialog on a terminal this small. Treat it as declined
       rather than handing newwin a degenerate size. */
    if (confirm_width < 20 || ui->term_height < confirm_height + 2) {
        return false;
    }

    start_y = (ui->term_height - confirm_height) / 2;
    start_x = (ui->term_width - confirm_width) / 2;

    confirm_win = newwin(confirm_height, confirm_width, start_y, start_x);
    if (!confirm_win) {
        return false;
    }

    box(confirm_win, 0, 0);

    wattron(confirm_win, A_BOLD | COLOR_PAIR(2));
    mvwprintw(confirm_win, 0, 2, " CONFIRMATION ");
    wattroff(confirm_win, A_BOLD | COLOR_PAIR(2));

    mvwprintw(confirm_win, 2, 2, "%s",
              truncate_to(buf, sizeof(buf), message, clamp_width(confirm_width - 4)));

    mvwprintw(confirm_win, 4, 2, "Press [Y] to confirm, [N] to cancel");

    wrefresh(confirm_win);

    /* Wait indefinitely: a destructive action should not time out into a
       default. */
    timeout(-1);
    ch = getch();
    timeout(KM_INPUT_TIMEOUT_MS);

    delwin(confirm_win);

    /* The dialog overwrote cells the four windows still believe they own, so
       mark them dirty. The next draw() repaints over where it was. */
    if (ui->system_win) touchwin(ui->system_win);
    if (ui->content_win) touchwin(ui->content_win);
    if (ui->status_win) touchwin(ui->status_win);
    if (ui->help_win) touchwin(ui->help_win);

    return ch == 'y' || ch == 'Y';
}
