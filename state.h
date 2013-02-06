#include <glib.h>

struct State {
	gchar *buf;
	gint len;		/* space allocated in buf */
	gint rlen;	/* received length  */
	gint which;	/* one of ID_PRIMARY, ID_CLIPBOARD */
	gint dbg;
};

extern struct State state;

void init_state(void);
void close_state(void);
