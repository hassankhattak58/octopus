set(srcs "esp_hid_device_main.c"
         "esp_hid_gap.c"
         "usb_hid.c"
         "ducky_script.c"
         "script_engine.c"
         "script_storage.c"
         "button_controller.c"
         "device_status.c")
set(include_dirs ".")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES esp_hid
                       PRIV_REQUIRES nvs_flash esp_driver_gpio esp_tinyusb littlefs)
