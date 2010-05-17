#include <agar/core.h>
#include <agar/gui.h>



#include "cgame.h"
#include "ui.h"
#include "videocapture.h"
#include "geekconsole.h"
#include <agar/core/types.h>
#include "ui_theme.h"

#include <celengine/starbrowser.h>
#include <celutil/directory.h>
#include <celutil/filetype.h>

#include "geekconsole.h"
#include "configfile.h"
#include "ui_theme.h"

#include <stdlib.h>

/**
 * UI subsystem
 **/

namespace UI
{
    static AG_Menu   *celMenu; // main menu

    static void playSnd(AG_Event *event)
    {
        int uisnd = (int ) AG_INT(1);
        if (uisnd >= 0 && uisnd < LASTUISND)
            playUISound((StdUiSounds) uisnd);
    }

    void addSndEvent(AG_Object *obj, const char *event_name, StdUiSounds snd)
    {
        AG_AddEvent(obj, event_name, playSnd, "%i", (int) snd);
    }

    /*  We need flagsets for menu binds
     *  there are several *Flags
     *  
     *  Struct RenderCfg contain presets for fast switching between 
     *  render quality (Ctrl+NUMBER)
     */

    typedef struct render_cfg {
        uint32  renderFlagCustom;
        uint32  labelFlagCelCustom;
        uint32  orbitFlagCustom;
        double  ambientLight;
        double  galaxyLight;
        int32   textureRes;
    } RenderCfg;


    RenderCfg renderCfgSets[6];
#define renderFlags renderCfgSets[0] // current render state flags
    bool showUI = true;

    RenderCfg renderFlagsOld;

    double ambMin=0.0;
    double ambMax=1.0;

    i32flags renderflags[] = {
        {"Stars",       0x0001},
        {"Planets",     0x0002},
        {"Galaxies",    0x0004},
        {"Diagrams",    0x0008},
        {"CloudMaps",   0x0010},
        {"Orbits",      0x0020},
        {"CelestialSphere", 0x0040},
        {"NightMaps",   0x0080},
        {"Atmospheres", 0x0100},
        {"SmoothLines", 0x0200},
        {"EclipseShadows", 0x0400},
        {"StarsAsPoints", 0x0800},
        {"RingShadows", 0x1000},
        {"Boundaries",  0x2000},
        {"AutoMag",     0x4000},
        {"CometTails",  0x8000},
        {"Markers",     0x10000},
        {"PartialTrajectories", 0x20000},
        {"Nebulae",     0x40000},
        {"OpenClusters", 0x80000},
        {"Globulars",   0x100000},
        {"CloudShadows", 0x200000},
        {"GalacticGrid", 0x400000},
        {"EclipticGrid", 0x800000},
        {"HorizonGrid", 0x1000000},
        {"Ecliptic",    0x2000000},
        {NULL}
    };

    i32flags labelflags[] = {
        {"Star",               0x001},
        {"Planet",             0x002},
        {"Moon",               0x004},
        {"Constellation",      0x008},
        {"Galaxy",             0x010},
        {"Asteroid",           0x020},
        {"Spacecraft",         0x040},
        {"Location",           0x080},
        {"Comet",              0x100},
        {"Nebula",             0x200},
        {"OpenCluster",        0x400},
        {"I18nConstellation",  0x800},
        {"DwarfPlanet",        0x1000},
        {"MinorMoon",          0x2000},
        {"Globular",           0x4000},
        {NULL}
    };

    i32flags orbitflag[] = {
        {"Planet",          0x01},
        {"Moon",            0x02},
        {"Asteroid",        0x04},
        {"Comet",           0x08},
        {"Spacecraft",      0x10},
        {"Invisible",       0x20},
        {"Barycenter",      0x40}, // Not used (invisible is used instead)
        {"SmallBody",       0x80}, // Not used
        {"DwarfPlanet",     0x100},
        {"Stellar",         0x200}, // only used for orbit mask
        {"SurfaceFeature",  0x400},
        {"Component",       0x800},
        {"MinorMoon",       0x1000},
        {"Diffuse",         0x2000},
        {"Unknown",         0x10000},
        {NULL}
    };

    void syncRenderToAgar()
    {
        renderFlags.renderFlagCustom = celAppCore->getRenderer()->getRenderFlags();
        renderFlags.labelFlagCelCustom = celAppCore->getRenderer()->getLabelMode();
        renderFlags.orbitFlagCustom = celAppCore->getRenderer()->getOrbitMask();
        renderFlags.ambientLight = celAppCore->getRenderer()->getAmbientLightLevel();
        renderFlags.textureRes = celAppCore->getRenderer()->getResolution();
        renderFlags.galaxyLight = Galaxy::getLightGain()*100; //percent
    }

    void syncRenderFromAgar()
    {
        if (renderFlags.renderFlagCustom != renderFlagsOld.renderFlagCustom)
        {
            celAppCore->getRenderer()->setRenderFlags(renderFlags.renderFlagCustom);
            renderFlagsOld.renderFlagCustom = renderFlags.renderFlagCustom;
        }
        if (renderFlagsOld.labelFlagCelCustom != renderFlags.labelFlagCelCustom)
        {
            celAppCore->getRenderer()->setLabelMode(renderFlags.labelFlagCelCustom);
            renderFlagsOld.labelFlagCelCustom = renderFlags.labelFlagCelCustom;
        }
        if (renderFlagsOld.orbitFlagCustom != renderFlags.orbitFlagCustom)
        {
            celAppCore->getRenderer()->setOrbitMask(renderFlags.orbitFlagCustom);
            renderFlagsOld.orbitFlagCustom = renderFlags.orbitFlagCustom;
        }
        if (renderFlagsOld.ambientLight != renderFlags.ambientLight)
        {
            celAppCore->getRenderer()->setAmbientLightLevel(renderFlags.ambientLight);
            renderFlagsOld.ambientLight = renderFlags.ambientLight;
        }
        if (renderFlagsOld.textureRes != renderFlags.textureRes)
        {
            celAppCore->getRenderer()->setResolution(renderFlags.textureRes);
            renderFlagsOld.textureRes = renderFlags.textureRes;
        }
        if (renderFlagsOld.galaxyLight != renderFlags.galaxyLight)
        {
            Galaxy::setLightGain(renderFlags.galaxyLight/100); // percent
            renderFlagsOld.galaxyLight = renderFlags.galaxyLight;
        }
    }
        
    void gameQuit(AG_Event *event)
    {
        gameTerminate();
    }

    static void gameQuitDlg(AG_Event *event)
    {
        AG_Button *btns[3];

        btns[0] = AG_ButtonNew(NULL, 0, NULL);
        btns[1] = AG_ButtonNew(NULL, 0, NULL);
        AG_Window *wDlg = AG_TextPromptOptions(btns, 2, _("Do you really want to quit this game:"));
        AG_ButtonText(btns[0], _("Yes"));
        AG_ButtonText(btns[1], _("No"));
        AG_SetEvent(btns[0], "button-pushed", gameQuit, NULL);
        AG_SetEvent(btns[1], "button-pushed", AGWINDETACH(wDlg));
        addSndEvent((AG_Object *) btns[1], "button-pushed", BTNCLICK);
    }

    namespace RenderCfgDial
    {
        AG_Window *celCfgWin = NULL;

