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

    extern void createMenuTheme(AG_MenuItem *menu);
}

#endif // _UI_THEME_H_
