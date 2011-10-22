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

#include "status.h"
#include "key.h"

#ifndef FLO_FSM
#define FLO_FSM

struct status;
struct key;

/* the type of event, used as input for the fsm table */
enum fsm_event {
	FSM_PRESS, /* the press event must be sent */
	FSM_RELEASE, /* the release event must be sent */
	FSM_PRESSED, /* the press event has been sent */
	FSM_RELEASED, /* the release event has been sent */
	FSM_EVENT_NUM
};

/* the type of the key calculated for the fsm table */
enum status_key_type {
	FSM_KEY_NORMAL,
	FSM_KEY_MODIFIER,
	FSM_KEY_LOCKER,
	FSM_KEY_TYPE_NUM
};

/* process the fsm actions and state change */
void fsm_process(struct status *status, struct key *key, enum fsm_event event);

#endif

