/* SPDX-License-Identifier: MIT */
#include "ducky_script.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define SHIFT 0x02

static bool fail(octopus_script_error_t *error, unsigned line, const char *message)
{
    if (error) {
        error->line = line;
        snprintf(error->message, sizeof(error->message), "%s", message);
    }
    return false;
}

bool octopus_ascii_to_hid(char ch, uint8_t *modifier, uint8_t *key)
{
    *modifier = 0;
    if (ch >= 'a' && ch <= 'z') { *key = 4 + ch - 'a'; return true; }
    if (ch >= 'A' && ch <= 'Z') { *modifier = SHIFT; *key = 4 + ch - 'A'; return true; }
    if (ch >= '1' && ch <= '9') { *key = 30 + ch - '1'; return true; }
    if (ch == '0') { *key = 39; return true; }
    static const char plain[] = " \n\t-=[]\\;',./`";
    static const uint8_t plain_keys[] = {44, 40, 43, 45, 46, 47, 48, 49, 51, 52, 54, 55, 56, 53};
    static const char shifted[] = "!@#$%^&*()_+{}|:\"<>?~";
    static const uint8_t shifted_keys[] = {30,31,32,33,34,35,36,37,38,39,45,46,47,48,49,51,52,54,55,56,53};
    for (size_t i = 0; i < sizeof(plain) - 1; ++i) {
        if (ch == plain[i]) { *key = plain_keys[i]; return true; }
    }
    for (size_t i = 0; i < sizeof(shifted) - 1; ++i) {
        if (ch == shifted[i]) { *modifier = SHIFT; *key = shifted_keys[i]; return true; }
    }
    return false;
}

static bool token_key(const char *token, uint8_t *key, uint8_t *modifier)
{
    struct named_key { const char *name; uint8_t code; };
    static const struct named_key names[] = {
        {"ENTER",40},{"RETURN",40},{"ESC",41},{"ESCAPE",41},{"BACKSPACE",42},
        {"TAB",43},{"SPACE",44},{"DELETE",76},{"UP",82},{"UPARROW",82},
        {"DOWN",81},{"DOWNARROW",81},{"LEFT",80},{"LEFTARROW",80},
        {"RIGHT",79},{"RIGHTARROW",79},{"HOME",74},{"END",77},
    };
    for (size_t i = 0; i < sizeof(names)/sizeof(names[0]); ++i) {
        if (strcmp(token, names[i].name) == 0) { *key = names[i].code; return true; }
    }
    if (token[0] && !token[1]) {
        char ch = token[0];
        if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
        return octopus_ascii_to_hid(ch, modifier, key);
    }
    if (token[0] == 'F' && token[1] >= '1' && token[1] <= '9' && !token[2]) {
        *key = 58 + token[1] - '1'; return true;
    }
    return false;
}

static bool parse_key_combo(char *input, octopus_action_t *action)
{
    char *cursor = input;
    bool have_key = false;
    while (*cursor) {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '+') ++cursor;
        if (!*cursor) break;
        char *end = cursor;
        while (*end && *end != ' ' && *end != '\t' && *end != '+') ++end;
        char saved = *end;
        *end = 0;
        uint8_t modifier = 0;
        if (!strcmp(cursor, "CTRL") || !strcmp(cursor, "CONTROL")) modifier = 0x01;
        else if (!strcmp(cursor, "SHIFT")) modifier = 0x02;
        else if (!strcmp(cursor, "ALT")) modifier = 0x04;
        else if (!strcmp(cursor, "GUI") || !strcmp(cursor, "WINDOWS") || !strcmp(cursor, "WIN")) modifier = 0x08;
        if (modifier) {
            if (have_key || (action->modifier & modifier)) return false;
            action->modifier |= modifier;
        } else {
            uint8_t key = 0;
            if (have_key || !token_key(cursor, &key, &modifier)) return false;
            action->key = key;
            action->modifier |= modifier;
            have_key = true;
        }
        *end = saved;
        cursor = end;
    }
    return have_key;
}

