/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2012 François Agrech

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

*/

#include "fsm.h"
#include "trace.h"

/* action for the fsm table */
typedef void (*fsm_action) (struct status *, struct key *);

/* what to do when state changes */
struct fsm_change {
	enum key_state new_state; /* new state after the change */
	fsm_action *actions; /* actions to do to switch state (NULL terminated list) */
};

/* FSM action groups */
void fsm_error (struct status *status, struct key *key);
static fsm_action fsm_press[]={ status_press, NULL };
static fsm_action fsm_release[]={ status_release, NULL };
static fsm_action fsm_press_update[]={ status_press, status_update_key, NULL };
static fsm_action fsm_release_update[]={ status_release, status_update_key, NULL };
static fsm_action fsm_latch[]={ status_latch, status_update_view, NULL };
static fsm_action fsm_unlatch_lock[]={ status_unlatch, status_lock, status_update_view, NULL };
static fsm_action fsm_lock[]={ status_lock, status_update_view, NULL };
static fsm_action fsm_unlock[]={ status_unlock, status_update_view, NULL };
static fsm_action fsm_press_latched[]={ status_press_latched, status_press, NULL };
static fsm_action fsm_release_latched[]={ status_release, status_release_latched, status_unlatch_all, status_update_view, NULL };
static fsm_action fsm_update[]={ status_update_view, NULL };
static fsm_action fsm_update_key[]={ status_update_key, NULL };
static fsm_action fsm_display_error[]={ fsm_error, NULL };

/* Mouse FSM - This FSM is adapted for Mouse input. Simulate sticky keys.
 * changes by type, event and state */
static struct fsm_change fsm_mouse[FSM_EVENT_NUM][FSM_KEY_TYPE_NUM][KEY_STATE_NUM]={
	{ /* PRESS event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_RELEASED, fsm_press_latched }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_LATCHED, fsm_latch }, /* RELEASED state */
			{ KEY_RELEASED, fsm_unlock }, /* LOCKED state */
			{ KEY_LOCKED, fsm_unlatch_lock } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, fsm_press }, /* RELEASED state */
			{ KEY_LOCKED, fsm_press } /* LOCKED state */
		}
	}, { /* RELEASE event */
		{ /* NORMAL key */
			{ KEY_PRESSED, fsm_release_latched }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, fsm_update_key }, /* RELEASED state */
			{ KEY_LOCKED, fsm_update_key }, /* LOCKED state */
			{ KEY_LATCHED, fsm_update_key } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, fsm_release_update }, /* RELEASED state */
			{ KEY_LOCKED, fsm_release_update } /* LOCKED state */
		}
	}, { /* PRESSED event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_PRESSED, fsm_update_key } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_PRESSED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_LOCKED, fsm_lock }, /* RELEASED state */
			{ KEY_RELEASED, fsm_unlock } /* LOCKED state */
		}
	}, { /* RELEASED event */
		{ /* NORMAL key */
			{ KEY_RELEASED, fsm_update_key }, /* PRESSED state */
			{ KEY_RELEASED, NULL } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL } /* LOCKED state */
		}
	}
};

/* Touch FSM - This FSM is adapted for Touch screen input.
 * changes by type, event and state */
static struct fsm_change fsm_touch[FSM_EVENT_NUM][FSM_KEY_TYPE_NUM][KEY_STATE_NUM]={
	{ /* PRESS event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_RELEASED, fsm_press_update }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, NULL }, /* PRESSED state */
			{ KEY_LATCHED, NULL }, /* RELEASED state */
			{ KEY_RELEASED, NULL }, /* LOCKED state */
			{ KEY_LOCKED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, NULL }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL } /* LOCKED state */
		}
	}, { /* RELEASE event */
		{ /* NORMAL key */
			{ KEY_PRESSED, fsm_release_latched }, /* PRESSED state */
			{ KEY_PRESSED, fsm_press_latched }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_LATCHED, fsm_latch }, /* RELEASED state */
			{ KEY_RELEASED, fsm_unlock }, /* LOCKED state */
			{ KEY_LOCKED, fsm_unlatch_lock } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, fsm_press }, /* RELEASED state */
			{ KEY_LOCKED, fsm_press } /* LOCKED state */
		}
	}, { /* PRESSED event */
		{ /* NORMAL key */
			{ KEY_PRESSED, fsm_release_latched }, /* PRESSED state */
			{ KEY_PRESSED, NULL } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_PRESSED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, fsm_release }, /* RELEASED state */
			{ KEY_LOCKED, fsm_release } /* LOCKED state */
		}
	}, { /* RELEASED event */
		{ /* NORMAL key */
			{ KEY_RELEASED, fsm_update }, /* PRESSED state */
			{ KEY_RELEASED, fsm_display_error } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_display_error }, /* PRESSED state */
			{ KEY_LOCKED, fsm_lock }, /* RELEASED state */
			{ KEY_RELEASED, fsm_unlock } /* LOCKED state */
		}
	}
};

/* process the fsm actions and state change */
void fsm_process(struct status *status, struct key *key, enum fsm_event event)
{
	START_FUNC
	enum key_state state;
	enum status_key_type type;
	guint idx;
	struct fsm_change (*fsm)[FSM_EVENT_NUM][FSM_KEY_TYPE_NUM][KEY_STATE_NUM]=
		status_im_get(status)==STATUS_IM_TOUCH?&fsm_touch:&fsm_mouse;
	if (key) {
		state=key->state;
		type=(key_get_modifier(key)?(key_is_locker(key)?
			FSM_KEY_LOCKER:FSM_KEY_MODIFIER):FSM_KEY_NORMAL);
		/* switch state */
		key_state_set(key, (*fsm)[event][type][state].new_state);
		/* execute actions */
		status_focus_window(status);
		flo_debug(TRACE_DEBUG, _("Key %p (type %d): Event %d received :"
			" Switching from state %d to %d (fsm %d)"),
			key, type, event, state, (*fsm)[event][type][state].new_state,
			status_im_get(status));
		if ((*fsm)[event][type][state].actions)
			for (idx=0;(*fsm)[event][type][state].actions[idx];idx++)
				(*fsm)[event][type][state].actions[idx](status, key);
	}
	END_FUNC
}

/* print a fsm error */
void fsm_error (struct status *status, struct key *key)
{
	START_FUNC
	flo_error(_("FSM state errorr: state=%d"), key->state);
	END_FUNC
}