        static void resetCelCfg(AG_Event *event)
        {
            int qualy = (int ) AG_INT(1);
            char buffer[128];
            cout << "resetting " << qualy << "\n";
            if (qualy == 0) // special case for current
            {
                cVarReset("render.current.show");
                cVarReset("render.current.orbit");
                cVarReset("render.current.label");
                cVarReset("render.current.ambientLight");
                cVarReset("render.current.galaxyLight");
                cVarReset("render.current.textureRes");
            } else {
                sprintf(buffer, "render.preset%i.show", qualy);
                cVarReset(buffer);
                sprintf(buffer, "render.preset%i.orbit", qualy);
                cVarReset(buffer);
                sprintf(buffer, "render.preset%i.label", qualy);
                cVarReset(buffer);
                sprintf(buffer, "render.preset%i.ambientLight", qualy);
                cVarReset(buffer);
                sprintf(buffer, "render.preset%i.galaxyLight", qualy);
                cVarReset(buffer);
                sprintf(buffer, "render.preset%i.textureRes", qualy);
                cVarReset(buffer);
            }
        }

        static void initDefaultFlags()
        {
            // current flags
            cVarBindFlags("render.current.show", &renderflags[0], &renderFlags.renderFlagCustom,
                          Renderer::ShowStars          |
                          Renderer::ShowPlanets        |
                          Renderer::ShowGalaxies       |
                          Renderer::ShowGlobulars      |
                          Renderer::ShowCloudMaps      |
                          Renderer::ShowAtmospheres    |
                          Renderer::ShowEclipseShadows |
                          Renderer::ShowRingShadows    |
                          Renderer::ShowCometTails     |
                          Renderer::ShowNebulae        |
                          Renderer::ShowOpenClusters   |
                          Renderer::ShowAutoMag        |
                          Renderer::ShowSmoothLines    |
                          Renderer::ShowMarkers);
            cVarBindFlags("render.current.label", &labelflags[0], &renderFlags.labelFlagCelCustom, 0);
            cVarBindFlags("render.current.orbit", &orbitflag[0], &renderFlags.orbitFlagCustom,
                          Body::Planet |
                          Body::Stellar|
                          Body::Moon);
            cVarBindFloat("render.current.ambientLight", &renderFlags.ambientLight, 0.08);
            cVarBindFloat("render.current.galaxyLight", &renderFlags.galaxyLight, 15.0);
            cVarBindInt32("render.current.textureRes", &renderFlags.textureRes, 2);

            // preset, base
            cVarBindFlags("render.preset1.show", &renderflags[0], &renderCfgSets[1].renderFlagCustom,
                          Renderer::ShowStars          |
                          Renderer::ShowPlanets        |
                          Renderer::ShowGalaxies       |
                          Renderer::ShowGlobulars      |
                          Renderer::ShowCloudMaps      |
                          Renderer::ShowAtmospheres    |
                          Renderer::ShowEclipseShadows |
                          Renderer::ShowRingShadows    |
                          Renderer::ShowCometTails     |
                          Renderer::ShowNebulae        |
                          Renderer::ShowOpenClusters   |
                          Renderer::ShowAutoMag        |
                          Renderer::ShowSmoothLines    |
                          Renderer::ShowMarkers);
            cVarBindFlags("render.preset1.label", &labelflags[0], &renderCfgSets[1].labelFlagCelCustom, 0);
            cVarBindFlags("render.preset1.orbit", &orbitflag[0], &renderCfgSets[1].orbitFlagCustom,
                          Body::Planet |
                          Body::Stellar|
                          Body::Moon);
            cVarBindFloat("render.preset1.ambientLight", &renderCfgSets[1].ambientLight, 0.08);
            cVarBindFloat("render.preset1.galaxyLight", &renderCfgSets[1].galaxyLight, 15.0);
            cVarBindInt32("render.preset1.textureRes", &renderCfgSets[1].textureRes, 2);

            // base + label
            cVarBindFlags("render.preset2.show", &renderflags[0], &renderCfgSets[2].renderFlagCustom,
                          Renderer::ShowStars          |
                          Renderer::ShowPlanets        |
                          Renderer::ShowGalaxies       |
                          Renderer::ShowGlobulars      |
                          Renderer::ShowCloudMaps      |
                          Renderer::ShowAtmospheres    |
                          Renderer::ShowEclipseShadows |
                          Renderer::ShowRingShadows    |
                          Renderer::ShowCometTails     |
                          Renderer::ShowNebulae        |
                          Renderer::ShowOpenClusters   |
                          Renderer::ShowAutoMag        |
                          Renderer::ShowSmoothLines    |
                          Renderer::ShowMarkers);
            cVarBindFlags("render.preset2.label", &labelflags[0], &renderCfgSets[2].labelFlagCelCustom,
                          Renderer::PlanetLabels|
                          Renderer::MoonLabels |
                          Renderer::DwarfPlanetLabels);
            cVarBindFlags("render.preset2.orbit", &orbitflag[0], &renderCfgSets[2].orbitFlagCustom,
                          Body::Planet |
                          Body::Stellar|
                          Body::Moon);
            cVarBindFloat("render.preset2.ambientLight", &renderCfgSets[2].ambientLight, 0.08);
            cVarBindFloat("render.preset2.galaxyLight", &renderCfgSets[2].galaxyLight, 15.0);
            cVarBindInt32("render.preset2.textureRes", &renderCfgSets[2].textureRes, 2);

            // base + label + orbit
            cVarBindFlags("render.preset3.show", &renderflags[0], &renderCfgSets[3].renderFlagCustom,
                          Renderer::ShowStars          |
                          Renderer::ShowPlanets        |
                          Renderer::ShowGalaxies       |
                          Renderer::ShowGlobulars      |
                          Renderer::ShowCloudMaps      |
                          Renderer::ShowAtmospheres    |
                          Renderer::ShowEclipseShadows |
                          Renderer::ShowRingShadows    |
                          Renderer::ShowCometTails     |
                          Renderer::ShowNebulae        |
                          Renderer::ShowOpenClusters   |
                          Renderer::ShowAutoMag        |
                          Renderer::ShowSmoothLines    |
                          Renderer::ShowOrbits         |
                          Renderer::ShowMarkers);
            cVarBindFlags("render.preset3.label", &labelflags[0], &renderCfgSets[3].labelFlagCelCustom,
                          Renderer::PlanetLabels|
                          Renderer::MoonLabels |
                          Renderer::DwarfPlanetLabels);
            cVarBindFlags("render.preset3.orbit", &orbitflag[0], &renderCfgSets[3].orbitFlagCustom,
                          Body::Planet |
                          Body::Stellar|
                          Body::Moon);
            cVarBindFloat("render.preset3.ambientLight", &renderCfgSets[3].ambientLight, 0.08);
            cVarBindFloat("render.preset3.galaxyLight", &renderCfgSets[3].galaxyLight, 15.0);
            cVarBindInt32("render.preset3.textureRes", &renderCfgSets[3].textureRes, 2);

            // cinema (no markers no labels)
            cVarBindFlags("render.preset4.show", &renderflags[0], &renderCfgSets[4].renderFlagCustom,
                          Renderer::ShowStars          |
                          Renderer::ShowPlanets        |
                          Renderer::ShowGalaxies       |
                          Renderer::ShowGlobulars      |
                          Renderer::ShowCloudMaps      |
                          Renderer::ShowAtmospheres    |
                          Renderer::ShowEclipseShadows |
                          Renderer::ShowRingShadows    |
                          Renderer::ShowCometTails     |
                          Renderer::ShowNebulae        |
                          Renderer::ShowOpenClusters   |
                          Renderer::ShowAutoMag        |
                          Renderer::ShowSmoothLines);
            cVarBindFlags("render.preset4.label", &labelflags[0], &renderCfgSets[4].labelFlagCelCustom, 0);
            cVarBindFlags("render.preset4.orbit", &orbitflag[0], &renderCfgSets[4].orbitFlagCustom,
                          Body::Planet |
                          Body::Stellar|
                          Body::Moon);
            cVarBindFloat("render.preset4.ambientLight", &renderCfgSets[4].ambientLight, 0.08);
            cVarBindFloat("render.preset4.galaxyLight", &renderCfgSets[4].galaxyLight, 15.0);
            cVarBindInt32("render.preset4.textureRes", &renderCfgSets[4].textureRes, 2);

            // minimum
            cVarBindFlags("render.preset5.show", &renderflags[0], &renderCfgSets[5].renderFlagCustom,
                          Renderer::ShowStars          |
                          Renderer::ShowPlanets        |
                          Renderer::ShowAtmospheres    |
                          Renderer::ShowCometTails     |
                          Renderer::ShowAutoMag        |
                          Renderer::ShowMarkers);
            cVarBindFlags("render.preset5.label", &labelflags[0], &renderCfgSets[5].labelFlagCelCustom, 0);
            cVarBindFlags("render.preset5.orbit", &orbitflag[0], &renderCfgSets[5].orbitFlagCustom,
                          Body::Planet |
                          Body::Stellar|
                          Body::Moon);
            cVarBindFloat("render.preset5.ambientLight", &renderCfgSets[5].ambientLight, 0.08);
            cVarBindFloat("render.preset5.galaxyLight", &renderCfgSets[5].galaxyLight, 15.0);
            cVarBindInt32("render.preset5.textureRes", &renderCfgSets[5].textureRes, 0);

        }

