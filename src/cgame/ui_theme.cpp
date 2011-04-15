// Copyright (C) 2010-2011 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <agar/core.h>
#include <agar/gui.h>

#include <celutil/directory.h>

#define _USE_AGAR_GUI_MATH 1
#include <agar/gui/gui_math.h>
#include <agar/core/types.h>
#include "ui_theme.h"
#include "ui.h"

#include <agar/gui/opengl.h>
#include "configfile.h"
#include "geekconsole.h"
#include "gvar.h"
#include <agar/gui/drv_gl_common.h>

namespace UI{

    static const string thmDir = "agar/themes/";
    AG_Window *winThemeCfg;

    bool themeInit = false;

    double g_maxAlpha = 1.0;
    double g_minAlpha = 0.3;
    double g_unfocusedWindowAlpha = 0.5;
    float precomputeAlpha = 0;

    float uiAlpha = 0;

    // default agars's primitives
    // AGAR-UPGRADE: drv_gl_common.c: check agar gl primitives
    void (*defDrawLineH)(void *drv, int x1, int x2, int y, AG_Color C);
    void (*defDrawLineV)(void *drv, int x, int y1, int y2, AG_Color C);
    void (*defDrawBoxRoundedTop)(void *drv, AG_Rect r, int z, int rad, AG_Color C[3]);
    void (*defDrawRectFilled)(void *drv, AG_Rect r, AG_Color C);

    static void actSetTheme(AG_Event *event)
    {
        const char *filename = AG_STRING(1);
        loadCfg(filename, "render.color.");
    }

    void addThemeTist2Menu(AG_MenuItem *mi)
    {
        if (!IsDirectory(thmDir))
        {
            cout << "Failed locate theme dir: " << thmDir << "\n";
            return;
        }
        Directory* dir = OpenDirectory(thmDir);
        if (dir == NULL)
            return;

        std::string filename;
        while (dir->nextFile(filename))
        {
            if (filename[0] == '.')
                continue;
            int extPos = filename.rfind('.');
            if (extPos == (int)string::npos)
                continue;
            std::string ext = string(filename, extPos, filename.length() - extPos + 1);

            if (compareIgnoringCase(".thm", ext) == 0)
            {
                //alloc theme name, this mem not been free, agar need pointer to string
                std::string *fullName = new std::string;
                *fullName = thmDir + filename;
                AG_MenuAction(mi, std::string(filename, 0, extPos).c_str(),
                              NULL, actSetTheme, "%s", fullName->c_str());
            }
        }
    }

    static int saveColorTheme(GeekConsole *gc, int state, std::string value)
    {
        switch (state)
        {
        case 1:
            AG_MkPath(thmDir.c_str());
            saveCfg(thmDir + value + ".thm", "render.color.");
            break;
        case 0:
        {
            gc->setInteractive(listInteractive, "color-theme", _("Theme name:"), _("Color theme name for saving"));
            Directory* dir = OpenDirectory(thmDir);
            if (dir == NULL)
                break;
            vector<string> colorthemes;
            std::string filename;
            while (dir->nextFile(filename))
            {
                if (filename[0] == '.')
                    continue;
                int extPos = filename.rfind('.');
                if (extPos == (int)string::npos)
                    continue;
                std::string ext = string(filename, extPos, filename.length() - extPos + 1);

                if (compareIgnoringCase(".thm", ext) == 0)
                {
                    colorthemes.push_back(std::string(filename, 0, extPos));
                }
            }
            listInteractive->setCompletion(colorthemes);
            break;
        }
        }
        return state;
    }

    static int loadColorTheme(GeekConsole *gc, int state, std::string value)
    {
        switch (state)
        {
        case 1:
            loadCfg(thmDir + value + ".thm", "render.color.");
            break;
        case 0:
        {
            gc->setInteractive(listInteractive, "color-theme", _("Theme name"), _("Color theme for loading"));
            Directory* dir = OpenDirectory(thmDir);
            if (dir == NULL)
                break;
            vector<string> colorthemes;
            std::string filename;
            while (dir->nextFile(filename))
            {
                if (filename[0] == '.')
                    continue;
                int extPos = filename.rfind('.');
                if (extPos == (int)string::npos)
                    continue;
                std::string ext = string(filename, extPos, filename.length() - extPos + 1);

                if (compareIgnoringCase(".thm", ext) == 0)
                {
                    colorthemes.push_back(std::string(filename, 0, extPos));
                }
            }
            listInteractive->setCompletion(colorthemes);
            break;
        }
        }
        return state;
    }

