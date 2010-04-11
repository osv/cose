#include <string.h>
#include <math.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <time.h>
#include <unistd.h>

#include "cgame.h" 
#include "ui.h"
#include "geekconsole.h"
#include "configfile.h"

#include <agar/core/types.h>
#include "ui_theme.h"

using namespace std;

GameCore *appGame = NULL;
CelestiaCore *celAppCore = NULL;

static bool fullscreen = false;
static bool ready = false;
static bool bgFocuse = false;
static AG_Timeout toRepeat;     /* Repeat timer */
static AG_Timeout toDelay;      /* Pre-repeat delay timer */

AG_Key repeatKey;               /* Last key */

float mProjection[16];          /* Projection matrix to load for 'background' */
float mModelview[16];           /* Modelview matrix to load for 'background' */
float mTexture[16];             /* Texture matrix to load for 'background' */

int curFPS = 0; 				/* Measured frame rate */

/* Stick main menu (f. e. when videocapture on) 
 * You can set it in any time
 */
bool agMainMenuSticky=false;

std::string startFile;

static bool leftButton = false;
static bool middleButton = false;
static bool rightButton = false;


// Mouse wheel button assignments
#define MOUSE_WHEEL_UP   3
#define MOUSE_WHEEL_DOWN 4

static void ToggleFullscreen();
static bool handleSpecialKey(int key, int state, bool down);
static void BG_Display(void);

static void startTogVidRecord()
{
    if (celAppCore->isCaptureActive())
    {
        if (celAppCore->isRecording())
        {
            celAppCore->recordPause();
            agMainMenuSticky=false;
        }
        else
        {
            celAppCore->recordBegin();
            agMainMenuSticky=true;
        }
    }
}

static void stopVidRecord()
{
    if (celAppCore->isCaptureActive())
        celAppCore->recordEnd();
    agMainMenuSticky=false;
}


static void ToggleFullscreen()
{
    // todo: fullscreen toggle: may use AG_ResizeDisplay but need flag setups
    // SDL_Event vexp;

    // fullscreen = !fullscreen;
    // if ((fullscreen && (agView->v->flags & SDL_FULLSCREEN) == 0) ||
    //     (!fullscreen && (agView->v->flags & SDL_FULLSCREEN))) {
    //     SDL_WM_ToggleFullScreen(agView->v);
    //     vexp.type = SDL_VIDEOEXPOSE;
    //     SDL_PushEvent(&vexp);       
    // }
}

static void registerAndBindKeys()
{
    geekConsole->registerFunction(GCFunc(startTogVidRecord), "toggle video record");
    geekConsole->registerFunction(GCFunc(stopVidRecord), "stop video record");
    geekConsole->registerAndBind("", "M-RET", GCFunc(ToggleFullscreen), "toggle fullscreen");
    geekConsole->bind("", "C-x C-g e @Earth@EXEC@goto object@", "select object");

    GeekBind *geekBindCel = geekConsole->createGeekBind("Celestia");
    geekBindCel->bind("C-x C-m", "quit");

    cVarBindInt32("geekconsole.type", &geekConsole->consoleType, geekConsole->consoleType);
}

static void BG_Display(void)
{
    celAppCore->draw();
}

