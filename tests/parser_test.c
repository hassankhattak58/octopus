/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ducky_script.h"

static octopus_script_t script;
static octopus_script_error_t error;

static bool parse(const char *text)
{
    return octopus_script_parse(&script, text, strlen(text), &error);
}

int main(void)
{
    assert(parse("REM Demo\r\nDEFAULT_DELAY 25\r\nGUI r\r\nDELAY 500\r\nSTRINGLN Hello, Octupus!\r\n"));
    assert(script.action_count == 3);
    assert(script.actions[0].modifier == 8 && script.actions[0].key == 21);
    assert(script.actions[1].type == OCTOPUS_ACT_DELAY && script.actions[1].delay_ms == 500);
    assert(script.actions[2].newline);
    assert(parse("CTRL ALT DELETE\nTAB\nSPACE\nBACKSPACE\nESC\nUP\nDOWN\nLEFT\nRIGHT\n"));
    assert(script.action_count == 9);
    assert(script.actions[0].modifier == 5 && script.actions[0].key == 76);
    assert(!parse("DELAY nope\n") && error.line == 1);
    assert(!parse("STRING non-ASCII: \xC3\xA9\n") && error.line == 1);
    assert(!parse("GUI\n") && error.line == 1);
    assert(!parse("DELAY 10001\n") && error.line == 1);
    assert(!parse("DELAY 10000\nDELAY 10000\nDELAY 10000\nDELAY 10000\nDELAY 10000\nDELAY 10000\nDELAY 1\n") && error.line == 7);
    uint8_t mod, key;
    assert(octopus_ascii_to_hid('!', &mod, &key) && mod == 2 && key == 30);
    puts("DuckyScript parser tests passed");
    return 0;
}
