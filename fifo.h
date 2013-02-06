#include <glib.h>

#define ID_PRIMARY   0
#define ID_CLIPBOARD 1

struct State {
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

extern struct State fifos;

void init_fifos(void);
void close_fifos(void);
