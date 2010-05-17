#ifndef _UI_H_
#define _UI_H_

#include <agar/core.h>
#include <agar/gui.h>

#include "../celestia/celestiacore.h"
#include "sound/mixer.h"

namespace UI
{
    extern bool showUI; //show agar's ui

    void Init();
    void gameQuit(AG_Event *event);
    void syncRenderFromAgar(); // sync render flags and other from agar ui sys
    void syncRenderToAgar(); // sync render flags and other to agar ui sys
    void riseUI(); // Show ui and take focus

    void addSndEvent(AG_Object *, const char event_name, StdUiSounds snd);
}

namespace ContextMenu
{
    // context menu callback
    void menuContext(float x, float y, Selection sel);

    // called when BG gain focus
    void hidePopupMenu();
}

#endif // _UI_H_