static bool handleSpecialKey(int key, int state, bool down)
{
    int k = -1;
    switch (key)
    {
    case AG_KEY_UP:
        k = CelestiaCore::Key_Up;
        break;
    case AG_KEY_DOWN:
        k = CelestiaCore::Key_Down;
        break;
    case AG_KEY_LEFT:
        k = CelestiaCore::Key_Left;
        break;
    case AG_KEY_RIGHT:
        k = CelestiaCore::Key_Right;
        break;
    case AG_KEY_HOME:
        k = CelestiaCore::Key_Home;
        break;
    case AG_KEY_END:
        k = CelestiaCore::Key_End;
        break;
    case AG_KEY_F1:
        k = CelestiaCore::Key_F1;
        break;
    case AG_KEY_F2:
        k = CelestiaCore::Key_F2;
        break;
    case AG_KEY_F3:
        k = CelestiaCore::Key_F3;
        break;
    case AG_KEY_F4:
        k = CelestiaCore::Key_F4;
        break;
    case AG_KEY_F5:
        k = CelestiaCore::Key_F5;
        break;
    case AG_KEY_F6:
        k = CelestiaCore::Key_F6;
        break;
    case AG_KEY_F7:
        k = CelestiaCore::Key_F7;
        break;
        // case AG_KEY_F10:
        //   todo: Capture Image
        //   break;
    case AG_KEY_F11:
        k = CelestiaCore::Key_F11;
        break;
    case AG_KEY_F12:
        k = CelestiaCore::Key_F12;
        break;
    case AG_KEY_KP0:
        k = CelestiaCore::Key_NumPad0;
        break;
    case AG_KEY_KP1:
        k = CelestiaCore::Key_NumPad1;
        break;
    case AG_KEY_KP2:
        k = CelestiaCore::Key_NumPad2;
        break;
    case AG_KEY_KP3:
        k = CelestiaCore::Key_NumPad3;
        break;
    case AG_KEY_KP4:
        k = CelestiaCore::Key_NumPad4;
        break;
    case AG_KEY_KP5:
        k = CelestiaCore::Key_NumPad5;
        break;
    case AG_KEY_KP6:
        k = CelestiaCore::Key_NumPad6;
        break;
    case AG_KEY_KP7:
        k = CelestiaCore::Key_NumPad7;
        break;
    case AG_KEY_KP8:
        k = CelestiaCore::Key_NumPad8;
        break;
    case AG_KEY_KP9:
        k = CelestiaCore::Key_NumPad9;
        break;
    case AG_KEY_A:
        k = 'a';
        break;
    case AG_KEY_Z:
        k = 'z';
        break;
    }

    if (k >= 0)
    {
        int mod = 0;
        if ((state & AG_KEYMOD_SHIFT) != 0)
            mod |= CelestiaCore::ShiftKey;
        if ((state & AG_KEYMOD_CTRL) != 0)
            mod |= CelestiaCore::ControlKey;

        if (down)
            celAppCore->keyDown(k, mod);
        else
            celAppCore->keyUp(k);
        return (k < 'a' || k > 'z');
    }
    else
    {
        return false;
    }
}


#ifdef MACOSX
static void killLastSlash(char *buf) {
    int i=strlen(buf);
    while (--i && buf[i]!='/') {}
    if (buf[i]=='/') buf[i]=0;
}
static void dirFixup(char *argv0) {
    char *myPath;
    assert(myPath=(char *)malloc(strlen(argv0)+128));
    strcpy(myPath,argv0);
    killLastSlash(myPath);
    killLastSlash(myPath);
    // BEWARE!  GLUT is going to put us here anyways, DO NOT TRY SOMEWHERE ELSE
    // or you will waste your goddamn time like I did
    // damn undocumented shit.
    strcat(myPath,"/Resources");
    chdir(myPath);
    free(myPath);
}
#endif /* MACOSX */

/*
 * Place focus on a Window following a click at the given coordinates.
 * Returns 1 if mouse is over window
 * Note: AG_WindowFocusAtPos can't be used because it not exclude wins with
 * AG_WINDOW_DENYFOCUS flag.
 */
int FocusWindowAt(AG_DriverSw *dsw, int x, int y)
{
	AG_Window *win;

	AG_ASSERT_CLASS(dsw, "AG_Driver:AG_DriverSw:*");
	AG_FOREACH_WINDOW_REVERSE(win, dsw) {
		AG_ObjectLock(win);
		if (!win->visible ||
		    !AG_WidgetArea(win, x,y)) {
			AG_ObjectUnlock(win);
			continue;
		}
		AG_ObjectUnlock(win);
		return (1);
	}
	return (0);
}


static void BG_Resize(unsigned int w, unsigned int h)
{
    glMatrixMode(GL_TEXTURE);   glPushMatrix(); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_ALL_ATTRIB_BITS );

    celAppCore->resize(w, h);

    glPopAttrib();

    glGetFloatv(GL_PROJECTION_MATRIX, mProjection);
    glGetFloatv(GL_MODELVIEW_MATRIX, mModelview);
    glGetFloatv(GL_TEXTURE_MATRIX, mTexture);
    
    glMatrixMode(GL_PROJECTION);    glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(GL_TEXTURE);   glPopMatrix();
}

