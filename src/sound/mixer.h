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
