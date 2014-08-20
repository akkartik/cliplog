#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

GtkClipboard* clipboard;

glong validate_utf8_text(gchar *text, glong nbytes) {
  const gchar *valid;
  text[nbytes] = 0;
  if (!g_utf8_validate(text, -1, &valid)) {
    g_printf("Truncating invalid utf8 text entry: ");
    nbytes = valid-text;
    text[nbytes] = 0;
    g_printf("'%s'\n", text);
  }
  return nbytes;
}

void log_clipboard(char* data) {
  static gchar* history_path = NULL;
  if (!history_path)
      history_path = g_build_filename(g_get_user_data_dir(), "clipboard_log", NULL);
  FILE* history_file = fopen(history_path, "a");
  if (!history_file) return;
  time_t now;
  time(&now);
  fprintf(history_file, "-- %s%s\n", ctime(&now), data);
  fclose(history_file);
}

gchar* update_clipboard() {
  static gchar *cprev = NULL;
  gchar *cnext = gtk_clipboard_wait_for_text(clipboard);
  if (cnext && g_strcmp0(cprev, cnext)) {
    if (validate_utf8_text(cnext, strlen(cnext))) {
      if (cprev)
        g_free(cprev);
      cprev = g_strdup(cnext);
      if (cprev) log_clipboard(cprev);
    }
  }
  if (cnext) g_free(cnext);
  return cprev;
}

gboolean check_clipboards(gpointer dummy) {
  update_clipboard(clipboard);
  return TRUE;
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  g_timeout_add(500/*ms*/, check_clipboards, NULL);
  gtk_main();
  return 0;
}