static void Resize(unsigned int w, unsigned int h)
{
    BG_Resize(w, h);
    if (geekConsole)
        geekConsole->resize(w, h);
}

// resize with menu,
// make sure that main menu not over BG
static void BG_ResizeWithMenu()
{
	Uint wDisp, hDisp, w, h;
	AG_Driver *drv = AGWIDGET(agAppMenuWin)->drv;
    if (!drv)
        return;
	if (AG_GetDisplaySize(drv, &wDisp, &hDisp) == -1) {
		wDisp = 0;
		hDisp = 0;
	}

    if (!AG_WindowIsVisible(agAppMenuWin))
    {
        w = wDisp;
        h = hDisp;
    }
    else
    {
        w = wDisp;
        h = hDisp - (AGWIDGET(agAppMenuWin)->h);
    }
    Resize(w, h);
}

void BG_GainFocus()
{
    bgFocuse = true;
    // hide menu
    if (!agMainMenuSticky)
        AG_WindowHide(agAppMenuWin);
    BG_ResizeWithMenu();
    celAppCore->flashFrame();
    AG_DelTimeout(NULL, &toDelay);
    AG_DelTimeout(NULL, &toRepeat);
    GeekBind *gb = geekConsole->getGeekBind("Celestia");
    if (gb)
        gb->isActive = true;
}

void BG_LostFocus()
{
    bgFocuse = false;
    //show menu
    AG_WindowShow(agAppMenuWin);
    BG_ResizeWithMenu();
    AG_DelTimeout(NULL, &toDelay);
    AG_DelTimeout(NULL, &toRepeat);
    GeekBind *gb = geekConsole->getGeekBind("Celestia");
    if (gb)
        gb->isActive = false;
}

/*
 * Background process event (mouse, key)
 * Returns 1 if the event was processed.
 */
