/* SPDX-License-Identifier: MIT */
#include "script_storage.h"

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_littlefs.h"

static const char *TAG = "OCTOPUS_STORAGE";
static const char *BASE = "/scripts";

static bool script_name(const char *name)
{
    size_t length = strlen(name);
    if (length < 5 || length >= OCTOPUS_SCRIPT_NAME_BYTES || strcmp(name + length - 4, ".txt")) return false;
    for (const char *p = name; *p; ++p) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' || *p == '.')) return false;
    }
    return true;
}

bool octopus_storage_init(octopus_script_catalog_t *catalog)
{
    memset(catalog, 0, sizeof(*catalog));
    esp_vfs_littlefs_conf_t config = {
        .base_path = BASE,
        .partition_label = "scripts",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };
    esp_err_t result = esp_vfs_littlefs_register(&config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "LittleFS mount failed: %s", esp_err_to_name(result));
        return false;
    }
    DIR *directory = opendir(BASE);
    if (!directory) { ESP_LOGE(TAG, "Cannot list scripts"); return false; }
    struct dirent *entry;
    while ((entry = readdir(directory)) && catalog->count < OCTOPUS_MAX_SCRIPTS) {
        if (!script_name(entry->d_name)) continue;
        memcpy(catalog->names[catalog->count], entry->d_name, strlen(entry->d_name) + 1);
        ++catalog->count;
    }
    closedir(directory);
    for (size_t i = 0; i < catalog->count; ++i) {
        for (size_t j = i + 1; j < catalog->count; ++j) {
            if (strcmp(catalog->names[i], catalog->names[j]) > 0) {
                char swap[OCTOPUS_SCRIPT_NAME_BYTES];
                memcpy(swap, catalog->names[i], sizeof(swap));
                memcpy(catalog->names[i], catalog->names[j], sizeof(swap));
                memcpy(catalog->names[j], swap, sizeof(swap));
            }
        }
        ESP_LOGI(TAG, "Script %u: %s", (unsigned)i, catalog->names[i]);
        if (!strcmp(catalog->names[i], "notepad.txt")) catalog->selected = i;
    }
    ESP_LOGI(TAG, "Selected: %s", catalog->count ? catalog->names[catalog->selected] : "none");
    return catalog->count > 0;
}

bool octopus_storage_load(const char *name, octopus_script_t *script, octopus_script_error_t *error)
{
    if (!name || !script_name(name)) {
        if (error) { error->line = 0; snprintf(error->message, sizeof(error->message), "Invalid script name"); }
        return false;
    }
    char path[64];
    snprintf(path, sizeof(path), "%s/%s", BASE, name);
    FILE *file = fopen(path, "rb");
    if (!file) {
        if (error) { error->line = 0; snprintf(error->message, sizeof(error->message), "Script missing"); }
        return false;
    }
    char *source = malloc(OCTOPUS_MAX_SCRIPT_BYTES + 1);
    if (!source) {
        fclose(file);
        if (error) { error->line = 0; snprintf(error->message, sizeof(error->message), "Out of memory"); }
        return false;
    }
    size_t length = fread(source, 1, OCTOPUS_MAX_SCRIPT_BYTES + 1, file);
    bool read_error = ferror(file);
    fclose(file);
    if (read_error || length > OCTOPUS_MAX_SCRIPT_BYTES) {
        if (error) { error->line = 0; snprintf(error->message, sizeof(error->message), "Script read error or too large"); }
        free(source);
        return false;
    }
    bool valid = octopus_script_parse(script, source, length, error);
    free(source);
    return valid;
}

void octopus_storage_select_next(octopus_script_catalog_t *catalog)
{
    if (!catalog->count) return;
    catalog->selected = (catalog->selected + 1) % catalog->count;
    ESP_LOGI(TAG, "Selected: %s", catalog->names[catalog->selected]);
}
