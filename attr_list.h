/* Copyright (C) 2011-2013 rickyrockrat <gpib at rickyrockrat dot net> */

#ifndef _ATTR_LIST_H_
#define  _ATTR_LIST_H_ 1
/**for add/find/remove from list  */
#define OPERATE_DELETE 1 /**delete_list  */
#define OPERATE_PERSIST 2	/**persist_list  */

PangoAttrInt *get_underline_attr(GtkLabel *label);
gboolean is_underline(GtkLabel *label);
void set_underline(GtkLabel *label, gboolean mode);
PangoAttrInt *get_strikethrough_attr(GtkLabel *label);
gboolean is_strikethrough(GtkLabel *label);
void set_strikethrough(GtkLabel *label, gboolean mode);
GList *find_h_item(GList *list,GtkWidget *w, GList *e);
void add_h_item(struct history_info *h, GtkWidget *w, GList* element, gint which);
void rm_h_item(struct history_info *h, GtkWidget *w, GList* element, gint which);
void handle_marking(struct history_info *h, GtkWidget *w, gint index, gint which);
#endif

