/* Copyright (C) 2011-2012 by rickyrockrat <gpib at rickyrockrat dot net> */

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <unistd.h>

G_BEGIN_DECLS

extern GMutex *hist_lock;

#define ACTIONS_TAB    2
#define POPUP_DELAY    100
#define CHECK_INTERVAL 500
#define CHECK_APPINDICATOR_INTERVAL 30000 /**check for existence of indicator-appmenu  */
#define ID_PRIMARY   0
#define ID_CLIPBOARD 1

#define HIST_DISPLAY_NORMAL     1
#define HIST_DISPLAY_PERSISTENT	2

#define KBUF_SIZE 200
struct widget_info{
	GtkWidget *menu; /**top level history list window  */
	GtkWidget *item; /**item we are looking at  */
	 GdkEventKey *event; /**event info where we filled this struct  */
	gint index;      /**index into the array  */
	gint tmp1;			 /**gp vars  */
	gint tmp2;
};
/**keeps track of each menu item and the element it's created from.  */
struct s_item_info {
	GtkWidget *item;
	GList *element;
};

struct history_info{
  GtkWidget *menu;			/**top level history menu  */
  GtkWidget *clip_item; /**currently selected history item (represents clipboard)  */
	gchar *element_text;	/**texts of selected history clipboard item  */
  GtkWidget *title_item;
	GList *delete_list; /**struct s_item_info - for the delete list  */
	GList *persist_list; /**struct s_item_info - for the persistent list  */
	struct widget_info wi;  /**temp  for usage in popups  */
	gint histno;           /**which history?  HIST_DISPLAY_NORMAL/HIST_DISPLAY_PERSISTENT*/
	gint change_flag;	/**bit wise flags for history state  */
};

int p_strcmp (const char *str1, const char *str2);
void phistory_hotkey(char *keystring, gpointer user_data);
void history_hotkey(char *keystring, gpointer user_data);
void actions_hotkey(char *keystring, gpointer user_data);
void menu_hotkey(char *keystring, gpointer user_data);
void postition_history(GtkMenu *menu,gint *x,gint *y,gboolean *push_in, gpointer user_data);

G_END_DECLS

G_BEGIN_DECLS

void check_dirs();

struct cmdline_opts *parse_options(int argc, char* argv[]);

#define FIFO_MODE_NONE 0x10
#define FIFO_MODE_PRI  0x20
#define FIFO_MODE_CLI  0x40

#define PROC_MODE_EXACT 1
#define PROC_MODE_STRSTR 2

struct p_fifo {
	int fifo_p;					/**primary fifo  */
	int fifo_c;					/**clipboard fifo  */
	GIOChannel *g_ch_p; /**so we can add watches in the main loop  */
	GIOChannel *g_ch_c;	/**so we can add watches in the main loop  */
	gchar *buf;	 /**the data itself  */
	gint len;		/**total len of buffer (alloc amount)  */
	gint rlen;	/**received lenght  */
	gint which;	/**which clipboard the buffer should be written to  */
	gint dbg;		/**set to debug fifo system  */
};
int proc_find(const char* name, int mode, pid_t *pid);
int create_fifo(void);
int open_fifos(struct p_fifo *fifo);
int read_fifo(struct p_fifo *f, int which);
int write_fifo(struct p_fifo *f, int which, char *buf, int len);
struct p_fifo* init_fifo();
void close_fifos(struct p_fifo *p);

G_END_DECLS
