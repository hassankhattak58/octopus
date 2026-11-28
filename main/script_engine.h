/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include "ducky_script.h"
#include "keyboard_transport.h"
void octopus_engine_cancel(void);
void octopus_engine_reset_cancel(void);
bool octopus_engine_execute(const octopus_script_t *script, const octopus_keyboard_transport_t *transport);
