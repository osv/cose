#include <agar/core.h>
#include <agar/gui.h>
#include <agar/gui/opengl.h>

#include "ui_theme.h"
#include "ui.h"

namespace UI{

    AG_PrimitiveOps defaultPrimitiveOps;
    bool themeInit = false;

    float  globalUIalpha = 1.0;

    float g_maxAlpha = 1.0;
    float g_minAlpha = 0.3;
    float g_unfocusedWindowAlpha = 0.5;
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

#define BeginBlending()					   \
    GLboolean blend_save;				   \
    GLint sfac_save, dfac_save;				   \
    glGetBooleanv(GL_BLEND, &blend_save);		   \
    glGetIntegerv(GL_BLEND_SRC, &sfac_save);		   \
    glGetIntegerv(GL_BLEND_DST, &dfac_save);		   \
    glEnable(GL_BLEND);					   \
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#define EndBlending()				\
    if (blend_save) {				\
	glEnable(GL_BLEND);			\
    } else {					\
	glDisable(GL_BLEND);			\
    }						\
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
	themeInit = true;
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
