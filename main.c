/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

/* NOTES: We keep track of a delete list while the history menu is up. We
 * add/remove items from that list until we get a selection done event, then
 * we delete those items from the real history */

#include "parcellite.h"



GtkWidget *hmenu;
/* Uncomment the next line to print a debug trace. */
/*#define DEBUG   */

#ifdef DEBUG
#  define TRACE(x) x
#else
#  define TRACE(x) do {} while (FALSE);
#endif
static GtkClipboard* primary;
static GtkClipboard* clipboard;
struct p_fifo *fifo;
static GtkStatusIcon *status_icon;
static GMutex *clip_lock=NULL;
GMutex *hist_lock=NULL;
static gboolean actions_lock = FALSE;
static int have_appindicator=0; /**if set, we have a running indicator-appmenu  */
static gchar *appindicator_process="indicator-appmenu"; /**process name  */
/**defines for moving between clipboard histories  */
#define HIST_MOVE_TO_CANCEL     0
#define HIST_MOVE_TO_OK         1
/**clipboard handling modes  */
#define H_MODE_INIT  0  /**clear out clipboards  */
#define H_MODE_NEW  1 /**new text, process it  */
#define H_MODE_LIST 2 /**from list, just put it on the clip  */
#define H_MODE_CHECK 3 /**see if there is new/lost contents.   */
#define H_MODE_LAST  4 /**just return the last updated value.  */
#define H_MODE_IGNORE 5 /**just put it on the clipboard, do not process
                       and do not add to hist list  */

#define EDIT_MODE_USE_RIGHT_CLICK 1 /**used in edit dialog creation to determine behaviour.
                                    If this is set, it will edit the entry, and replace it in the history.  */

/**protos in this file  */
void create_app_indicator(void);

/**Turns up in 2.16  */
int p_strcmp (const char *str1, const char *str2)
{
#if (GTK_MAJOR_VERSION > 2 || ( GTK_MAJOR_VERSION == 2 && GTK_MAJOR_VERSION >= 16))
  return g_strcmp0(str1,str2);
#else
  if(NULL ==str1 && NULL == str2)
    return 0;
  if(NULL ==str1 && str2)
    return -1;
  if(NULL ==str2 && str1)
    return 1;
  return strcmp(str1,str2);
#endif
}

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
  g_mutex_lock(hist_lock);

  struct history_item *c;
  if(NULL == (c=new_clip_item(strlen(item),item)) )
    return;

  history_list = g_list_prepend(history_list, c);
  log_clipboard();
  g_mutex_unlock(hist_lock);
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
  if(0 == p_strcmp(*existing, changed) ){
    g_free(changed);                    /**no change, do nothing  */
    changed=NULL;
  } else {
    if(NULL != (processed=process_new_item(changed)) ){
      if(0 == p_strcmp(processed,changed)) set=0;
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

  if(H_MODE_LIST == mode && 0 != p_strcmp(intext,*existing)){ /**just set clipboard contents. Already in list  */
    last=_update_clipboard(clip,intext,existing,1);
    if(NULL != last){/**maintain persistence, if set  */
      append_item(last);
    }
    goto done;
  }else if(H_MODE_NEW==mode){
    if(NULL != (processed=process_new_item(intext)) ){
      if(0 == p_strcmp(processed,*existing))set=0;
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

void update_clipboards(gchar *intext, gint mode)
{
  update_clipboard(clipboard, intext, mode);
}

/* Checks the clipboards and fifos for changes. */
void check_clipboards(gint mode)
{
  gchar *ptext, *ctext, *last;
  int n=0;
  if(fifo->rlen >0){
    switch(fifo->which){
      case ID_PRIMARY:
        fifo->rlen=validate_utf8_text(fifo->buf, fifo->rlen);
        if(fifo->dbg) g_printf("Setting PRI '%s'\n",fifo->buf);
        update_clipboard(primary, fifo->buf, H_MODE_NEW);
        fifo->rlen=0;
        n=1;
        break;
      case ID_CLIPBOARD:
        fifo->rlen=validate_utf8_text(fifo->buf, fifo->rlen);
        if(fifo->dbg) g_printf("Setting CLI '%s'\n",fifo->buf);
        update_clipboard(clipboard, fifo->buf, H_MODE_NEW);
        n=2;
        fifo->rlen=0;
        break;
      default:
        fifo->rlen=validate_utf8_text(fifo->buf, fifo->rlen);
        g_printf("CLIP not set, discarding '%s'\n",fifo->buf);
        fifo->rlen=0;
        break;
    }
  }
  ptext=update_clipboard(primary, NULL, H_MODE_CHECK);
  ctext=update_clipboard(clipboard, NULL, H_MODE_CHECK);

  if(NULL==ptext && NULL ==ctext) return;
  last=update_clipboard(NULL, NULL, H_MODE_LAST);
  if( NULL != last && 0 != p_strcmp(ptext,ctext)){
    /**last is a copy, of things that may be deallocated  */
    last=strdup(last);
    update_clipboards(last, H_MODE_LIST);
    g_free(last);
  }
}

/* Called every CHECK_INTERVAL seconds to check for new items */
gboolean check_clipboards_tic(gpointer data)
{
  check_clipboards(H_MODE_CHECK);
  return TRUE;
}

/* Startup calls and initializations */
static void parcellite_init()
{
/* Create clipboard */
  primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

  if(FALSE ==g_thread_supported()){
    g_printf("g_thread not init!\n");
  }
  clip_lock= g_mutex_new();
  hist_lock= g_mutex_new();
  g_mutex_unlock(clip_lock);

  g_timeout_add(CHECK_INTERVAL, check_clipboards_tic, NULL);
}


/* which - which fifo we write to. */
void write_stdin(struct p_fifo *fifo, int which)
{
  if (!isatty(fileno(stdin)))   {
    GString* piped_string = g_string_new(NULL);
    /* Append stdin to string */
    while (1)    {
      gchar* buffer = (gchar*)g_malloc(256);
      if (fgets(buffer, 256, stdin) == NULL)  {
        g_free(buffer);
        break;
      }
      g_string_append(piped_string, buffer);
      g_free(buffer);
    }
    /* Check if anything was piped in */
    if (piped_string->len > 0) {
      /* Truncate new line character */
      /* Copy to clipboard */
     write_fifo(fifo,which,piped_string->str,piped_string->len);
     /*sleep(10); */
      /* Exit */
    }
    g_string_free(piped_string, TRUE);

  }
}

int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);

  fifo=init_fifo();

  parcellite_init();
  gtk_main();

  g_list_free(history_list);
  close_fifos(fifo);
  return 0;
}
