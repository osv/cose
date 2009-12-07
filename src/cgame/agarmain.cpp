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
#include "ui_theme.h"
#include "geekconsole.h"
#include "configfile.h"

using namespace std;

GameCore *appGame = NULL;
CelestiaCore *celAppCore = NULL;

static bool fullscreen = false;
static bool ready = false;
static bool bgFocuse = false;
static AG_Timeout toRepeat;     /* Repeat timer */
static AG_Timeout toDelay;      /* Pre-repeat delay timer */

SDLKey repeatKey;               /* Last keysym */
SDLMod repeatMod;               /* Last keymod */
Uint32 repeatUnicode;           /* Last unicode translated key */

float mProjection[16];          /* Projection matrix to load for 'background' */
float mModelview[16];           /* Modelview matrix to load for 'background' */
float mTexture[16];             /* Texture matrix to load for 'background' */

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
    SDL_Event vexp;

    fullscreen = !fullscreen;
    if (agView == NULL)
        return;
    if ((fullscreen && (agView->v->flags & SDL_FULLSCREEN) == 0) ||
        (!fullscreen && (agView->v->flags & SDL_FULLSCREEN))) {
        SDL_WM_ToggleFullScreen(agView->v);
        vexp.type = SDL_VIDEOEXPOSE;
        SDL_PushEvent(&vexp);       
    }
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
    case SDLK_UP:
        k = CelestiaCore::Key_Up;
        break;
    case SDLK_DOWN:
        k = CelestiaCore::Key_Down;
        break;
    case SDLK_LEFT:
        k = CelestiaCore::Key_Left;
        break;
    case SDLK_RIGHT:
        k = CelestiaCore::Key_Right;
        break;
    case SDLK_HOME:
        k = CelestiaCore::Key_Home;
        break;
    case SDLK_END:
        k = CelestiaCore::Key_End;
        break;
    case SDLK_F1:
        k = CelestiaCore::Key_F1;
        break;
    case SDLK_F2:
        k = CelestiaCore::Key_F2;
        break;
    case SDLK_F3:
        k = CelestiaCore::Key_F3;
        break;
    case SDLK_F4:
        k = CelestiaCore::Key_F4;
        break;
    case SDLK_F5:
        k = CelestiaCore::Key_F5;
        break;
    case SDLK_F6:
        k = CelestiaCore::Key_F6;
        break;
    case SDLK_F7:
        k = CelestiaCore::Key_F7;
        break;
        // case SDLK_F10:
        //   todo: Capture Image
        //   break;
    case SDLK_F11:
        k = CelestiaCore::Key_F11;
        break;
    case SDLK_F12:
        k = CelestiaCore::Key_F12;
        break;
    case SDLK_KP0:
        k = CelestiaCore::Key_NumPad0;
        break;
    case SDLK_KP1:
        k = CelestiaCore::Key_NumPad1;
        break;
    case SDLK_KP2:
        k = CelestiaCore::Key_NumPad2;
        break;
    case SDLK_KP3:
        k = CelestiaCore::Key_NumPad3;
        break;
    case SDLK_KP4:
        k = CelestiaCore::Key_NumPad4;
        break;
    case SDLK_KP5:
        k = CelestiaCore::Key_NumPad5;
        break;
    case SDLK_KP6:
        k = CelestiaCore::Key_NumPad6;
        break;
    case SDLK_KP7:
        k = CelestiaCore::Key_NumPad7;
        break;
    case SDLK_KP8:
        k = CelestiaCore::Key_NumPad8;
        break;
    case SDLK_KP9:
        k = CelestiaCore::Key_NumPad9;
        break;
    case SDLK_a:
        k = 'a';
        break;
    case SDLK_z:
        k = 'z';
        break;
    }

    if (k >= 0)
    {
        int mod = 0;
        if ((state & KMOD_SHIFT) != 0)
            mod |= CelestiaCore::ShiftKey;
        if ((state & KMOD_CTRL) != 0)
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
 * Returns 1 if the focus state has changed as a result.
 * Note: AG_WindowFocusAtPos can't be used because it exclude wins with
 * AG_WINDOW_DENYFOCUS flag.
 */
int
FocusWindowAt(int x, int y)
{
    AG_Window *win;

    AG_TAILQ_FOREACH_REVERSE(win, &agView->windows, ag_windowq, windows) {
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
    int w;
    int h;
    if (!AG_WindowIsVisible(agAppMenuWin))
    {
        w = agView->w;
        h = agView->h;
    }
    else
    {
        w = agView->w;
        h = agView->h - (AGWIDGET(agAppMenuWin)->h);
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
static int BG_ProcessEvent(SDL_Event *ev)
{
    int rv = 0;
    switch (ev->type) {
    case SDL_KEYDOWN:
    {
        UI::syncRenderFromAgar();

        repeatKey = ev->key.keysym.sym;
        repeatMod = ev->key.keysym.mod;
        repeatUnicode = ev->key.keysym.unicode;
        if (!handleSpecialKey(repeatKey, repeatMod, true))
            celAppCore->charEntered((char) repeatUnicode);
        UI::syncRenderToAgar();
        return 1;
    }
    case SDL_KEYUP:
    {       
        UI::syncRenderFromAgar();

        SDLKey sym = ev->key.keysym.sym;
        SDLMod mod = ev->key.keysym.mod;

        handleSpecialKey(sym, mod, false);
        UI::syncRenderToAgar();
        return 1;
    }   
    case SDL_MOUSEBUTTONDOWN:
    {
        int x = ev->button.x;
        int y = ev->button.y;

        UI::syncRenderFromAgar();
        switch (ev->button.button) {
        case SDL_BUTTON_WHEELUP:
            celAppCore->mouseWheel(-1.0f, 0);
            break;
        case SDL_BUTTON_WHEELDOWN:
            celAppCore->mouseWheel(1.0f, 0);
            break;
        case SDL_BUTTON_RIGHT:
            rightButton = true;
            celAppCore->mouseButtonDown(x, y, CelestiaCore::RightButton);
            break;
        case SDL_BUTTON_LEFT:
            leftButton = true;
            celAppCore->mouseButtonDown(x, y, CelestiaCore::LeftButton);
            break;
        case SDL_BUTTON_MIDDLE:
            middleButton = true;
            celAppCore->mouseButtonDown(x, y, CelestiaCore::MiddleButton);
            break;
        }
        UI::syncRenderToAgar();
        return 1;
    }
    case SDL_MOUSEBUTTONUP:
    {
        int x = ev->button.x;
        int y = ev->button.y;
    
        UI::syncRenderFromAgar();

        switch (ev->button.button) {
        case SDL_BUTTON_RIGHT:
            if (rightButton)
            {
                rightButton = false;
                celAppCore->mouseButtonUp(x, y, CelestiaCore::RightButton);
            }
            break;
        case SDL_BUTTON_LEFT:
            if (leftButton)
            {
                leftButton = false;
                celAppCore->mouseButtonUp(x, y, CelestiaCore::LeftButton);
            }
            break;
        case SDL_BUTTON_MIDDLE:
            middleButton = false;
            celAppCore->mouseButtonUp(x, y, CelestiaCore::MiddleButton);
            break;
        }
        UI::syncRenderToAgar();
        return 1;
    }
    case SDL_MOUSEMOTION:
    {
        int buttons = ev->motion.state;
        SDLMod mod = SDL_GetModState();
        if (leftButton)
            buttons |= CelestiaCore::LeftButton;
        if (rightButton)
            buttons |= CelestiaCore::RightButton;
        if (middleButton)
            buttons |= CelestiaCore::MiddleButton;
        if ((mod & KMOD_SHIFT) != 0)
            buttons |= CelestiaCore::ShiftKey;
        if ((mod & KMOD_CTRL) != 0)
            buttons |= CelestiaCore::ControlKey;

        // autohide main menu (agAppMenuWin) & autoresize BG
        if (UI::showUI && AG_WidgetArea(agAppMenuWin, ev->motion.x, ev->motion.y)){
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
        celAppCore->mouseMove(ev->motion.xrel, ev->motion.yrel, buttons);
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
        if (repeatMod & KMOD_CTRL)
            mod |=  GeekBind::CTRL;
        if (repeatMod & KMOD_SHIFT)
            mod |=  GeekBind::SHIFT;
        if (repeatMod & KMOD_ALT)
            mod |=  GeekBind::META;
        if (repeatUnicode && !geekConsole->charEntered(repeatKey, repeatUnicode, mod))
            if (!handleSpecialKey(repeatKey, repeatMod, true))
                celAppCore->charEntered((char) repeatUnicode);
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

/**
 * Process an SDL event. Returns 1 if the event was processed in some
 * way, -1 if application is exiting.
 * At first process mouse event:
 *  if LMB down, find focused object (widget), if last
 *        focused object == new focused than send event LMB to him
 *     else
 *        last focused = new focused; 
 */
int CL_ProcessEvent(SDL_Event *ev)
{
    int rv = 0;
    int x,y;
    int mod = 0;
    switch (ev->type) {
    case SDL_MOUSEBUTTONDOWN:
        x = ev->button.x;
        y = ev->button.y;
        AG_LockVFS(agView);
        
#ifdef OPENGL_INVERTED_Y
        if (agView->opengl)
            y = agView->h - y;
#endif
        if (UI::showUI && FocusWindowAt(x,y))
        {
            AG_UnlockVFS(agView);
            // is focus of backgraund lost
            if (bgFocuse)
                BG_LostFocus();
            return AG_ProcessEvent(ev);
        }
        else
        {
            if (bgFocuse)
                return BG_ProcessEvent(ev);
            else 
                BG_GainFocus();
        }
        AG_UnlockVFS(agView);
        break;
    case SDL_KEYDOWN:
        AG_DelTimeout(NULL, &toRepeat);
        repeatMod = ev->key.keysym.mod;
        repeatKey = ev->key.keysym.sym;
        repeatUnicode = ev->key.keysym.unicode;
        if (geekConsole)
        {
            if (ev->key.keysym.mod & KMOD_CTRL)
                mod |=  GeekBind::CTRL;
            if (ev->key.keysym.mod & KMOD_SHIFT)
                mod |=  GeekBind::SHIFT;
            if (ev->key.keysym.mod & KMOD_ALT)
                mod |=  GeekBind::META;
            // SDL specific: if repeatUnicode = 0 then just mod pressed
            if (repeatUnicode && geekConsole->charEntered(repeatKey, repeatUnicode, mod))
            {
                AG_ScheduleTimeout(NULL, &toDelay, agKbdDelay);
                return 1;
            }
        }
        if (bgFocuse || !UI::showUI) // no key repeat if agar focused
            AG_ScheduleTimeout(NULL, &toDelay, agKbdDelay);
        goto def;
    case SDL_KEYUP:
        if (repeatKey == ev->key.keysym.sym) {
            AG_DelTimeout(NULL, &toRepeat);
            AG_DelTimeout(NULL, &toDelay);
        }
    default:
    def:
        if (!bgFocuse && UI::showUI)
        {
            rv = AG_ProcessEvent(ev);
        }
        else
        {
            if (BG_ProcessEvent(ev) !=1)
                rv = AG_ProcessEvent(ev);
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


void EventLoop_FixedFPS(void)
{
    SDL_Event ev;
    AG_Window *win;
    Uint32 Tr1, Tr2 = 0;

    Tr1 = SDL_GetTicks();
    for (;;) 
    {
        Tr2 = SDL_GetTicks();
        if (Tr2-Tr1 >= agView->rNom) {
            AG_LockVFS(agView);
            AG_BeginRendering();

            // draw background
            BG_Draw();

            //draw agar's windows
            UI::updateGUIalpha(Tr2-Tr1, !bgFocuse);
            if (UI::showUI)
            {
                AG_TAILQ_FOREACH(win, &agView->windows, windows) {
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

            AG_EndRendering();
            SDL_GL_SwapBuffers();
            AG_UnlockVFS(agView);
            
            /* Recalibrate the effective refresh rate. */
            Tr1 = SDL_GetTicks();
            agView->rCur = agView->rNom - (Tr1-Tr2);
            if (agView->rCur < 1) {
                agView->rCur = 1;
            }
        } else if (SDL_PollEvent(&ev) != 0) {
            if (CL_ProcessEvent(&ev) == -1)
                return;
        } else if (AG_TIMEOUTS_QUEUED()) {      /* Safe */
            AG_ProcessTimeouts(Tr2);
        } else if (agView->rCur > agIdleThresh) {
            SDL_Delay(agView->rCur - agIdleThresh);
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
    if (AG_InitVideo(800, 600, 32, AG_VIDEO_OPENGL|AG_VIDEO_RESIZABLE|AG_VIDEO_OVERLAY)
        == -1) {
        fprintf(stderr, "%s\n", AG_GetError());
        return (-1);
    }
    AG_SetVideoResizeCallback(Resize);

    /* repeat key timeouts init */
    AG_SetTimeout(&toRepeat, RepeatTimeout, NULL, 0);
    AG_SetTimeout(&toDelay, DelayTimeout, NULL, 0);
    repeatKey = (SDLKey) 0;
    repeatMod = KMOD_NONE;
    repeatUnicode = 0;

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
    EventLoop_FixedFPS();
    AG_Destroy();

    return 0;
}

