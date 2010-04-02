/* 
 * florence - Florence is a simple virtual keyboard for Gnome.

 * Copyright (C) 2008, 2009, 2010 François Agrech

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

*/

#include "system.h"
#include "xkeyboard.h"
#include "trace.h"
#include <gdk/gdkx.h>

/* liberate groups memory */
void xkeyboard_groups_free(struct xkeyboard *xkeyboard) {
	while(xkeyboard->groups) {
		g_free(g_list_first(xkeyboard->groups)->data);
		xkeyboard->groups=g_list_delete_link(xkeyboard->groups, g_list_first(xkeyboard->groups));
	}
}

#ifdef ENABLE_XKB

/* resurns TRUE is the symbol is a layout name */
gboolean xkeyboard_is_sym(gchar *symbol)
{
	gboolean ret=TRUE;
	static gchar *nonSymbols[]={"group", "inet", "pc", "terminate", "compose", "level", NULL};
	int i;
	for(i=0; nonSymbols[i]; i++) {
		if (!strcmp(symbol, nonSymbols[i])) { ret=FALSE; break; }
	}
	return ret;
}

/* append a symbol to the list */
void xkeyboard_list_append(gchar *symbol, GList **list)
{
	gchar *newsymbol=g_malloc(sizeof(gchar)*(strlen(symbol)+1));
	strcpy(newsymbol, symbol);
	*list=g_list_append(*list, newsymbol);
	flo_debug(_("new xkb symbol found: <%s>"), newsymbol);
}

/* parse the symbol string from xkb to extract the layout names */
GList *xkeyboard_symParse(gchar *symbols, gint count)
{
	GList *ret=NULL;
	gboolean inSymbol=FALSE;
	gchar curSymbol[32];
	guint i;
	gchar ch;
	gint c=0;

	curSymbol[0]='\0';
	for (i=0 ; i<strlen(symbols) ; i++) {
		ch=symbols[i];
		if (ch=='+') {
			if (inSymbol) {
				if (xkeyboard_is_sym(curSymbol)) {
					xkeyboard_list_append(curSymbol, &ret);
					c++; if (c>=count) break;
				}
				curSymbol[0]='\0';
			} else inSymbol=TRUE;
		} else if (inSymbol && (ISALPHA(ch) || ch=='_')) {
			if (strlen(curSymbol)<31) {
				curSymbol[strlen(curSymbol)+1]='\0';
				curSymbol[strlen(curSymbol)]=ch;
			}
		} else if (inSymbol) {
			if (xkeyboard_is_sym(curSymbol))
				xkeyboard_list_append(curSymbol, &ret);
				c++; if (c>=count) break;
			curSymbol[0]='\0';
			inSymbol=FALSE;
		}
	}

	if (inSymbol && curSymbol[0] && xkeyboard_is_sym(curSymbol) && (c<count))
		xkeyboard_list_append(curSymbol, &ret);

	return ret;
}

/* returns the name of currently selected keyboard layout in XKB */
void xkeyboard_layout(struct xkeyboard *xkeyboard)
{
	gint i;
	gint groupCount;
	Atom* tmpGroupSource=None;
	Atom curGroupAtom, symNameAtom;
	gchar *groupName, *symName;
	Display *disp=(Display *)gdk_x11_get_default_xdisplay();
	XkbDescRec* kbdDescPtr=XkbAllocKeyboard();

	if (!kbdDescPtr) {
		flo_warn("Unable to allocate memory to get keyboard description");
		return;
	}
	XkbGetControls(disp, XkbAllControlsMask, kbdDescPtr);
	XkbGetNames(disp, XkbSymbolsNameMask, kbdDescPtr);
	XkbGetNames(disp, XkbGroupNamesMask, kbdDescPtr);
	if (!kbdDescPtr->names) {
		flo_warn("Unable to get keyboard description");
		return;
	}

	/* Count the number of configured groups. */
	const Atom* groupSource = kbdDescPtr->names->groups;
	if (kbdDescPtr->ctrls) {
		groupCount=kbdDescPtr->ctrls->num_groups;
	} else {
		groupCount=0;
		while ((groupCount<XkbNumKbdGroups) && (groupSource[groupCount]!=None))
			groupCount++;
	}

	/* There is always at least one group. */
	if (!groupCount) {
		groupCount=1;
	} else {
		/* Get the group names. */
		tmpGroupSource=kbdDescPtr->names->groups;
		for (i=0 ; i<groupCount ; i++) {
			if ((curGroupAtom=tmpGroupSource[i])!=None) {
				groupName=XGetAtomName(disp, curGroupAtom);
				flo_debug(_("keyboard layout found: <%s>"), groupName);
				XFree(groupName);
			}
		}
	}

	/* Get the symbol name and parse it for layout symbols. */
	symNameAtom=kbdDescPtr->names->symbols;
	if (symNameAtom!=None) {
		symName=XGetAtomName(disp, symNameAtom);
		flo_debug(_("keyboard layout symbol name=<%s>"), symName);
		xkeyboard->groups=xkeyboard_symParse(symName, groupCount);
		if (!symName) return;
		XFree(symName);
	}

	/* Select events */
	/* XkbSelectEventDetails(disp, XkbUseCoreKbd, XkbStateNotify,
		XkbAllStateComponentsMask, XkbGroupStateMask); */
}

