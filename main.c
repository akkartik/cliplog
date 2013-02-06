/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#include "fifo.h"

#include <gtk/gtk.h>

#include <string.h>
#include <unistd.h>

GtkClipboard* primary;
GtkClipboard* clipboard;

GMutex* state_lock = NULL;

/**clipboard handling modes  */
#define H_MODE_INIT  0  /**clear out clipboards  */
#define H_MODE_NEW  1 /**new text, process it  */
#define H_MODE_LIST 2 /**from list, just put it on the clip  */
#define H_MODE_CHECK 3 /**see if there is new/lost contents.   */
#define H_MODE_LAST  4 /**just return the last updated value.  */
#define H_MODE_IGNORE 5 /**just put it on the clipboard, do not process
                       and do not add to hist list  */

/* Pass in the text via the struct. We assume len is correct, and BYTE based,
 * not character. Returns length of resulting string. */
glong validate_utf8_text(gchar *text, glong len)
{
  const gchar *valid;
  text[len]=0;
  if(FALSE == g_utf8_validate(text,-1,&valid)) {
    g_printf("Truncating invalid utf8 text entry: ");
    len=valid-text;
    text[len]=0;
    g_printf("'%s'\n",text);
  }
  return len;
}

struct history_item {
	guint32 len; /**length of data item, MUST be first in structure  */
	gchar text[8]; /**reserve 64 bits (8 bytes) for pointer to data.  */
}__attribute__((__packed__));

GList* history_list=NULL;

void log_clipboard() {
  check_dirs();
  static gchar* history_path = NULL;
  if (!history_path)
      history_path = g_build_filename(g_get_user_data_dir(), "clipboard_log", NULL);
  FILE* history_file = fopen(history_path, "a");
  if (!history_file) return;
  /* Write most recent element */
  fprintf(history_file, "%s: %s\n", "AA",
      ((struct history_item*)history_list->data)->text);
  fclose(history_file);
}

struct history_item *new_clip_item(guint32 len, void *data)
{
  struct history_item *c;
  if(NULL == (c=g_malloc0(sizeof(struct history_item)+len))){
    printf("Hit NULL for malloc of history_item!\n");
    return NULL;
  }

  memcpy(c->text,data,len);
  c->len=len;
  return c;
}

void append_item(gchar* item)
{
  gint node=-1;
  if(NULL == item)
    return;
  g_mutex_lock(state_lock);

  struct history_item *c;
  if(NULL == (c=new_clip_item(strlen(item),item)) )
    return;

  history_list = g_list_prepend(history_list, c);
  log_clipboard();
  g_mutex_unlock(state_lock);
}

gchar* process_new_item(gchar* ntext) {
  return validate_utf8_text(ntext, strlen(ntext)) ? ntext : NULL;
}

gchar *_update_clipboard (GtkClipboard *clip, gchar *n, gchar **old, int set)
{

  if(NULL != n) {
    if( set)
      gtk_clipboard_set_text(clip, n, -1);
    if(NULL != *old)
      g_free(*old);
    *old=g_strdup(n);
    return *old;
  }else{
    if(NULL != *old)
      g_free(*old);
    *old=NULL;
  }

  return NULL;
}

gboolean is_clipboard_empty(GtkClipboard *clip)
{
  int count;
  GdkAtom *targets;
  gboolean contents = gtk_clipboard_wait_for_targets(clip, &targets, &count);
  g_free(targets);
  return !contents;
}

