/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#include <gtk/gtk.h>

#include <string.h>
#include <unistd.h>

GtkClipboard* clipboard;

GMutex* state_lock = NULL;

/* Pass in the text via the struct. We assume len is correct, and BYTE based,
 * not character. Returns length of resulting string. */
glong validate_utf8_text(gchar *text, glong len)
{
  const gchar *valid;
  text[len] = 0;
  if (!g_utf8_validate(text, -1, &valid)) {
    g_printf("Truncating invalid utf8 text entry: ");
    len = valid-text;
    text[len] = 0;
    g_printf("'%s'\n", text);
  }
  return len;
}

struct history_item {
	guint32 len; /**length of data item, MUST be first in structure  */
	gchar text[8]; /**reserve 64 bits (8 bytes) for pointer to data.  */
}__attribute__((__packed__));

GList* history_list = NULL;

void log_clipboard() {
  static gchar* history_path = NULL;
  if (!history_path)
      history_path = g_build_filename(g_get_user_data_dir(), "clipboard_log", NULL);
  FILE* history_file = fopen(history_path, "a");
  if (!history_file) return;
  /* Write most recent element */
  time_t now;
  time(&now);
  fprintf(history_file, "-- %s%s\n", ctime(&now),
      ((struct history_item*)history_list->data)->text);
  fclose(history_file);
}

struct history_item *new_clip_item(guint32 len, void *data)
{
  struct history_item *c;
  if (!(c=g_malloc0(sizeof(struct history_item)+len))) {
    printf("Hit NULL for malloc of history_item!\n");
    return NULL;
  }

  memcpy(c->text, data, len);
  c->len = len;
  return c;
}

void append_item(gchar* item)
{
  gint node = -1;
  if (!item)
    return;
  g_mutex_lock(state_lock);

  struct history_item *c;
  if (!(c=new_clip_item(strlen(item), item)))
    return;

  history_list = g_list_prepend(history_list, c);
  log_clipboard();
  g_mutex_unlock(state_lock);
}

gchar* process_new_item(gchar* ntext) {
  return validate_utf8_text(ntext, strlen(ntext)) ? ntext : NULL;
}

gboolean is_clipboard_empty(GtkClipboard *clip)
{
  int count;
  GdkAtom *targets;
  gboolean contents = gtk_clipboard_wait_for_targets(clip, &targets, &count);
  g_free(targets);
  return !contents;
}

gchar* update_clipboard()
{
  /**current/last item in clipboard  */
  static gchar *ctext = NULL;
  static gchar *last = NULL;
  gchar *changed = NULL;
  gchar *processed;
  int set = 1;

  changed = gtk_clipboard_wait_for_text(clipboard);
  if (changed == NULL) {
    // do nothing
  } else  if (0 == g_strcmp0(ctext, changed)) {
    // do nothing
  } else {
    if (processed=process_new_item(changed)) {
      if (ctext)
        g_free(ctext);
      ctext = g_strdup(processed);
      last = ctext;
      if (last) append_item(last);
    }
  }
  if (changed) g_free(changed);
  return ctext;
}

gboolean check_clipboards(gpointer dummy) {
  update_clipboard(clipboard);
  return TRUE;
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  g_timeout_add(500/*ms*/, check_clipboards, NULL);

  state_lock = g_mutex_new();
  gtk_main();
  return 0;
}
