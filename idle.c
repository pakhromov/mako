#include <stdio.h>
#include <string.h>
#include <wayland-client.h>

#include "dbus.h"
#include "event-loop.h"
#include "idle.h"
#include "mako.h"

// How long to linger after the last notification goes away. A short grace
// period keeps a burst of notifications inside a single process instead of
// paying for a fresh D-Bus activation between each one.
#define IDLE_TIMEOUT_MS 1000

static bool try_shutdown(struct mako_state *state) {
	// Give up the well-known name first. Once it's unowned the bus routes new
	// calls to a freshly activated instance rather than to a process that is
	// on its way out.
	int ret = release_service_name(state);
	if (ret < 0) {
		fprintf(stderr, "failed to release service name: %s\n",
			strerror(-ret));
		return false;
	}

	// The bus may have handed us messages before the release took effect.
	// Drain them so that nothing gets dropped in the gap.
	do {
		ret = sd_bus_process(state->bus, NULL);
	} while (ret > 0);
	if (ret < 0) {
		fprintf(stderr, "failed to process D-Bus: %s\n", strerror(-ret));
		// The bus is unusable, there's nothing left for us to serve.
		return true;
	}

	if (wl_list_empty(&state->notifications)) {
		return true;
	}

	// A notification slipped in under the wire. Take the name back and carry
	// on; if another instance beat us to it, it will serve the next one.
	ret = request_service_name(state);
	if (ret < 0) {
		fprintf(stderr, "failed to re-acquire service name: %s\n",
			strerror(-ret));
	}
	return false;
}

static void handle_idle_timer(void *data) {
	struct mako_state *state = data;
	state->idle_timer = NULL;

	if (!wl_list_empty(&state->notifications)) {
		return;
	}

	if (try_shutdown(state)) {
		state->event_loop.running = false;
	} else {
		update_idle_timer(state);
	}
}

void update_idle_timer(struct mako_state *state) {
	if (!state->event_loop.running) {
		// Already shutting down.
		return;
	}

	if (!wl_list_empty(&state->notifications)) {
		destroy_timer(state->idle_timer);
		state->idle_timer = NULL;
		return;
	}

	if (state->idle_timer != NULL) {
		return;
	}

	state->idle_timer = add_event_loop_timer(&state->event_loop,
		IDLE_TIMEOUT_MS, handle_idle_timer, state);
}
