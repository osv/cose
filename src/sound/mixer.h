// Copyright (C) 2010 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _MIXER_H_
#define _MIXER_H_

extern bool mixerInit();
extern void mixerShutdown();

// standart ui sounds
enum StdUiSounds {
    BTNCLICK,
    MATCH, // like button click
    BTNSWICH,
    BEEP,
    LASTUISND
};

void playUISound(StdUiSounds s);

#endif // _MIXER_H_
