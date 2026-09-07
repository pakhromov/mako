# mako-daemonless

This is a fork of [upstream mako][upstream], a lightweight notification daemon for Wayland.

Upstream mako already supports the D-Bus activation for the first received notification, in other words you do not have to have it running in the background in order to receive the first notification. However after showing the first notification, mako keeps running in the background and serving any future notifications, just like any other notification daemon does - fnott, swaync, etc.

This fork changes that behavior by making mako exit after all of the active notifications have been dismissed, expired, or acted upon. Every new notification after that goes through the same way as the very first one - D-Bus activates mako each time (unless mako already has an active notification showing when a new one comes - in this case the existing instance will be reused). This can be desirable for those minimalists looking for the absolute minimum background processes running, while still having notifications working.

The downside is no notification history can be saved, as the upstream keeps it in memory of the running daemon. Multiple notifications can still be shown at the same time, but nothing is remembered after they are dismissed.

Additionally, 2 new options are added to this fork which allow ignoring requests from the applications to close or replace their previous notifications.

<p align="center">
  <img src="https://github.com/user-attachments/assets/25582bd6-bd3b-4bb3-b248-87fa7f88e967" alt="mako screenshot">
</p>

mako implements the [FreeDesktop Notifications Specification][spec].

## Differences from upstream

### Added options

- `ignore-replace=0|1` — ignore requests from applications to replace an
  existing notification. The replacement is shown as a new, separate
  notification instead. Useful for apps like Discord that only ever show the
  latest message by replacing the previous one.
- `ignore-close=0|1` — ignore requests from applications to close a
  notification over D-Bus. Some apps replace their notifications by closing the
  previous one rather than by using replaces_id; this covers that case.

Both are style options, so they can be set globally or narrowed to a criteria section:

```
ignore-replace=1
ignore-close=1

[app-name=osd]
ignore-replace=0
ignore-close=0
```

### Removed

Notification history is gone entirely, along with everything that only existed to support it or that cannot outlive a single notification:

| Removed | Was |
|---|---|
| max-history, history | history buffer options |
| dismiss --no-history | binding action; use dismiss |
| makoctl restore, makoctl history | history commands |
| mode criteria, makoctl mode | modes cannot outlive the process |
| makoctl reload | the config is re-read on every start |
| invisible | only the first notification of a group is drawn |

Removed options are a hard parse error, so they must be deleted from an existing config or mako will refuse to start.

`makoctl` no longer starts mako. When nothing is running there is nothing to act on, so it exits silently with status 0 instead of activating a daemon.

### Running

`mako` will run automatically when a notification is emitted. This happens via D-Bus activation, and it is the only supported way to start it: mako shuts itself down once no notification is left on screen, so starting it by hand or from a service manager just gives you a process that exits a second later. You should remove mako from your compositor autostart options when using this fork. If mako is not starting on the first emitted notification, try running:

```shell
dbus-update-activation-environment WAYLAND_DISPLAY XDG_CURRENT_DESKTOP XDG_SESSION_TYPE XDG_RUNTIME_DIR XDG_SESSION_DESKTOP
```

If another notification daemon activates instead of mako, you can create a user D-Bus override which takes precedence. D-Bus reads `~/.local/share/dbus-1/services` before `/usr/share/dbus-1/services`, so a service file there wins regardless of what else is installed:

```shell
mkdir -p ~/.local/share/dbus-1/services
cp build/fr.emersion.mako.service ~/.local/share/dbus-1/services/
```
Or create ~/.local/share/dbus-1/services/mako.service by hand:

```ini
[D-BUS Service]
Name=org.freedesktop.Notifications
Exec=/absolute/path/to/mako
```

## Configuration

`mako` can be extensively configured and customized - feel free to read more using the command `man 5 mako`

For control of mako during runtime, `makoctl` can be used; see `man makoctl`

## Building

Install dependencies:

* meson (build-time dependency)
* wayland
* pango
* cairo
* systemd, elogind or [basu] (for the sd-bus library)
* gdk-pixbuf (optional, for icons support)
* dbus (runtime dependency, user-session support is required)
* scdoc (optional, for man pages)

Then run:

```shell
meson setup build
ninja -C build
```

<p align="center">
  <img src="https://github.com/user-attachments/assets/4b32fef6-61d9-4ad1-8820-d4e5a245a76c" width="512" alt="mako">
</p>

## I have a question!

See the [faq section in the wiki](https://github.com/emersion/mako/wiki/Frequently-asked-questions).

## License

MIT

[irc]: https://web.libera.chat/gamja/#emersion
[upstream]: https://github.com/emersion/mako
[spec]: https://specifications.freedesktop.org/notification-spec/latest/
[basu]: https://github.com/emersion/basu
