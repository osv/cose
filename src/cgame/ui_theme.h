// Copyright (C) 2010-2011 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

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
