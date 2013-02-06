/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#include "state.h"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct State state;

int clipboard_fifo = 0;
GIOChannel* clipboard_io_channel;

void check_dirs() {
  gchar* config_dir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), "cliplog",  NULL);
  if (!g_file_test(config_dir, G_FILE_TEST_EXISTS)) {
    if (g_mkdir_with_parents(config_dir, 0755) != 0)
      g_warning("Couldn't create directory: %s\n", config_dir);
  }
  g_free(config_dir);
}

gboolean fifo_read_cb (GIOChannel *src, GIOCondition cond, gpointer unused) {
  if (src != clipboard_io_channel) {
    g_printf("Unable to determine fifo\n");
    return 0;
  }
  if (cond & G_IO_HUP) {
    if (state.dbg) g_printf("gothup ");
    return FALSE;
  }
  if (cond & G_IO_NVAL) {
    if (state.dbg) g_printf("readnd ");
    return FALSE;
  }
  if (cond & G_IO_IN) {
    if (state.dbg) g_printf("norm ");
  }

  if (state.dbg) g_printf("0x%X Waiting on chars\n", cond);
  state.rlen = 0;
    int s;

  read_fifo();

  return TRUE;
}

gint _create_fifo(gchar *f) {
  int i = 0;
  if (0 == access(f, F_OK))
    unlink(f);
  if (-1 ==mkfifo(f, 0660)) {
    perror("mkfifo ");
    i= -1;
  }
  g_free(f);
  return i;
}

int create_fifo(void) {
  gchar *f;
  int i = 0;
  check_dirs();
  f = g_build_filename(g_get_user_data_dir(), "cliplog/clipboard", NULL);
  if (-1 ==  _create_fifo(f))
    --i;
  return i;
}

int _open_fifo(char *path, int flg) {
  int fd;
  mode_t mode = 0660;
  fd = open(path, flg, mode);
  if (fd<3) {
    perror("Unable to open fifo");
  }
  g_free(path);
  return fd;
}

int open_state() {
  int flg;
  gchar *f;

  /* if you set O_RDONLY, you get 100%cpu usage from HUP */
  flg = O_RDWR|O_NONBLOCK;/*|O_EXCL; */

  f = g_build_filename(g_get_user_data_dir(), "cliplog/clipboard", NULL);
  if ((clipboard_fifo=_open_fifo(f, flg)) > 2) {
    clipboard_io_channel = g_io_channel_unix_new (clipboard_fifo);
    g_io_add_watch(clipboard_io_channel, G_IO_IN|G_IO_HUP, fifo_read_cb, NULL);
  }
  if (state.dbg)
    g_printf("CLI fifo %d\n", clipboard_fifo);
  if (clipboard_fifo < 3)
    return -1;
  return 0;
}

int read_fifo() {
  int bytes_read=0, curr=0, fd=0;

  fd = clipboard_fifo;

  if (fd <3 ||NULL == state.buf|| state.len <=0)
    return -1;

  while (1) {
    bytes_read = read(fd, state.buf, state.len-curr);
    if (bytes_read <= 0) break;
    curr += bytes_read;
  }
  if (bytes_read == -1 && errno != EAGAIN)
    perror("read_fifo");
  state.buf[curr] = 0;
  state.rlen = curr;
  if (curr > 0)
    if (state.dbg) g_printf("Got %d '%s'\n", curr, state.buf);
  return curr;
}

void init_state(void) {
  if (NULL == (state.buf=(gchar*)g_malloc0(8000))) {
    g_printf("Unable to alloc 8k buffer for fifo! Command Line Input will be ignored.\n");
    return;
  }
  /**set debug here for debug messages */
  state.dbg = 1;
  state.len = 7999;

  if (create_fifo() < 0) {
    g_printf("error creating fifo\n");
    goto err;
  }
  if (open_state(&state) < 0) {
    g_printf("Error opening fifo. Exit\n");
    goto err;
  }
  return;

err:
  close_state();
}

void close_state(void) {
  if (NULL != clipboard_io_channel)
    g_io_channel_shutdown(clipboard_io_channel, TRUE, NULL);
  if (clipboard_fifo > 0)
    close(clipboard_fifo);
}
