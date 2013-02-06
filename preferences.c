/* Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com> */

#include "parcellite.h"
#define MAX_HISTORY 1000

#define DEF_SAVE_HISTORY      TRUE
#define DEF_HISTORY_LIMIT     25
#define DEF_CONFIRM_CLEAR     TRUE
#define DEF_SINGLE_LINE       TRUE
#define DEF_REVERSE_HISTORY   FALSE
#define DEF_ITEM_LENGTH       50
#define DEF_ELLIPSIZE         2
#define DEF_PHISTORY_KEY      "<Ctrl><Alt>X"
#define DEF_HISTORY_KEY       "<Ctrl><Alt>H"
#define DEF_ACTIONS_KEY       "<Ctrl><Alt>A"
#define DEF_MENU_KEY          "<Ctrl><Alt>P"

/**allow lower nibble to become the number of items of this type  */
#define PREF_TYPE_TOGGLE 0x10
#define PREF_TYPE_SPIN   0x20
#define PREF_TYPE_COMBO  0x30
#define PREF_TYPE_ENTRY  0x40 /**gchar *  */
#define PREF_TYPE_ALIGN  0x50 /**label, then align box  */
#define PREF_TYPE_SPACER 0x60
#define PREF_TYPE_MASK   0xF0
#define PREF_TYPE_NMASK  0xF
#define PREF_TYPE_SINGLE_LINE 1

#define PREF_SEC_NONE 0
#define PREF_SEC_CLIP 1
#define PREF_SEC_HIST 2
#define PREF_SEC_MISC 3
#define PREF_SEC_DISP 4
#define PREF_SEC_ACT  5

struct myadj {
  gdouble lower;
  gdouble upper;
  gdouble step;
  gdouble page;
};

struct myadj align_hist_xy={1,0,10,100};
struct myadj align_data_lim={0,1000,1,10};
struct myadj align_hist_lim={5, MAX_HISTORY, 1, 10};
struct myadj align_line_lim={5, 100, 1, 5};
struct pref_item {
  gchar *name;    /**name/id to find pref  */
  gint32 val;     /**int val  */
  gchar *cval;   /**char val  */
  GtkWidget *w;  /**widget in menu  */
  gint type;    /**PREF_TYPE_  */
  gchar *desc; /**shows up in menu  */
  gchar *tip; /**tooltip */
  gint sec; /**cliboard,history, misc,display,hotkeys  */
  gchar *sig;      /**signal, if any  */
  GCallback sfunc; /**function to call  */
  struct myadj *adj;
};
static struct pref_item dummy[2];

struct pref_item myprefs[]={
/**Behaviour  */
  /**History  */
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="save_history",.type=PREF_TYPE_TOGGLE,.desc="Save history",.tip="Save history to a file.",.val=DEF_SAVE_HISTORY},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="history_pos",.type=PREF_TYPE_TOGGLE|PREF_TYPE_SINGLE_LINE,.desc="Position history",.tip="If checked, use X, Y to position the history list",.val=0},
  {.adj=&align_hist_xy,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="history_x",.type=PREF_TYPE_SPIN|PREF_TYPE_SINGLE_LINE,.desc="<b>X</b>",.tip=NULL,.val=1},
  {.adj=&align_hist_xy,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="history_y",.type=PREF_TYPE_SPIN|PREF_TYPE_SINGLE_LINE,.desc="<b>Y</b>",.tip="Position in pixels from the top of the screen",.val=1},
  {.adj=&align_hist_lim,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="history_limit",.type=PREF_TYPE_SPIN,.desc="Items in history",.tip="Maximun number of cliboard entries to keep",.val=DEF_HISTORY_LIMIT},
  {.adj=&align_data_lim,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="data_size",.type=PREF_TYPE_SPIN,.desc="Max Data Size(MB)",.tip="Maximum data size of entire history list",.val=0},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="nop",.type=PREF_TYPE_SPACER,.desc=" ",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="automatic_paste",.type=PREF_TYPE_TOGGLE|PREF_TYPE_SINGLE_LINE,.desc="Auto Paste",.tip="If checked, will use xdotool to paste wherever the mouse is.",.val=0},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="auto_key",.type=PREF_TYPE_TOGGLE|PREF_TYPE_SINGLE_LINE,.desc="Key",.tip="If checked, will use Ctrl-V paste.",.val=0},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_HIST,.name="auto_mouse",.type=PREF_TYPE_TOGGLE|PREF_TYPE_SINGLE_LINE,.desc="Mouse",.tip="If checked, will use middle mouse to paste.",.val=1},

  /**Miscellaneous  */
/**Display  add icon here...*/
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="current_on_top",.type=PREF_TYPE_TOGGLE,.desc="Current entry on top",.tip="If checked, places current clipboard entry at top of list. If not checked, history does not get sorted.",.val=TRUE},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="single_line",.type=PREF_TYPE_TOGGLE,.desc="Show in a single line",.tip=NULL,.val=DEF_SINGLE_LINE},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="reverse_history",.type=PREF_TYPE_TOGGLE,.desc="Show in reverse order",.tip="If checked, the current entry will be on the bottom instead of the top.",.val=DEF_REVERSE_HISTORY},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="nop",.type=PREF_TYPE_SPACER,.desc=" ",.tip=NULL},
  {.adj=&align_line_lim,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="item_length",.type=PREF_TYPE_SPIN,.desc="  Character length of items",.tip=NULL,.val=DEF_ITEM_LENGTH},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="nop",.type=PREF_TYPE_SPACER,.desc=" ",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="persistent_history",.type=PREF_TYPE_TOGGLE,.desc="Persistent History",.tip="If checked, enables the persistent history.",.val=FALSE},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="persistent_separate",.type=PREF_TYPE_TOGGLE,.desc="Persistent As Separate List",.tip="If checked, puts the persistent history in a new list.",.val=FALSE},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="persistent_on_top",.type=PREF_TYPE_TOGGLE,.desc="Persistent On Top",.tip="If checked, puts the persistent history at the top of the history list.",.val=FALSE},
  {.adj=NULL,.cval="\\n",.sig=NULL,.sec=PREF_SEC_DISP,.name="persistent_delim",.type=PREF_TYPE_ENTRY,.desc="Paste All Entry Delimiter",.tip="This string will be inserted between each line of history for paste all."},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_DISP,.name="nop",.type=PREF_TYPE_SPACER,.desc=" ",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_NONE,.name="ellipsize",.type=PREF_TYPE_COMBO,.desc="Omit items in the:",.tip=NULL,.val=DEF_ELLIPSIZE},

/**Action Keys  */
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_ACT,.name="history_key",.type=PREF_TYPE_ENTRY,.desc="History key combination:",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_ACT,.name="phistory_key",.type=PREF_TYPE_ENTRY,.desc="Persistent history key:",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_ACT,.name="actions_key",.type=PREF_TYPE_ENTRY,.desc="Actions key combination:",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_ACT,.name="menu_key",.type=PREF_TYPE_ENTRY,.desc="Menu key combination",.tip=NULL},
  {.adj=NULL,.cval=NULL,.sig=NULL,.sec=PREF_SEC_NONE,.name=NULL},
};

struct pref_item* get_pref(char *name)
{
  int i;
  for (i=0;NULL != myprefs[i].name; ++i){
    if(!g_strcmp0(myprefs[i].name,name))
      return &myprefs[i];
  }
  return &dummy[0];
}

gint32 get_pref_int32 (char *name)
{
  struct pref_item *p=get_pref(name);
  if(NULL == p)
    return -1;
  return p->val;
}
