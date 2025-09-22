#ifndef APOLLO_TEXT_EDITOR_H
#define APOLLO_TEXT_EDITOR_H

#include <stdint.h>
#include <stdbool.h>

#define TEXT_EDITOR_MAX_LINES 100
#define TEXT_EDITOR_MAX_LINE_LENGTH 80
#define TEXT_EDITOR_MAX_FILENAME 32

void text_editor_initialize(void);

void text_editor_run(const char* filename);

bool text_editor_load_file(const char* filename);
bool text_editor_save_file(const char* filename);
void text_editor_new_file(void);

bool text_editor_is_active(void);
bool text_editor_has_unsaved_changes(void);

#endif