/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#ifndef HISTORY_H
#define HISTORY_H

G_BEGIN_DECLS

#define HISTORY_FILE "parcellite/history"

#define CLIP_TYPE_TEXT       0x1
#define CLIP_TYPE_IMG        0x2
#define CLIP_TYPE_PERSISTENT 0x4

#define HIST_DEL 0x10
#define HIST_CHECKDUP 1
#define HIST_KEEP_FLAGS 2
/**give us a genric struct for future expansion  */
struct history_gen{ /**16 bytes, for overlay over res, below.  */
	gint16 a;
	gint16 b;
	gint16 c;
	gint16 d;
}__attribute__((__packed__));
struct history_item {
	guint32 len; /**length of data item, MUST be first in structure  */
	gint16 type; /**currently, text or image  */
	gint16 flags;	/**persistence, or??  */
	guint32 res[4];
	gchar text[8]; /**reserve 64 bits (8 bytes) for pointer to data.  */
}__attribute__((__packed__));


extern GList* history_list;

glong validate_utf8_text(gchar *text, glong len);

void read_history();

void save_history();

struct history_item *new_clip_item(gint type, guint32 len, void *data);
gint is_duplicate(gchar* item, int mode,int *type);

void append_item(gchar* item, int checkdup);

void truncate_history();

gpointer get_last_item();

void delete_duplicate(gchar* item);

void history_save_as(GtkMenuItem *menu_item, gpointer user_data);
G_END_DECLS

#endif