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
