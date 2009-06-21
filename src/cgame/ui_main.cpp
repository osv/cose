#include <agar/core.h>
#include <agar/gui.h>



#include "cgame.h"
#include "ui.h"
#include "videocapture.h"

#include <celengine/starbrowser.h>
#include <celutil/directory.h>
#include <celutil/filetype.h>

#include "gkey.h"

#include <stdlib.h>

/**
 * UI subsystem
 **/

static AG_Menu	 *celMenu;

namespace UI
{
    /*	We need flagsets for menu binds
     *	there are several *Flags
     *	
     *	Struct RenderCfg contain presets for fast switching between 
     *	render quality (Ctrl+NUMBER)
     */


    bool showUI = true;

    typedef struct render_cfg {
        Uint32  renderFlagCustom;
        Uint32  labelFlagCelCustom;
        Uint32  orbitFlagCustom;
        float   ambientLight;
        float   galaxyLight;
        int     textureRes;
    } RenderCfg;
        
    RenderCfg renderCfgSets[5];
    RenderCfg renderFlags; // current render state flags
    RenderCfg renderFlagsOld;

    float ambMin=0.0;
    float ambMax=1.0;

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
    }

    namespace RenderCfgDial
    {
        AG_Window *celCfgWin = NULL;

        static void resetCelCfg(AG_Event *event)
        {
            RenderCfg *cfg = (RenderCfg *) AG_PTR(1);
            cfg->renderFlagCustom = Renderer::ShowStars          |
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
                Renderer::ShowMarkers;
            cfg->labelFlagCelCustom = Renderer::PlanetLabels|
                Renderer::MoonLabels|
                Renderer::DwarfPlanetLabels;
            cfg->orbitFlagCustom = Body::Planet|
                Body::Stellar|
                Body::Moon;
            cfg->ambientLight = 0.1f;
            cfg->textureRes = 2;
            cfg->galaxyLight = 15; 
        }

        static void initDefaultFlags()
        {
            AG_Event event;
            AG_EventArgs(&event, "%p", &renderCfgSets[0]);
            resetCelCfg(&event);
            AG_EventArgs(&event, "%p", &renderCfgSets[1]);
            resetCelCfg(&event);
            AG_EventArgs(&event, "%p", &renderCfgSets[2]);
            resetCelCfg(&event);
            AG_EventArgs(&event, "%p", &renderCfgSets[3]);
            resetCelCfg(&event);
            AG_EventArgs(&event, "%p", &renderCfgSets[4]);
            resetCelCfg(&event);

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

        void setCelRenderFlagsSets1()
        {
            copyRenderFlag(&renderFlags, &renderCfgSets[0]);
        }

        void setCelRenderFlagsSets2()
        {
            copyRenderFlag(&renderFlags, &renderCfgSets[1]);
        }

        void setCelRenderFlagsSets3()
        {
            copyRenderFlag(&renderFlags, &renderCfgSets[2]);
        }

        void setCelRenderFlagsSets4()
        {
            copyRenderFlag(&renderFlags, &renderCfgSets[3]);
        }

        void setCelRenderFlagsSets5()
        {
            copyRenderFlag(&renderFlags, &renderCfgSets[4]);
        }

        static void setPreset(AG_Event *event)
        {
            int i = AG_INT(1);
            if (i<5 && i >=0)
            {
                copyRenderFlag(&renderFlags, &renderCfgSets[i]);
            }
        }

        // Show cel pref window
        static void celPreference(AG_Event *event)
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
                
        static AG_NotebookTab *celCfgWinNewTab(std::string tabName, AG_Notebook *nb, RenderCfg *renderCfg)
        {
            Uint32 *renderFlag = &renderCfg->renderFlagCustom, 
                *labelFlagCel = &renderCfg->labelFlagCelCustom, 
                *orbitFlag = &renderCfg->orbitFlagCustom;
            float  *ambientLevel = &renderCfg->ambientLight;
            float  *galaxyLight = &renderCfg->galaxyLight;
            AG_NotebookTab *ntab;
            ntab = AG_NotebookAddTab(nb, tabName.c_str(), AG_BOX_VERT);
            {
                AG_Box *hBox, *hBox2, *vBox;
                hBox = AG_BoxNewHoriz(ntab, AG_BOX_HFILL|AG_BOX_HOMOGENOUS);
                {               
                    vBox = AG_BoxNewVert(hBox, AG_BOX_VFILL|AG_BOX_FRAME);
                    {
                        AG_LabelNewStaticString(vBox, 0, _("Show label/object:"));
                                        
                        hDblChkbox(vBox, _("Stars"), labelFlagCel, Renderer::StarLabels, renderFlag, Renderer::ShowStars);                      
                        hDblChkbox(vBox, _("Galaxies"), labelFlagCel, Renderer::GalaxyLabels, renderFlag, Renderer::ShowGalaxies);
                        hDblChkbox(vBox, _("Constellations"), labelFlagCel, Renderer::ConstellationLabels, renderFlag, Renderer::ShowDiagrams);
                        hDblChkbox(vBox, _("Nebulae"), labelFlagCel, Renderer::NebulaLabels, renderFlag, Renderer::ShowNebulae);
                        hDblChkbox(vBox, _("Open Clusters"), labelFlagCel, Renderer::OpenClusterLabels, renderFlag, Renderer::ShowOpenClusters);
                        hDblChkbox(vBox, _("Globulars"), labelFlagCel, Renderer::GlobularLabels, renderFlag, Renderer::ShowGlobulars);
                        hDblChkbox(vBox, _("Planets"), labelFlagCel, Renderer::PlanetLabels, renderFlag, Renderer::ShowPlanets);
                        AG_SeparatorNew(vBox, AG_SEPARATOR_HORIZ);
                        AG_LabelNewStaticString(vBox, 0, _("Show object:"));
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
                        AG_Numerical *amb = AG_NumericalNewFltR(vBox, NULL, NULL, _("Ambient level"),
                                                                ambientLevel, ambMin, ambMax);
                        AG_NumericalSetIncrement(amb, 0.025);
                        amb = AG_NumericalNewFltR(vBox, NULL, NULL, _("Galaxy light gain"),
                                                  galaxyLight, 0, 100); 
                        AG_NumericalSetIncrement(amb, 5.0);
                    }
                    vBox = AG_BoxNewVert(hBox, AG_BOX_VFILL|AG_BOX_FRAME);
                    {

                        AG_LabelNewStaticString(vBox, 0, _("Show label:"));
                        AG_CheckboxNewFlag (vBox, 0, _("Dwarf Planets"), labelFlagCel, Renderer::DwarfPlanetLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Moons"), labelFlagCel, Renderer::MoonLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Minor Moons"), labelFlagCel, Renderer::MinorMoonLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Comets"), labelFlagCel, Renderer::CometLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Asteroids"), labelFlagCel, Renderer::AsteroidLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Spacecraft"), labelFlagCel, Renderer::SpacecraftLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("Locations"), labelFlagCel, Renderer::LocationLabels);
                        AG_CheckboxNewFlag (vBox, 0, _("I18nConstellation"), labelFlagCel, Renderer::I18nConstellationLabels);
                        AG_ButtonNewFn(vBox, 0, _("Show Body labels"),
                                       setBodyLabelsCelCfg, "%p", labelFlagCel);
                        AG_ButtonNewFn(vBox, 0, _("Disable labels"),
                                       resetLabelCelCfg, "%p", labelFlagCel);
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
                        AG_ButtonNewFn(hBox2, 0, _("Reset"),
                                       resetCelCfg, "%p", renderCfg);
                        AG_Radio *radTextureRes;
                        const char *radioItems[] = {
                            _("Low Texture resolution"),
                            _("Medium"),
                            _("Hi"),
                            NULL
                        };
                        radTextureRes = AG_RadioNew(hBox2, 0, radioItems);
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

            celCfgWin = AG_WindowNewNamed(0, "cel pref");
            AG_WindowSetCaption(celCfgWin, _("Celestia's render preference"));
            AG_Notebook *nb = AG_NotebookNew(celCfgWin, AG_NOTEBOOK_EXPAND);

            tab = celCfgWinNewTab(std::string("Current"), nb, &renderFlags);
            AG_Box * hBox = AG_BoxNewHoriz(tab, AG_BOX_HFILL|AG_BOX_FRAME);
            {                           
                AG_LabelNewStaticString(hBox, 0, _("Set current as preset:"));
                for (i = 0; i < 5; i++)
                {
                    AG_Button *btn = AG_ButtonNewFn(hBox, 0, NULL, setCurrentAsPreset, "%p", &renderCfgSets[i]);
                    AG_ButtonText(btn, "%i", i+1);
                }                               
            }
                                
            // 0..5 celestia explore preference, also bind keys
            for (i=0; i<5; i++)
            {
                char buff[32];
                sprintf(buff,"%d",i+1);
                tab = celCfgWinNewTab(std::string("preset ") +  buff, nb, 
                                      &renderCfgSets[i]);
                std::string str = std::string(_("To set this preference use Ctrl+"))+ buff;
                AG_SeparatorNew(tab, AG_SEPARATOR_HORIZ);
                AG_ButtonNewFn(tab, 0, str.c_str(), 
                               setPreset, "%i", i);
            }

            GK_BindGlobalKey(SDLK_1, KMOD_LCTRL, setCelRenderFlagsSets1);
            GK_BindGlobalKey(SDLK_2, KMOD_LCTRL, setCelRenderFlagsSets2);
            GK_BindGlobalKey(SDLK_3, KMOD_LCTRL, setCelRenderFlagsSets3);
            GK_BindGlobalKey(SDLK_4, KMOD_LCTRL, setCelRenderFlagsSets4);
            GK_BindGlobalKey(SDLK_5, KMOD_LCTRL, setCelRenderFlagsSets5);
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
            case SDLK_RIGHT:
            case SDLK_LEFT:
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
                Point3f pStar = star->getPosition();
                Vec3d v(pStar.x * 1e6 - (float)ucPos.x, 
                        pStar.y * 1e6 - (float)ucPos.y, 
                        pStar.z * 1e6 - (float)ucPos.z);
                float d = v.length() * 1e-6;
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
                AG_ButtonNewFn(hBox, 0, _("Refresh"),
                               actUpdateTable, NULL);
                const char *radioItems[] = {
                    _("Nearest"),
                    _("Brightest (App.)"),
                    _("Brightest (Abs.)"),
                    _("With Planets"),
                    NULL
                };
                                
                radStarEntry = AG_RadioNewFn(hBox, 0, radioItems, radioChanged, NULL);
                                
            }

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
                          RenderCfgDial::celPreference, NULL);
            if (appGame->getGameMode() == GameCore::VIEWER)
            {
                AG_MenuAction(mi, _("Reload Solar systems"), NULL,
                              actReloadAllSolSys, NULL);
            }
        }

        static void mnuRender(AG_Event *event)
        {
            AG_MenuItem *mi = (AG_MenuItem *)AG_SENDER();
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
                
        static void actSetTheme(AG_Event *event)
        {
            const char *filename = AG_STRING(1);
            if (AG_ColorsLoad(filename) == -1)
                AG_TextMsg(AG_MSG_ERROR, _("Failed to load color scheme: %s"),
                           AG_GetError());                      
            else
            {
                // TODO: save option: last theme
            }
        }

        // search in agar/themes/ dir *.acs files and add to menuitem
        static void addThemeTist(AG_MenuItem *mi)
        {
            const string thmDir = "agar/themes/";
            if (!IsDirectory(thmDir))
            {
                cout << "Failed locate theme dir:" << thmDir << "\n";
                return;
            }
            Directory* dir = OpenDirectory(thmDir);
            if (dir == NULL)                    
                return;
                        
            std::string filename;
            while (dir->nextFile(filename))
            {
                if (filename[0] == '.')
                    continue;
                int extPos = filename.rfind('.');
                if (extPos == (int)string::npos)
                    continue;
                std::string ext = string(filename, extPos, filename.length() - extPos + 1);

                if (compareIgnoringCase(".acs", ext) == 0)
                {
                    //alloc theme name, this mem not been free, agar need pointer to string
                    std::string *fullName = new std::string;
                    *fullName = thmDir + filename;
                    AG_MenuAction(mi, std::string(filename, 0, extPos).c_str(),
                                  NULL, actSetTheme, "%s", fullName->c_str());
                }                               
            }
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
            AG_MenuSetPollFn(itemopt, mnuRender, NULL);
            itemopt = AG_MenuNode(celMenu->root, _("Customize"), NULL);
            itemopt2 = AG_MenuNode(itemopt, _("UI Theme"), NULL);
            addThemeTist(itemopt2);
        }               
    }

    void closeCurrentFocusedWindow()
    {
        AG_Window *w = AG_WindowFindFocused();
        if (w)
            AG_WindowHide(w);
    }

    void toggleShowUI()
    {
        if (showUI)
            BG_GainFocus();
        else
            BG_LostFocus();
        showUI = !showUI;
    }

    void startTogVidRecord()
    {
        if (celAppCore->isCaptureActive())
        {
	    if (celAppCore->isRecording())
	    {
		celAppCore->recordPause();
		agMainMenuSticky=false;
	    }
	    else
	    {
		celAppCore->recordBegin();
		agMainMenuSticky=true;
	    }
	}
    }

    void stopVidRecord()
    {
        if (celAppCore->isCaptureActive())
            celAppCore->recordEnd();
	agMainMenuSticky=false;
    }
        
    void Init()
    {
        RenderCfgDial::setCelRenderFlagsSets1();

        RenderCfgDial::initDefaultFlags();      

        RenderCfgDial::createCelCfgWindow();

        Menu::initMenu();

	celAppCore->setContextMenuCallback(ContextMenu::menuContext);
        agTextAntialiasing = 1;
        GK_BindGlobalKey(SDLK_ESCAPE, KMOD_LCTRL, closeCurrentFocusedWindow);
        GK_BindGlobalKey(SDLK_o, KMOD_LCTRL, toggleShowUI);
	// rebind some celestia key
        GK_BindGlobalKey(SDLK_F11, KMOD_NONE, startTogVidRecord);
        GK_BindGlobalKey(SDLK_F12, KMOD_NONE, stopVidRecord);

    }
} //namespace UI
