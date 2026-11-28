/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OCTOPUS_MAX_SCRIPT_BYTES 8192
#define OCTOPUS_MAX_ACTIONS 128
#define OCTOPUS_MAX_RUNTIME_MS 60000
#define OCTOPUS_MAX_LINE_BYTES 256

typedef enum {
    OCTOPUS_ACT_TEXT,
    OCTOPUS_ACT_KEY,
    OCTOPUS_ACT_DELAY,
} octopus_action_type_t;

typedef struct {
    octopus_action_type_t type;
    uint16_t line;
    uint16_t delay_ms;
    uint16_t text_offset;
    uint16_t text_length;
    uint8_t modifier;
    uint8_t key;
    bool newline;
} octopus_action_t;

typedef struct {
    char text[OCTOPUS_MAX_SCRIPT_BYTES + 1];
    octopus_action_t actions[OCTOPUS_MAX_ACTIONS];
    size_t action_count;
    uint32_t estimated_ms;
} octopus_script_t;

typedef struct {
    uint16_t line;
    char message[96];
} octopus_script_error_t;

bool octopus_script_parse(octopus_script_t *script, const char *source, size_t length,
                          octopus_script_error_t *error);
bool octopus_ascii_to_hid(char ch, uint8_t *modifier, uint8_t *key);
