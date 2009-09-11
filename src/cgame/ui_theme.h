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
    extern void setupThemes(PrimitiveStyle primitiveStyle);

    extern void updateGUIalpha(Uint32 tickDelta, bool agarUIfocused);
    extern void precomputeGUIalpha(bool isWinFocused);

}

#endif // _UI_THEME_H_