    static void showConfigureTheme(AG_Event *event)
    {
        AG_WindowHide(winThemeCfg);
        AGWIDGET(winThemeCfg)->x = -1;
        AGWIDGET(winThemeCfg)->y = -1;
        AG_WindowSetPosition(winThemeCfg, AG_WINDOW_TC, 0);
        AG_WindowShow(winThemeCfg);
    }

    void updateGUIalpha(Uint32 tickDelta, bool agarUIfocused)
    {
        if (agarUIfocused)
            uiAlpha += (g_maxAlpha - g_minAlpha ) *
                (float) tickDelta / 30;
        else
            uiAlpha -= (g_maxAlpha - g_minAlpha ) *
                (float) tickDelta / 60;
        if (uiAlpha > g_maxAlpha)
            uiAlpha = g_maxAlpha;
        else if (uiAlpha < g_minAlpha)
            uiAlpha = g_minAlpha;
    }

    void precomputeGUIalpha(bool isWinFocused)
    {
        precomputeAlpha = uiAlpha;
        if (!isWinFocused)
            precomputeAlpha *= g_unfocusedWindowAlpha;
    }

    inline void SetColorRGBA(AG_Color color)
    {
        float alpha;

        alpha = color.a;
        alpha *= precomputeAlpha;
        glColor4ub(color.r, color.g, color.b,
                   (Uint8) alpha);
    }

#define BeginBlending()                                 \
    GLboolean blend_save;                               \
    GLint sfac_save, dfac_save;                         \
    glGetBooleanv(GL_BLEND, &blend_save);               \
    glGetIntegerv(GL_BLEND_SRC, &sfac_save);            \
    glGetIntegerv(GL_BLEND_DST, &dfac_save);            \
    glEnable(GL_BLEND);                                 \
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#define EndBlending()                           \
    if (blend_save) {                           \
        glEnable(GL_BLEND);                     \
    } else {                                    \
        glDisable(GL_BLEND);                    \
    }                                           \
    glBlendFunc(sfac_save, dfac_save);


    /* agar primitives */

    void
    _boxRoundedTopGL(void *obj, AG_Rect r, int z, int radius, AG_Color C[3])
    {
        BeginBlending();

        float rad = (float)radius;
        float i, nFull = 10.0, nQuart = nFull/4.0;

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        glTranslatef((float)r.x + rad,
                     (float)r.y + rad,
                     0.0);

        glBegin(GL_POLYGON);
        SetColorRGBA(C[0]);
        {
            glVertex2f(-rad, (float)r.h - rad);

            for (i = 0.0; i < nQuart; i++) {
                glVertex2f(-rad*Cos((2.0*AG_PI*i)/nFull),
                           -rad*Sin((2.0*AG_PI*i)/nFull));
            }
            glVertex2f(0.0, -rad);

            for (i = nQuart; i > 0.0; i--) {
                glVertex2f(r.w - rad*2 + rad*Cos((2.0*AG_PI*i)/nFull),
                           - rad*Sin((2.0*AG_PI*i)/nFull));
            }
            glVertex2f((float)r.w - rad,
                       (float)r.h - rad);
        }
        glEnd();
	
        glBegin(GL_LINE_STRIP);
        {
            SetColorRGBA(C[1]);
            glVertex2i(-rad, r.h-rad);
            for (i = 0.0; i < nQuart; i++) {
                glVertex2f(-(float)rad*Cos((2.0*AG_PI*i)/nFull),
                           -(float)rad*Sin((2.0*AG_PI*i)/nFull));
            }
            glVertex2f(0.0, -rad);
            glVertex2f(r.w - rad*2 + rad*Cos((2.0*AG_PI*nQuart)/nFull),
                       - rad*Sin((2.0*AG_PI*nQuart)/nFull));

            SetColorRGBA(C[2]);
            for (i = nQuart-1; i > 0.0; i--) {
                glVertex2f(r.w - rad*2 + rad*Cos((2.0*AG_PI*i)/nFull),
                           - rad*Sin((2.0*AG_PI*i)/nFull));
            }
            glVertex2f((float)r.w - rad,
                       (float)r.h - rad);
        }
        glEnd();

        EndBlending();

        glPopMatrix();
    }