static int BG_ProcessEvent(AG_DriverEvent *dev)
{
	AG_Driver *drv;
    AG_Keyboard *kbd;

	drv = (AG_Driver *) agDriverSw;
    kbd = drv->kbd;
    int rv = 0;
    switch (dev->type) {
    case AG_DRIVER_KEY_DOWN:
    {
        cout << "KEY_DOWN" << "\n";
        UI::syncRenderFromAgar();

        if (!handleSpecialKey(repeatKey.sym, repeatKey.mod, true))
            celAppCore->charEntered((char) repeatKey.uch);
        UI::syncRenderToAgar();
        return 1;
    }
    case AG_DRIVER_KEY_UP:
    {       
        cout << "KEY_UP" << "\n";
        UI::syncRenderFromAgar();

        handleSpecialKey(dev->data.key.ks,
                         drv->kbd->modState, false);
        UI::syncRenderToAgar();
        return 1;
    }   
    case AG_DRIVER_MOUSE_BUTTON_DOWN:
    {
        int x = dev->data.button.x;
        int y = dev->data.button.y;

        UI::syncRenderFromAgar();
        switch (dev->data.button.which) {
        case AG_MOUSE_WHEELUP:
            celAppCore->mouseWheel(-1.0f, 0);
            break;
        case AG_MOUSE_WHEELDOWN:
            celAppCore->mouseWheel(1.0f, 0);
            break;
        case AG_MOUSE_RIGHT:
            rightButton = true;
            celAppCore->mouseButtonDown(x, y, CelestiaCore::RightButton);
            break;
        case AG_MOUSE_LEFT:
            leftButton = true;
            celAppCore->mouseButtonDown(x, y, CelestiaCore::LeftButton);
            break;
        case AG_MOUSE_MIDDLE:
            middleButton = true;
            celAppCore->mouseButtonDown(x, y, CelestiaCore::MiddleButton);
            break;
        default:
            break;
        }
        UI::syncRenderToAgar();
        return 1;
    }
    case AG_DRIVER_MOUSE_BUTTON_UP:
    {
        int x = dev->data.button.x;
        int y = dev->data.button.y;
    
        UI::syncRenderFromAgar();

        switch (dev->data.button.which) {
        case AG_MOUSE_RIGHT:
            if (rightButton)
            {
                rightButton = false;
                celAppCore->mouseButtonUp(x, y, CelestiaCore::RightButton);
            }
            break;
        case AG_MOUSE_LEFT:
            if (leftButton)
            {
                leftButton = false;
                celAppCore->mouseButtonUp(x, y, CelestiaCore::LeftButton);
            }
            break;
        case AG_MOUSE_MIDDLE:
            middleButton = false;
            celAppCore->mouseButtonUp(x, y, CelestiaCore::MiddleButton);
            break;
        default:
            break;
        }
        UI::syncRenderToAgar();
        return 1;
    }
    case AG_DRIVER_MOUSE_MOTION:
    {
        int buttons = 0;
        Uint mod = AG_GetModState(kbd);
        // TODO: use AG_MouseGetState to get mouse state
        if (leftButton)
            buttons |= CelestiaCore::LeftButton;
        if (rightButton)
            buttons |= CelestiaCore::RightButton;
        if (middleButton)
            buttons |= CelestiaCore::MiddleButton;
        if ((mod & AG_KEYMOD_SHIFT) != 0)
            buttons |= CelestiaCore::ShiftKey;
        if ((mod & AG_KEYMOD_CTRL) != 0)
            buttons |= CelestiaCore::ControlKey;

        // autohide main menu (agAppMenuWin) & autoresize BG
        if (UI::showUI && AG_WidgetArea(agAppMenuWin,
                                        dev->data.motion.x,
                                        dev->data.motion.y)) {
            if (!AG_WindowIsVisible(agAppMenuWin))
            {
                AG_WindowShow(agAppMenuWin);
                BG_ResizeWithMenu();
            }
        }
        else
            // resize only when agMainmenu no Sticky
            // to prevent all time resize
            if (AG_WindowIsVisible(agAppMenuWin) && !agMainMenuSticky)
            {
                if (!agMainMenuSticky)
                    AG_WindowHide(agAppMenuWin);
                BG_ResizeWithMenu();
                celAppCore->flashFrame();
            }
        celAppCore->mouseMove(drv->mouse->xRel, drv->mouse->yRel, buttons);
        return 1;
    }

    } //switch
    return (rv);
}

/* key repeater */
static Uint32
RepeatTimeout(void *obj, Uint32 ival, void *arg)
{
    if (geekConsole)
    {
        int mod = 0;
        if (repeatKey.mod & AG_KEYMOD_CTRL)
            mod |=  GeekBind::CTRL;
        if (repeatKey.mod & AG_KEYMOD_SHIFT)
            mod |=  GeekBind::SHIFT;
        if (repeatKey.mod & AG_KEYMOD_ALT)
            mod |=  GeekBind::META;
        if (repeatKey.uch && !geekConsole->charEntered(repeatKey.sym, repeatKey.uch, mod))
            if (!handleSpecialKey(repeatKey.sym, repeatKey.mod, true))
                celAppCore->charEntered((char) repeatKey.uch);
            else
                return 0;
        return (agKbdRepeat);
    }
    return 0;
}

static Uint32
DelayTimeout(void *obj, Uint32 ival, void *arg)
{
    AG_ScheduleTimeout(NULL, &toRepeat, agKbdRepeat);
    return (0);
}

// Analog of AG_SDL_PostEventCallback
//
// Some event  may change focused  window but not be  procceed because
// BG_ProcessEvent be calles instead of AG_ProcessEvent
//
// AGAR-UPGRADE: gui/drv_sdl_common.c: check AG_SDL_PostEventCallback(void *obj) for changes
int My_PostEventCallback()
{
	if (!AG_TAILQ_EMPTY(&agWindowDetachQ)) {
		AG_FreeDetachedWindows();
	}
	if (agWindowToFocus != NULL) {
		AG_WM_CommitWindowFocus(agWindowToFocus);
		agWindowToFocus = NULL;
	}
	return (1);
}

