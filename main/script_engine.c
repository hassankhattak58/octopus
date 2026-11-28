/* SPDX-License-Identifier: MIT */
#include "script_engine.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static volatile bool s_cancel;
static volatile bool s_running;
void octopus_engine_cancel(void) { s_cancel = true; }
void octopus_engine_reset_cancel(void) { s_cancel = false; }

static bool wait_ms(unsigned milliseconds)
{
    while (milliseconds && !s_cancel) {
        unsigned slice = milliseconds > 10 ? 10 : milliseconds;
        vTaskDelay(pdMS_TO_TICKS(slice) ? pdMS_TO_TICKS(slice) : 1);
        milliseconds -= slice;
    }
    return !s_cancel;
}

static bool send_key(const octopus_keyboard_transport_t *transport, uint8_t modifier, uint8_t key)
{
    if (s_cancel || !transport->ready() || !transport->report(modifier, key)) return false;
    bool completed = wait_ms(35);
    transport->release_all();
    return completed && wait_ms(35) && transport->ready();
}

bool octopus_engine_execute(const octopus_script_t *script, const octopus_keyboard_transport_t *transport)
{
    if (s_running || !script || !transport || !transport->ready()) return false;
    s_running = true;
    bool success = true;
    for (size_t i = 0; i < script->action_count && success && !s_cancel; ++i) {
        const octopus_action_t *action = &script->actions[i];
        if (action->type == OCTOPUS_ACT_DELAY) success = wait_ms(action->delay_ms);
        else if (action->type == OCTOPUS_ACT_KEY) success = send_key(transport, action->modifier, action->key);
        else {
            for (size_t j = 0; j < action->text_length && success; ++j) {
                uint8_t modifier, key;
                success = octopus_ascii_to_hid(script->text[action->text_offset + j], &modifier, &key) && send_key(transport, modifier, key);
            }
            if (success && action->newline) success = send_key(transport, 0, 40);
        }
        if (success && action->type != OCTOPUS_ACT_DELAY) success = wait_ms(action->delay_ms);
        if (!success) ESP_LOGW("OCTOPUS_ENGINE", "Stopped at line %u", action->line);
    }
    transport->release_all();
    s_running = false;
    return success && !s_cancel;
}
