/* SPDX-License-Identifier: MIT */
#pragma once
typedef enum { OCTOPUS_IDLE, OCTOPUS_VALIDATING, OCTOPUS_RUNNING, OCTOPUS_CANCELLED, OCTOPUS_ERROR } octopus_state_t;
void octopus_status_set(octopus_state_t state, const char *detail);
octopus_state_t octopus_status_get(void);
