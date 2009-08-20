#ifndef _UI_THEME_H_
#define _UI_THEME_H_

namespace UI
{
    enum PrimitiveStyle
    {
	Default,
	SimpleTransparent,
	FullTransparent
    };
    extern void initThemes();
    extern void setupThemes(PrimitiveStyle themestyle);

    extern void updateGUIalpha(Uint32 tickDelta, bool agarUIfocused);
    extern void precomputeGUIalpha(bool isWinFocused);

}

#endif // _UI_THEME_H_
