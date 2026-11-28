/* SPDX-License-Identifier: MIT */
#include "device_status.h"
#include "esp_log.h"
static volatile octopus_state_t s_state;
void octopus_status_set(octopus_state_t state, const char *detail)
{
    s_state = state;
    ESP_LOGI("OCTOPUS_STATUS", "state=%d %s", state, detail ? detail : "");
}
octopus_state_t octopus_status_get(void) { return s_state; }