    void
    _rectGL(void *obj, AG_Rect r, AG_Color C)
    {
        int x2 = r.x + r.w - 1;
        int y2 = r.y + r.h - 1;

        BeginBlending();
        SetColorRGBA(C);

        glBegin(GL_POLYGON);
        glVertex2i(r.x, r.y);
        glVertex2i(x2, r.y);
        glVertex2i(x2, y2);
        glVertex2i(r.x, y2);
        glEnd();

        EndBlending();
    }

    static void
    _lineHGL(void *obj, int x1, int x2, int y, AG_Color C)
    {
        BeginBlending();
        SetColorRGBA(C);

        glBegin(GL_LINES);
        glVertex2s(x1, y);
        glVertex2s(x2, y);
        glEnd();

        EndBlending();
    }

    static void
    _lineVGL(void *obj, int x, int y1, int y2, AG_Color C)
    {
        BeginBlending();
        SetColorRGBA(C);

        glBegin(GL_LINES);
        glVertex2s(x, y1);
        glVertex2s(x, y2);
        glEnd();

        EndBlending();
    }

    static void
    BindSelectedColor(AG_Event *event)
    {
        AG_HSVPal *hsv = (AG_HSVPal *)AG_PTR(1);
        AG_TlistItem *it = (AG_TlistItem *)AG_PTR(2);
        // pointer to int32 color
        Uint8 *c = (Uint8 *)it->p1;
        AG_BindUint8(hsv, "RGBAv", c);
    }