/**
 * Process an AGAR event. Returns 1 if the event was processed in some
 * way, -1 if application is exiting.
 * At first process mouse event:
 *  if LMB down, find focused object (widget), if last
 *        focused object == new focused than send event LMB to him
 *     else
 *        last focused = new focused; 
 */
int CL_ProcessEvent(AG_DriverEvent *dev)
{
    int rv = 0;
    int x,y;
    int mod = 0;
	AG_Driver *drv;

    drv = (AG_Driver *) agDriverSw;
    switch (dev->type) {
    case AG_DRIVER_MOUSE_BUTTON_DOWN:
    {
        x = dev->data.button.x;
        y = dev->data.button.y;

        AG_DriverSw *dsw = agDriverSw;
        if (UI::showUI && FocusWindowAt(dsw, x,y))
        {
            // is focus of backgraund lost
            if (bgFocuse)
                BG_LostFocus();
            /* Forward the event to Agar. */
            return AG_ProcessEvent(NULL, dev);
        }
        else
        {
            if (bgFocuse)
                return BG_ProcessEvent(dev);
            else 
                BG_GainFocus();
        }
        break;
    }
    case AG_DRIVER_KEY_DOWN:
        AG_DelTimeout(NULL, &toRepeat);
        repeatKey.sym = dev->data.key.ks;
        repeatKey.mod = drv->kbd->modState;
        repeatKey.uch = dev->data.key.ucs;

        if (geekConsole)
        {
            if (repeatKey.mod & AG_KEYMOD_CTRL)
                mod |=  GeekBind::CTRL;
            if (repeatKey.mod & AG_KEYMOD_SHIFT)
                mod |=  GeekBind::SHIFT;
            if (repeatKey.mod & AG_KEYMOD_ALT)
                mod |=  GeekBind::META;
            // if repeatUnicode = 0 then just mod key pressed
            if (repeatKey.uch)
            {
                if (geekConsole->charEntered(repeatKey.sym, repeatKey.uch, mod))
                {
                    AG_ScheduleTimeout(NULL, &toDelay, agKbdDelay);
                    return My_PostEventCallback();
                }
            }
        }
        if (bgFocuse || !UI::showUI) // no key repeat if agar focused
            AG_ScheduleTimeout(NULL, &toDelay, agKbdDelay);
        goto def;
    case AG_DRIVER_KEY_UP:
        if (repeatKey.sym == dev->data.key.ks) {
            AG_DelTimeout(NULL, &toRepeat);
            AG_DelTimeout(NULL, &toDelay);
        }
    default:
    def:
        if (!bgFocuse && UI::showUI)
        {
            rv = AG_ProcessEvent(NULL, dev);
        }
        else
        {
            if (BG_ProcessEvent(dev) !=1)
                rv = AG_ProcessEvent(NULL, dev);
            else return 1;
        }
    }
    return (rv);
}

static void BG_Draw()
{
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadMatrixf(mTexture);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(mProjection);
        
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(mModelview);

    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);

    glPushAttrib(GL_ALL_ATTRIB_BITS );
    if (ready)
    {      
        UI::syncRenderFromAgar();
        celAppCore->tick();

        UI::syncRenderToAgar();

        // push state for agar. See agar/gui/view.c:InitGL(
        glEnable(GL_CULL_FACE);
        glShadeModel(GL_SMOOTH);
        BG_Display();
        glShadeModel(GL_FLAT);
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(0,0, 0,0);
        GLenum glewErr = glewInit();
        { 
            if (GLEW_OK != glewErr)
            {
                cerr << "Celestia was unable to initialize OpenGL extensions (error %1). Graphics quality will be reduced\n";
            }
        }
        celAppCore->initRenderer(); 
        glDisable(GL_SCISSOR_TEST);
        // Set the simulation starting time to the current system time
        time_t curtime=time(NULL);
        celAppCore->start((double) curtime / 86400.0 + (double) astro::Date(1970, 1, 1));
#if defined(MACOSX) || defined(__FreeBSD__)
        /* localtime in Darwin is is reentrant only
           equiv to Linux localtime_r()
           should probably port !MACOSX code to use this too, available since
           libc 5.2.5 according to manpage */
        struct tm *temptime=localtime(&curtime);
        celAppCore->setTimeZoneBias(temptime->tm_gmtoff);
        celAppCore->setTimeZoneName(temptime->tm_zone);
#else
        localtime(&curtime); // Only doing this to set timezone as a side effect
        celAppCore->setTimeZoneBias(-timezone);
        celAppCore->setTimeZoneName(tzname[daylight?0:1]);
#endif
        //celAppCore->getRenderer()->setVideoSync(true);
        UI::syncRenderToAgar();

        // LoadLuaGeekConsoleLibrary(celAppCore->celxScript->getState());
        // LoadLuaGeekConsoleLibrary(celAppCore->luaHook->getState());
        // LoadLuaGeekConsoleLibrary(celAppCore->luaSandbox->getState());
        
        if (!startFile.empty())
        {
            cout << "*** Using CEL File: " << startFile << endl;
            celAppCore->runScript(startFile);
        }

        ready = true;

    }
    glPopAttrib();
    
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

}

