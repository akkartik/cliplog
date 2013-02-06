/* Copyright (C) 2002  Red Hat, Inc.
 * Developed by Havoc Pennington */

#ifndef __EGG_ACCELERATORS_H__
#define __EGG_ACCELERATORS_H__

#include <gtk/gtkaccelgroup.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

/* Where a value is also in GdkModifierType we coincide,
 * otherwise we don't overlap.
 */
typedef enum
{
  EGG_VIRTUAL_SHIFT_MASK    = 1 << 0,
  EGG_VIRTUAL_LOCK_MASK	    = 1 << 1,
  EGG_VIRTUAL_CONTROL_MASK  = 1 << 2,

  EGG_VIRTUAL_ALT_MASK      = 1 << 3, /* fixed as Mod1 */

  EGG_VIRTUAL_MOD2_MASK	    = 1 << 4,
  EGG_VIRTUAL_MOD3_MASK	    = 1 << 5,
  EGG_VIRTUAL_MOD4_MASK	    = 1 << 6,
  EGG_VIRTUAL_MOD5_MASK	    = 1 << 7,

#if 0
  GDK_BUTTON1_MASK  = 1 << 8,
  GDK_BUTTON2_MASK  = 1 << 9,
  GDK_BUTTON3_MASK  = 1 << 10,
  GDK_BUTTON4_MASK  = 1 << 11,
  GDK_BUTTON5_MASK  = 1 << 12,
  /* 13, 14 are used by Xkb for the keyboard group */
#endif

  EGG_VIRTUAL_META_MASK = 1 << 24,
  EGG_VIRTUAL_SUPER_MASK = 1 << 25,
  EGG_VIRTUAL_HYPER_MASK = 1 << 26,
  EGG_VIRTUAL_MODE_SWITCH_MASK = 1 << 27,
  EGG_VIRTUAL_NUM_LOCK_MASK = 1 << 28,
  EGG_VIRTUAL_SCROLL_LOCK_MASK = 1 << 29,

  /* Also in GdkModifierType */
  EGG_VIRTUAL_RELEASE_MASK  = 1 << 30,

  /*     28-31 24-27 20-23 16-19 12-15 8-11 4-7 0-3
   *       7     f     0     0     0    0    f   f
   */
  EGG_VIRTUAL_MODIFIER_MASK = 0x7f0000ff

} EggVirtualModifierType;

gboolean egg_accelerator_parse_virtual        (const gchar            *accelerator,
                                               guint                  *accelerator_key,
                                               EggVirtualModifierType *accelerator_mods);
void     egg_keymap_resolve_virtual_modifiers (GdkKeymap              *keymap,
                                               EggVirtualModifierType  virtual_mods,
                                               GdkModifierType        *concrete_mods);
void     egg_keymap_virtualize_modifiers      (GdkKeymap              *keymap,
                                               GdkModifierType         concrete_mods,
                                               EggVirtualModifierType *virtual_mods);

gchar* egg_virtual_accelerator_name (guint                  accelerator_key,
                                     EggVirtualModifierType accelerator_mods);

G_END_DECLS


#endif /* __EGG_ACCELERATORS_H__ */
