/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#ifndef UTILS_H
#define UTILS_H

G_BEGIN_DECLS
#include "parcellite.h"

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

#endif