void MyEventLoop(void)
{
    AG_Window *win;
    Uint32 t1, t2;
    AG_DriverEvent dev;

    t1 = AG_GetTicks();
    for (;;) {
        t2 = AG_GetTicks();

        if (agDriverSw && t2-t1 >= agDriverSw->rNom) {
            /*
             * Case 1: Update the video display.
             */
            AG_LockVFS(&agDrivers);

            /* Render the Agar windows */
            if (agDriverSw) {
                /* With single-window drivers (e.g., sdlfb). */
                AG_BeginRendering(agDriverSw);

                BG_Draw();

                //draw agar's windows
                UI::updateGUIalpha(t2-t1, !bgFocuse);
                if (UI::showUI)
                {
                    AG_FOREACH_WINDOW(win, agDriverSw) {
                        AG_ObjectLock(win);
                        UI::precomputeGUIalpha(AG_WindowIsFocused(win));
                        AG_WindowDraw(win);
                        AG_ObjectUnlock(win);
                    }
                }

                glMatrixMode(GL_TEXTURE);
                glPushMatrix();

                glMatrixMode(GL_PROJECTION);
                glPushMatrix();

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glPushAttrib(GL_ALL_ATTRIB_BITS );

                if (geekConsole)
                    geekConsole->render();
                glPopAttrib();
                glMatrixMode(GL_MODELVIEW);
                glPopMatrix();
                glMatrixMode(GL_TEXTURE);
                glPopMatrix();
                glMatrixMode(GL_PROJECTION);
                glPopMatrix();

                AG_EndRendering(agDriverSw);
                //SDL_GL_SwapBuffers();
            }
            AG_UnlockVFS(&agDrivers);

            t1 = AG_GetTicks();
            curFPS = agDriverSw->rNom - (t1-t2);
            if (curFPS < 1) { curFPS = 1; }

        } else if (AG_PendingEvents(NULL) > 0) {
            /*
             * Case 2: There are events waiting to be processed.
             */
            do {
                /* Retrieve the next queued event. */
                if (AG_GetNextEvent(NULL, &dev) == 1) {
                    if (CL_ProcessEvent(&dev) == -1)
                        return;
                }
            } while (AG_PendingEvents(NULL) > 0);

        } else if (AG_TIMEOUTS_QUEUED()) {
            /*
             * Case 3: There are AG_Timeout(3) callbacks to run.
             */
            AG_ProcessTimeouts(t2);
        } else {
            /*
             * Case 4: Nothing to do, idle.
             */
            AG_Delay(1);
        }
    }
}

void gameTerminate()
{
    saveCfg();
    if (fullscreen)
        ToggleFullscreen();
    if (geekConsole)
        delete geekConsole;
    destroyGCInteractives();
    // memory leak 
    Core::removeAllSolSys();
    AG_ConfigSave();
    freeCfg();
    AG_Quit();
}

