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

/*  Adds item to the end of history. */
void append_item(gchar* item, int checkdup)
{
  gint flags=0,node=-1;
  if(NULL == item)
    return;
  g_mutex_lock(hist_lock);
/**delete if HIST_DEL flag is set.  */
  if( checkdup & HIST_CHECKDUP){
    node=is_duplicate(item, checkdup & HIST_DEL, &flags);
    if(node > -1){ /**found it  */
      if(!(checkdup & HIST_DEL))
        return;
    }
  }

  struct history_item *c;
  if(NULL == (c=new_clip_item(CLIP_TYPE_TEXT,strlen(item),item)) )
    return;
  if(node > -1 && (checkdup & HIST_KEEP_FLAGS) ){
    c->flags=flags;
  }

   /* Prepend new item */
  history_list = g_list_prepend(history_list, c);
  /* Shorten history if necessary */
  truncate_history();
  g_mutex_unlock(hist_lock);
}

/* Truncates history to history_limit items, while preserving persistent data,
 * if specified by the user. FIXME: This may not shorten the history */
void truncate_history()
{
  int p=get_pref_int32("persistent_history");
  if (history_list)  {
    guint ll=g_list_length(history_list);
    guint lim=get_pref_int32("history_limit");
    if(ll > lim){ /* Shorten history if necessary */
      GList* last = g_list_last(history_list);
      while (last->prev && ll>lim)   {
        struct history_item *c=(struct history_item *)last->data;
        last=last->prev;
        if(!p || !(c->flags&CLIP_TYPE_PERSISTENT)){
          history_list=g_list_remove(history_list,c);
          --ll;
        }
      }
    }

    log_clipboard();
  }
}

/* Returns pointer to last item in history */
gpointer get_last_item()
{
  if (history_list)
  {
    if (history_list->data)
    {
      /* Return the last element */
      gpointer last_item = history_list->data;
      return last_item;
    }
    else
      return NULL;
  }
  else
    return NULL;
}
