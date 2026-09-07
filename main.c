#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "dbus.h"
#include "idle.h"
#include "mako.h"
#include "notification.h"
#include "render.h"
#include "surface.h"
#include "wayland.h"

static const char usage[] =
	"Usage: mako [options...]\n"
	"\n"
	"  -h, --help                          Show help message and quit.\n"
	"  -c, --config <path>                 Path to config file.\n"
	"      --font <font>                   Font family and size.\n"
	"      --background-color <color>      Background color.\n"
	"      --text-color <color>            Text color.\n"
	"      --width <px>                    Notification width.\n"
	"      --height <px>                   Max notification height.\n"
	"      --outer-margin <px>[,<px>...]   Outer margin values, comma separated.\n"
	"                                      Up to four values, with the same\n"
	"                                      meaning as in CSS.\n"
	"      --margin <px>[,<px>...]         Margin values, comma separated.\n"
	"                                      Up to four values, with the same\n"
	"                                      meaning as in CSS.\n"
	"      --padding <px>[,<px>...]        Padding values, comma separated.\n"
	"                                      Up to four values, with the same\n"
	"                                      meaning as in CSS.\n"
	"      --border-size <px>              Border size.\n"
	"      --border-color <color>          Border color.\n"
	"      --border-radius <px>[,<px>...]  Corner radius, comma separated.\n"
	"                                      Up to four values, with the same\n"
	"                                      meaning as in CSS.\n"
	"      --progress-color <color>        Progress indicator color.\n"
	"      --icons <0|1>                   Show icons in notifications.\n"
	"      --icon-location <position>      Position of the icon relative to the\n"
	"                                      text.\n"
	"      --icon-path <path>[:<path>...]  Icon search path, colon delimited.\n"
	"      --max-icon-size <px>            Set max size of icons.\n"
	"      --icon-border-radius <px>       Icon's corner radius.\n"
	"      --markup <0|1>                  Enable/disable markup.\n"
	"      --actions <0|1>                 Enable/disable application action\n"
	"                                      execution.\n"
	"      --format <format>               Format string.\n"
	"      --max-visible <n>               Max number of visible notifications.\n"
	"      --sort <sort_criteria>          Sorts incoming notifications by time\n"
	"                                      and/or priority in ascending(+) or\n"
	"                                      descending(-) order.\n"
	"      --group-by <field>[,...]        Group notifications sharing these\n"
	"                                      criteria fields.\n"
	"      --default-timeout <timeout>     Default timeout in milliseconds.\n"
	"      --ignore-timeout <0|1>          Enable/disable notification timeout.\n"
	"      --ignore-replace <0|1>          Ignore requests to replace a\n"
	"                                      notification.\n"
	"      --ignore-close <0|1>            Ignore requests to close a notification.\n"
	"      --output <name>                 Show notifications on this output.\n"
	"      --layer <layer>                 Arrange notifications at this layer.\n"
	"      --anchor <position>             Position on output to put notifications.\n"
	"      --on-button-left <action>       Action on left pointer button.\n"
	"      --on-button-middle <action>     Action on middle pointer button.\n"
	"      --on-button-right <action>      Action on right pointer button.\n"
	"      --on-touch <action>             Action on touch.\n"
	"\n"
	"Colors can be specified with the format #RRGGBB or #RRGGBBAA.\n";

static bool init(struct mako_state *state) {
	if (!init_dbus(state)) {
		return false;
	}
	if (!init_wayland(state)) {
		finish_dbus(state);
		return false;
	}
	if (!init_event_loop(&state->event_loop, state->bus, state->display)) {
		finish_dbus(state);
		finish_wayland(state);
		return false;
	}
	wl_list_init(&state->notifications);
	return true;
}

static void finish(struct mako_state *state) {
	struct mako_notification *notif, *tmp;
	wl_list_for_each_safe(notif, tmp, &state->notifications, link) {
		destroy_notification(notif);
	}

	struct mako_surface *surface, *stmp;
	wl_list_for_each_safe(surface, stmp, &state->surfaces, link) {
		destroy_surface(surface);
	}
	finish_event_loop(&state->event_loop);
	finish_wayland(state);
	finish_dbus(state);
}

int main(int argc, char *argv[]) {
	struct mako_state state = {0};

	wl_list_init(&state.surfaces);

	init_default_config(&state.config);
	int ret = load_config(&state.config, argc, argv);

	if (ret < 0) {
		finish_config(&state.config);
		return EXIT_FAILURE;
	} else if (ret > 0) {
		finish_config(&state.config);
		printf("%s", usage);
		return EXIT_SUCCESS;
	}

	if (!init(&state)) {
		finish_config(&state.config);
		return EXIT_FAILURE;
	}

	// Nothing to show yet: start the clock, so that a process nobody ends up
	// talking to doesn't stay around.
	update_idle_timer(&state);

	ret = run_event_loop(&state.event_loop);

	finish(&state);
	finish_config(&state.config);

	return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
