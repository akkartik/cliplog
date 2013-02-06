/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#ifndef PREFERENCES_H
#define PREFERENCES_H

G_BEGIN_DECLS

#define ACTIONS_FILE          "parcellite/actions"
#define FIFO_FILE_C          "parcellite/fifo_c"
#define FIFO_FILE_P          "parcellite/fifo_p"
#define PREFERENCES_FILE      "parcellite/parcelliterc"

/*struct pref_item* get_pref(char *name); */
int get_first_pref(int section);
int init_pref( void );
int set_pref_widget (char *name, GtkWidget *w);
GtkWidget *get_pref_widget (char *name);
gint32 set_pref_int32(char *name, gint32 val);
gint32 get_pref_int32 (char *name);
int set_pref_string (char *name, char *string);
gchar *get_pref_string (char *name);

void read_preferences();

void show_preferences(gint tab);

G_END_DECLS

#endif
