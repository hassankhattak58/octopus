#pragma once

#define OCTOPUS_RUN_DIALOG_DELAY_MS 800

static const octopus_demo_action_t octopus_demo_actions[] = {
    { OCTOPUS_ACTION_STRING, 0, "Hello Octupus" },
};
#define OCTOPUS_DEMO_ACTION_COUNT (sizeof(octopus_demo_actions) / sizeof(octopus_demo_actions[0]))
