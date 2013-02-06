/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#include "parcellite.h"

GList* history_list=NULL;

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

void log_clipboard() {
  check_dirs();
  static gchar* history_path = NULL;
  if (!history_path)
      history_path = g_build_filename(g_get_user_data_dir(), "clipboard_log", NULL);
  FILE* history_file = fopen(history_path, "a");
  if (!history_file) return;
  /* Write most recent element */
  fprintf(history_file, "%s: %s\n", "AA: ",
      ((struct history_item*)history_list->data)->text);
  fclose(history_file);
}

struct history_item *new_clip_item(gint type, guint32 len, void *data)
{
  struct history_item *c;
  if(NULL == (c=g_malloc0(sizeof(struct history_item)+len))){
    printf("Hit NULL for malloc of history_item!\n");
    return NULL;
  }

  c->type=type;
  memcpy(c->text,data,len);
  c->len=len;
  return c;
}

/* checks to see if text is already in history. Also is a find text
 * Arguments: if mode is 1, delete it too.
 * Returns: -1 if not found, or nth element. */
gint is_duplicate(gchar* item, int mode, gint *flags)
{
  GList* element;
  gint i;
  if(NULL ==item)
    return -1;
  /* Go through each element compare each */
  for (i=0,element = history_list; element != NULL; element = element->next,++i) {
    struct history_item *c;
    c=(struct history_item *)element->data;
    if(CLIP_TYPE_TEXT == c->type){
      if (g_strcmp0((gchar*)c->text, item) == 0) {
        if(mode){
          if(NULL != flags && (CLIP_TYPE_PERSISTENT&c->flags)){
            *flags=c->flags;
          }
          g_free(element->data);
          history_list = g_list_delete_link(history_list, element);
        }
        return i;
        break;
      }
    }
  }
  return -1;
}

void append_item(gchar* item)
{
  gint flags=0,node=-1;
  if(NULL == item)
    return;
  g_mutex_lock(hist_lock);

  struct history_item *c;
  if(NULL == (c=new_clip_item(CLIP_TYPE_TEXT,strlen(item),item)) )
    return;

  history_list = g_list_prepend(history_list, c);
  log_clipboard();
  g_mutex_unlock(hist_lock);
}
