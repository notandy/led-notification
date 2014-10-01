/*
 * Led-notification
 * Copyright (C) 2006  Simo Mattila
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Simo Mattila <simo.h.mattila@gmail.com>
 *
 * set ts=4 ;)
 */

#define PURPLE_PLUGINS

#define VERSION "0.3"

#include <glib.h>
#include <glib/gi18n.h>

#include <string.h>
#include <stdio.h>

#include "notify.h"
#include "plugin.h"
#include "version.h"
#include "debug.h"
#include "cmds.h"
#include "prefs.h"

void led_set(gboolean state) {
  const char *filename=purple_prefs_get_string("/plugins/gtk/gtk-simom-lednot/filename");
  const char *format=purple_prefs_get_string("/plugins/gtk/gtk-simom-lednot/format");
  char *xfile=malloc(15*sizeof(char));
  FILE *file=NULL;

  if(format[0] == 'x'){
    if (xfile==NULL) {
      purple_debug_error("Led-notification","Unable to allocate memory for xset command line");
      return;
    }
  } else {
    file=fopen(filename, "w");
    if(file==NULL) {
      purple_debug_error("Led-notification","Error opening file '%s'\n",filename);
      return;
    }
  }

  if (state) {
    switch (format[0]) {
    case 'n': fputs("1",file); break;
    case 'w': fputs("on",file); break;
    case 'p': fputs("0 blink",file); break;
    case 'x':
      snprintf(xfile, 15*sizeof(char), "xset led %s", filename);
      file = popen(xfile, "w");
      pclose(file);
      break;
    default:
      purple_debug_error("Led-notification","Erroneous option '%s'\n",format);
      break;
    }
  } else {
    switch (format[0]) {
    case 'n': fputs("0",file); break;
    case 'w': fputs("off",file); break;
    case 'p': fputs("0 on",file); break;
    case 'x':
      snprintf(xfile, 15*sizeof(char), "xset -led %s", filename);
      file = popen(xfile, "w");
      pclose(file);
      break;
    default:
      purple_debug_error("Led-notification","Erroneous option '%s'\n",format);
      break;
    }
  }

  if(format[0]=='x'){
    free(xfile);
  } else {
    fclose(file);
  }
}

gboolean get_pending_list() {
  const char *im=purple_prefs_get_string("/plugins/gtk/gtk-simom-lednot/im");
  const char *chat=purple_prefs_get_string("/plugins/gtk/gtk-simom-lednot/chat");
  GList *l = NULL;

  if(im != NULL && chat != NULL)
    l = purple_get_conversations();
  else if(im != NULL)
    l = purple_get_ims();
  else if(chat != NULL)
    l = purple_get_chats();
  else
    return FALSE;

  for (; l != NULL; l = l->next) {
    PurpleConversation *conv = (PurpleConversation*)l->data;
    if(conv == NULL)
      continue;
      
    if(GPOINTER_TO_INT(purple_conversation_get_data(conv, "unseen-count")))
      return TRUE;
  }

  return FALSE;
}

static void lednot_conversation_updated(PurpleConversation *conv,
                                        PurpleConvUpdateType type) {
  if( type != PURPLE_CONV_UPDATE_UNSEEN ) {return;}

#if 0
  purple_debug_info("Led-notification", "Change in unseen conversations\n");
#endif

  if(!get_pending_list()) {
    led_set(FALSE);
  } else {
    led_set(TRUE);
  }
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
  PurplePluginPrefFrame *frame;
  PurplePluginPref *ppref;

  frame = purple_plugin_pref_frame_new();

  ppref = purple_plugin_pref_new_with_label("Inform about unread...");
  purple_plugin_pref_frame_add(frame, ppref);

  ppref = purple_plugin_pref_new_with_name_and_label(
    "/plugins/gtk/gtk-simom-lednot/im",
    _("Instant Messages:")
  );
  purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
  purple_plugin_pref_add_choice(ppref, "Never", "never");
  purple_plugin_pref_add_choice(ppref, "Always", "always");
  purple_plugin_pref_frame_add(frame, ppref);

  ppref = purple_plugin_pref_new_with_name_and_label(
    "/plugins/gtk/gtk-simom-lednot/chat",
    _("Chat Messages:")
  );
  purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
  purple_plugin_pref_add_choice(ppref, "Never", "never");
  purple_plugin_pref_add_choice(ppref, "Always", "always");
  purple_plugin_pref_frame_add(frame, ppref);

  ppref = purple_plugin_pref_new_with_label("Hardware setup:");
  purple_plugin_pref_frame_add(frame, ppref);

  ppref = purple_plugin_pref_new_with_name_and_label(
    "/plugins/gtk/gtk-simom-lednot/filename",
    _("File to control led:")
  );
  purple_plugin_pref_frame_add(frame, ppref);

  ppref = purple_plugin_pref_new_with_name_and_label(
    "/plugins/gtk/gtk-simom-lednot/format",
    _("LED file format:")
  );
  purple_plugin_pref_set_type(ppref, PURPLE_PLUGIN_PREF_CHOICE);
  purple_plugin_pref_add_choice(ppref, "1 = on, 0 = off", "num");
  purple_plugin_pref_add_choice(ppref, "'on' = on, 'off' = off", "word");
  purple_plugin_pref_add_choice(ppref, "'0 blink' = on, '0 on' = off", "pair");
  purple_plugin_pref_add_choice(ppref, "XSet (Specify LED number in filename)", "xset");
  purple_plugin_pref_frame_add(frame, ppref);

  return frame;
}

static void init_plugin(PurplePlugin *plugin) {
  purple_prefs_add_none("/plugins/gtk/gtk-simom-lednot");
  purple_prefs_add_string("/plugins/gtk/gtk-simom-lednot/im", "always");
  purple_prefs_add_string("/plugins/gtk/gtk-simom-lednot/chat", "nick");
  purple_prefs_add_string("/plugins/gtk/gtk-simom-lednot/filename",
			  "/proc/acpi/asus/mled");
  purple_prefs_add_string("/plugins/gtk/gtk-simom-lednot/format", "num");
}

static gboolean plugin_load(PurplePlugin *plugin) {
  purple_signal_connect(purple_conversations_get_handle(),
			"conversation-updated", plugin,
			PURPLE_CALLBACK(lednot_conversation_updated), NULL);
  return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
  led_set(FALSE); /* Turn the led off */
  purple_signal_disconnect(purple_conversations_get_handle(),
			   "conversation-updated", plugin,
			   PURPLE_CALLBACK(lednot_conversation_updated));
  return TRUE;
}

static PurplePluginUiInfo prefs_info = 
{
	get_plugin_pref_frame
};

static PurplePluginInfo info = {
  PURPLE_PLUGIN_MAGIC,
  PURPLE_MAJOR_VERSION,
  PURPLE_MINOR_VERSION,
  PURPLE_PLUGIN_STANDARD,
  NULL,
  0,
  NULL,
  PURPLE_PRIORITY_DEFAULT,

  "gtk-simom-lednot",
  "Led-notification",
  VERSION,

  "Led notification on laptops",
  "Informs for new messages with your laptop or keyboard led. Based on version by Simo Mattila",
  "Thomas Lake <tswsl1989@sucs.org>",
  "http://github.com/tswsl1989/led-notification",

  plugin_load,   /* load */
  plugin_unload, /* unload */
  NULL,          /* destroy */

  NULL,
  NULL,
  &prefs_info,
  NULL
};

PURPLE_INIT_PLUGIN(lednot, init_plugin, info);
