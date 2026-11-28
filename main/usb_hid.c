/*
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
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

static bool usb_report(uint8_t modifier, uint8_t key)
{
    uint8_t keys[6] = {key};
    return octopus_usb_hid_ready() && tud_hid_keyboard_report(0, modifier, key ? keys : NULL);
}

static void usb_release_all(void)
{
    if (octopus_usb_hid_ready()) tud_hid_keyboard_report(0, 0, NULL);
}

const octopus_keyboard_transport_t *octopus_usb_transport(void)
{
    static const octopus_keyboard_transport_t transport = {"USB", octopus_usb_hid_ready, usb_report, usb_release_all};
    return &transport;
}

void octopus_usb_hid_init(void)
{
    tinyusb_config_t config = TINYUSB_DEFAULT_CONFIG();
    config.descriptor.full_speed_config = configuration_descriptor;
    config.descriptor.string = string_descriptor;
    config.descriptor.string_count = sizeof(string_descriptor) / sizeof(string_descriptor[0]);
    ESP_ERROR_CHECK(tinyusb_driver_install(&config));
    ESP_LOGI(TAG, "USB HID keyboard ready for native USB host");
}
