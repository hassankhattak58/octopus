/* SPDX-License-Identifier: MIT */
#include "button_controller.h"
#include "ble_hid.h"
#include "device_status.h"
#include "script_engine.h"
#include "script_storage.h"
#include "usb_hid.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "OCTOPUS_BUTTON";
static octopus_script_catalog_t s_catalog;
static octopus_script_t s_script;
static TaskHandle_t s_execution_task;
static bool s_user_selected;

static const octopus_keyboard_transport_t *active_transport(void)
{
    if (octopus_usb_transport()->ready()) return octopus_usb_transport();
    if (octopus_ble_transport()->ready()) return octopus_ble_transport();
    return NULL;
}

static void execute_task(void *argument)
{
    const octopus_keyboard_transport_t *transport = argument;
    const char *name = s_catalog.names[s_catalog.selected];
    octopus_script_error_t error = {0};
    octopus_status_set(OCTOPUS_VALIDATING, name);
    bool valid = octopus_storage_load(name, &s_script, &error);
    if (!valid) {
        ESP_LOGE(TAG, "%s line %u: %s", name, error.line, error.message);
        octopus_status_set(OCTOPUS_ERROR, error.message);
    } else if (!transport->ready()) {
        octopus_status_set(OCTOPUS_ERROR, "Transport disconnected");
    } else {
        ESP_LOGI(TAG, "Running %s over %s (%u actions)", name, transport->name, (unsigned)s_script.action_count);
        octopus_status_set(OCTOPUS_RUNNING, name);
        bool completed = octopus_engine_execute(&s_script, transport);
        octopus_status_set(completed ? OCTOPUS_IDLE : OCTOPUS_CANCELLED,
                           completed ? "Completed" : "Cancelled or transport disconnected");
    }
    transport->release_all();
    s_execution_task = NULL;
    vTaskDelete(NULL);
}

static void start_execution(void)
{
    if (s_execution_task || !s_catalog.count) return;
    const octopus_keyboard_transport_t *transport = active_transport();
    if (!transport) { ESP_LOGW(TAG, "No USB or BLE keyboard host connected"); return; }
    if (!s_user_selected && transport == octopus_ble_transport()) {
        for (size_t i = 0; i < s_catalog.count; ++i) {
            if (!strcmp(s_catalog.names[i], "hello.txt")) { s_catalog.selected = i; break; }
        }
    }
    octopus_engine_reset_cancel();
    if (xTaskCreate(execute_task, "script_exec", 6144, (void *)transport, 5, &s_execution_task) != pdPASS) {
        octopus_status_set(OCTOPUS_ERROR, "Cannot create execution task");
    }
}

static void button_task(void *argument)
{
    (void)argument;
    const gpio_num_t pin = (gpio_num_t)CONFIG_OCTOPUS_TRIGGER_GPIO;
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    bool was_pressed = gpio_get_level(pin) == 0;
    bool pressed_during_run = false;
    TickType_t down_at = 0;
    while (true) {
        bool pressed = gpio_get_level(pin) == 0;
        if (pressed && !was_pressed) {
            vTaskDelay(pdMS_TO_TICKS(25));
            pressed = gpio_get_level(pin) == 0;
            if (pressed) {
                down_at = xTaskGetTickCount();
                pressed_during_run = s_execution_task != NULL;
                if (pressed_during_run) {
                    octopus_engine_cancel();
                    ESP_LOGW(TAG, "Physical cancellation requested");
                }
            }
        } else if (!pressed && was_pressed && !pressed_during_run) {
            uint32_t held_ms = pdTICKS_TO_MS(xTaskGetTickCount() - down_at);
            if (held_ms >= 800) start_execution();
            else if (held_ms >= 25 && !s_execution_task) {
                octopus_storage_select_next(&s_catalog);
                s_user_selected = true;
            }
        }
        was_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void octopus_button_controller_init(void)
{
    octopus_status_set(OCTOPUS_IDLE, "Initializing");
    if (!octopus_storage_init(&s_catalog)) octopus_status_set(OCTOPUS_ERROR, "No scripts available");
    ESP_LOGI(TAG, "Short press selects; hold 800 ms and release executes; press while running cancels");
    BaseType_t created = xTaskCreate(button_task, "button", 3072, NULL, 6, NULL);
    ESP_ERROR_CHECK(created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}
