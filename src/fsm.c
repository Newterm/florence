/* 
   Florence - Florence is a simple virtual keyboard for Gnome.

   Copyright (C) 2011 François Agrech

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
typedef void (*fsm_action) (struct status *, struct key *, enum fsm_event);

/* what to do when state changes */
struct fsm_change {
	enum key_state new_state; /* new state after the change */
	fsm_action *actions; /* actions to do to switch state (NULL terminated list) */
};

/* FSM action groups */
static fsm_action fsm_action_s[]={ status_send, NULL };
static fsm_action fsm_action_la[]={ status_latch, status_update_view, NULL };
static fsm_action fsm_action_ulalo[]={ status_unlatch, status_lock, status_update_view, NULL };
static fsm_action fsm_action_lo[]={ status_lock, status_update_view, NULL };
static fsm_action fsm_action_ul[]={ status_unlock, status_update_view, NULL };
static fsm_action fsm_action_slas[]={ status_send_latched, status_send, NULL };
static fsm_action fsm_action_sslaulaa[]={ status_send, status_send_latched, status_unlatch_all, status_update_view, NULL };
static fsm_action fsm_action_uv[]={ status_update_view, NULL };
static fsm_action fsm_action_err[]={ status_error, NULL };

/* status FSM - simulate sticky keys
 * changes by type, event and state */
static struct fsm_change fsm[FSM_EVENT_NUM][FSM_KEY_TYPE_NUM][KEY_STATE_NUM]={
	{ /* PRESS event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_slas }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_LATCHED, fsm_action_la }, /* RELEASED state */
			{ KEY_RELEASED, fsm_action_ul }, /* LOCKED state */
			{ KEY_LOCKED, fsm_action_ulalo } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_s }, /* RELEASED state */
			{ KEY_LOCKED, fsm_action_s } /* LOCKED state */
		}
	}, { /* RELEASE event */
		{ /* NORMAL key */
			{ KEY_PRESSED, fsm_action_sslaulaa }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_s }, /* RELEASED state */
			{ KEY_LOCKED, fsm_action_s } /* LOCKED state */
		}
	}, { /* PRESSED event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_PRESSED, fsm_action_uv } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_PRESSED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_LOCKED, fsm_action_lo }, /* RELEASED state */
			{ KEY_RELEASED, fsm_action_ul } /* LOCKED state */
		}
	}, { /* RELEASED event */
		{ /* NORMAL key */
			{ KEY_RELEASED, fsm_action_uv }, /* PRESSED state */
			{ KEY_RELEASED, NULL } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL } /* LOCKED state */
		}
	}
};

/* process the fsm actions and state change */
void fsm_process(struct status *status, struct key *key, enum fsm_event event)
{
	enum key_state state;
	enum status_key_type type;
	guint idx;
	if (key) {
		state=key->state;
		type=(key_get_modifier(key)?(key_is_locker(key)?
			FSM_KEY_LOCKER:FSM_KEY_MODIFIER):FSM_KEY_NORMAL);
		/* switch state */
		key_state_set(key, fsm[event][type][state].new_state);
		/* execute actions */
		status_focus_window(status);
		flo_debug(_("Event %d received :type %d. Switching from state %d to %d"),
			event, type, state, fsm[event][type][state].new_state);
		if (fsm[event][type][state].actions)
			for (idx=0;fsm[event][type][state].actions[idx];idx++)
				fsm[event][type][state].actions[idx](status, key, event);
	}
}

