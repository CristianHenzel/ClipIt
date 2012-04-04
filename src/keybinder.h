/* keybinder.h
 * Developed by Alex Graveley for Tomboy
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __KEY_BINDER_H__
#define __KEY_BINDER_H__

#include <glib.h>

G_BEGIN_DECLS

typedef void (* BindkeyHandler) (char *keystring, gpointer user_data);

void keybinder_init   (void);

void keybinder_bind   (const char           *keystring,
			      BindkeyHandler  handler,
			      gpointer              user_data);

void keybinder_unbind (const char           *keystring,
			      BindkeyHandler  handler);

gboolean keybinder_is_modifier (guint keycode);

guint32 keybinder_get_current_event_time (void);

G_END_DECLS

#endif /* __KEY_BINDER_H__ */