    void initThemes()
    {
        if (themeInit)
            return;

        g_maxAlpha = 1.0;
        g_minAlpha = 0.3;
        g_unfocusedWindowAlpha = 0.5;

        // save default agar primitive
        ag_driver_class* cls = AGDRIVER_CLASS(agDriverSw);
        defDrawLineH = cls->drawLineH;
        defDrawLineV = cls->drawLineV;
        defDrawBoxRoundedTop = cls->drawBoxRoundedTop;
        defDrawRectFilled = cls->drawRectFilled;

        // create theme config window
        winThemeCfg = AG_WindowNewNamed(0, "config-theme");
        AG_WindowSetCaption(winThemeCfg, _("Config theme settings"));
        AG_Notebook *nb = AG_NotebookNew(winThemeCfg,
                                         AG_NOTEBOOK_HFILL|AG_NOTEBOOK_VFILL);
        AG_NotebookTab *tab;
        tab = AG_NotebookAddTab(nb, _("General"), AG_BOX_VERT);
        {
            AG_Numerical *amb;
            amb = AG_NumericalNewDblR(tab, NULL, NULL, _("Min Alpha"),
                                      &g_minAlpha, 0, 1);
            AG_NumericalSetIncrement(amb, 0.1);
            amb = AG_NumericalNewDblR(tab, NULL, NULL, _("Max Alpha"),
                                      &g_maxAlpha, 0, 1);
            AG_NumericalSetIncrement(amb, 0.1);
            amb = AG_NumericalNewDblR(tab, NULL, NULL, _("Unfocused window alpha"),
                                      &g_unfocusedWindowAlpha, 0, 1);
            AG_NumericalSetIncrement(amb, 0.1);
        }

        // bind some config vars
        cVarBindFloat("render.agar.maxAlpha", &g_maxAlpha, g_maxAlpha);
        cVarBindFloat("render.agar.minAlpha", &g_minAlpha, g_minAlpha);
        cVarBindFloat("render.agar.unfocusedWindowAlpha", &g_unfocusedWindowAlpha, g_unfocusedWindowAlpha);

        typedef struct agar_color_vars_t {
            const char *name;
            int index;
        };

        agar_color_vars_t agar_color_vars[] = {
            {"BG", BG_COLOR},
            {"FRAME", FRAME_COLOR},
            {"LINE", LINE_COLOR},
            {"TEXT", TEXT_COLOR},
            {"WINDOW_BG", WINDOW_BG_COLOR},
            {"WINDOW_HI", WINDOW_HI_COLOR},
            {"WINDOW_LO", WINDOW_LO_COLOR},
            {"TITLEBAR_FOCUSED", TITLEBAR_FOCUSED_COLOR},
            {"TITLEBAR_UNFOCUSED", TITLEBAR_UNFOCUSED_COLOR},
            {"TITLEBAR_CAPTION", TITLEBAR_CAPTION_COLOR},
            {"BUTTON", BUTTON_COLOR},
            {"BUTTON_TXT", BUTTON_TXT_COLOR},
            {"DISABLED", DISABLED_COLOR},
            {"CHECKBOX", CHECKBOX_COLOR},
            {"CHECKBOX_TXT", CHECKBOX_TXT_COLOR},
            {"GRAPH_BG", GRAPH_BG_COLOR},
            {"GRAPH_XAXIS", GRAPH_XAXIS_COLOR},
            {"HSVPAL_CIRCLE", HSVPAL_CIRCLE_COLOR},
            {"HSVPAL_TILE1", HSVPAL_TILE1_COLOR},
            {"HSVPAL_TILE2", HSVPAL_TILE2_COLOR},
            {"MENU_UNSEL", MENU_UNSEL_COLOR},
            {"MENU_SEL", MENU_SEL_COLOR},
            {"MENU_OPTION", MENU_OPTION_COLOR},
            {"MENU_TXT", MENU_TXT_COLOR},
            {"MENU_SEP1", MENU_SEP1_COLOR},
            {"MENU_SEP2", MENU_SEP2_COLOR},
            {"NOTEBOOK_BG", NOTEBOOK_BG_COLOR},
            {"NOTEBOOK_SEL", NOTEBOOK_SEL_COLOR},
            {"NOTEBOOK_TXT", NOTEBOOK_TXT_COLOR},
            {"RADIO_SEL", RADIO_SEL_COLOR},
            {"RADIO_OVER", RADIO_OVER_COLOR},
            {"RADIO_HI", RADIO_HI_COLOR},
            {"RADIO_LO", RADIO_LO_COLOR},
            {"RADIO_TXT", RADIO_TXT_COLOR},
            {"SCROLLBAR", SCROLLBAR_COLOR},
            {"SCROLLBAR_BTN", SCROLLBAR_BTN_COLOR},
            {"SCROLLBAR_ARR1", SCROLLBAR_ARR1_COLOR},
            {"SCROLLBAR_ARR2", SCROLLBAR_ARR2_COLOR},
            {"SEPARATOR_LINE1", SEPARATOR_LINE1_COLOR},
            {"SEPARATOR_LINE2", SEPARATOR_LINE2_COLOR},
            {"TABLEVIEW", TABLEVIEW_COLOR},
            {"TABLEVIEW_HEAD", TABLEVIEW_HEAD_COLOR},
            {"TABLEVIEW_HTXT", TABLEVIEW_HTXT_COLOR},
            {"TABLEVIEW_CTXT", TABLEVIEW_CTXT_COLOR},
            {"TABLEVIEW_LINE", TABLEVIEW_LINE_COLOR},
            {"TABLEVIEW_SEL", TABLEVIEW_SEL_COLOR},
            {"TEXTBOX", TEXTBOX_COLOR},
            {"TEXTBOX_TXT", TEXTBOX_TXT_COLOR},
            {"TEXTBOX_CURSOR", TEXTBOX_CURSOR_COLOR},
            {"TLIST_TXT", TLIST_TXT_COLOR},
            {"TLIST_BG", TLIST_BG_COLOR},
            {"TLIST_LINE", TLIST_LINE_COLOR},
            {"TLIST_SEL", TLIST_SEL_COLOR},
            {"MAPVIEW_GRID", MAPVIEW_GRID_COLOR},
            {"MAPVIEW_CURSOR", MAPVIEW_CURSOR_COLOR},
            {"MAPVIEW_TILE1", MAPVIEW_TILE1_COLOR},
            {"MAPVIEW_TILE2", MAPVIEW_TILE2_COLOR},
            {"MAPVIEW_MSEL", MAPVIEW_MSEL_COLOR},
            {"MAPVIEW_ESEL", MAPVIEW_ESEL_COLOR},
            {"TILEVIEW_TILE1", TILEVIEW_TILE1_COLOR},
            {"TILEVIEW_TILE2", TILEVIEW_TILE2_COLOR},
            {"TILEVIEW_TEXTBG", TILEVIEW_TEXTBG_COLOR},
            {"TILEVIEW_TEXT", TILEVIEW_TEXT_COLOR},
            {"TRANSPARENT", TRANSPARENT_COLOR},
            {"HSVPAL_BAR1", HSVPAL_BAR1_COLOR},
            {"HSVPAL_BAR2", HSVPAL_BAR2_COLOR},
            {"PANE", PANE_COLOR},
            {"PANE_CIRCLE", PANE_CIRCLE_COLOR},
            {"MAPVIEW_RSEL", MAPVIEW_RSEL_COLOR},
            {"MAPVIEW_ORIGIN", MAPVIEW_ORIGIN_COLOR},
            {"FOCUS", FOCUS_COLOR},
            {"TABLE", TABLE_COLOR},
            {"TABLE_LINE", TABLE_LINE_COLOR},
            {"FIXED_BG", FIXED_BG_COLOR},
            {"FIXED_BOX", FIXED_BOX_COLOR},
            {"TEXT_DISABLED", TEXT_DISABLED_COLOR},
            {"MENU_TXT_DISABLED", MENU_TXT_DISABLED_COLOR},
            {"SOCKET", SOCKET_COLOR},
            {"SOCKET_LABEL", SOCKET_LABEL_COLOR},
            {"SOCKET_HIGHLIGHT", SOCKET_HIGHLIGHT_COLOR},
            {"PROGRESS_BAR", PROGRESS_BAR_COLOR},
            {"WINDOW_BORDER", WINDOW_BORDER_COLOR},
            {NULL}
        };

        typedef struct gconsole_color_vars_t {
            const char *name;
            Color32 *c;
        };

        gconsole_color_vars_t gconsole_color_vars[] = {
            {"BG", clBackground},
            {"BG_INTERACTIVE", clBgInteractive},
            {"BG_INTERACTIVE_BRD", clBgInteractiveBrd},
            {"INTERACTIVE_FNT", clInteractiveFnt},
            {"INTERACTIVE_PREFIX_FNT", clInteractivePrefixFnt},
            {"INTERACTIVE_EXPAND", clInteractiveExpand},
            {"DESCRIPTION", clDescrFnt},
            {"COMPLETION", clCompletionFnt},
            {"COMPLETION_MATCH", clCompletionMatchCharFnt},
            {"COMPLETION_MATCH_BG", clCompletionMatchCharBg},
            {"COMPLETION_AFTER_MATCH", clCompletionAfterMatch},
            {"COMPLETION_EXPAND_BG", clCompletionExpandBg},
            {"INFOTEXT_FNT", clInfoTextFnt},
            {"INFOTEXT_BG", clInfoTextBg},
            {"INFOTEXT_BRD", clInfoTextBrd},
            {NULL}
        };

        // bind vars for color theme
        tab = AG_NotebookAddTab(nb, _("Agar"), AG_BOX_VERT);
        {
            AG_Tlist *tl;
            AG_TlistItem *it;
            AG_Pane *hPane;
            AG_HSVPal *hsv;

            hPane = AG_PaneNew(tab, AG_PANE_HORIZ, AG_PANE_EXPAND);
            {
                tl = AG_TlistNew(hPane->div[0], AG_TLIST_EXPAND);
                AG_TlistSizeHint(tl, "Tileview text background", 10);
                // color list, fist agar's colors
                int i = 0;

                while (agar_color_vars[i].name) {
                    std::string bname("render.color.agar.");
                    Color32 *c = (Color32 *) &agColors[agar_color_vars[i].index];

                    // bind cfg var
                    cVarBindInt32Hex(bname + agar_color_vars[i].name,
                                     (int32 *)&c->i,
                                     c->i);
                    std::string vname("render/color/agar/");
                    gVar.BindColor(vname + agar_color_vars[i].name,
                                   &c->i,
                                   c->i);

                    // add to tlist
                    it = AG_TlistAdd(tl, NULL, _(agColorNames[i]));
                    it->p1 = &agColors[i];
                    i++;
                }
                hsv = AG_HSVPalNew(hPane->div[1], AG_HSVPAL_EXPAND);
                AG_SetEvent(tl, "tlist-selected", BindSelectedColor,
                            "%p", hsv);
            }
        }
        tab = AG_NotebookAddTab(nb, _("geekconsole"), AG_BOX_VERT);
        {
            AG_Tlist *tl;
            AG_TlistItem *it;
            AG_Pane *hPane;
            AG_HSVPal *hsv;

            hPane = AG_PaneNew(tab, AG_PANE_HORIZ, AG_PANE_EXPAND);
            {
                tl = AG_TlistNew(hPane->div[0], AG_TLIST_EXPAND);
                AG_TlistSizeHint(tl, "Tileview COMPLETION_AFTER_MATCH", 10);
                int i = 0;
                while (gconsole_color_vars[i].name) {
                    std::string bname("render.color.gconsole.");

                    // bind cfg var
                    cVarBindInt32Hex(bname + gconsole_color_vars[i].name,
                                     (int32 *)&gconsole_color_vars[i].c->i,
                                     gconsole_color_vars[i].c->i);

                    it = AG_TlistAdd(tl, NULL, _(gconsole_color_vars[i].name));
                    it->p1 = gconsole_color_vars[i].c->rgba;
                    i++;
                }

                hsv = AG_HSVPalNew(hPane->div[1], AG_HSVPAL_EXPAND);
                AG_SetEvent(tl, "tlist-selected", BindSelectedColor,
                            "%p", hsv);
            }
        }

        AG_WindowSetPosition(winThemeCfg, AG_WINDOW_TC, 0);
        AG_WindowShow(winThemeCfg);
        AG_WindowHide(winThemeCfg);
        themeInit = true;

        getGeekConsole()->registerFunction(GCFunc(loadColorTheme), "load color theme");
        getGeekConsole()->registerFunction(GCFunc(saveColorTheme), "save color theme old");
    }

