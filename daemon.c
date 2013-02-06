/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#include "parcellite.h"
#include <glib.h>
#include <gtk/gtk.h>
#include "daemon.h"
#include "utils.h"

static gint timeout_id;
static gchar* primary_text;
static gchar* clipboard_text;
static GtkClipboard* primary;
static GtkClipboard* clipboard;


/* Called during the daemon loop to protect primary/clipboard contents */
static void daemon_check()
{
  /* Get current primary/clipboard contents */
  gchar* primary_temp = gtk_clipboard_wait_for_text(primary);
  gchar* clipboard_temp = gtk_clipboard_wait_for_text(clipboard);
  /* Check if primary contents were lost */
  if ((primary_temp == NULL) && (primary_text != NULL))
  {
    /* Check contents */
    gint count;
		GdkAtom *targets;
		gboolean contents = gtk_clipboard_wait_for_targets(primary, &targets, &count);
		g_free(targets);
		/* Only recover lost contents if there isn't any other type of content in the clipboard */
		if (!contents)
      gtk_clipboard_set_text(primary, primary_text, -1);
  }
  else
  {
    /* Get the button state to check if the mouse button is being held */
    GdkModifierType button_state;
    gdk_window_get_pointer(NULL, NULL, NULL, &button_state);
    if ((primary_temp != NULL) && !(button_state & GDK_BUTTON1_MASK))
    {
      g_free(primary_text);
      primary_text = p_strdup(primary_temp);
    }
  }

  /* Check if clipboard contents were lost */
  if ((clipboard_temp == NULL) && (clipboard_text != NULL))
  {
    /* Check contents */
    gint count;
		GdkAtom *targets;
		gboolean contents = gtk_clipboard_wait_for_targets(clipboard, &targets, &count);
		g_free(targets);
		/* Only recover lost contents if there isn't any other type of content in the clipboard */
		if (!contents)
      gtk_clipboard_set_text(clipboard, clipboard_text, -1);
  }
  else
  {
    g_free(clipboard_text);
    clipboard_text = p_strdup(clipboard_temp);
  }
  g_free(primary_temp);
  g_free(clipboard_temp);
}

/* Called if timeout was destroyed */
static void reset_daemon(gpointer data)
{
  if (timeout_id != 0)
    g_source_remove(timeout_id);
  /* Add the daemon loop */
  timeout_id = g_timeout_add_full(G_PRIORITY_LOW,
                                  DAEMON_INTERVAL,
                                  (GSourceFunc)daemon_check,
                                  NULL,
                                  (GDestroyNotify)reset_daemon);
}

/* Initializes daemon mode */
void init_daemon_mode()
{
  /* Create clipboard and primary and connect signals */
  primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  /* Add the daemon loop */
  timeout_id = g_timeout_add_full(G_PRIORITY_LOW,
                                  DAEMON_INTERVAL,
                                  (GSourceFunc)daemon_check,
                                  NULL,
                                  (GDestroyNotify)reset_daemon);

  /* Start daemon loop */
  gtk_main();
}