/* handles events from XKB */
GdkFilterReturn xkeyboard_event_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XkbEvent *xkbev;
	XEvent *ev=(XEvent *)xevent;
	struct xkeyboard *xkeyboard=(struct xkeyboard *)data;
	if (ev->xany.type==(xkeyboard->base_event_code+XkbEventCode)) {
		xkbev=(XkbEvent *)ev;
		if ((xkbev->any.xkb_type==XkbStateNotify)||(xkbev->any.xkb_type==XkbNewKeyboardNotify)) {
			if (xkbev->any.xkb_type==XkbNewKeyboardNotify) {
				xkeyboard_groups_free(xkeyboard);
				xkeyboard_layout(xkeyboard);
			}
			flo_debug(_("XKB state notify event received"));
			xkeyboard->event_cb(xkeyboard->user_data);
		}
	}
	return GDK_FILTER_CONTINUE;
}

#endif

/* returns the next layout name */
gchar *xkeyboard_next_layout_get(struct xkeyboard *xkeyboard)
{
#ifdef ENABLE_XKB
	guint newgroup;
	XkbStateRec xkbState;
	Display *disp=(Display *)gdk_x11_get_default_xdisplay();
	XkbGetState(disp, XkbUseCoreKbd, &xkbState);
	newgroup=(xkbState.group+1)%g_list_length(xkeyboard->groups);
	return (gchar *)g_list_nth_data(xkeyboard->groups, newgroup);
#else
	return (gchar *)g_list_nth_data(xkeyboard->groups, 0);
#endif
}

/* switch keyboard layout */
void xkeyboard_layout_change(struct xkeyboard *xkeyboard)
{
#ifdef ENABLE_XKB
	guint newgroup;
	XkbStateRec xkbState;
	Display *disp=(Display *)gdk_x11_get_default_xdisplay();
	XkbGetState(disp, XkbUseCoreKbd, &xkbState);
	newgroup=(xkbState.group+1)%g_list_length(xkeyboard->groups);
	if (XkbLockGroup(disp, XkbUseCoreKbd, newgroup)) {
		flo_debug(_("switching to xkb layout %s"),
			g_list_nth_data(xkeyboard->groups, newgroup));
	} else flo_warn(_("Failed to switch xkb layout"));
#endif
}

/* register xkb events */
void xkeyboard_register_events(struct xkeyboard *xkeyboard, xkeyboard_layout_changed event_cb, gpointer user_data)
{
	xkeyboard->event_cb=event_cb;
	xkeyboard->user_data=user_data;
}

/* get keyval according to modifier */
/* TODO: cache group state value */
guint xkeyboard_getKeyval(struct xkeyboard *xkeyboard, guint code, GdkModifierType mod)
{
	guint keyval=0;
	XkbStateRec xkbState;
	Display *disp=(Display *)gdk_x11_get_default_xdisplay();
	XkbGetState(disp, XkbUseCoreKbd, &xkbState);
	if (!gdk_keymap_translate_keyboard_state(gdk_keymap_get_default(), code, mod, xkbState.group,
		&keyval, NULL, NULL, NULL)) {
		keyval=0;
	}
	return keyval;
}