    void setupThemeStyle(PrimitiveStyle primitiveStyle)
    {
        if (!themeInit)
            return;

        ag_driver_class* cls = AGDRIVER_CLASS(agDriverSw);
        // reset to default
        cls->drawLineH = defDrawLineH;
        cls->drawLineV = defDrawLineV;
        cls->drawBoxRoundedTop = defDrawBoxRoundedTop;
        cls->drawRectFilled = defDrawRectFilled;
 
        switch (primitiveStyle)
        {
        case FullTransparent:
            cls->drawLineH = _lineHGL;
            cls->drawLineV = _lineVGL;
        case SimpleTransparent:
            cls->drawBoxRoundedTop = _boxRoundedTopGL;
            cls->drawRectFilled = _rectGL;
            break;
        default:
            break;
        }
    }

    static void mnuThemes(AG_Event *event) {
        AG_MenuItem *mi = (AG_MenuItem *)AG_SENDER();
        addThemeTist2Menu(mi);
    }

    void createMenuTheme(AG_MenuItem *menu)
    {
        AG_MenuItem *item;
        AG_MenuAction(menu, _("Configure theme..."), NULL,
                      showConfigureTheme, NULL);

        item = AG_MenuNode(menu, _("Theme"), NULL);
        // polled theme list
        AG_MenuSetPollFn(item, mnuThemes, NULL);
    }
}