static void usage()
{
    printf(
        _("usage: openspace_client  [options]\n"
          "options:\n"
          "   -g, --gamedir game_dir    Select a game directory.\n"
          "   -f, --startfile filename  Select a startup filename.\n"
          "   -r, --reset [force]       Reset saved configuration\n"
          "   -m, --mode mode           Set game mode:\n"
          "                             [game|viewer]\n"
          "   -d, --debug  n            Turn debugging on.\n"
          "   --ag-primitive style      Set GUI primitive style\n"
          "                             [transparent-full|transparent-simple|default]\n"
          "\n"
            ));
}

int main(int argc, char* argv[])
{
    int i;
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);

    // Not ready to render yet
    ready = false;

    GameCore::GameMode gameMode = GameCore::GAME;

    if (AG_InitCore("openspace", 0) == -1) {
        fprintf(stderr, "%s\n", AG_GetError());
        return (1);
    }

    // parse command line
    char *s;
    bool loadCfgFile = true;
    static int32 agPrimitive;
    cVarBindInt32Hex("render.agar.primitive", &agPrimitive, UI::FullTransparent);
    for (i = 1; i < argc; ++i)
    {
        s = argv[i];
        if (*s == '-')
        {
            ++s;
            if (*s == '-')
                ++s;
        }
        if ((!strcmp(s, "g") || !strcmp(s, "gamedir")) && i < argc - 1)
        {
            if (chdir(argv[++i]) == -1)
            {
                cerr << "Cannot chdir to '" << argv[++i] << "'\n";
            }
        }
        else if ((!strcmp(s, "f") || !strcmp(s, "startfile")) && i < argc - 1)
            startFile = argv[++i];
        else if ((!strcmp(s, "d") || !strcmp(s, "debug")) && i < argc - 1)
            SetDebugVerbosity(atoi(argv[++i]));
        else if ((!strcmp(s, "m") || !strcmp(s, "mode")) && i < argc - 1)
        { 
            char *mode=argv[++i];
            if (!strcmp(mode, "viewer"))
                gameMode = GameCore::VIEWER;
            else if (!strcmp(mode, "game"))
                gameMode = GameCore::GAME;
        }
        else if (strcmp(s, "ag-primitive") == 0)
        {
            char *mode=argv[++i];
            if (!strcmp(mode, "default"))
                agPrimitive = UI::Default;
            else if (!strcmp(mode, "transparent-full"))
                agPrimitive = UI::FullTransparent;
            else if (!strcmp(mode, "transparent-simple"))
                agPrimitive = UI::SimpleTransparent;
        }
        else if (strcmp(s, "help") == 0 || strcmp(s, "h") == 0)
        {
            usage();
            exit(0);
        }
        else if (strcmp(s, "reset") == 0 || strcmp(s, "r") == 0)
            loadCfgFile = false;
        else
        {
            printf(_("Bad arg: %s\n"), argv[i]);
            usage();
            exit(0);
        }
    }

    if (loadCfgFile)
        loadCfg();

    /* Pass AG_VIDEO_OPENGL flag to require an OpenGL display. */
    AG_InitGraphics("sdlgl");

    AG_SetVideoResizeCallback(Resize);

    /* repeat key timeouts init */
    AG_SetTimeout(&toRepeat, RepeatTimeout, NULL, 0);
    AG_SetTimeout(&toDelay, DelayTimeout, NULL, 0);
    repeatKey.sym = AG_KEY_NONE;
    repeatKey.mod = 0;
    repeatKey.uch = 0;

    celAppCore = new CelestiaCore();

    // init geek console
    geekConsole = new GeekConsole(celAppCore);

    initGCInteractives(geekConsole);
    initGCStdInteractivsFunctions(geekConsole);

    registerAndBindKeys();

    UI::Init();

    if (celAppCore == NULL)
    {
        cerr << _("Failed to initialize Celestia core.\n");
        return 1;
    }
    if (!celAppCore->initSimulation())
    {
        return 1;
    }
    appGame = new GameCore();
    appGame->setGameMode(gameMode);
    
    AG_SetRefreshRate(160);
    MyEventLoop();
    AG_Destroy();

    return 0;
}

