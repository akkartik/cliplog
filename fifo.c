/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#include "fifo.h"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct State fifos;

void check_dirs() {
  gchar* data_dir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_data_dir(), "cliplog",  NULL);
  if (!g_file_test(data_dir, G_FILE_TEST_EXISTS)) {
    if (g_mkdir_with_parents(data_dir, 0755) != 0)
      g_warning("Couldn't create directory: %s\n", data_dir);
  }
  gchar* config_dir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), "cliplog",  NULL);
  if (!g_file_test(config_dir, G_FILE_TEST_EXISTS)) {
    if (g_mkdir_with_parents(config_dir, 0755) != 0)
      g_warning("Couldn't create directory: %s\n", config_dir);
  }
  g_free(data_dir);
  g_free(config_dir);
}

gboolean fifo_read_cb (GIOChannel *src, GIOCondition cond, gpointer unused) {
  int which;
  if(src == fifos.g_ch_p)
    which = PRIMARY;
  else if(src == fifos.g_ch_c)
    which = CLIPBOARD;
  else{
    g_printf("Unable to determine fifo!!\n");
    return 0;
  }
  if(cond & G_IO_HUP){
    if(fifos.dbg) g_printf("gothup ");
    return FALSE;
  }
  if(cond & G_IO_NVAL){
    if(fifos.dbg) g_printf("readnd ");
    return FALSE;
  }
  if(cond & G_IO_IN){
    if(fifos.dbg) g_printf("norm ");
  }

  if(fifos.dbg) g_printf("0x%X Waiting on chars\n",cond);
  fifos.rlen=0;
    int s;

  read_fifo(which);

  return TRUE;
}

gint _create_fifo(gchar *f) {
  int i=0;
  if(0 == access(f,F_OK)  )
    unlink(f);
  if(-1 ==mkfifo(f,0660)){
    perror("mkfifo ");
    i= -1;
  }
  g_free(f);
  return i;
}

int create_fifo(void)
{
  gchar *f;
  int i=0;
  check_dirs();
  f=g_build_filename(g_get_user_data_dir(), "cliplog/clipboard", NULL);
  if(-1 ==  _create_fifo(f) )
    --i;
  f=g_build_filename(g_get_user_data_dir(), "cliplog/primary", NULL);
  if(-1 ==  _create_fifo(f) )
    --i;
  return i;
}

int _open_fifo(char *path, int flg)
{
  int fd;
  mode_t mode=0660;
  fd=open(path,flg,mode);
  if(fd<3){
    perror("Unable to open fifo");
  }
  g_free(path);
  return fd;
}

int open_fifos() {
  int flg;
  gchar *f;

  /* if you set O_RDONLY, you get 100%cpu usage from HUP */
  flg=O_RDWR|O_NONBLOCK;/*|O_EXCL; */

  f=g_build_filename(g_get_user_data_dir(), "cliplog/primary", NULL);
  if ((fifos.primary_fifo=_open_fifo(f,flg)) > 2) {
    if(fifos.dbg) g_printf("PRI fifo %d\n",fifos.primary_fifo);
    fifos.g_ch_p=g_io_channel_unix_new (fifos.primary_fifo);
    g_io_add_watch(fifos.g_ch_p,G_IO_IN|G_IO_HUP,fifo_read_cb, NULL);
  }

  f=g_build_filename(g_get_user_data_dir(), "cliplog/clipboard", NULL);
  if ((fifos.clipboard_fifo=_open_fifo(f,flg)) > 2) {
    fifos.g_ch_c=g_io_channel_unix_new (fifos.clipboard_fifo);
    g_io_add_watch(fifos.g_ch_c,G_IO_IN|G_IO_HUP,fifo_read_cb, NULL);
  }
  if (fifos.dbg)
    g_printf("CLI fifo %d PRI fifo %d\n",fifos.clipboard_fifo,fifos.primary_fifo);
  if(fifos.clipboard_fifo <3 || fifos.primary_fifo <3)
    return -1;
  return 0;
}


int read_fifo(int which) {
  int i,t, fd;
  i=t=0;

  switch(which){
    case PRIMARY:
      fd=fifos.primary_fifo;
      fifos.which=PRIMARY;
      break;
    case CLIPBOARD:
      fd=fifos.clipboard_fifo;
      fifos.which=CLIPBOARD;
      break;
    default:
      g_printf("Unknown fifo %d!\n",which);
      return -1;
  }

  if(fd <3 ||NULL == fifos.buf|| fifos.len <=0)
    return -1;

  while(1){
    i=read(fd,fifos.buf,fifos.len-t);
    if(i>0)
      t+=i;
    else
      break;
  }
  if( -1 == i){
    if( EAGAIN != errno){
      perror("read_fifo");
      return -1;
    }
  }
  fifos.buf[t]=0;
  fifos.rlen=t;
  if(t>0)
    if(fifos.dbg) g_printf("%s: Got %d '%s'\n",fifos.primary_fifo==fd?"PRI":"CLI",t,fifos.buf);
  return t;
}

int write_fifo(int which, char *buf, int len) {
  int i, l,fd;
  l=0;
  switch(which){
    case PRIMARY:
      if(fifos.dbg) g_printf("Using pri fifo for write\n");
      fd=fifos.primary_fifo;
      break;
    case CLIPBOARD:
      if(fifos.dbg) g_printf("Using cli fifo for write\n");
      fd=fifos.clipboard_fifo;
      break;
    default:
      g_printf("Unknown fifo %d!\n",which);
      return -1;
  }
  if(fd <3 || NULL ==buf)
    return -1;
  if(fifos.dbg) g_printf("writing '%s'\n",buf);
  while(len){
    i=write(fd,buf,len);
    if(i>0)
      len-=i;
    ++l;
    if(l>0x7FFE){
      g_printf("Maxloop hit\n");
      break;
    }

  }
  if( -1 == i){
    if( EAGAIN != errno){
      perror("write_fifo");
      return -1;
    }
  }
  return 0;
}

void init_fifos(void) {
  if(NULL == (fifos.buf=(gchar *)g_malloc0(8000)) ){
    g_printf("Unable to alloc 8k buffer for fifo! Command Line Input will be ignored.\n");
    return;
  }
  /**set debug here for debug messages */
  fifos.dbg=1;
  fifos.len=7999;

  if(create_fifo() < 0 ){
    g_printf("error creating fifo\n");
    goto err;
  }
  if(open_fifos(&fifos) < 0 ){
    g_printf("Error opening fifo. Exit\n");
    goto err;
  }
  return;

err:
  close_fifos();
}

void close_fifos(void) {
  if(NULL != fifos.g_ch_p)
    g_io_channel_shutdown(fifos.g_ch_p,TRUE,NULL);
  if(fifos.primary_fifo>0)
    close(fifos.primary_fifo);

  if(NULL != fifos.g_ch_c)
    g_io_channel_shutdown(fifos.g_ch_c,TRUE,NULL);
  if(fifos.clipboard_fifo>0)
    close(fifos.clipboard_fifo);
}
