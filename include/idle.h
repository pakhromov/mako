#ifndef MAKO_IDLE_H
#define MAKO_IDLE_H

struct mako_state;

// Arm or disarm the idle-exit timer, depending on whether any notifications
// are still on screen. Call this whenever a notification is inserted or
// destroyed.
void update_idle_timer(struct mako_state *state);

#endif
