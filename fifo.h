/* Copyright (C) 2011-2012 by rickyrockrat <gpib at rickyrockrat dot net> */

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define ID_PRIMARY   0
#define ID_CLIPBOARD 1

struct p_fifo {
	int primary_fifo;
	int clipboard_fifo;
	GIOChannel *g_ch_p;
	GIOChannel *g_ch_c;
	gchar *buf;
	gint len;		/* space allocated in buf */
	gint rlen;	/* received length  */
	gint which;	/* one of ID_PRIMARY, ID_CLIPBOARD */
	gint dbg;
};

int proc_find(const char* name, int mode, pid_t *pid);
int create_fifo(void);
int open_fifos(struct p_fifo *fifo);
int read_fifo(struct p_fifo *f, int which);
int write_fifo(struct p_fifo *f, int which, char *buf, int len);
struct p_fifo* init_fifo();
void close_fifos(struct p_fifo *p);

G_END_DECLS
