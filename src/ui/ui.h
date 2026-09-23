#ifndef KM_UI_H
#define KM_UI_H

#include <ncurses.h>
#include <stdbool.h>
#include <stddef.h>

#include "../process/process_monitor.h"
#include "../system/system_monitor.h"

#define KM_STATUS_MSG_MAX 256

typedef enum {
    KM_VIEW_PROCESS_LIST,
    KM_VIEW_PROCESS_DETAIL,
    KM_VIEW_PROCESS_TREE,
    KM_VIEW_EVENTS
} km_view_mode;

typedef struct {
    WINDOW *main_win;
    WINDOW *system_win;
    WINDOW *content_win;
    WINDOW *status_win;
    WINDOW *help_win;

    km_view_mode view_mode;
    size_t selected_index;
    size_t scroll_offset;

    /* PIDs in the order the current view shows them, so that a selection made
       on screen maps back to a process. */
    pid_t *displayed_pids;
    size_t displayed_count;
    size_t displayed_capacity;

    /* Scratch for sorting the process list without copying the structs. */
    const km_process_info **sorted;
    size_t sorted_capacity;

    char status_message[KM_STATUS_MSG_MAX];
    bool status_is_error;

    int term_width;
    int term_height;
} km_ui;

void km_ui_init_state(km_ui *ui);
bool km_ui_initialize(km_ui *ui);
void km_ui_cleanup(km_ui *ui);

void km_ui_draw(km_ui *ui, const km_system_monitor *sys, const km_process_monitor *proc);

int km_ui_get_input(void);

void km_ui_set_view_mode(km_ui *ui, km_view_mode mode);
km_view_mode km_ui_get_view_mode(const km_ui *ui);

void km_ui_select_next(km_ui *ui);
void km_ui_select_previous(km_ui *ui);
pid_t km_ui_selected_pid(const km_ui *ui);

void km_ui_scroll_up(km_ui *ui);
void km_ui_scroll_down(km_ui *ui);
void km_ui_page_up(km_ui *ui);
void km_ui_page_down(km_ui *ui);

void km_ui_set_status(km_ui *ui, const char *message, bool is_error);

bool km_ui_confirm(km_ui *ui, const char *message);

#endif /* KM_UI_H */
