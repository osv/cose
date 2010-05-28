// Copyright (C) 2010 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "ui.h"
#include "cgame.h"

namespace ContextMenu
{

    static AG_PopupMenu *popup = NULL;

    void hidePopupMenu()
    {
        if (popup)
            AG_PopupHide(popup);
    }

    static void action(AG_Event *event)
    {
    }

    static void ContextualMenu(AG_Event *event)
    {
        AG_MenuItem *item;
        AG_MenuItem *mi = (AG_MenuItem *) AG_SENDER();
        item = AG_MenuNode(mi, "...", NULL);
        {
            AG_MenuAction(item, "...", NULL, action, NULL);
        }
    }

    /* ENTRY: Context menu (event handled by appCore)
     *        Normally, float x and y, but unused, so removed. 
     */
    void menuContext(float x, float y, Selection sel)
    {
        AG_Menu *menu;

        BG_LostFocus();

        if (!popup)
        {
            popup = AG_PopupNew(agAppMenuWin);
            AG_MenuSetPollFn(popup->item, ContextualMenu, NULL);
        }
        AG_PopupShow(popup);
    }
}
