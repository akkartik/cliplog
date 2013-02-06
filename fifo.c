/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#include "fifo.h"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO_MODE_NONE 0x10
#define FIFO_MODE_PRI  0x20
#define FIFO_MODE_CLI  0x40

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

gboolean fifo_read_cb (GIOChannel *src,  GIOCondition cond, gpointer data)
{
  struct p_fifo *f=(struct p_fifo *)data;
  int which;
  if(src == f->g_ch_p)
    which=FIFO_MODE_PRI;
  else if(src == f->g_ch_c)
    which=FIFO_MODE_CLI;
  else{
    g_printf("Unable to determine fifo!!\n");
    return 0;
  }
  if(cond & G_IO_HUP){
    if(f->dbg) g_printf("gothup ");
    return FALSE;
  }
  if(cond & G_IO_NVAL){
    if(f->dbg) g_printf("readnd ");
    return FALSE;
  }
  if(cond & G_IO_IN){
    if(f->dbg) g_printf("norm ");
  }

  if(f->dbg) g_printf("0x%X Waiting on chars\n",cond);
  f->rlen=0;
    int s;

    s=read_fifo(f,which);

  return TRUE;
}

gint _create_fifo(gchar *f)
{
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

int open_fifos(struct p_fifo *fifo)
{
  int flg;
  gchar *f;

  /* if you set O_RDONLY, you get 100%cpu usage from HUP */
  flg=O_RDWR|O_NONBLOCK;/*|O_EXCL; */

  f=g_build_filename(g_get_user_data_dir(), "cliplog/primary", NULL);
  if((fifo->primary_fifo=_open_fifo(f,flg)) > 2) {
    if(fifo->dbg) g_printf("PRI fifo %d\n",fifo->primary_fifo);
    fifo->g_ch_p=g_io_channel_unix_new (fifo->primary_fifo);
    g_io_add_watch (fifo->g_ch_p,G_IO_IN|G_IO_HUP,fifo_read_cb,(gpointer)fifo);
  }

  f=g_build_filename(g_get_user_data_dir(), "cliplog/clipboard", NULL);
  if((fifo->clipboard_fifo=_open_fifo(f,flg)) > 2) {
    fifo->g_ch_c=g_io_channel_unix_new (fifo->clipboard_fifo);
    g_io_add_watch (fifo->g_ch_c,G_IO_IN|G_IO_HUP,fifo_read_cb,(gpointer)fifo);
  }
  if(fifo->dbg) g_printf("CLI fifo %d PRI fifo %d\n",fifo->clipboard_fifo,fifo->primary_fifo);
  if(fifo->clipboard_fifo <3 || fifo->primary_fifo <3)
    return -1;
  return 0;
}


int read_fifo(struct p_fifo *f, int which)
{
  int i,t, fd;
  i=t=0;

  switch(which){
    case FIFO_MODE_PRI:
      fd=f->primary_fifo;
      f->which=ID_PRIMARY;
      break;
    case FIFO_MODE_CLI:
      fd=f->clipboard_fifo;
      f->which=ID_CLIPBOARD;
      break;
    default:
      g_printf("Unknown fifo %d!\n",which);
      return -1;
  }
  if(NULL ==f || fd <3 ||NULL == f->buf|| f->len <=0)
    return -1;
  while(1){
    i=read(fd,f->buf,f->len-t);
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
  f->buf[t]=0;
  f->rlen=t;
  if(t>0)
    if(f->dbg) g_printf("%s: Got %d '%s'\n",f->primary_fifo==fd?"PRI":"CLI",t,f->buf);
  return t;
}

int write_fifo(struct p_fifo *f, int which, char *buf, int len)
{
  int i, l,fd;
  l=0;
  switch(which){
    case FIFO_MODE_PRI:
      if(f->dbg) g_printf("Using pri fifo for write\n");
      fd=f->primary_fifo;
      break;
    case FIFO_MODE_CLI:
      if(f->dbg) g_printf("Using cli fifo for write\n");
      fd=f->clipboard_fifo;
      break;
    default:
      g_printf("Unknown fifo %d!\n",which);
      return -1;
  }
  if(NULL ==f || fd <3 || NULL ==buf)
    return -1;
  if(f->dbg) g_printf("writing '%s'\n",buf);
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

/* Figure out who we are, then open the fifo accordingly.

      GIOChannel* g_io_channel_unix_new(int fd);

      guint g_io_add_watch(GIOChannel *channel,
                           G_IO_IN GIOFunc func,
                           gpointer user_data);

      g_io_channel_shutdown(channel,TRUE,NULL); */
struct p_fifo *init_fifo()
{
  struct p_fifo *f=g_malloc0(sizeof(struct p_fifo));
  if(NULL == f){
    g_printf("Unable to allocate for fifo!!\n");
    return NULL;
  }
  if(NULL == (f->buf=(gchar *)g_malloc0(8000)) ){
    g_printf("Unable to alloc 8k buffer for fifo! Command Line Input will be ignored.\n");
    g_free(f);
    return NULL;
  }
  /**set debug here for debug messages */
  f->dbg=1;
  f->len=7999;

  if(create_fifo() < 0 ){
    g_printf("error creating fifo\n");
    goto err;
  }
  if(open_fifos(f) < 0 ){
    g_printf("Error opening fifo. Exit\n");
    goto err;
  }
  return f;

err:
  close_fifos(f);
  return NULL;

}

void close_fifos(struct p_fifo *f)
{
  if(NULL ==f)
    return;

  if(NULL != f->g_ch_p)
    g_io_channel_shutdown(f->g_ch_p,TRUE,NULL);
  if(f->primary_fifo>0)
    close(f->primary_fifo);

  if(NULL != f->g_ch_c)
    g_io_channel_shutdown(f->g_ch_c,TRUE,NULL);
  if(f->clipboard_fifo>0)
    close(f->clipboard_fifo);
  g_free(f);
}
