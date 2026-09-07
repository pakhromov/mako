#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "surface.h"
#include "dbus.h"
#include "mako.h"
#include "notification.h"
#include "wayland.h"

static const char *service_path = "/fr/emersion/Mako";
static const char *service_interface = "fr.emersion.Mako";

static int handle_dismiss(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {
	struct mako_state *state = data;

	uint32_t id = 0;
	int group = 0;
	int all = 0;

	int ret = sd_bus_message_enter_container(msg, 'a', "{sv}");
	if (ret < 0) {
		return ret;
	}

	while (true) {
		ret = sd_bus_message_enter_container(msg, 'e', "sv");
		if (ret < 0) {
			return ret;
		} else if (ret == 0) {
			break;
		}

		const char *key = NULL;
		ret = sd_bus_message_read(msg, "s", &key);
		if (ret < 0) {
			return ret;
		}

		if (strcmp(key, "id") == 0) {
			ret = sd_bus_message_read(msg, "v", "u", &id);
		} else if (strcmp(key, "group") == 0) {
			ret = sd_bus_message_read(msg, "v", "b", &group);
		} else if (strcmp(key, "all") == 0) {
			ret = sd_bus_message_read(msg, "v", "b", &all);
		} else {
			ret = sd_bus_message_skip(msg, "v");
		}
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_exit_container(msg);
		if (ret < 0) {
			return ret;
		}
	}

	// These don't make sense together
	if (all && group) {
		return -EINVAL;
	} else if ((all || group) && id != 0) {
		return -EINVAL;
	}

	struct mako_notification *notif;
	wl_list_for_each(notif, &state->notifications, link) {
		if (notif->id == id || id == 0) {
			struct mako_surface *surface = notif->surface;
			if (group) {
				close_group_notifications(notif, MAKO_NOTIFICATION_CLOSE_DISMISSED);
			} else if (all) {
				close_all_notifications(state, MAKO_NOTIFICATION_CLOSE_DISMISSED);
			} else {
				close_notification(notif, MAKO_NOTIFICATION_CLOSE_DISMISSED);
			}
			set_dirty(surface);
			break;
		}
	}

	return sd_bus_reply_method_return(msg, "");
}

static int handle_invoke_action(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {
	struct mako_state *state = data;

	uint32_t id = 0;
	const char *action_key;
	int ret = sd_bus_message_read(msg, "us", &id, &action_key);
	if (ret < 0) {
		return ret;
	}

	struct mako_notification *notif;
	wl_list_for_each(notif, &state->notifications, link) {
		if (notif->id == id || id == 0) {
			struct mako_action *action;
			wl_list_for_each(action, &notif->actions, link) {
				if (strcmp(action->key, action_key) == 0) {
					notify_action_invoked(action, NULL);
					break;
				}
			}
			break;
		}
	}

	return sd_bus_reply_method_return(msg, "");
}

static int handle_list_for_each(sd_bus_message *reply, struct wl_list *list) {

	int ret = sd_bus_message_open_container(reply, 'a', "a{sv}");
	if (ret < 0) {
		return ret;
	}

	struct mako_notification *notif;
	wl_list_for_each(notif, list, link) {
		int ret = sd_bus_message_open_container(reply, 'a', "{sv}");
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append(reply, "{sv}", "app-name",
			"s", notif->app_name);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append(reply, "{sv}", "app-icon",
			"s", notif->app_icon);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append(reply, "{sv}", "category",
			"s", notif->category);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append(reply, "{sv}", "desktop-entry",
			"s", notif->desktop_entry);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append(reply, "{sv}", "summary",
			"s", notif->summary);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append(reply, "{sv}", "body",
			"s", notif->body);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append(reply, "{sv}", "id",
			"u", notif->id);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append(reply, "{sv}", "urgency",
			"y", notif->urgency);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_open_container(reply, 'e', "sv");
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_append_basic(reply, 's', "actions");
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_open_container(reply, 'v', "a{ss}");
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_open_container(reply, 'a', "{ss}");
		if (ret < 0) {
			return ret;
		}

		struct mako_action *action;
		wl_list_for_each(action, &notif->actions, link) {
			ret = sd_bus_message_append(reply, "{ss}", action->key, action->title);
			if (ret < 0) {
				return ret;
			}
		}

		ret = sd_bus_message_close_container(reply);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_close_container(reply);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_close_container(reply);
		if (ret < 0) {
			return ret;
		}

		ret = sd_bus_message_close_container(reply);
		if (ret < 0) {
			return ret;
		}
	}

	ret = sd_bus_message_close_container(reply);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int handle_list(sd_bus_message *msg, struct wl_list *list) {
	sd_bus_message *reply = NULL;
	int ret = sd_bus_message_new_method_return(msg, &reply);
	if (ret < 0) {
		return ret;
	}

	ret = handle_list_for_each(reply, list);
	if (ret < 0) {
		return ret;
	}

	ret = sd_bus_send(NULL, reply, NULL);
	if (ret < 0) {
		return ret;
	}

	sd_bus_message_unref(reply);
	return 0;
}

static int handle_list_notifications(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {
	struct mako_state *state = data;
	return handle_list(msg, &state->notifications);
}

static int get_notifications(sd_bus *bus, const char *path,
		     const char *interface, const char *property,
		     sd_bus_message *reply, void *data,
		     sd_bus_error *ret_error) {
	struct mako_state *state = data;

	int ret = handle_list_for_each(reply, &state->notifications);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

void emit_notifications_changed(struct mako_state *state) {
	sd_bus_emit_properties_changed(state->bus, service_path, service_interface, "Notifications", NULL);
}

static const sd_bus_vtable service_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("DismissNotifications", "a{sv}", "", handle_dismiss, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("InvokeAction", "us", "", handle_invoke_action, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("ListNotifications", "", "aa{sv}", handle_list_notifications, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_PROPERTY("Notifications", "aa{sv}", get_notifications, 0, SD_BUS_VTABLE_PROPERTY_EMITS_INVALIDATION),
	SD_BUS_VTABLE_END
};

int init_dbus_mako(struct mako_state *state) {
	return sd_bus_add_object_vtable(state->bus, &state->mako_slot, service_path,
		service_interface, service_vtable, state);
}
