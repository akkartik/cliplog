#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

GtkClipboard* clipboard;

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
    log_clipboard(cnext);
    if (cprev) g_free(cprev);
    cprev = cnext;
    cnext = NULL;
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
