#include "ui.h"
#include "cgame.h"

namespace ContextMenu
{
    static void lostFocus(AG_Event *event)
    {
	AG_Window *contxMenu = (AG_Window *)AG_SELF();
	AG_ViewDetach(contxMenu);
    }

    static void action(AG_Event *event)
    {
    }
    /* ENTRY: Context menu (event handled by appCore)
     *        Normally, float x and y, but unused, so removed. 
     */
    void menuContext(float x, float y, Selection sel)
    {
	int w;
	AG_Window *win;
	AG_Menu *menu;
	AG_MenuItem *item;
	    
	BG_LostFocus();

	win = AG_WindowNew(AG_WINDOW_NOMINIMIZE|AG_WINDOW_NOMAXIMIZE);
	AG_TextSize("X",&w, NULL);

//	    AG_WindowSetPadding(win, 0, 0, 0, 0);

	AG_TextboxNew(win, 0, "St: ");
	menu = AG_MenuNew(win, 0);
	item = AG_MenuAddItem(menu, "goto");
	{
	    AG_MenuAction(item, "Load", NULL, action, NULL);
	    AG_MenuAction(item, "Save", NULL, action, NULL);
	}
	menu = AG_MenuNew(win, 0);
	item = AG_MenuAddItem(menu, "select");
	{
	    AG_MenuAction(item, "Load", NULL, action, NULL);
	    AG_MenuAction(item, "Save", NULL, action, NULL);
	}
	    
	AG_WindowSetGeometry(win, x, y, -1, -1);
	AG_WindowShow(win);
	AG_ExpandVert(menu);
//	    AG_AddEvent(contxMenu, "window-lostfocus", lostFocus, NULL);
//	    AG_SetEvent(contxMenu, "window-mousebuttondown", mouseButtonDown, "%p", contxMenu);
    }
}
