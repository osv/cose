#ifndef _UI_THEME_H_
#define _UI_THEME_H_

namespace UI
{
    enum PrimitiveStyle
    {
        FullTransparent,
        SimpleTransparent,
        Default,
    };
    extern void initThemes();
    extern void setupThemeStyle(PrimitiveStyle primitiveStyle);

    extern void updateGUIalpha(Uint32 tickDelta, bool agarUIfocused);
    extern void precomputeGUIalpha(bool isWinFocused);

    extern void addThemeTist2Menu(AG_MenuItem *mi);
}

#endif // _UI_THEME_H_