/* get the key properties (locker and modifier) from xkb */
void xkeyboard_key_properties_get(struct xkeyboard *xkeyboard, guint code, GdkModifierType *mod, gboolean *locker)
{
	*locker=FALSE;
	*mod=0;
#ifdef ENABLE_XKB
	*mod=xkeyboard->xkb_desc->map->modmap[code];
	if (XkbKeyAction(xkeyboard->xkb_desc, code, 0)) {
		switch (XkbKeyAction(xkeyboard->xkb_desc, code, 0)->type) {
			case XkbSA_LockMods:*locker=TRUE; break;
			case XkbSA_SetMods:*mod=XkbKeyAction(xkeyboard->xkb_desc, code, 0)->mods.mask;
				break;
		}
	}
#else
	gchar *symbolname symbolname=gdk_keyval_name(xkeyboard_getKeyval(xkeyboard, code, 0));
	if (symbolname) if (!strcmp(symbolname, "Caps_Lock")) {
		*locker=TRUE;
		*mod=2;
	} else if (!strcmp(symbolname, "Num_Lock")) {
		*locker=TRUE;
		*mod=16;
	} else if (!strcmp(symbolname, "Shift_L") ||
		!strcmp(symbolname, "Shift_R")) {
		*mod=1;
	} else if ( !strcmp(symbolname, "Alt_L") ||
		!strcmp(symbolname, "Alt_R")) {
		*mod=8;
	} else if (!strcmp(symbolname, "Control_L") ||
		!strcmp(symbolname, "Control_R")) {
		*mod=4;
	} else if (!strcmp(symbolname, "ISO_Level3_Shift")) {
		*mod=128;
	}
#endif
}

/* returns a new allocated structure containing data from xkb */
struct xkeyboard *xkeyboard_new()
{
	struct xkeyboard *xkeyboard=g_malloc(sizeof(struct xkeyboard));
	if (!xkeyboard) flo_fatal(_("Unable to allocate memory for xkeyboard data"));
	memset(xkeyboard, 0, sizeof(struct xkeyboard));

#ifdef ENABLE_XKB
	int maj=XkbMajorVersion;
	int min=XkbMinorVersion;
	int opcode_rtrn=0, error_rtrn=0;
	int ret=0;

	/* Check XKB Version */
	if (!(ret=XkbLibraryVersion(&maj, &min))) {
		flo_fatal(_("Unable to initialize XKB library. version=%d.%d rc=%d"), maj, min, ret);
	}
	if (!(ret=XkbQueryExtension((Display *)gdk_x11_get_default_xdisplay(),
		&opcode_rtrn, &(xkeyboard->base_event_code), &error_rtrn, &maj, &min))) {
		flo_fatal(_("Unable to query XKB extension from X server version=%d.%d rc=%d"), maj, min, ret);
	}

	xkeyboard_layout(xkeyboard);
	XkbSelectEvents(GDK_DISPLAY(), XkbUseCoreKbd, XkbNewKeyboardNotifyMask, XkbNewKeyboardNotifyMask);
	XkbSelectEventDetails(GDK_DISPLAY(), XkbUseCoreKbd, XkbStateNotify, XkbAllStateComponentsMask, XkbGroupStateMask);
	gdk_window_add_filter(NULL, xkeyboard_event_handler, xkeyboard);

	/* get the modifier map from xkb */
	xkeyboard->xkb_desc=XkbGetMap((Display *)gdk_x11_get_default_xdisplay(),
	XkbKeyActionsMask|XkbModifierMapMask, XkbUseCoreKbd);
	/* get modifiers state */
	XkbGetState((Display *)gdk_x11_get_default_xdisplay(), XkbUseCoreKbd, &(xkeyboard->xkb_state));
#else
	flo_warn(_("XKB not compiled: startup keyboard sync is disabled. You should make sure all locker keys are released."));
#endif
	if (!xkeyboard->groups) {
	       flo_warn(_("No xkb group found. Using default us group"));
	       xkeyboard_list_append("us", &(xkeyboard->groups));
	}
	return xkeyboard;
}

/* liberate memory used by the modifier map */
void xkeyboard_client_map_free(struct xkeyboard *xkeyboard)
{
#ifdef ENABLE_XKB
	/* Free the modifiers map */
	XkbFreeClientMap(xkeyboard->xkb_desc, XkbKeyActionsMask|XkbModifierMapMask, True);
#endif
}

/* liberate any memory used to record xkb data */
void xkeyboard_free(struct xkeyboard *xkeyboard)
{
	xkeyboard_groups_free(xkeyboard);
	g_free(xkeyboard);
}

