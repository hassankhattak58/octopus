/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "ducky_script.h"

#define OCTOPUS_MAX_SCRIPTS 16
#define OCTOPUS_SCRIPT_NAME_BYTES 48

typedef struct {
    char names[OCTOPUS_MAX_SCRIPTS][OCTOPUS_SCRIPT_NAME_BYTES];
    size_t count;
    size_t selected;
} octopus_script_catalog_t;

bool octopus_storage_init(octopus_script_catalog_t *catalog);
bool octopus_storage_load(const char *name, octopus_script_t *script, octopus_script_error_t *error);
void octopus_storage_select_next(octopus_script_catalog_t *catalog);