gchar *update_clipboard(GtkClipboard *clip,gchar *intext,  gint mode)
{
  /**current/last item in clipboard  */
  static gchar *ptext=NULL;
  static gchar *ctext=NULL;
  static gchar *last=NULL; /**last text change, for either clipboard  */
  gchar **existing, *changed=NULL;
  gchar *processed;
  GdkModifierType button_state;
  int set=1;
  if( H_MODE_LAST == mode)
    return last;
  if(clip==primary){
    existing=&ptext;
  } else{
    existing=&ctext;
  }

  if( H_MODE_INIT == mode){
    if(NULL != *existing)
      g_free(*existing);
    *existing=NULL;
    if(NULL != intext)
    gtk_clipboard_set_text(clip, intext, -1);
    return NULL;
  }
  /**check that our clipboards are valid and user wants to use them  */
  if(clip != clipboard)
      return NULL;

  if(H_MODE_CHECK==mode &&clip == primary){/*fix auto-deselect of text in applications like DevHelp and LyX*/
    gdk_window_get_pointer(NULL, NULL, NULL, &button_state);
    if ( button_state & (GDK_BUTTON1_MASK|GDK_SHIFT_MASK) ) /**button down, done.  */
      goto done;
  }
  if( H_MODE_IGNORE == mode){ /**ignore processing and just put it on the clip.  */
    gtk_clipboard_set_text(clip, intext, -1);
    return intext;
  }
  /**check for lost contents and restore if lost */
  /* Only recover lost contents if there isn't any other type of content in the clipboard */
  if(is_clipboard_empty(clip) && NULL != *existing ) {
    gtk_clipboard_set_text(clip, *existing, -1);
    last=*existing;
  }
  /**check for changed clipboard content - in all modes */
  changed=gtk_clipboard_wait_for_text(clip);
  if(0 == g_strcmp0(*existing, changed) ){
    g_free(changed);                    /**no change, do nothing  */
    changed=NULL;
  } else {
    if(NULL != (processed=process_new_item(changed)) ){
      if(0 == g_strcmp0(processed,changed)) set=0;
      else set=1;

      last=_update_clipboard(clip,processed,existing,set);
    }else {/**restore clipboard  */
      gchar *d;

      if(NULL ==*existing && NULL != history_list){
        struct history_item *c;
        c=(struct history_item *)(history_list->data);
        d=c->text;
      }else
        d=*existing;
      if(NULL != d){
        last=_update_clipboard(clip,d,existing,1);
      }

    }
    if(NULL != last)
      append_item(last);
    g_free(changed);
    changed=NULL;
  }
  if( H_MODE_CHECK==mode ){
    goto done;
  }

  if(H_MODE_LIST == mode && 0 != g_strcmp0(intext,*existing)){ /**just set clipboard contents. Already in list  */
    last=_update_clipboard(clip,intext,existing,1);
    if(NULL != last){/**maintain persistence, if set  */
      append_item(last);
    }
    goto done;
  }else if(H_MODE_NEW==mode){
    if(NULL != (processed=process_new_item(intext)) ){
      if(0 == g_strcmp0(processed,*existing))set=0;
      else set=1;
      last=_update_clipboard(clip,processed,existing,set);
      if(NULL != last)
        append_item(last);
    }else
      return NULL;
  }

done:
  return *existing;
}

/* Checks the clipboards and fifos for changes. */
void check_clipboards() {
  gchar *ptext, *ctext, *last;
  int n=0;
  if(fifos.rlen >0){
    switch(fifos.which){
      case PRIMARY:
        fifos.rlen=validate_utf8_text(fifos.buf, fifos.rlen);
        if(fifos.dbg) g_printf("Setting PRI '%s'\n",fifos.buf);
        update_clipboard(primary, fifos.buf, H_MODE_NEW);
        fifos.rlen=0;
        n=1;
        break;
      case CLIPBOARD:
        fifos.rlen=validate_utf8_text(fifos.buf, fifos.rlen);
        if(fifos.dbg) g_printf("Setting CLI '%s'\n",fifos.buf);
        update_clipboard(clipboard, fifos.buf, H_MODE_NEW);
        n=2;
        fifos.rlen=0;
        break;
      default:
        fifos.rlen=validate_utf8_text(fifos.buf, fifos.rlen);
        g_printf("CLIP not set, discarding '%s'\n",fifos.buf);
        fifos.rlen=0;
        break;
    }
  }
  ptext=update_clipboard(primary, NULL, H_MODE_CHECK);
  ctext=update_clipboard(clipboard, NULL, H_MODE_CHECK);

  if(NULL==ptext && NULL ==ctext) return;
  last=update_clipboard(NULL, NULL, H_MODE_LAST);
  if( NULL != last && 0 != g_strcmp0(ptext,ctext)){
    /**last is a copy, of things that may be deallocated  */
    last=strdup(last);
    update_clipboard(clipboard, last, H_MODE_LIST);
    g_free(last);
  }
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  init_fifos();

  primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  g_timeout_add(500/*ms*/, check_clipboards, NULL);

  state_lock = g_mutex_new();

  gtk_main();

  close_fifos();
  return 0;
}