        static void setBodyLabelsCelCfg(AG_Event *event)
        {
            unsigned int *labelFlag = (unsigned int *) AG_PTR(1);
            *labelFlag = *labelFlag | Renderer::BodyLabelMask;
        }

        static void resetLabelCelCfg(AG_Event *event)
        {
            unsigned int *labelFlag = (unsigned int *) AG_PTR(1);
            *labelFlag = 0;     
        }
        
        static void copyRenderFlag(RenderCfg *rdest, RenderCfg *rsrc)
        {
            memcpy(rdest, rsrc, sizeof(RenderCfg));
            // rdest->renderFlagCustom = rsrc->renderFlagCustom;
            // rdest->orbitFlagCustom = rsrc->orbitFlagCustom;
            // rdest->labelFlagCelCustom = rsrc->orbitFlagCustom;
            // rdest->ambientLight = rsrc->ambientLight;
        }

        static void setPreset(AG_Event *event)
        {
            int i = AG_INT(1);
            if (i<6 && i >=1)
            {
                copyRenderFlag(&renderFlags, &renderCfgSets[i]);
            }
        }

        // Show cel pref window
        static void showCelPreference(AG_Event *event)
        {
            if (celCfgWin != NULL)
            {
                AG_WindowHide(celCfgWin);
                AGWIDGET(celCfgWin)->x = -1;
                AGWIDGET(celCfgWin)->y = -1;
                AG_WindowSetPosition(celCfgWin, AG_WINDOW_TC, 0);
                AG_WindowShow(celCfgWin);
            }   
        }

        static void hDblChkbox(void *parent, char *text, Uint32 *flag1, Uint32 mask1, Uint32 *flag2, Uint32 mask2)
        {
            AG_Box *hBox = AG_BoxNewHoriz(parent, AG_BOX_HFILL);
            {
                AG_BoxSetPadding(hBox, 0);
                AG_CheckboxNewFlag32 (hBox, 0, "", flag1, mask1);
                AG_CheckboxNewFlag32 (hBox, 0, text, flag2, mask2);
            }

        }
                
        static void setCurrentAsPreset(AG_Event *event)
        {
            RenderCfg *preset = (RenderCfg *) AG_PTR(1);                
            copyRenderFlag(preset, &renderFlags);
        }
                
