/*
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"

#include "usb_hid.h"

static const char *TAG = "OCTOPUS_USB";
static const uint8_t report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

static const char *string_descriptor[] = {
    (const char[]){0x09, 0x04},
    "Octopus",
    "Octopus USB Keyboard",
    "0001",
    "Keyboard",
};

static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(report_descriptor), 0x81, 16, 10),
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t requested_length)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)requested_length;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t length)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)length;
}

bool octopus_usb_hid_ready(void)
{
    return tud_mounted() && tud_hid_ready();
}

static bool send_key(uint8_t modifier, uint8_t key)
{
    uint8_t keys[6] = {key};
    for (int attempt = 0; attempt < 50 && !octopus_usb_hid_ready(); ++attempt) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (!octopus_usb_hid_ready()) {
        return false;
    }
    if (!tud_hid_keyboard_report(0, modifier, keys)) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(35));
    if (!tud_hid_keyboard_report(0, 0, NULL)) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(35));
    return true;
}

static bool send_char(char ch)
{
    if (ch >= 'a' && ch <= 'z') {
        return send_key(0, HID_KEY_A + (ch - 'a'));
    }
    if (ch >= 'A' && ch <= 'Z') {
        return send_key(KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_A + (ch - 'A'));
    }
    if (ch == ' ') {
        return send_key(0, HID_KEY_SPACE);
    }
    if (ch == ',') {
        return send_key(0, HID_KEY_COMMA);
    }
    if (ch == '!') {
        return send_key(KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_1);
    }
    return false;
}

static void open_notepad_and_type(void)
{
    ESP_LOGI(TAG, "Opening Notepad and typing welcome text");
    if (!send_key(KEYBOARD_MODIFIER_LEFTGUI, HID_KEY_R)) {
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(350));
    const char *command = "notepad";
    for (const char *ch = command; *ch; ++ch) {
        if (!send_char(*ch)) {
            return;
        }
    }
    if (!send_key(0, HID_KEY_ENTER)) {
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(1500));
    const char *message = "Welcome, Octupus Here!";
    for (const char *ch = message; *ch; ++ch) {
        if (!send_char(*ch)) {
            return;
        }
    }
}

static void usb_button_task(void *argument)
{
    (void)argument;
    const gpio_num_t pin = (gpio_num_t)CONFIG_OCTOPUS_TRIGGER_GPIO;
    gpio_config_t input = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&input));
    bool was_pressed = gpio_get_level(pin) == 0;
    while (true) {
        bool pressed = gpio_get_level(pin) == 0;
        if (pressed && !was_pressed && octopus_usb_hid_ready()) {
            vTaskDelay(pdMS_TO_TICKS(30));
            if (gpio_get_level(pin) == 0 && octopus_usb_hid_ready()) {
                open_notepad_and_type();
            }
        }
        was_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void octopus_usb_hid_init(void)
{
    tinyusb_config_t config = TINYUSB_DEFAULT_CONFIG();
    config.descriptor.full_speed_config = configuration_descriptor;
    config.descriptor.string = string_descriptor;
    config.descriptor.string_count = sizeof(string_descriptor) / sizeof(string_descriptor[0]);
    ESP_ERROR_CHECK(tinyusb_driver_install(&config));
    BaseType_t created = xTaskCreate(usb_button_task, "usb_button", 3072, NULL, 5, NULL);
    ESP_ERROR_CHECK(created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_LOGI(TAG, "USB HID keyboard ready for native USB host");
}
