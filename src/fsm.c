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
typedef void (*fsm_action) (struct status *, struct key *);

/* what to do when state changes */
struct fsm_change {
	enum key_state new_state; /* new state after the change */
	fsm_action *actions; /* actions to do to switch state (NULL terminated list) */
};

/* FSM action groups */
void fsm_error (struct status *status, struct key *key);
static fsm_action fsm_action_p[]={ status_press, NULL };
static fsm_action fsm_action_r[]={ status_release, NULL };
static fsm_action fsm_action_pu[]={ status_press, status_update_key, NULL };
static fsm_action fsm_action_ru[]={ status_release, status_update_key, NULL };
static fsm_action fsm_action_la[]={ status_latch, status_update_view, NULL };
static fsm_action fsm_action_ulalo[]={ status_unlatch, status_lock, status_update_view, NULL };
static fsm_action fsm_action_lo[]={ status_lock, status_update_view, NULL };
static fsm_action fsm_action_ul[]={ status_unlock, status_update_view, NULL };
static fsm_action fsm_action_plap[]={ status_press_latched, status_press, NULL };
static fsm_action fsm_action_rrlaulaa[]={ status_release, status_release_latched, status_unlatch_all, status_update_view, NULL };
static fsm_action fsm_action_uv[]={ status_update_view, NULL };
static fsm_action fsm_action_u[]={ status_update_key, NULL };
static fsm_action fsm_action_err[]={ fsm_error, NULL };

/* Mouse FSM - This FSM is adapted for Mouse input. Simulate sticky keys.
 * changes by type, event and state */
static struct fsm_change fsm_mouse[FSM_EVENT_NUM][FSM_KEY_TYPE_NUM][KEY_STATE_NUM]={
	{ /* PRESS event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_plap }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_LATCHED, fsm_action_la }, /* RELEASED state */
			{ KEY_RELEASED, fsm_action_ul }, /* LOCKED state */
			{ KEY_LOCKED, fsm_action_ulalo } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_p }, /* RELEASED state */
			{ KEY_LOCKED, fsm_action_p } /* LOCKED state */
		}
	}, { /* RELEASE event */
		{ /* NORMAL key */
			{ KEY_PRESSED, fsm_action_rrlaulaa }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_u }, /* RELEASED state */
			{ KEY_LOCKED, fsm_action_u }, /* LOCKED state */
			{ KEY_LATCHED, fsm_action_u } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_ru }, /* RELEASED state */
			{ KEY_LOCKED, fsm_action_ru } /* LOCKED state */
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

/* Touch FSM - This FSM is adapted for Touch screen input.
 * changes by type, event and state */
static struct fsm_change fsm_touch[FSM_EVENT_NUM][FSM_KEY_TYPE_NUM][KEY_STATE_NUM]={
	{ /* PRESS event */
		{ /* NORMAL key */
			{ KEY_PRESSED, NULL }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_pu }, /* RELEASED state */
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
			{ KEY_PRESSED, fsm_action_rrlaulaa }, /* PRESSED state */
			{ KEY_PRESSED, fsm_action_plap }, /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_LATCHED, fsm_action_la }, /* RELEASED state */
			{ KEY_RELEASED, fsm_action_ul }, /* LOCKED state */
			{ KEY_LOCKED, fsm_action_ulalo } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_p }, /* RELEASED state */
			{ KEY_LOCKED, fsm_action_p } /* LOCKED state */
		}
	}, { /* PRESSED event */
		{ /* NORMAL key */
			{ KEY_PRESSED, fsm_action_rrlaulaa }, /* PRESSED state */
			{ KEY_PRESSED, NULL } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_PRESSED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_r }, /* RELEASED state */
			{ KEY_LOCKED, fsm_action_r } /* LOCKED state */
		}
	}, { /* RELEASED event */
		{ /* NORMAL key */
			{ KEY_RELEASED, fsm_action_uv }, /* PRESSED state */
			{ KEY_RELEASED, fsm_action_err } /* RELEASED state */
		}, { /* MODIFIER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_RELEASED, NULL }, /* RELEASED state */
			{ KEY_LOCKED, NULL }, /* LOCKED state */
			{ KEY_LATCHED, NULL } /* LATHCED state */
		}, { /* LOCKER key */
			{ KEY_RELEASED, fsm_action_err }, /* PRESSED state */
			{ KEY_LOCKED, fsm_action_lo }, /* RELEASED state */
			{ KEY_RELEASED, fsm_action_ul } /* LOCKED state */
		}
	}
};

/* process the fsm actions and state change */
void fsm_process(struct status *status, struct key *key, enum fsm_event event)
{
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
		flo_debug(_("Key %p (type %d): Event %d received : Switching from state %d to %d"),
			key, type, event, state, (*fsm)[event][type][state].new_state);
		if ((*fsm)[event][type][state].actions)
			for (idx=0;(*fsm)[event][type][state].actions[idx];idx++)
				(*fsm)[event][type][state].actions[idx](status, key);
	}
}

/* print a fsm error */
void fsm_error (struct status *status, struct key *key)
{
	flo_error(_("FSM state errorr: state=%d"), key->state);
}