        static AG_NotebookTab *celCfgWinNewTab(std::string tabName, AG_Notebook *nb, RenderCfg *renderCfg, int qualy)
        {
            Uint32 *renderFlag = &renderCfg->renderFlagCustom, 
                *labelFlagCel = &renderCfg->labelFlagCelCustom, 
                *orbitFlag = &renderCfg->orbitFlagCustom;
            double  *ambientLevel = &renderCfg->ambientLight;
            double  *galaxyLight = &renderCfg->galaxyLight;
            AG_NotebookTab *ntab;
            AG_Button *btn;

            ntab = AG_NotebookAddTab(nb, tabName.c_str(), AG_BOX_VERT);
            {
                AG_Box *hBox, *hBox2, *vBox;
                hBox = AG_BoxNewHoriz(ntab, AG_BOX_HFILL|AG_BOX_HOMOGENOUS);
                {               
                    vBox = AG_BoxNewVert(hBox, AG_BOX_VFILL|AG_BOX_FRAME);
                    {
                        AG_LabelNewStatic(vBox, 0, _("Show label/object:"));
                                        
                        hDblChkbox(vBox, _("Stars"), labelFlagCel, Renderer::StarLabels, renderFlag, Renderer::ShowStars);                      
                        hDblChkbox(vBox, _("Galaxies"), labelFlagCel, Renderer::GalaxyLabels, renderFlag, Renderer::ShowGalaxies);
                        hDblChkbox(vBox, _("Constellations"), labelFlagCel, Renderer::ConstellationLabels, renderFlag, Renderer::ShowDiagrams);
                        hDblChkbox(vBox, _("Nebulae"), labelFlagCel, Renderer::NebulaLabels, renderFlag, Renderer::ShowNebulae);
                        hDblChkbox(vBox, _("Open Clusters"), labelFlagCel, Renderer::OpenClusterLabels, renderFlag, Renderer::ShowOpenClusters);
                        hDblChkbox(vBox, _("Globulars"), labelFlagCel, Renderer::GlobularLabels, renderFlag, Renderer::ShowGlobulars);
                        hDblChkbox(vBox, _("Planets"), labelFlagCel, Renderer::PlanetLabels, renderFlag, Renderer::ShowPlanets);
                        AG_SeparatorNew(vBox, AG_SEPARATOR_HORIZ);
                        AG_LabelNewStatic(vBox, 0, _("Show object:"));
                        AG_CheckboxNewFlag(vBox, 0, _("Atmospheres"), renderFlag, Renderer::ShowAtmospheres);
                        AG_CheckboxNewFlag(vBox, 0, _("Clouds"), renderFlag, Renderer::ShowCloudMaps);
                        AG_CheckboxNewFlag(vBox, 0, _("CloudShadows"), renderFlag, Renderer::ShowCloudShadows);
                        AG_CheckboxNewFlag(vBox, 0, _("Night Side Lights"), renderFlag, Renderer::ShowNightMaps);
                        AG_CheckboxNewFlag(vBox, 0, _("Eclipse Shadows"), renderFlag, Renderer::ShowEclipseShadows);
                        AG_CheckboxNewFlag(vBox, 0, _("Ring Shadows"), renderFlag, Renderer::ShowRingShadows);
                        AG_CheckboxNewFlag(vBox, 0, _("Antialiasing"), renderFlag, Renderer::ShowSmoothLines);
                        AG_CheckboxNewFlag(vBox, 0, _("Celestial Grid"), renderFlag, Renderer::ShowCelestialSphere);
                        AG_CheckboxNewFlag(vBox, 0, _("Constellation Boundaries"), renderFlag, Renderer::ShowBoundaries);
                        AG_CheckboxNewFlag(vBox, 0, _("Galactic Grid"), renderFlag, Renderer::ShowGalacticGrid);
                        AG_CheckboxNewFlag(vBox, 0, _("Horizon Grid"), renderFlag, Renderer::ShowHorizonGrid);
                        AG_CheckboxNewFlag(vBox, 0, _("Ecliptic"), renderFlag, Renderer::ShowEcliptic);
                        AG_CheckboxNewFlag(vBox, 0, _("PartialTrajectories"), renderFlag, Renderer::ShowPartialTrajectories);
                        AG_CheckboxNewFlag(vBox, 0, _("Auto Magnitude"), renderFlag, Renderer::ShowAutoMag);
                        AG_CheckboxNewFlag(vBox, 0, _("Markers"), renderFlag, Renderer::ShowMarkers);
                        AG_CheckboxNewFlag(vBox, 0, _("Comets Tails"), renderFlag, Renderer::ShowCometTails);

                        AG_SeparatorNew(vBox, AG_SEPARATOR_HORIZ);
                        AG_Numerical *amb = AG_NumericalNewDblR(vBox, NULL, NULL, _("Ambient level"),
                                                                ambientLevel, ambMin, ambMax);
                        AG_NumericalSetIncrement(amb, 0.025);
                        amb = AG_NumericalNewDblR(vBox, NULL, NULL, _("Galaxy light gain"),
                                                  galaxyLight, 0, 100); 
                        AG_NumericalSetIncrement(amb, 5.0);
                    }
                    vBox = AG_BoxNewVert(hBox, AG_BOX_VFILL|AG_BOX_FRAME);
                    {

                        AG_LabelNewStatic(vBox, 0, _("Show label:"));
                        AG_CheckboxNewFlag (vBox, 0, _("Dwarf Planets"), labelFlagCel, Renderer::DwarfPlanetLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Moons"), labelFlagCel, Renderer::MoonLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Minor Moons"), labelFlagCel, Renderer::MinorMoonLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Comets"), labelFlagCel, Renderer::CometLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Asteroids"), labelFlagCel, Renderer::AsteroidLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Spacecraft"), labelFlagCel, Renderer::SpacecraftLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Locations"), labelFlagCel, Renderer::LocationLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("I18nConstellation"), labelFlagCel, Renderer::I18nConstellationLabels);
                        btn = AG_ButtonNewFn(vBox, 0, _("Show Body labels"),
                                       setBodyLabelsCelCfg, "%p", labelFlagCel);
                        addSndEvent((AG_Object *) btn, "button-pushed", BTNCLICK);
                        btn = AG_ButtonNewFn(vBox, 0, _("Disable labels"),
                                       resetLabelCelCfg, "%p", labelFlagCel);
                        addSndEvent((AG_Object *) btn, "button-pushed", BTNCLICK);
                        AG_SeparatorNew(vBox, AG_SEPARATOR_HORIZ);
                        //label = AG_LabelNewString (vBox, 0, _("Show Orbits:"));
                        AG_CheckboxNewFlag(vBox, 0, _("Show orbits"), renderFlag, Renderer::ShowOrbits);
                        AG_CheckboxNewFlag (vBox, 0, _("Stellar"), orbitFlag, Body::Stellar);
                        AG_CheckboxNewFlag (vBox, 0, _("Planet"), orbitFlag, Body::Planet);
                        AG_CheckboxNewFlag (vBox, 0, _("Moon"), orbitFlag, Body::Moon);
                        AG_CheckboxNewFlag (vBox, 0, _("Minor Moon"), orbitFlag, Body::MinorMoon);
                        AG_CheckboxNewFlag (vBox, 0, _("Asteroid"), orbitFlag, Body::Asteroid);
                        AG_CheckboxNewFlag (vBox, 0, _("Comet"), orbitFlag, Body::Comet);
                        AG_CheckboxNewFlag (vBox, 0, _("Spacecraft"), orbitFlag, Body::Spacecraft);
                        AG_CheckboxNewFlag (vBox, 0, _("Dwarf Planet"), orbitFlag, Body::DwarfPlanet);
                    }
                    hBox2 = AG_BoxNewHoriz(ntab, AG_BOX_HFILL|AG_BOX_HOMOGENOUS);
                    {
                        btn = AG_ButtonNewFn(hBox2, 0, _("Reset"),
                                       resetCelCfg, "%i", qualy);
                        addSndEvent((AG_Object *) btn, "button-pushed", BTNCLICK);
                        AG_Radio *radTextureRes;
                        const char *radioItems[] = {
                            _("Low Texture resolution"),
                            _("Medium"),
                            _("Hi"),
                            NULL
                        };
                        radTextureRes = AG_RadioNew(hBox2, 0, radioItems);
                        addSndEvent((AG_Object *) radTextureRes, "radio-changed", BTNSWICH);
                        AG_BindInt(radTextureRes, "value", &renderCfg->textureRes);
                    }
                }
            }
            return ntab;
        }

        void createCelCfgWindow()

        {
            int i;
            AG_NotebookTab *tab;
            AG_Button *btn;

            celCfgWin = AG_WindowNewNamed(0, "cel pref");
            AG_WindowSetCaption(celCfgWin, _("Celestia's render preference"));
            AG_Notebook *nb = AG_NotebookNew(celCfgWin, AG_NOTEBOOK_EXPAND);

            tab = celCfgWinNewTab(std::string("Current"), nb, &renderFlags, 0);
            AG_Box * hBox = AG_BoxNewHoriz(tab, AG_BOX_HFILL|AG_BOX_FRAME);
            {                           
                AG_LabelNewStatic(hBox, 0, _("Set current as preset:"));
                for (i = 1; i < 6; i++)
                {
                    btn = AG_ButtonNewFn(hBox, 0, NULL, setCurrentAsPreset, "%p", &renderCfgSets[i]);
                    addSndEvent((AG_Object *) btn, "button-pushed", BTNCLICK);
                    AG_ButtonText(btn, "%i", i);
                }                               
            }
                                
            // 0..5 celestia explore preference, also bind keys
            for (i=1; i<6; i++)
            {
                char buff[32];
                sprintf(buff,"%d",i);
                tab = celCfgWinNewTab(std::string("preset ") +  buff, nb, 
                                      &renderCfgSets[i], i);
                std::string str = std::string(_("To set this preference use C-x r "))+ buff;
                AG_SeparatorNew(tab, AG_SEPARATOR_HORIZ);
                btn = AG_ButtonNewFn(tab, 0, str.c_str(), 
                               setPreset, "%i", i);
                addSndEvent((AG_Object *) btn, "button-pushed", BTNCLICK);
                sprintf(buff,"C-x r %d @%d", i, i);
                geekConsole->bind("Global", buff,
                                  "set render preset");
            }
            // show and hide, need for "swith window" proper work
            showCelPreference(NULL); 
            AG_WindowHide(celCfgWin);
        }

        int GCSetPreset(GeekConsole *gc, int state, std::string value)
        {
            switch (state)
            {
            case 1:
            {
                int i = atoi(value.c_str());
                if (i > 0 && i <= 5 )
                    copyRenderFlag(&renderFlags, &renderCfgSets[i]);
                gc->finish();
                break;
            }
            case 0:
                gc->setInteractive(listInteractive, "r.preset", "Preset", "Set current preset");
                listInteractive->setCompletionFromSemicolonStr("1;2;3;4;5");
                listInteractive->setMatchCompletion(true);
                break;
            }
            return state;
        }

        int GCShowCelPreference(GeekConsole *gc, int state, std::string value)
        {
            showCelPreference(NULL);
            gc->finish();
            riseUI();
            return state;
        }

    }

    namespace SolarSysBrowser
    {
        static void addPlanetarySystemToTree(AG_Tlist *tl, const PlanetarySystem* sys, int depth);
        static void addBodyToTree(AG_Tlist *tl, Body* body, const char *tname, int depth)
        {
            AG_TlistItem *ti;
            const PlanetarySystem* satellites;

            ti = AG_TlistAdd(tl, NULL, "%s (%s)", body->getName().c_str(), tname);
            ti->depth = depth;
            ti->p1 = body;
            ti->cat = "body";
            satellites = body->getSatellites();

            if (satellites && satellites->getSystemSize()>0) {
                ti->flags |= AG_TLIST_HAS_CHILDREN;
            }
            if ((ti->flags & AG_TLIST_HAS_CHILDREN) &&
                AG_TlistVisibleChildren(tl, ti)) {
                addPlanetarySystemToTree(tl, satellites, depth + 1);
            }
        }

        static void addPlanetarySystemToTree(AG_Tlist *tl, const PlanetarySystem* sys, int depth)
        {
                        
            Body* body;
            const char *type;
            // first add planets, dwarf, moons, minor moons then rest
            for (int i = 0; i < sys->getSystemSize(); i++)
            {
                body = sys->getBody(i);
                switch(body->getClassification())
                {
                case Body::Planet:
                    type = _("Planet");
                    addBodyToTree(tl, body, type, depth);
                    break;
                default:
                    break;
                }                               
            }
            for (int i = 0; i < sys->getSystemSize(); i++)
            {
                body = sys->getBody(i);
                switch(body->getClassification())
                {
                case Body::DwarfPlanet:
                    type = _("Dwarf Planet");
                    addBodyToTree(tl, body, type, depth);
                    break;
                default:
                    break;
                }                               
            }
            for (int i = 0; i < sys->getSystemSize(); i++)
            {
                body = sys->getBody(i);
                switch(body->getClassification())
                {
                case Body::Moon:
                    type = _("Moon");
                    addBodyToTree(tl, body, type, depth);
                    break;
                default:
                    break;
                }                               
            }
            for (int i = 0; i < sys->getSystemSize(); i++)
            {
                body = sys->getBody(i);
                switch(body->getClassification())
                {
                case Body::MinorMoon:
                    type = _("Minor Moon");
                    addBodyToTree(tl, body, type, depth+1);
                    break;
                default:
                    break;
                }                               
            }

            for (int i = 0; i < sys->getSystemSize(); i++)
            {
                body = sys->getBody(i);
                switch(body->getClassification())
                {
                case Body::Planet:
                    break;
                case Body::Moon:
                    break;
                case Body::Asteroid:
                    type = _("Asteroid");
                    addBodyToTree(tl, body, type, depth);                               
                    break;
                case Body::Comet:
                    type = _("Comet");
                    addBodyToTree(tl, body, type, depth+1);
                    break;
                case Body::Spacecraft:
                    type = _("Spacecraft");
                    addBodyToTree(tl, body, type, depth);
                case Body::DwarfPlanet:
                    break;
                case Body::MinorMoon:
                    break;
                case Body::Barycenter: // no barycenters in tree
                    break;
                case Body::Unknown:
                default:
                    type = "-";
                    addBodyToTree(tl, body, type, depth);                                                                       
                    break;
                }
            }
        }

        static void pollSys(AG_Event *event)
        {
            AG_Tlist *tl = (AG_Tlist *)AG_SELF();
            AG_TlistItem *ti;
            Star *star = (Star *) AG_PTR(1);
            AG_TlistClear(tl);
            // add root
            ti = AG_TlistAdd(tl, NULL, celAppCore->getSimulation()->getUniverse()->getStarCatalog()->getStarName(*star).c_str());
            ti->depth = 0;
            ti->p1 = star;
            ti->cat = "star";
            SolarSystem* solarSys = celAppCore->getSimulation()->getUniverse()->getSolarSystem(star);
            if (solarSys)
            {
                PlanetarySystem* planetSys = solarSys->getPlanets();
                if (planetSys && planetSys->getSystemSize() > 0) {
                    ti->flags |= AG_TLIST_HAS_CHILDREN; 
                }

                if ((ti->flags & AG_TLIST_HAS_CHILDREN) &&
                    AG_TlistVisibleChildren(tl, ti)) {
                    addPlanetarySystemToTree(tl, planetSys, 1);
                }
            }
            AG_TlistRestore(tl);
        }



        static void actSelectBody(AG_Event *event)
        {
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_TlistItem *ti = AG_TlistSelectedItem(tl);
            celAppCore->getSimulation()->setSelection(Selection((Body *)ti->p1));
        }
        static void actGotoBody(AG_Event *event)
        {
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_TlistItem *ti = AG_TlistSelectedItem(tl);
            celAppCore->getSimulation()->setSelection(Selection((Body *)ti->p1));
            celAppCore->charEntered('g');
        }
        static void actCenterBody(AG_Event *event)
        {
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_TlistItem *ti = AG_TlistSelectedItem(tl);
            celAppCore->getSimulation()->setSelection(Selection((Body *)ti->p1));
            celAppCore->charEntered('c');
        }               
        static void     treeBodyPopMenu(AG_Event *event)
        {
            AG_MenuItem *mi = (AG_MenuItem *)AG_SENDER();
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_MenuAction(mi, _("Select"), NULL,
                          actSelectBody, "%p", tl);
            AG_MenuAction(mi, _("Go to"), NULL,
                          actGotoBody, "%p", tl);
            AG_MenuAction(mi, _("Center"), NULL,
                          actCenterBody, "%p", tl);
        }
        static void actSelectStar(AG_Event *event)
        {
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_TlistItem *ti = AG_TlistSelectedItem(tl);
            celAppCore->getSimulation()->setSelection(Selection((Star *)ti->p1));
        }
        static void actGotoStar(AG_Event *event)
        {
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_TlistItem *ti = AG_TlistSelectedItem(tl);
            celAppCore->getSimulation()->setSelection(Selection((Star *)ti->p1));
            celAppCore->charEntered('g');
        }
        static void actCenterStar(AG_Event *event)
        {
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_TlistItem *ti = AG_TlistSelectedItem(tl);
            celAppCore->getSimulation()->setSelection(Selection((Star *)ti->p1));
            celAppCore->charEntered('c');
        }               
        static void treeStarPopMenu(AG_Event *event)
        {
            AG_MenuItem *mi = (AG_MenuItem *)AG_SENDER();
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_MenuAction(mi, _("Select"), NULL,
                          actSelectStar, "%p", tl);
            AG_MenuAction(mi, _("Go to"), NULL,
                          actGotoStar, "%p", tl);
            AG_MenuAction(mi, _("Center"), NULL,
                          actCenterStar, "%p", tl);
        }
                
        static void treeDblClick(AG_Event *event)
        {
            AG_Tlist *tl = (AG_Tlist *)AG_PTR(1);
            AG_TlistItem *ti = AG_TlistSelectedItem(tl);
            if (strcmp(ti->cat, "star") == 0)
                celAppCore->getSimulation()->setSelection(Selection((Star *)ti->p1));
            else
                if (strcmp(ti->cat, "body") == 0)
                    celAppCore->getSimulation()->setSelection(Selection((Body *)ti->p1));
            //center view
            celAppCore->charEntered('c');                       
        }

        static void treeKeyDown(AG_Event *event)
        {
            int keysym = AG_INT(1);
            AG_Tlist *tl = (AG_Tlist *)AG_SELF();
            AG_TlistItem *ti = AG_TlistSelectedItem(tl);
            Selection sel;
            if (!ti)
                return;
            if (strcmp(ti->cat, "star") == 0)
                sel = Selection((Star *)ti->p1);
            else
                if (strcmp(ti->cat, "body") == 0)
                    sel = Selection((Body *)ti->p1);
            switch (keysym)
            {
            case 'g': // go to
                celAppCore->getSimulation()->setSelection(sel);
                celAppCore->charEntered('g');
                break;
            case 'd': 
                Core::removeBody(sel.body());
                break;
            case 'r': // refresh
//                AG_TlistRefresh(tl);
                break;
            case 's':
            case '\n':
            case '\r':
                celAppCore->getSimulation()->setSelection(sel);
            break;
            case 'c': // center view
                celAppCore->getSimulation()->setSelection(sel);
                celAppCore->charEntered('c');
                break;
            case '+': // expand
            case '-':
            case AG_KEY_RIGHT:
            case AG_KEY_LEFT:
                {
                    if (ti->flags & AG_TLIST_VISIBLE_CHILDREN) 
                    {
                        ti->flags &= ~AG_TLIST_VISIBLE_CHILDREN;
                    }
                    else {
                        ti->flags |=  AG_TLIST_VISIBLE_CHILDREN;
                    }
                    tl->flags |= AG_TLIST_REFRESH;              
                }
            break;
            default:
                break;
            }
        }

        static void createStarBrwWindow(const Star* star)
        {
            AG_Tlist *tl;
            AG_MenuItem *mi;
            if (!star)
                return;
            std::string starName = celAppCore->getSimulation()->getUniverse()->getStarCatalog()->getStarName(*star);
            AG_Window *win = AG_WindowNew(0);
            if (!win) 
                return;
            std::string s = starName + _(": Solar System Browser");
            AG_WindowSetCaption(win, s.c_str());

            tl = AG_TlistNewPolled(win, AG_TLIST_EXPAND, pollSys, "%p", star);                  
            {                           
                AG_TlistSizeHint(tl, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 22);
                mi = AG_TlistSetPopup(tl, "body");
                AG_MenuSetPollFn(mi, treeBodyPopMenu, "%p", tl);
                mi = AG_TlistSetPopup(tl, "star");
                AG_MenuSetPollFn(mi, treeStarPopMenu, "%p", tl);
                AG_TlistSetDblClickFn(tl, treeDblClick, "%p", tl);
                AG_AddEvent(tl, "window-keydown", treeKeyDown, NULL);
            }
            AG_WindowSetPosition(win, AG_WINDOW_MR, 0);
            AG_WindowShow(win);
            AG_TlistSetRefresh(tl, 500); // no auto refresh
        }

        static void browseNearestSolarSystem(AG_Event *event)
        {
            const SolarSystem* solarSys = celAppCore->getSimulation()->getNearestSolarSystem();
            if (solarSys)
                createStarBrwWindow(solarSys->getStar());
        }

        // callback for geekconsole
        int GCBrowseNearestSolarSystem(GeekConsole *gc, int state, std::string value)
        {
            browseNearestSolarSystem(NULL);
            gc->finish();
            riseUI();
            return state;
        }
    }

    namespace StarBrowserDialog
    {
        AG_Window *starBrowWin = NULL;
        StarBrowser starBrowser;
        AG_Radio *radStarEntry;
        AG_Table *table = NULL;
        int bGreekNames = 1;

        int numListStars = 100;
        int minListStars = 2;
        int maxListStars = 500;

        void createStarBrwWindow();
                
        static void showStarBrowser(AG_Event *event)
        {
            if (starBrowWin == NULL)
                createStarBrwWindow();
            if (starBrowWin != NULL)
            {
                AG_WindowHide(starBrowWin);
                AGWIDGET(starBrowWin)->x = -1;
                AGWIDGET(starBrowWin)->y = -1;
                int w;
                AG_TextSize("X",&w, NULL);
                AG_WindowSetGeometryAligned(starBrowWin, AG_WINDOW_ML, w*(40+9*3), 600);
                AG_WindowShow(starBrowWin);
            }   
        }
                
        static void actUpdateTable(AG_Event *event)
        {
            AG_Table *t = StarBrowserDialog::table;
            StarDatabase* stardb;
            vector<const Star*> *stars;
            unsigned int currentLength;
            UniversalCoord ucPos;
            if (!t) return;
            /* Load the catalogs and set data */
            stardb = celAppCore->getSimulation()->getUniverse()->getStarCatalog();                      
            starBrowser.refresh();
            stars = starBrowser.listStars(numListStars);
            if (!stars)
                return;
            currentLength = (*stars).size();
            if (currentLength == 0)
                return;
            celAppCore->getSimulation()->setSelection(Selection((Star *)(*stars)[0]));
            ucPos = celAppCore->getSimulation()->getObserver().getPosition();

            AG_TableBegin(t);
            for (unsigned int i = 0; i < currentLength; i++)
            {
                const Star *star=(*stars)[i];
                float d = ucPos.offsetFromLy(star->getPosition()).norm();
                AG_TableAddRow(t, "%s:%.03f:%s:%.03f:%.03f:%p", 
                               bGreekNames  ? ReplaceGreekLetterAbbr(stardb->getStarName(*star)).c_str()
                               :stardb->getStarName(*star).c_str(),
                               d,
                               star->getSpectralType(),
                               astro::absToAppMag(star->getAbsoluteMagnitude(), d),
                               star->getAbsoluteMagnitude(),
                               star);
            }
            AG_TableEnd(t);
        }

        static void actSelect(AG_Event *event)
        {
            for (unsigned int i = 0; i < StarBrowserDialog::table->m; i++ )
            {
                if (AG_TableRowSelected(StarBrowserDialog::table, i))
                {
                    Star *selStar= (Star *)StarBrowserDialog::table->cells[i][5].data.p;
                    if (selStar)
                    {                                   
                        celAppCore->getSimulation()->setSelection(Selection(selStar));
                    }
                    return;
                }
            }
        }

        static void actGoto(AG_Event *event)
        {
            for (unsigned int i = 0; i < StarBrowserDialog::table->m; i++ )
            {
                if (AG_TableRowSelected(StarBrowserDialog::table, i))
                {
                    Star *selStar= (Star *)StarBrowserDialog::table->cells[i][5].data.p;
                    if (selStar)
                    {                                   
                        celAppCore->getSimulation()->setSelection(Selection(selStar));
                        celAppCore->charEntered('g');
                    }
                    return;
                }
            }
        }

        static void actCenterView(AG_Event *event)
        {
            for (unsigned int i = 0; i < StarBrowserDialog::table->m; i++ )
            {
                if (AG_TableRowSelected(StarBrowserDialog::table, i))
                {
                    Star *selStar= (Star *)StarBrowserDialog::table->cells[i][5].data.p;
                    if (selStar)
                    {                                   
                        celAppCore->getSimulation()->setSelection(Selection(selStar));
                        celAppCore->charEntered('c');
                    }
                    return;
                }
            }
        }

        static void actBrowse(AG_Event *event)
        {
            for (unsigned int i = 0; i < StarBrowserDialog::table->m; i++ )
            {
                if (AG_TableRowSelected(StarBrowserDialog::table, i))
                {
                    Star *selStar= (Star *)StarBrowserDialog::table->cells[i][5].data.p;
                    if (selStar)
                    {                                   
                        SolarSysBrowser::createStarBrwWindow(selStar);
                    }
                    return;
                }
            }
        }

        static void tblKeyDown(AG_Event *event)
        {
            int keysym = AG_INT(1);
            switch (keysym)
            {
            case 'g':
                actGoto(NULL);          
                break;
            case 's':
            case '\n':
            case '\r':
                actSelect(NULL);
            break;
            case 'c':
                actCenterView(NULL);
                break;
            case 'r':
                actUpdateTable(NULL);
                break;
            case 'b':
                actBrowse(NULL);
                break;
            default:
                break;
            }                                                   
        }
                
        static void radioChanged(AG_Event *event)
        {
            int i =  AG_INT(1);
            starBrowser.setPredicate(i);
            actUpdateTable(NULL);
        }

        void createStarBrwWindow()
        {
            AG_Button *btn;
            starBrowWin = AG_WindowNewNamed(0, "cel star browser");
            starBrowser.setSimulation(celAppCore->getSimulation());
            AG_WindowSetCaption(starBrowWin, _("Star browser"));
            table = AG_TableNew(starBrowWin, AG_TABLE_EXPAND);
            {
                AG_TableAddCol(table, _("Name"), "<XXXXXXXXXXXXXXXXXXXXXXXXXXXX>", NULL);
                AG_TableAddCol(table, _("Distance"), "<XXXXXXXXX>", NULL);
                AG_TableAddCol(table, _("Type"), "<XXXXX>", NULL);
                AG_TableAddCol(table, _("App. Mag"), "<XXXXXXXXX>", NULL);
                AG_TableAddCol(table, _("Abs. Mag"), "<XXXXXXXXX>", NULL);
                AG_TableAddCol(table, NULL, NULL, NULL);

            }
            // key events for table
            AG_AddEvent(table, "window-keydown", tblKeyDown, NULL);

            AG_MenuItem *mi;
            mi = AG_TableSetPopup(table, -1, -1);
            {
                AG_MenuAction(mi, _("Select"),
                              NULL, actSelect, NULL);
                AG_MenuAction(mi, _("Center"),
                              NULL, actCenterView, NULL);
                AG_MenuAction(mi, _("Go to"),
                              NULL, actGoto, NULL);
                AG_MenuAction(mi, _("Browse"),
                              NULL, actBrowse, NULL);                           
            }
            AG_CheckboxNewInt(starBrowWin, 0, _("Greek names"), &bGreekNames);
            AG_Box *hBox = AG_BoxNewHoriz(starBrowWin, AG_BOX_HFILL| AG_BOX_HOMOGENOUS);
            {
                AG_NumericalNewIntR(hBox, NULL, NULL, _("Stars: "), &numListStars, 
                                    minListStars, maxListStars);
                AG_SliderNewInt(hBox, AG_SLIDER_HORIZ,
                                NULL,
                                &numListStars, &minListStars, &maxListStars);
            }
            hBox = AG_BoxNewHoriz(starBrowWin, AG_BOX_HFILL| AG_BOX_HOMOGENOUS);
            {
                btn = AG_ButtonNewFn(hBox, 0, _("Refresh"),
                               actUpdateTable, NULL);
                addSndEvent((AG_Object *) btn, "button-pushed", BTNCLICK);
                const char *radioItems[] = {
                    _("Nearest"),
                    _("Brightest (App.)"),
                    _("Brightest (Abs.)"),
                    _("With Planets"),
                    NULL
                };
                                
                radStarEntry = AG_RadioNewFn(hBox, 0, radioItems, radioChanged, NULL);
                addSndEvent((AG_Object *) radStarEntry, "radio-changed", BTNSWICH);
            }

        }

        // callback for geekconsole
        int GCShowStarBrowser(GeekConsole *gc, int state, std::string value)
        {
            showStarBrowser(NULL);
            gc->finish();
            riseUI();
            return state;
        }

    } // StarBrowserDialog

    namespace Menu
    {
                
        static void mnuNavigation(AG_Event *event)
        {
            AG_MenuItem *mi = (AG_MenuItem *)AG_SENDER();
            AG_MenuAction(mi, _("Star browser"), 0,
                          StarBrowserDialog::showStarBrowser, NULL);
            AG_MenuAction(mi, _("Solar system browser"), 0,
                          SolarSysBrowser::browseNearestSolarSystem , NULL);
        }

        static void actReloadAllSolSys(AG_Event *event)
        {
            Core::reloadAllSolSys();
        }               

        static void mnuOption(AG_Event *event)
        {
            AG_MenuItem *mi = (AG_MenuItem *)AG_SENDER();
            AG_MenuAction(mi, _("Preferences..."), agIconMagnifier.s,
                          RenderCfgDial::showCelPreference, NULL);
            if (appGame->getGameMode() == GameCore::VIEWER)
            {
                AG_MenuAction(mi, _("Reload Solar systems"), NULL,
                              actReloadAllSolSys, NULL);
            }
        }

        static void createMenuRender(AG_MenuItem *mi)
        {
            AG_MenuItem *item = AG_MenuNode(mi, _("Show"), NULL);               
            AG_MenuInt32Flags(item, _("Stars"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowStars, 0);                  
            AG_MenuInt32Flags(item, _("Galaxies"),agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowGalaxies, 0);
            AG_MenuInt32Flags(item, _("Constellations"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowDiagrams, 0);
            AG_MenuInt32Flags(item, _("Nebulae"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowNebulae, 0);
            AG_MenuInt32Flags(item, _("Open Clusters"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowOpenClusters, 0);
            AG_MenuInt32Flags(item, _("Globulars"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowGlobulars, 0);
            AG_MenuInt32Flags(item, _("Planets"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowPlanets, 0);
            AG_MenuInt32Flags(item, _("Atmospheres"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowAtmospheres, 0);
            AG_MenuInt32Flags(item, _("Clouds"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowCloudMaps, 0);
            AG_MenuInt32Flags(item, _("CloudShadows"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowCloudShadows, 0);
            AG_MenuInt32Flags(item, _("Night Side Lights"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowNightMaps, 0);
            AG_MenuInt32Flags(item, _("Eclipse Shadows"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowEclipseShadows, 0);
            AG_MenuInt32Flags(item, _("Ring Shadows"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowRingShadows, 0);
            AG_MenuInt32Flags(item, _("Antialiasing"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowSmoothLines, 0);
            AG_MenuInt32Flags(item, _("Celestial Grid"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowCelestialSphere, 0);
            AG_MenuInt32Flags(item, _("Constellation Boundaries"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowBoundaries, 0);
            AG_MenuInt32Flags(item, _("Galactic Grid"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowGalacticGrid, 0);
            AG_MenuInt32Flags(item, _("Horizon Grid"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowHorizonGrid, 0);
            AG_MenuInt32Flags(item, _("Ecliptic"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowEcliptic, 0);
            AG_MenuInt32Flags(item, _("PartialTrajectories"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowPartialTrajectories, 0);
            AG_MenuInt32Flags(item, _("Auto Magnitude"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowAutoMag, 0);
            AG_MenuInt32Flags(item, _("Markers"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowMarkers, 0);
            AG_MenuInt32Flags(item, _("Comets Tails"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowCometTails, 0);
            AG_MenuInt32Flags(item, _("Orbits"), agIconClose.s, &renderFlags.renderFlagCustom, Renderer::ShowOrbits, 0);
            item = AG_MenuNode(mi, _("Label"), NULL);
            AG_MenuInt32Flags(item, _("Stars"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::StarLabels, 0);                       
            AG_MenuInt32Flags(item, _("Galaxies"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::GalaxyLabels, 0);
            AG_MenuInt32Flags(item, _("Constellations"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::ConstellationLabels, 0);
            AG_MenuInt32Flags(item, _("Nebulae"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::NebulaLabels, 0);
            AG_MenuInt32Flags(item, _("Open Clusters"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::OpenClusterLabels, 0);
            AG_MenuInt32Flags(item, _("Globulars"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::GlobularLabels, 0);
            AG_MenuInt32Flags(item, _("Planets"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::PlanetLabels, 0);
            AG_MenuInt32Flags(item, _("Dwarf Planets"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::DwarfPlanetLabels, 0);
            AG_MenuInt32Flags(item, _("Moons"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::MoonLabels, 0);
            AG_MenuInt32Flags(item, _("Minor Moons"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::MinorMoonLabels, 0);
            AG_MenuInt32Flags(item, _("Comets"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::CometLabels, 0);
            AG_MenuInt32Flags(item, _("Asteroids"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::AsteroidLabels, 0);
            AG_MenuInt32Flags(item, _("Spacecraft"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::SpacecraftLabels, 0);
            AG_MenuInt32Flags(item, _("Locations"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::LocationLabels, 0);
            AG_MenuInt32Flags(item, _("I18nConstellation"), agIconClose.s, &renderFlags.labelFlagCelCustom, Renderer::I18nConstellationLabels, 0);
            AG_MenuSeparator(item);
            AG_MenuAction(item, _("Disable labels"), NULL, RenderCfgDial::resetLabelCelCfg, "%p", &renderFlags.labelFlagCelCustom);
            item = AG_MenuNode(mi, _("Orbit"), NULL);
            AG_MenuInt32Flags(item, _("Stellar"), agIconClose.s, &renderFlags.orbitFlagCustom, Body::Stellar, 0);
            AG_MenuInt32Flags(item, _("Planet"), agIconClose.s, &renderFlags.orbitFlagCustom, Body::Planet, 0);
            AG_MenuInt32Flags(item, _("Moon"), agIconClose.s, &renderFlags.orbitFlagCustom, Body::Moon, 0);
            AG_MenuInt32Flags(item, _("Minor Moon"), agIconClose.s, &renderFlags.orbitFlagCustom, Body::MinorMoon, 0);
            AG_MenuInt32Flags(item, _("Asteroid"), agIconClose.s, &renderFlags.orbitFlagCustom, Body::Asteroid, 0);
            AG_MenuInt32Flags(item, _("Comet"), agIconClose.s, &renderFlags.orbitFlagCustom, Body::Comet, 0);
            AG_MenuInt32Flags(item, _("Spacecraft"), agIconClose.s, &renderFlags.orbitFlagCustom, Body::Spacecraft, 0);
            AG_MenuInt32Flags(item, _("Dwarf Planet"), agIconClose.s, &renderFlags.orbitFlagCustom, Body::DwarfPlanet, 0);
        }

        static void initMenu()
        {
            AG_MenuItem *itemopt, *itemopt2;
            celMenu = AG_MenuNewGlobal(0);
            itemopt = AG_MenuNode(celMenu->root, _("Game"), NULL);
            AG_MenuAction(itemopt, _("Movie capture..."), NULL,
                          showVidCaptureDlg, NULL);
            AG_MenuAction(itemopt, _("Quit..."), agIconMagnifier.s,
                          gameQuitDlg, NULL);

            itemopt = AG_MenuNode(celMenu->root, _("Navigation"), NULL);
            AG_MenuSetPollFn(itemopt, mnuNavigation, NULL);

            itemopt = AG_MenuNode(celMenu->root, _("Option"), NULL);
            AG_MenuSetPollFn(itemopt, mnuOption, NULL);

            itemopt = AG_MenuNode(celMenu->root, _("Render"), NULL);
            createMenuRender(itemopt);
            itemopt = AG_MenuNode(celMenu->root, _("Customize"), NULL);
            itemopt2 = AG_MenuNode(itemopt, _("UI Theme"), NULL);
            UI::createMenuTheme(itemopt2);
        }
    }

    void closeCurrentFocusedWindow()
    {
        AG_Window *w = AG_WindowFindFocused();
        if (w)
            AG_WindowHide(w);
    }

    void maximizeCurrentFocusedWindow()
    {
        AG_Window *w = AG_WindowFindFocused();
        if (w) {
            if (w->flags & AG_WINDOW_MAXIMIZED)
                AG_WindowUnmaximize(w);
            else
                AG_WindowMaximize(w);
        }
    }

    void minimizeCurrentFocusedWindow()
    {
        AG_Window *w = AG_WindowFindFocused();
        if (w)
            AG_WindowMinimize(w);
    }

    void toggleShowUI()
    {
        if (showUI)
            BG_GainFocus();
        else
            BG_LostFocus();
        showUI = !showUI;
    }

    void riseUI()
    {
        BG_LostFocus();
        showUI = true;
    }

    // switch to agar's window
    int switchWindow(GeekConsole *gc, int state, std::string value)
    {
        AG_Window *win;
        switch (state)
        {
        case 0:
        {
            std::vector<std::string> completion;
            AG_FOREACH_WINDOW_REVERSE(win, agDriverSw) {
                AG_ObjectLock(win);
                if ((win->flags & AG_WINDOW_DENYFOCUS) == 0)
                    completion.push_back(std::string(win->caption));
                AG_ObjectUnlock(win);
            }
            gc->setInteractive(listInteractive, "agar-window",
                               _("Window"), _("Switch to window"));
            listInteractive->setCompletion(completion);
            listInteractive->setMatchCompletion(true);
            break;
        }
        case 1:
            riseUI();
            AG_FOREACH_WINDOW_REVERSE(win, agDriverSw) {
                AG_ObjectLock(win);
                if ((win->flags & AG_WINDOW_DENYFOCUS) == 0)
                    if (strcmp(value.c_str(), win->caption) == 0)
                    {
                        AG_WindowUnminimize(win);
                    }
                AG_ObjectUnlock(win);
            }
            gc->finish();
            break;
        default:
            break;
        }
        return state;
    }

    int quitFunction(GeekConsole *gc, int state, std::string value)
    {
        switch (state)
        {
        case 1:
            if (value == "yes")
                gameTerminate();
            else
                gc->finish();
            break;
        case 0:
            gc->setInteractive(listInteractive, "quit", "Are You Sure?", "Quit from game");
            listInteractive->setCompletionFromSemicolonStr("yes;no");
            listInteractive->setMatchCompletion(true);
            break;
        }
        return state;
    }

    void Init()
    {
        UI::initThemes();
        UI::setupThemeStyle(
            (PrimitiveStyle) cVarGetInt32("render.agar.primitive"));

        RenderCfgDial::initDefaultFlags();

        RenderCfgDial::createCelCfgWindow();

        Menu::initMenu();

        celAppCore->setContextMenuCallback(ContextMenu::menuContext);

        geekConsole->registerAndBind("", "C-x k",
                                     GCFunc(closeCurrentFocusedWindow), "close window");
        geekConsole->registerFunction(GCFunc(minimizeCurrentFocusedWindow), "minimize window");
        geekConsole->registerAndBind("", "C-x 1",
                                     GCFunc(maximizeCurrentFocusedWindow), "maximize window");
        geekConsole->registerAndBind("", "C-o",
                                     GCFunc(toggleShowUI), "toggle ui");
        geekConsole->registerAndBind("", "C-x r r",
                                     GCFunc(RenderCfgDial::GCSetPreset), "set render preset");
        // geekconsole
        geekConsole->registerFunction(GCFunc(StarBrowserDialog::GCShowStarBrowser), "show solar browser");
        geekConsole->registerFunction(GCFunc(SolarSysBrowser::GCBrowseNearestSolarSystem), "show solar system browser");
        geekConsole->registerFunction(GCFunc(RenderCfgDial::GCShowCelPreference), "show preference");
        geekConsole->registerAndBind("", "C-x C-c",
                                     GCFunc(quitFunction), "quit");
        geekConsole->registerAndBind("", "C-x b",
                                     GCFunc(switchWindow), "switch window");

    }
} //namespace UI
