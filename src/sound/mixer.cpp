#include "mixer.h"
#include <cgame/configfile.h>
#include <math.h>
#include <iostream>

#include "source.h"

#include "snd_local.h"

using namespace std;

clunk::Context *context = NULL; //main clunk context

clunk::Object *uio;

ResourceHandle uis[LASTUISND];

bool mixerInit()
{
    if (context)
        return true;
    context = new clunk::Context();
    int rate = cVarGetInt32("mixer.s_khz");
    switch (rate)
    {
    case 22:
        rate = 22050;
        break;
    default:
        cVarReset("mixer.s_khz");
    case 44:
        rate = 44100;
        break;
    }

    try {
        context->init(rate, 2, 1024);
    } catch (const std::exception &_e) {
        cout << "Mixer: " << _e.what() << "\n" << "Sound will be diabled.\n";
        delete context;
        context = NULL;
        return false;
    }

    uio = context->create_object();

    // preload ui sounds
    uis[BTNCLICK] = GetSampleManager()->getHandle(SamplerInfo("ui/btnclick.wav"));
    uis[MATCH] = GetSampleManager()->getHandle(SamplerInfo("ui/match.wav"));
    uis[BTNSWICH] = GetSampleManager()->getHandle(SamplerInfo("ui/switch.wav"));
    uis[BEEP] = GetSampleManager()->getHandle(SamplerInfo("ui/beep.wav"));

    // preload
    GetSampleManager()->find(uis[BTNCLICK]);
    GetSampleManager()->find(uis[BTNSWICH]);
    GetSampleManager()->find(uis[BEEP]);

    return true;
}

void mixerShutdown()
{
    if (!context)
        return;
    context->deinit();
    delete context;
}

void playUISound(StdUiSounds s)
{
    if (!context && s == LASTUISND)
        return;
    clunk::Sample *sample = GetSampleManager()->find(uis[s]);
    if (sample)
    {
        uio->play("ui", new clunk::Source(sample, false));
    }

}
