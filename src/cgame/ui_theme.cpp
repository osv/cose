#include <agar/core.h>
#include <agar/gui.h>

#include <celutil/directory.h>

#include "ui_theme.h"
#include "ui.h"

#include <agar/gui/opengl.h>
#include "configfile.h"
#include "geekconsole.h"

namespace UI{

    static const string thmDir = "agar/themes/";

    static void actSetTheme(AG_Event *event)
    {
        const char *filename = AG_STRING(1);
        loadCfg(filename, "render.color.");
    }

    void addThemeTist2Menu(AG_MenuItem *mi)
    {
        if (!IsDirectory(thmDir))
        {
            cout << "Failed locate theme dir:" << thmDir << "\n";
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
            saveCfg(thmDir + value + ".thm", "render.color.");
            gc->finish();
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
            gc->finish();
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

    AG_PrimitiveOps defaultPrimitiveOps;
    bool themeInit = false;

    double g_maxAlpha = 1.0;
    double g_minAlpha = 0.3;
    double g_unfocusedWindowAlpha = 0.5;
    float precomputeAlpha = 0;

    float uiAlpha = 0;

    void updateGUIalpha(Uint32 tickDelta, bool agarUIfocused)
    {
        if (agarUIfocused)
            uiAlpha += (g_maxAlpha - g_minAlpha ) *
                (float) tickDelta / 60;
        else
            uiAlpha -= (g_maxAlpha - g_minAlpha ) *
                (float) tickDelta / 100;
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

    inline void SetColorRGBA(Uint32 color)
    {
        Uint8 red, green, blue, alpha;
        float v;
        AG_GetRGBA(color, agVideoFmt, &red,&green,&blue,&alpha);
        v = alpha;
        v *= precomputeAlpha;
        glColor4ub(red, green, blue, (Uint8) v);
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

    /* Add to individual RGB components of a pixel. */
    /* TODO use SIMD to compute the components in parallel */
    /* check agar/gui/primitives.c */
    static __inline__ Uint32
    ColorShift(Uint32 pixel, Sint8 *shift)
    {
        Sint8 r = shift[0];
        Sint8 g = shift[1];
        Sint8 b = shift[2];
        Uint32 rv = 0;
        int v1, v2;

        v1 = ((pixel & agVideoFmt->Rmask) >> agVideoFmt->Rshift);
        v2 = ((v1 << agVideoFmt->Rloss) + (v1 >> (8 - agVideoFmt->Rloss))) + r;
        if (v2 < 0) {
            v2 = 0;
        } else if (v2 > 255) {
            v2 = 255;
        }
        rv |= (v2 >> agVideoFmt->Rloss) << agVideoFmt->Rshift;

        v1 = ((pixel & agVideoFmt->Gmask) >> agVideoFmt->Gshift);
        v2 = ((v1 << agVideoFmt->Gloss) + (v1 >> (8 - agVideoFmt->Gloss))) + g;
        if (v2 < 0) {
            v2 = 0;
        } else if (v2 > 255) {
            v2 = 255;
        }
        rv |= (v2 >> agVideoFmt->Gloss) << agVideoFmt->Gshift;

        v1 = ((pixel & agVideoFmt->Bmask) >> agVideoFmt->Bshift);
        v2 = ((v1 << agVideoFmt->Bloss) + (v1 >> (8 - agVideoFmt->Bloss))) + b;
        if (v2 < 0) {
            v2 = 0;
        } else if (v2 > 255) {
            v2 = 255;
        }
        rv |= (v2 >> agVideoFmt->Bloss) << agVideoFmt->Bshift;

        rv |= agVideoFmt->Amask;
        return (rv);
    }

    static void
    _boxRoundedTopGL(void *p, AG_Rect r, int z, int rad, Uint32 cBg)
    {
        AG_Widget *wid = (AG_Widget *)p;

        BeginBlending();

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(wid->rView.x1 + r.x,
                     wid->rView.y1 + r.y, 0);
        glBegin(GL_POLYGON);
        {
            SetColorRGBA(cBg);
            glVertex2i(0, r.h);
            glVertex2i(0, rad);
            glVertex2i(rad, 0);
            glVertex2i(r.w-rad, 0);
            glVertex2i(r.w, rad);
            glVertex2i(r.w, r.h);
        }
        glEnd();
        if (z >= 0) {
            glBegin(GL_LINE_STRIP);
            {
                SetColorRGBA(ColorShift(cBg, agHighColorShift));
                glVertex2i(0, r.h);
                glVertex2i(0, rad);
                glVertex2i(rad, 0);
            }
            glEnd();
            glBegin(GL_LINES);
            {
                SetColorRGBA(ColorShift(cBg, agLowColorShift));
                glVertex2i(r.w-1, r.h);
                glVertex2i(r.w-1, rad);
            }
            glEnd();
        }
        glPopMatrix();

        EndBlending();
    }

    static void
    _rectGL(void *p, AG_Rect r, Uint32 color)
    {
        AG_Widget *wid = (AG_Widget *)p;
        int x1 = wid->rView.x1 + r.x;
        int y1 = wid->rView.y1 + r.y;
        int x2 = x1 + r.w - 1;
        int y2 = y1 + r.h - 1;
        BeginBlending();
        SetColorRGBA(color);
        glBegin(GL_POLYGON);
        glVertex2i(x1, y1);
        glVertex2i(x2, y1);
        glVertex2i(x2, y2);
        glVertex2i(x1, y2);
        glEnd();
        EndBlending();
    }

    static void
    _lineHGL(void *p, int x1, int x2, int py, Uint32 color)
    {
        AG_Widget *wid = (AG_Widget *)p;
        int y = wid->rView.y1 + py;
        BeginBlending();
        SetColorRGBA(color);
        glBegin(GL_LINES);
        glVertex2s(wid->rView.x1 + x1, y);
        glVertex2s(wid->rView.x1 + x2, y);
        glEnd();
        EndBlending();
    }

    static void
    _lineVGL(void *p, int px, int y1, int y2, Uint32 color)
    {
        AG_Widget *wid = (AG_Widget *)p;
        int x = wid->rView.x1 + px;
        BeginBlending();
        SetColorRGBA(color);
        glBegin(GL_LINES);
        glVertex2s(x, wid->rView.y1 + y1);
        glVertex2s(x, wid->rView.y1 + y2);
        glEnd();
        EndBlending();
    }

    void initThemes()
    {
        defaultPrimitiveOps = agPrim;

        g_maxAlpha = 1.0;
        g_minAlpha = 0.3;
        g_unfocusedWindowAlpha = 0.5;

        // bind some config vars
        cVarBindFloat("render.agar.maxAlpha", &g_maxAlpha, g_maxAlpha);
        cVarBindFloat("render.agar.minAlpha", &g_minAlpha, g_minAlpha);
        cVarBindFloat("render.agar.unfocusedWindowAlpha", &g_unfocusedWindowAlpha, g_unfocusedWindowAlpha);

        // color theme
        cVarBindInt32Hex("render.color.agar.BG", (int32*)&agColors[BG_COLOR], agColors[BG_COLOR]);
        cVarBindInt32Hex("render.color.agar.FRAME", (int32*)&agColors[FRAME_COLOR], agColors[FRAME_COLOR]);
        cVarBindInt32Hex("render.color.agar.LINE", (int32 *)&agColors[LINE_COLOR], agColors[LINE_COLOR]);
        cVarBindInt32Hex("render.color.agar.TEXT", (int32 *)&agColors[TEXT_COLOR], agColors[TEXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.WINDOW_BG", (int32 *)&agColors[WINDOW_BG_COLOR], agColors[WINDOW_BG_COLOR]);
        cVarBindInt32Hex("render.color.agar.WINDOW_HI", (int32 *)&agColors[WINDOW_HI_COLOR], agColors[WINDOW_HI_COLOR]);
        cVarBindInt32Hex("render.color.agar.WINDOW_LO", (int32 *)&agColors[WINDOW_LO_COLOR], agColors[WINDOW_LO_COLOR]);
        cVarBindInt32Hex("render.color.agar.TITLEBAR_FOCUSED", (int32 *)&agColors[TITLEBAR_FOCUSED_COLOR], agColors[TITLEBAR_FOCUSED_COLOR]);
        cVarBindInt32Hex("render.color.agar.TITLEBAR_UNFOCUSED", (int32 *)&agColors[TITLEBAR_UNFOCUSED_COLOR], agColors[TITLEBAR_UNFOCUSED_COLOR]);
        cVarBindInt32Hex("render.color.agar.TITLEBAR_CAPTION", (int32 *)&agColors[TITLEBAR_CAPTION_COLOR], agColors[TITLEBAR_CAPTION_COLOR]);
        cVarBindInt32Hex("render.color.agar.BUTTON", (int32 *)&agColors[BUTTON_COLOR], agColors[BUTTON_COLOR]);
        cVarBindInt32Hex("render.color.agar.BUTTON_TXT", (int32 *)&agColors[BUTTON_TXT_COLOR], agColors[BUTTON_TXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.DISABLED", (int32 *)&agColors[DISABLED_COLOR], agColors[DISABLED_COLOR]);
        cVarBindInt32Hex("render.color.agar.CHECKBOX", (int32 *)&agColors[CHECKBOX_COLOR], agColors[CHECKBOX_COLOR]);
        cVarBindInt32Hex("render.color.agar.CHECKBOX_TXT", (int32 *)&agColors[CHECKBOX_TXT_COLOR], agColors[CHECKBOX_TXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.GRAPH_BG", (int32 *)&agColors[GRAPH_BG_COLOR], agColors[GRAPH_BG_COLOR]);
        cVarBindInt32Hex("render.color.agar.GRAPH_XAXIS", (int32 *)&agColors[GRAPH_XAXIS_COLOR], agColors[GRAPH_XAXIS_COLOR]);
        cVarBindInt32Hex("render.color.agar.HSVPAL_CIRCLE", (int32 *)&agColors[HSVPAL_CIRCLE_COLOR], agColors[HSVPAL_CIRCLE_COLOR]);
        cVarBindInt32Hex("render.color.agar.HSVPAL_TILE1", (int32 *)&agColors[HSVPAL_TILE1_COLOR], agColors[HSVPAL_TILE1_COLOR]);
        cVarBindInt32Hex("render.color.agar.HSVPAL_TILE2", (int32 *)&agColors[HSVPAL_TILE2_COLOR], agColors[HSVPAL_TILE2_COLOR]);
        cVarBindInt32Hex("render.color.agar.MENU_UNSEL", (int32 *)&agColors[MENU_UNSEL_COLOR], agColors[MENU_UNSEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.MENU_SEL", (int32 *)&agColors[MENU_SEL_COLOR], agColors[MENU_SEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.MENU_OPTION", (int32 *)&agColors[MENU_OPTION_COLOR], agColors[MENU_OPTION_COLOR]);
        cVarBindInt32Hex("render.color.agar.MENU_TXT", (int32 *)&agColors[MENU_TXT_COLOR], agColors[MENU_TXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.MENU_SEP1", (int32 *)&agColors[MENU_SEP1_COLOR], agColors[MENU_SEP1_COLOR]);
        cVarBindInt32Hex("render.color.agar.MENU_SEP2", (int32 *)&agColors[MENU_SEP2_COLOR], agColors[MENU_SEP2_COLOR]);
        cVarBindInt32Hex("render.color.agar.NOTEBOOK_BG", (int32 *)&agColors[NOTEBOOK_BG_COLOR], agColors[NOTEBOOK_BG_COLOR]);
        cVarBindInt32Hex("render.color.agar.NOTEBOOK_SEL", (int32 *)&agColors[NOTEBOOK_SEL_COLOR], agColors[NOTEBOOK_SEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.NOTEBOOK_TXT", (int32 *)&agColors[NOTEBOOK_TXT_COLOR], agColors[NOTEBOOK_TXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.RADIO_SEL", (int32 *)&agColors[RADIO_SEL_COLOR], agColors[RADIO_SEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.RADIO_OVER", (int32 *)&agColors[RADIO_OVER_COLOR], agColors[RADIO_OVER_COLOR]);
        cVarBindInt32Hex("render.color.agar.RADIO_HI", (int32 *)&agColors[RADIO_HI_COLOR], agColors[RADIO_HI_COLOR]);
        cVarBindInt32Hex("render.color.agar.RADIO_LO", (int32 *)&agColors[RADIO_LO_COLOR], agColors[RADIO_LO_COLOR]);
        cVarBindInt32Hex("render.color.agar.RADIO_TXT", (int32 *)&agColors[RADIO_TXT_COLOR], agColors[RADIO_TXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.SCROLLBAR", (int32 *)&agColors[SCROLLBAR_COLOR], agColors[SCROLLBAR_COLOR]);
        cVarBindInt32Hex("render.color.agar.SCROLLBAR_BTN", (int32 *)&agColors[SCROLLBAR_BTN_COLOR], agColors[SCROLLBAR_BTN_COLOR]);
        cVarBindInt32Hex("render.color.agar.SCROLLBAR_ARR1", (int32 *)&agColors[SCROLLBAR_ARR1_COLOR], agColors[SCROLLBAR_ARR1_COLOR]);
        cVarBindInt32Hex("render.color.agar.SCROLLBAR_ARR2", (int32 *)&agColors[SCROLLBAR_ARR2_COLOR], agColors[SCROLLBAR_ARR2_COLOR]);
        cVarBindInt32Hex("render.color.agar.SEPARATOR_LINE1", (int32 *)&agColors[SEPARATOR_LINE1_COLOR], agColors[SEPARATOR_LINE1_COLOR]);
        cVarBindInt32Hex("render.color.agar.SEPARATOR_LINE2", (int32 *)&agColors[SEPARATOR_LINE2_COLOR], agColors[SEPARATOR_LINE2_COLOR]);
        cVarBindInt32Hex("render.color.agar.TABLEVIEW", (int32 *)&agColors[TABLEVIEW_COLOR], agColors[TABLEVIEW_COLOR]);
        cVarBindInt32Hex("render.color.agar.TABLEVIEW_HEAD", (int32 *)&agColors[TABLEVIEW_HEAD_COLOR], agColors[TABLEVIEW_HEAD_COLOR]);
        cVarBindInt32Hex("render.color.agar.TABLEVIEW_HTXT", (int32 *)&agColors[TABLEVIEW_HTXT_COLOR], agColors[TABLEVIEW_HTXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.TABLEVIEW_CTXT", (int32 *)&agColors[TABLEVIEW_CTXT_COLOR], agColors[TABLEVIEW_CTXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.TABLEVIEW_LINE", (int32 *)&agColors[TABLEVIEW_LINE_COLOR], agColors[TABLEVIEW_LINE_COLOR]);
        cVarBindInt32Hex("render.color.agar.TABLEVIEW_SEL", (int32 *)&agColors[TABLEVIEW_SEL_COLOR], agColors[TABLEVIEW_SEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.TEXTBOX", (int32 *)&agColors[TEXTBOX_COLOR], agColors[TEXTBOX_COLOR]);
        cVarBindInt32Hex("render.color.agar.TEXTBOX_TXT", (int32 *)&agColors[TEXTBOX_TXT_COLOR], agColors[TEXTBOX_TXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.TEXTBOX_CURSOR", (int32 *)&agColors[TEXTBOX_CURSOR_COLOR], agColors[TEXTBOX_CURSOR_COLOR]);
        cVarBindInt32Hex("render.color.agar.TLIST_TXT", (int32 *)&agColors[TLIST_TXT_COLOR], agColors[TLIST_TXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.TLIST_BG", (int32 *)&agColors[TLIST_BG_COLOR], agColors[TLIST_BG_COLOR]);
        cVarBindInt32Hex("render.color.agar.TLIST_LINE", (int32 *)&agColors[TLIST_LINE_COLOR], agColors[TLIST_LINE_COLOR]);
        cVarBindInt32Hex("render.color.agar.TLIST_SEL", (int32 *)&agColors[TLIST_SEL_COLOR], agColors[TLIST_SEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.MAPVIEW_GRID", (int32 *)&agColors[MAPVIEW_GRID_COLOR], agColors[MAPVIEW_GRID_COLOR]);
        cVarBindInt32Hex("render.color.agar.MAPVIEW_CURSOR", (int32 *)&agColors[MAPVIEW_CURSOR_COLOR], agColors[MAPVIEW_CURSOR_COLOR]);
        cVarBindInt32Hex("render.color.agar.MAPVIEW_TILE1", (int32 *)&agColors[MAPVIEW_TILE1_COLOR], agColors[MAPVIEW_TILE1_COLOR]);
        cVarBindInt32Hex("render.color.agar.MAPVIEW_TILE2", (int32 *)&agColors[MAPVIEW_TILE2_COLOR], agColors[MAPVIEW_TILE2_COLOR]);
        cVarBindInt32Hex("render.color.agar.MAPVIEW_MSEL", (int32 *)&agColors[MAPVIEW_MSEL_COLOR], agColors[MAPVIEW_MSEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.MAPVIEW_ESEL", (int32 *)&agColors[MAPVIEW_ESEL_COLOR], agColors[MAPVIEW_ESEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.TILEVIEW_TILE1", (int32 *)&agColors[TILEVIEW_TILE1_COLOR], agColors[TILEVIEW_TILE1_COLOR]);
        cVarBindInt32Hex("render.color.agar.TILEVIEW_TILE2", (int32 *)&agColors[TILEVIEW_TILE2_COLOR], agColors[TILEVIEW_TILE2_COLOR]);
        cVarBindInt32Hex("render.color.agar.TILEVIEW_TEXTBG", (int32 *)&agColors[TILEVIEW_TEXTBG_COLOR], agColors[TILEVIEW_TEXTBG_COLOR]);
        cVarBindInt32Hex("render.color.agar.TILEVIEW_TEXT", (int32 *)&agColors[TILEVIEW_TEXT_COLOR], agColors[TILEVIEW_TEXT_COLOR]);
        cVarBindInt32Hex("render.color.agar.TRANSPARENT", (int32 *)&agColors[TRANSPARENT_COLOR], agColors[TRANSPARENT_COLOR]);
        cVarBindInt32Hex("render.color.agar.HSVPAL_BAR1", (int32 *)&agColors[HSVPAL_BAR1_COLOR], agColors[HSVPAL_BAR1_COLOR]);
        cVarBindInt32Hex("render.color.agar.HSVPAL_BAR2", (int32 *)&agColors[HSVPAL_BAR2_COLOR], agColors[HSVPAL_BAR2_COLOR]);
        cVarBindInt32Hex("render.color.agar.PANE", (int32 *)&agColors[PANE_COLOR], agColors[PANE_COLOR]);
        cVarBindInt32Hex("render.color.agar.PANE_CIRCLE", (int32 *)&agColors[PANE_CIRCLE_COLOR], agColors[PANE_CIRCLE_COLOR]);
        cVarBindInt32Hex("render.color.agar.MAPVIEW_RSEL", (int32 *)&agColors[MAPVIEW_RSEL_COLOR], agColors[MAPVIEW_RSEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.MAPVIEW_ORIGIN", (int32 *)&agColors[MAPVIEW_ORIGIN_COLOR], agColors[MAPVIEW_ORIGIN_COLOR]);
        cVarBindInt32Hex("render.color.agar.FOCUS", (int32 *)&agColors[FOCUS_COLOR], agColors[FOCUS_COLOR]);
        cVarBindInt32Hex("render.color.agar.TABLE", (int32 *)&agColors[TABLE_COLOR], agColors[TABLE_COLOR]);
        cVarBindInt32Hex("render.color.agar.TABLE_LINE", (int32 *)&agColors[TABLE_LINE_COLOR], agColors[TABLE_LINE_COLOR]);
        cVarBindInt32Hex("render.color.agar.FIXED_BG", (int32 *)&agColors[FIXED_BG_COLOR], agColors[FIXED_BG_COLOR]);
        cVarBindInt32Hex("render.color.agar.FIXED_BOX", (int32 *)&agColors[FIXED_BOX_COLOR], agColors[FIXED_BOX_COLOR]);
        cVarBindInt32Hex("render.color.agar.TEXT_DISABLED", (int32 *)&agColors[TEXT_DISABLED_COLOR], agColors[TEXT_DISABLED_COLOR]);
        cVarBindInt32Hex("render.color.agar.MENU_TXT_DISABLED", (int32 *)&agColors[MENU_TXT_DISABLED_COLOR], agColors[MENU_TXT_DISABLED_COLOR]);
        cVarBindInt32Hex("render.color.agar.SOCKET", (int32 *)&agColors[SOCKET_COLOR], agColors[SOCKET_COLOR]);
        cVarBindInt32Hex("render.color.agar.SOCKET_LABEL", (int32 *)&agColors[SOCKET_LABEL_COLOR], agColors[SOCKET_LABEL_COLOR]);
        cVarBindInt32Hex("render.color.agar.SOCKET_HIGHLIGHT", (int32 *)&agColors[SOCKET_HIGHLIGHT_COLOR], agColors[SOCKET_HIGHLIGHT_COLOR]);
        cVarBindInt32Hex("render.color.agar.PROGRESS_BAR", (int32 *)&agColors[PROGRESS_BAR_COLOR], agColors[PROGRESS_BAR_COLOR]);
        cVarBindInt32Hex("render.color.agar.WINDOW_BORDER", (int32 *)&agColors[WINDOW_BORDER_COLOR], agColors[WINDOW_BORDER_COLOR]);

        cVarBindInt32Hex("render.color.gconsole.BG", (int32 *)&clBackground->i, clBackground->i);
        cVarBindInt32Hex("render.color.gconsole.BG_INTERACTIVE", (int32 *)&clBgInteractive->i, clBgInteractive->i);
        cVarBindInt32Hex("render.color.gconsole.BG_INTERACTIVE_BRD", (int32 *)&clBgInteractiveBrd->i, clBgInteractiveBrd->i);
        cVarBindInt32Hex("render.color.gconsole.INTERACTIVE_FNT", (int32 *)&clInteractiveFnt->i, clInteractiveFnt->i);
        cVarBindInt32Hex("render.color.gconsole.INTERACTIVE_PREFIX_FNT", (int32 *)&clInteractivePrefixFnt->i, clInteractivePrefixFnt->i);
        cVarBindInt32Hex("render.color.gconsole.INTERACTIVE_EXPAND", (int32 *)&clInteractiveExpand->i, clInteractiveExpand->i);
        cVarBindInt32Hex("render.color.gconsole.DESCRIPTION", (int32 *)&clDescrFnt->i, clDescrFnt->i);
        cVarBindInt32Hex("render.color.gconsole.COMPLETION", (int32 *)&clCompletionFnt->i, clCompletionFnt->i);
        cVarBindInt32Hex("render.color.gconsole.COMPLETION_MATCH", (int32 *)&clCompletionMatchCharFnt->i, clCompletionMatchCharFnt->i);
        cVarBindInt32Hex("render.color.gconsole.COMPLETION_MATCH_BG", (int32 *)&clCompletionMatchCharBg->i, clCompletionMatchCharBg->i);
        cVarBindInt32Hex("render.color.gconsole.COMPLETION_AFTER_MATCH", (int32 *)&clCompletionAfterMatch->i, clCompletionAfterMatch->i);
        cVarBindInt32Hex("render.color.gconsole.COMPLETION_EXPAND_BG", (int32 *)&clCompletionExpandBg->i, clCompletionExpandBg->i);
        cVarBindInt32Hex("render.color.gconsole.INFOTEXT_FNT", (int32 *)&clInfoTextFnt->i, clInfoTextFnt->i);
        cVarBindInt32Hex("render.color.gconsole.INFOTEXT_BG", (int32 *)&clInfoTextBg->i, clInfoTextBg->i);
        cVarBindInt32Hex("render.color.gconsole.INFOTEXT_BRD", (int32 *)&clInfoTextBrd->i, clInfoTextBrd->i);

        
        themeInit = true;

        geekConsole->registerFunction(GCFunc(loadColorTheme), "load color theme");
        geekConsole->registerFunction(GCFunc(saveColorTheme), "save color theme");

    }

    void setupThemes(PrimitiveStyle primitiveStyle)
    {
        if (!themeInit)
            return;
        agPrim = defaultPrimitiveOps;
        switch (primitiveStyle)
        {
        case FullTransparent:
            agPrim.LineH = _lineHGL;
            agPrim.LineV = _lineVGL;
        case SimpleTransparent:
            agPrim.RectFilled = _rectGL;
            agPrim.BoxRoundedTop = _boxRoundedTopGL;
            break;
        default:
            break;
        }
    }
}
