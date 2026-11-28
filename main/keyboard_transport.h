/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char *name;
    bool (*ready)(void);
    bool (*report)(uint8_t modifier, uint8_t key);
    void (*release_all)(void);
} octopus_keyboard_transport_t;