bool octopus_script_parse(octopus_script_t *script, const char *source, size_t length,
                          octopus_script_error_t *error)
{
    if (!script || !source) return fail(error, 0, "Missing script buffer");
    memset(script, 0, sizeof(*script));
    if (!length || length > OCTOPUS_MAX_SCRIPT_BYTES) return fail(error, 0, "Script size out of range");
    if (memchr(source, '\0', length)) return fail(error, 0, "NUL byte in script");
    memcpy(script->text, source, length);
    unsigned line = 0;
    unsigned default_delay = 0;
    char *cursor = script->text;
    while ((size_t)(cursor - script->text) < length) {
        ++line;
        char *end = strchr(cursor, '\n');
        if (!end) end = script->text + length;
        if (end - cursor > OCTOPUS_MAX_LINE_BYTES) return fail(error, line, "Line too long");
        bool more = *end == '\n';
        *end = 0;
        if (end > cursor && end[-1] == '\r') end[-1] = 0;
        char *command = cursor;
        while (*command == ' ' || *command == '\t') ++command;
        if (!*command) { cursor = end + more; continue; }
        char *arg = command;
        while (*arg && *arg != ' ' && *arg != '\t') ++arg;
        if (*arg) { *arg++ = 0; while (*arg == ' ' || *arg == '\t') ++arg; }
        for (char *p = command; *p; ++p) *p = (char)toupper((unsigned char)*p);
        if (!strcmp(command, "REM")) { cursor = end + more; continue; }
        if (script->action_count == OCTOPUS_MAX_ACTIONS) return fail(error, line, "Too many actions");
        octopus_action_t *action = &script->actions[script->action_count];
        action->line = line;
        if (!strcmp(command, "STRING") || !strcmp(command, "STRINGLN")) {
            action->type = OCTOPUS_ACT_TEXT;
            action->text_offset = (uint16_t)(arg - script->text);
            action->text_length = strlen(arg);
            action->newline = !strcmp(command, "STRINGLN");
            for (char *p = arg; *p; ++p) {
                uint8_t modifier, key;
                if (!octopus_ascii_to_hid(*p, &modifier, &key)) return fail(error, line, "Unsupported text character (US ASCII only)");
            }
            script->estimated_ms += (action->text_length + action->newline) * 70;
        } else if (!strcmp(command, "DELAY") || !strcmp(command, "DEFAULT_DELAY") || !strcmp(command, "DEFAULTDELAY")) {
            if (!*arg) return fail(error, line, "Missing delay");
            unsigned value = 0;
            for (char *p = arg; *p; ++p) {
                if (!isdigit((unsigned char)*p)) return fail(error, line, "Delay must be numeric");
                value = value * 10 + (*p - '0');
                if (value > 10000) return fail(error, line, "Delay exceeds 10000 ms");
            }
            if (!strcmp(command, "DELAY")) { action->type = OCTOPUS_ACT_DELAY; action->delay_ms = value; script->estimated_ms += value; }
            else { default_delay = value; cursor = end + more; continue; }
        } else {
            action->type = OCTOPUS_ACT_KEY;
            char combo[OCTOPUS_MAX_LINE_BYTES + 1];
            int written = snprintf(combo, sizeof(combo), "%s%s%s", command, *arg ? " " : "", arg);
            if (written < 0 || written >= sizeof(combo)) return fail(error, line, "Key combination too long");
            for (char *p = combo; *p; ++p) *p = (char)toupper((unsigned char)*p);
            if (!parse_key_combo(combo, action)) return fail(error, line, "Unknown command or invalid key combination");
            script->estimated_ms += 70;
        }
        action->delay_ms = action->type == OCTOPUS_ACT_DELAY ? action->delay_ms : default_delay;
        script->estimated_ms += action->type == OCTOPUS_ACT_DELAY ? 0 : default_delay;
        if (script->estimated_ms > OCTOPUS_MAX_RUNTIME_MS) return fail(error, line, "Estimated runtime exceeds 60 seconds");
        ++script->action_count;
        cursor = end + more;
    }
    if (!script->action_count) return fail(error, 0, "Script has no actions");
    return true;
}
