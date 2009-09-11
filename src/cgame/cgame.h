#ifndef _CGAME_H_
#define _CGAME_H_

#include <agar/core.h>
#include <agar/gui.h>

#include "../celestia/celestiacore.h"
#include "gamecore.h"

extern CelestiaCore* celAppCore;
extern GameCore* appGame;

extern void gameTerminate();

extern void BG_GainFocus();
extern void BG_LostFocus();

extern bool agMainMenuSticky;

namespace Core
{
    /** Remove body from solar system and satellites

        Also unref celestiacore if body if main refpoint, 
        sync solarsystem browsers for changes
    */
    extern void removeBody(Body *body); 

    /** Remove all bodys from planetary system
        but not deleting of planetary system it self
    */
    extern void clearPlanetarySystem(PlanetarySystem *planetary);

    /** Remove solar systems
     */
    extern void removeAllSolSys();
    /** Clear planetary and reload .ssc files 
     */
    extern void reloadAllSolSys();
}

#endif // _CGAME_H_
