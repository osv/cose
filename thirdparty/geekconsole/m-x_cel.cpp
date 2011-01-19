// Copyright (C) 2010-2011 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
//
// geekconsole is free software; you can redistribute it and/or modify
// it  under the terms  of the  GNU Lesser  General Public  License as
// published by the Free Software  Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// Alternatively, you  can redistribute it and/or modify  it under the
// terms of  the GNU General Public  License as published  by the Free
// Software Foundation; either  version 2 of the License,  or (at your
// option) any later version.
//
// geekconsole is distributed in the  hope that it will be useful, but
// WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
// MERCHANTABILITY or  FITNESS FOR A  PARTICULAR PURPOSE. See  the GNU
// Lesser General Public License or the GNU General Public License for
// more details.
//
// You should  have received a copy  of the GNU  Lesser General Public
// License. If not, see <http://www.gnu.org/licenses/>.

#include "geekconsole.h"
#include <celengine/starbrowser.h>
#include <celengine/marker.h>
#include <celengine/eigenport.h>
#include <celestia/url.h>

// some redefs from celestiacore.cpp
#ifdef _WIN32
#define TIMERATE_PRINTF_FORMAT "%.12g"
#else
#define TIMERATE_PRINTF_FORMAT "%'.12g"
#endif
static const double MaximumTimeRate = 1.0e15;
static const double MinimumTimeRate = 1.0e-15;

using namespace Eigen;

// referenceMarkNames maybe  different for celestia and OS  project, so it
// be public not static.
std::vector<std::string> referenceMarkNames;

static int gotoBody(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
        gc->setInteractive(listInteractive, "duration", "Duration");
    else if (state == 1)
    {
        Simulation *sim = gc->getCelCore()->getSimulation();
        float dur = atof(value.c_str());
        if (dur <= 0.01)
            dur = 5.0f;
        if (sim->getFrame()->getCoordinateSystem() == ObserverFrame::Universal)
            sim->follow();
        sim->gotoSelection(dur, Vector3f::UnitY(), ObserverFrame::ObserverLocal);
    }
    return state;
}

static int gotoBodyGC(GeekConsole *gc, int state, std::string value)
{
    Simulation *sim = gc->getCelCore()->getSimulation();
    if (sim->getFrame()->getCoordinateSystem() == ObserverFrame::Universal)
        sim->follow();
    sim->getObserver().gotoSelectionGC(
        gc->getCelCore()->getSimulation()->getSelection(),
        5.0, 0.0, 0.5,
        Vector3f::UnitY(), ObserverFrame::ObserverLocal);
    return state;
}

static int selectBody(GeekConsole *gc, int state, std::string value)
{
    switch (state)
    {
    case 1:
    {
        Selection sel = gc->getCelCore()->getSimulation()->findObjectFromPath(value, true);
        if (!sel.empty())
        {
            gc->getCelCore()->addToHistory();
            gc->getCelCore()->getSimulation()->setSelection(sel);
        }
    }
    break;
    case 0:
        gc->setInteractive(celBodyInteractive, "select", _("Target name: "), _("Enter target for select"));
        celBodyInteractive->setLastFromHistory();
        break;
    }
    return state;
}

static int selectStar(GeekConsole *gc, int state, std::string value)
{
    static StarBrowser starBrowser;

    const std::string nearest("Nearest");
    const std::string brightestApp("Brightest App");
    const std::string brightestAbs("Brightest Abs");
    const std::string wplanets("With Planets");
    std::vector<std::string> completion;

    switch (state)
    {
    case 0: // sort stars by..
    {
        completion.push_back(nearest);      completion.push_back(brightestApp);
        completion.push_back(brightestAbs); completion.push_back(wplanets);
        gc->setInteractive(listInteractive, "sort-star", _("Sort by:"), "");
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
        listInteractive->setLastFromHistory();
        break;
    }
    case 1:
    {
        // set sort type
        if (compareIgnoringCase(value, nearest) == 0)
            starBrowser.setPredicate(0);
        else if (compareIgnoringCase(value, brightestApp) == 0)
            starBrowser.setPredicate(1);
        else if (compareIgnoringCase(value, brightestAbs) == 0)
            starBrowser.setPredicate(2);
        else if (compareIgnoringCase(value, wplanets) == 0)
            starBrowser.setPredicate(3);
        // interact for num of stars
        gc->setInteractive(listInteractive, "num-of-star", _("Num of stars:"), "");
        listInteractive->setCompletionFromSemicolonStr("16;50;100;200;300;400;500");
        listInteractive->setLastFromHistory();
        break;
    }
    case 2: // select star
    {
        int numListStars = atoi(value.c_str());
        if (numListStars < 1)
            numListStars = 1;
        /* Load the catalogs and set data */
        Simulation* sim = gc->getCelCore()->getSimulation();
        starBrowser.setSimulation(sim);
        StarDatabase* stardb = sim->getUniverse()->getStarCatalog();
        starBrowser.refresh();
        vector<const Star*> *stars = starBrowser.listStars(numListStars);
        if (!stars)
        {
            break;
        }
        int currentLength = (*stars).size();
        if (currentLength == 0)
        {
            break;
        }
        sim->setSelection(Selection((Star *)(*stars)[0]));

        for (int i = 0; i < currentLength; i++)
        {
            const Star *star=(*stars)[i];
            completion.push_back(stardb->getStarName(*star));
        }

        gc->setInteractive(celBodyInteractive, "select-star", _("Select star"), "");
        celBodyInteractive->setCompletion(completion);
        celBodyInteractive->setLastFromHistory();
        break;
    }
    case 3: // finish
    {
        Selection sel = gc->getCelCore()->getSimulation()->findObjectFromPath(value, true);
        if (!sel.empty())
        {
            gc->getCelCore()->addToHistory();
            gc->getCelCore()->getSimulation()->setSelection(sel);
        }
        break;
    }
    default:
        break;
    }
    return state;
}

static int unmarkAll(GeekConsole *gc, int state, std::string value)
{
    Simulation* sim = gc->getCelCore()->getSimulation();
    sim->getUniverse()->unmarkAll();
    return state;
}

static int markObject(GeekConsole *gc, int state, std::string value)
{
    static bool labelled;
    static MarkerRepresentation::Symbol markSymbol;
    static std::string markType;
    static float markSize;
    static Color markColor;
    static std::string markColorStr;
    std::stringstream ss;
    switch (state)
    {
    case 0:
        // ask for labels on marks
        gc->setInteractive(listInteractive, "labelled-mark", _("Show label on mark"), "");
        listInteractive->setCompletionFromSemicolonStr("yes;no");
        listInteractive->setMatchCompletion(true);
        break;
    case 1:
        if (value == "yes")
            labelled = true;
        else
            labelled = false;

        // ask size
        gc->setInteractive(listInteractive, "mark-size", _("Mark size"),_("Choose mark size"));
        listInteractive->setCompletionFromSemicolonStr("5;7;9;14;20;32;45");
        break;
    case 2:
        markSize = atof(value.c_str());

        // ask for mark type
        ss << _("Mark size ") << markSize << _(", with type");
        gc->setInteractive(listInteractive, "mark-type",
                           ss.str(),
                           _("Choose mark type for marking object"));
        listInteractive->setCompletionFromSemicolonStr("diamond;triangle;square;filledsquare;plus;x;leftarrow;rightarrow;uparrow;downarrow;circle;disk");
        listInteractive->setMatchCompletion(true);
        break;
    case 3:
        if (value == "diamond")
            markSymbol = MarkerRepresentation::Diamond;
        else if (value == "triangle")
            markSymbol = MarkerRepresentation::Triangle;
        else if (value == "square")
            markSymbol = MarkerRepresentation::Square;
        else if (value == "filledsquare")
            markSymbol = MarkerRepresentation::FilledSquare;
        else if (value == "plus")
            markSymbol = MarkerRepresentation::Plus;
        else if (value == "x")
            markSymbol = MarkerRepresentation::X;
        else if (value == "leftarrow")
            markSymbol = MarkerRepresentation::LeftArrow;
        else if (value == "rightarrow")
            markSymbol = MarkerRepresentation::RightArrow;
        else if (value == "uparrow")
            markSymbol = MarkerRepresentation::UpArrow;
        else if (value == "downarrow")
            markSymbol = MarkerRepresentation::DownArrow;
        else if (value == "circle")
            markSymbol = MarkerRepresentation::Circle;
        else if (value == "disk")
            markSymbol = MarkerRepresentation::Disk;
        else
            markSymbol = MarkerRepresentation::Diamond;
        markType = value;

        // ask for color
        ss << _("Mark size ") << markSize << ", \"" << markType << _("\" with color");
        gc->setInteractive(colorChooserInteractive, "mark-color", ss.str(),
                           _("Color of marker"));
        break;
    case 4:
        markColor = getColorFromText(value);
        markColorStr = value;
        // ask for object
        ss << _("Marker: \"") << markType << "\", " << markSize << ", " << markColorStr;
        gc->setInteractive(celBodyInteractive, "select", _("Target to mark:"),
                           ss.str());
        break;
    case 5:
    {
        Selection sel = gc->getCelCore()->getSimulation()->findObjectFromPath(value, true);
        std::string askStr(_("Other target to mark:"));
        if (!sel.empty())
        {
            gc->getCelCore()->getSimulation()->setSelection(sel);
            Simulation* sim = gc->getCelCore()->getSimulation();
            MarkerRepresentation markerRep(markSymbol);
            markerRep.setSize(markSize);
            markerRep.setColor(markColor);
            markerRep.setLabel(sel.getName());
            sim->getUniverse()->unmarkObject(sel, 4);

            sim->getUniverse()->markObject(sel, markerRep, 4);
            askStr = _("Marked \"") + value + "\". " + askStr;
        }
        // ask for next object
        ss << _("Next object to mark (\"") << markType << "\", " << markSize << ", "
           << markColorStr << _("), C-g or ESC to cancel");
        gc->setInteractive(celBodyInteractive, "select", askStr,
                           ss.str());
        return 4;
        break;
    }
    }
    return state;
}

/* prompt script name and run */
static int openScript(GeekConsole *gc, int state, std::string value)
{
    switch (state)
    {
    case 0:
        gc->setInteractive(fileInteractive, "run-script", _("Select script to run"));
        fileInteractive->setDir("scripts/");
        fileInteractive->setLastFromHistory();
        fileInteractive->setMatchCompletion(true);
        fileInteractive->setFileExtenstion(".celx;.cel");
        break;
    case 1:
        gc->getCelCore()->runScript("./" + value);
        break;
    }
    return state;
}

/* render flag config */
struct flags_s {
    const char *name;
    uint32 mask;
};

flags_s renderflags[] = {
    {"Stars",       0x0001},
    {"Planets",     0x0002},
    {"Galaxies",    0x0004},
    {"Diagrams",    0x0008},
    {"Cloud Maps",   0x0010},
    {"Orbits",      0x0020},
    {"Celestial Sphere", 0x0040},
    {"Night Maps",   0x0080},
    {"Atmospheres", 0x0100},
    {"Smooth Lines", 0x0200},
    {"Eclipse Shadows", 0x0400},
    {"Stars As Points", 0x0800},
    {"Ring Shadows", 0x1000},
    {"Boundaries",  0x2000},
    {"AutoMag",     0x4000},
    {"Comet Tails",  0x8000},
    {"Markers",     0x10000},
    {"Partial Trajectories", 0x20000},
    {"Nebula",     0x40000},
    {"Open Clusters", 0x80000},
    {"Globulars",   0x100000},
    {"Cloud Shadows", 0x200000},
    {"Galactic Grid", 0x400000},
    {"Ecliptic Grid", 0x800000},
    {"Horizon Grid", 0x1000000},
    {"Ecliptic",    0x2000000},
    {NULL}
};

flags_s labelflags[] = {
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
    {"Open Cluster",        0x400},
    {"Constellation in latin",  0x800},
    {"DwarfPlanet",        0x1000},
    {"MinorMoon",          0x2000},
    {"Globular",           0x4000},
    {NULL}
};

flags_s orbitflag[] = {
    {"Planet",          0x01},
    {"Moon",            0x02},
    {"Asteroid",        0x04},
    {"Comet",           0x08},
    {"Spacecraft",      0x10},
    {"Invisible",       0x20},
//    {"Barycenter",      0x40}, // Not used (invisible is used instead)
//    {"SmallBody",       0x80}, // Not used
    {"Dwarf Planet",     0x100},
    {"Stellar",         0x200}, // only used for orbit mask
    {"Surface Feature",  0x400},
    {"Component",       0x800},
    {"Minor Moon",       0x1000},
    {"Diffuse",         0x2000},
    {"Unknown",         0x10000},
    {NULL}
};

static flags_s *flagsToSet = renderflags;

static int setFlag(GeekConsole *gc, int state, std::string value)
{
    static uint32 var; // flag here
    enum {
        SET = 1,
        UNSET = 2,
        TOGGLE =3
    };
    static int setUnset; // true set flag, flase - unset
    std::stringstream ss;

    switch(state)
    {
    case 0:
        gc->setInteractive(listInteractive, "set-unset", _("Show/Hide/Toggle objects"));
        listInteractive->setCompletionFromSemicolonStr("show;hide;toggle");
        listInteractive->setMatchCompletion(true);
        break;
    case 1:
        if (value == "show")
        {
            ss << _("Show");
            setUnset = SET;
        } else if (value == "hide")
        {
            ss << _("Hide");
            setUnset = UNSET;
        } else
        {
            ss << _("Toggle");
            setUnset = TOGGLE;
        }

        gc->setInteractive(listInteractive, "flag-type", ss.str(),
                           setUnset == SET ?
                           _("Which type of flag to set") :
                           setUnset == UNSET ? _("Which type of flag to unset") :
                           _("Which type of flag to Toggle"));
        listInteractive->setCompletionFromSemicolonStr("objects;labels;orbits");
        listInteractive->setMatchCompletion(true);
        break;
    case 2:
    {
        Renderer *r = gc->getCelCore()->getRenderer();
        if (value == "labels")
        {
            flagsToSet = labelflags;
            var = r->getLabelMode();
        } else if (value == "orbits")
        {
            flagsToSet = orbitflag;
            var = r->getOrbitMask();
        } else {
            flagsToSet = renderflags;
            var = r->getRenderFlags();
        }

        std::vector<std::string> completion;
        int j = 0;
        while(flagsToSet[j].name != NULL)
        {
            // add only if flag not set currently if need to set flag and so on
            if (setUnset == SET)
            {
                if (!(var & flagsToSet[j].mask))
                    completion.push_back(flagsToSet[j].name);
            } else if (setUnset == UNSET)
            {
                if ((var & flagsToSet[j].mask))
                    completion.push_back(flagsToSet[j].name);
            } else
                completion.push_back(flagsToSet[j].name);
            j++;
        }

        ss << (setUnset == SET ? _("Show") :
               setUnset == UNSET ? _("Hide") : _("Toggle")) << " " << value;
        gc->setInteractive(flagInteractive, "flag-" + value, ss.str());
        flagInteractive->setCompletion(completion);
        flagInteractive->setMatchCompletion(true);
        break;
    }
    case 3:
    {
        Renderer *r = gc->getCelCore()->getRenderer();
        CelestiaCore *c = gc->getCelCore();
        // split flags
        std::vector<std::string> strArray =
            splitString(value, flagInteractive->getDefaultDelim());
        std::vector<string>::iterator it;

        // set flag
        for (it = strArray.begin();
             it != strArray.end(); it++)
        {
            int j = 0;
            while(flagsToSet[j].name != NULL)
            {
                string s = *it;
                if(s == flagsToSet[j].name )
                {
                    if (setUnset == SET)
                        var |= flagsToSet[j].mask;
                    else if (setUnset == UNSET)
                        var &= ~ flagsToSet[j].mask;
                    else // toggle
                        var ^= flagsToSet[j].mask;
                    break;
                }
                j++;
            }
        }
        if (flagsToSet == renderflags)
        {
            r->setRenderFlags(var);
            c->notifyWatchers(CelestiaCore::RenderFlagsChanged);
        }
        else if (flagsToSet == labelflags)
        {
            r->setLabelMode(var);
            c->notifyWatchers(CelestiaCore::LabelFlagsChanged);
        }
        else if (flagsToSet == orbitflag)
        {
            r->setOrbitMask(var);
            c->notifyWatchers(CelestiaCore::RenderFlagsChanged);
        }
        break;
    } // case 3:
    }
    return state;
}

static int toggleReferenceMark(GeekConsole *gc, int state, std::string value)
{
    std::vector<std::string>::iterator it1;
    std::vector<std::string>::iterator it2;
    
    switch(state)
    {
    case 0:
        gc->setInteractive(flagInteractive, "refmark");
        flagInteractive->setCompletion(referenceMarkNames);
        flagInteractive->setMatchCompletion(true);
        break;
    case 1:
    {
        Selection sel = gc->getCelCore()->
            getSimulation()->getSelection();
        if (sel.empty())
            break;
        std::vector<std::string> values =
            splitString(value, flagInteractive->getDefaultDelim());

        // toggle ref marks
        for (it1 = referenceMarkNames.begin();
             it1 != referenceMarkNames.end(); it1++)
        {
            for (it2 = values.begin();
                 it2 != values.end(); it2++)
                if (*it1 == *it2)
                {
                    gc->getCelCore()->toggleReferenceMark(*it2, sel);
                    break;
                }
        }
        break;
    }
    default: break;
    }
    return state;
}

static int disableReferenceMark(GeekConsole *gc, int state, std::string value)
{
    Selection sel = gc->getCelCore()->
        getSimulation()->getSelection();

    if (sel.empty())
        return state;

    for (std::vector<std::string>::iterator it1 = referenceMarkNames.begin();
         it1 != referenceMarkNames.end(); it1++)
    {
        if (gc->getCelCore()->referenceMarkEnabled(*it1, sel))
            gc->getCelCore()->toggleReferenceMark(*it1, sel);
    }

    return state;
}

/* pass to celestia core some chars */
static int celCharEntered(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
        gc->setInteractive(listInteractive, "");
    else if (state == 1)
        gc->getCelCore()->charEntered(value.c_str());
    return state;
}

/* pass to celestia core some chars with Ctrl mod key*/
static int celCtrlCharEntered(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
        gc->setInteractive(listInteractive, "");
    else if (state == 1)
    {
        if (!value.empty())
        {
            value = toCtrl(value[0]);
            gc->getCelCore()->charEntered(value.c_str(),
                                          CelestiaCore::ControlKey);
        }
    }
    return state;
}

static void addToHistory()
{
    getGeekConsole()->getCelCore()->addToHistory();
}

static void historyBack()
{
    getGeekConsole()->getCelCore()->back();
}

static void historyForward()
{
    getGeekConsole()->getCelCore()->forward();
}

static int celTimeScaleIncr(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
        gc->setInteractive(listInteractive, "time-scale", _("Increase time scale"));
    else if (state == 1)
    {
        Simulation *sim = gc->getCelCore()->getSimulation();
        if (abs(sim->getTimeScale()) > MinimumTimeRate &&
            abs(sim->getTimeScale()) < MaximumTimeRate)
        {
            float scale = atof(value.c_str());
            if (scale != 0 &&
                abs(scale) > MinimumTimeRate / 2 &&
                abs(scale) < MaximumTimeRate / 2)
            {
                sim->setTimeScale(sim->getTimeScale() * scale);
                char buf[128];
                setlocale(LC_NUMERIC, "");
                sprintf(buf, "%s: " TIMERATE_PRINTF_FORMAT,  _("Time rate"), sim->getTimeScale());
                setlocale(LC_NUMERIC, "C");
                gc->getCelCore()->flash(buf);
            }
        }
    }
    return state;
}

static int selectPlanet(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
        gc->setInteractive(listInteractive, "select-planet",
                           _("Select planet by number"));
    else if (state == 1)
    {
        Simulation *sim = gc->getCelCore()->getSimulation();
        if (abs(sim->getTimeScale()) > MinimumTimeRate &&
            abs(sim->getTimeScale()) < MaximumTimeRate)
        {
            uint planet = atoi(value.c_str());
            sim->selectPlanet(planet);
        }
    }
    return state;
}

/* init */
void initGCStdInteractivsFunctions(GeekConsole *gc)
{
    referenceMarkNames.push_back("body axes");
    referenceMarkNames.push_back("frame axes");
    referenceMarkNames.push_back("sun direction");
    referenceMarkNames.push_back("velocity vector");
    referenceMarkNames.push_back("spin vector");
    referenceMarkNames.push_back("frame center direction");
    referenceMarkNames.push_back("planetographic grid");
    referenceMarkNames.push_back("terminator");

    gc->registerAndBind("", "C-m",
                        GCFunc(selectBody), "select object");
    gc->registerFunction(GCFunc(gotoBody, _("Goto an object")),
                         ".goto object");
    gc->registerFunction(GCFunc(".goto object", "#5.0", _("Goto an object with duration 5 sec")),
                         "goto object");
    gc->registerAndBind("", "C-c g",
                        GCFunc(gotoBodyGC, _("Goto object with greate circle")), "goto object gc");
    gc->registerFunction(GCFunc(selectStar), "select star");
    gc->registerFunction(GCFunc(unmarkAll), "unmark all");
    gc->registerFunction(GCFunc(markObject), "mark object");
    gc->registerAndBind("", "C-x 4 f",
                        GCFunc(openScript, _("Run lua or celestia script")), "open script");
    gc->registerFunction(GCFunc(setFlag, _("Show or hide objects, labels, orbits")), "celestia options");
    gc->registerFunction(GCFunc(toggleReferenceMark, _("Toggle reference marks for selected object")),
                         "toggle ref marks");
    gc->registerFunction(GCFunc(disableReferenceMark, _("Disable all reference marks for selected object")),
                         "disable ref marks");
    // aliases
    gc->registerFunction(GCFunc("celestia options", "#show", _("Select objects, labels or orbits to show")),
                         "show objects");
    gc->registerFunction(GCFunc("celestia options", "#hide", _("Select objects, labels or orbits to hide")),
                         "hide objects");
    gc->registerFunction(GCFunc("celestia options", "#toggle", _("Select objects, labels or orbits to toggle")),
                         "toggle objects");

    gc->registerFunction(GCFunc(celCharEntered,  _("Send chars to celestia core.\n"
                                    "You usually don't need use it")),
                         ".!charEnter");
    gc->registerFunction(GCFunc(celCtrlCharEntered,  _("Send chars to celestia core with Ctrl.\n"
                                    "You usually don't need use it")),
                         ".!charEnterCtrl");
    gc->registerFunction(GCFunc(celTimeScaleIncr,  _("Increase time scale")),
                         ".time scale increment");
    gc->registerFunction(GCFunc(addToHistory,  _("Push to history current url")),
                         ".history add");
    gc->registerFunction(GCFunc(historyBack,  _("Back history")),
                         ".history back");
    gc->registerFunction(GCFunc(historyForward,  _("Forward history")),
                         ".history forward");
    gc->registerFunction(GCFunc(selectPlanet, _("Select planet by number")),
                         ".select planet");

    // celestia core have many usefull binds, so map to him

    struct std_cel_key_t {
        const char key;
        const char *funname;
        const char *descript;
    };
    std_cel_key_t std_cel_key[] = {
        {'a', ".increase velocity", _("Increase velocity")},
        {'c', ".center on selected", _("Center on selected object")},
        {'d', ".run demo.cel", _("Run demo script (demo.cel)")},
        {'f', ".follow selected object", _("Follow selected object")},
        {'h', ".select our sun", _("Select our sun (Home)")},
        {'j', ".toggle forw/rev time", _("Toggle Forward/Reverse time")},
        {'q', ".reverse direction", _("Reverse direction")},
        {'r', ".lower texture res", _("Lower texture resolution")},
        {'s', ".stop motion", _("Stop motion")},
        {'t', ".track selected", _("Track selected object (keep selected object centered in view)")},
        {'o', ".toggle orbits", _("Toggle planet orbits")},
        {'v', ".toggle verbosity", _("Toggle verbosity of information text")},
        {'x', ".movement toward center", _("Set movement direction toward center of screen")},
        {'y', ".sync orbit", _("Sync Orbit the selected object, at a rate synced to its rotation")},
        {'z', ".decrease velocity", _("Decrease velocity")},
        {'C', ".center on selected CO", _("Center/orbit--center the selected object without changing the position\n of the reference object.")},
        {'R', ".raise texture res", _("Raise texture resolution")},
        {'`', ".toggle fps", _("Show frames rendered per second")},
        {'~', ".toggle load info", _("Display file loading info")},
        {'!', ".set time to current", _("Set time to current date and time")},
        {'@', ".edit mode", _("Edit Mode")},
        {'%', ".toggle star color tbl", _("Toggle star color tables")},
        {'*', ".look back", _("Look back")},
        {'(', ".decrease galaxy brightness", _("Decrease galaxy brightness independent of star brightness")},
        {')', ".increase galaxy brightness", _("Increase galaxy brightness independent of star brightness")},
        {'-', ".subtract light-travel delay", _("(hyphen) Subtract light-travel delay from current simulation time")},
        {'+', ".toggle limit of knowledge", _("Switch between artistic and limit of knowledge planet textures")},
        {'[', ".decrease limiting magnitude", _("If autoMag ON :\n Decrease limiting magnitude at 45 deg field of view\n"
                                                "If autoMag OFF:\n Decrease limiting magnitude (fewer stars visible)")},
        {']', ".Increase limiting magnitude", _("If autoMag ON :\n Increase limiting magnitude at 45 deg field of view\n"
                                                "If autoMag OFF:\n Increase limiting magnitude (more stars visible)")},
        {'{', ".decrease ambient illumination", _("Decrease ambient illumination")},
        {'}', ".increase ambient illumination", _("Increase ambient illumination")},
        {';', ".earth equat coord sphere", _("Show an earth-based equatorial coordinate sphere")},
        {':', ".lock 2 object as one", _("Lock two objects together as one.\n"
                                         "Select #1, \"follow selected\", select #2, \"lock 2 object\".\n"
                                         "(If use default keys: select #1, \"f\", select #2, \":\")")},
        {'"', ".chase selected", _("Chase selected object\n(orientation is based on selection's velocity)")},
        {',', ".FOV narrow", _("Narrow field of view (FOV)")},
        {'.', ".FOV widen", _("Widen field of view (FOV)")},
        {'<', ".brightness increase", _("Decrease brightness")},
        {'>', ".brightness decrease", _("Increase brightness")},
        {'?', ".toggle light-travel delay", _("Display light-travel delay between observer and selected object")},
        {'\\', ".real time", _("Real time (cancels x factors and backward time)")},
        {'|', ".toggle bloom", _("Toggle bloom")},
        {'\033', ".cancel script and travel", _("Cancel script and travel")},
        {' ', ".pause/restart time and script", _("Pause/Restart time and scripts")},
        {'\b', ".select parent", _("If the selection is a star, clears the selection\n"
                                   "If the selection is a body, selects the parent body or star\n"
                                   "If the selection is a location, selects the parent body")},
        {'\t', ".cycle views", _("Cycle through currently active views")},
        {'\n', ".select object (cel)", _("Select a star or planet by typing its name")},
        {NULL}
    };

    // ctrl+
    std_cel_key_t std_ctrl_cel_key[] = {
        {'d', ".single view", _("Single View")},
        {'f', ".toggle alt-azimuth mode", _("Toggle Alt-Azimuth mode (used with Ctrl-g Go to surface)\n"
                                            "Changes Left and Right Arrow keys to Yaw Left / Right")},
        {'g', ".goto surface", _("Go to surface of selected object")},
        {'k', ".toggle markers", _("Toggle display of object markers")},
        {'p', ".mark selected", _("Mark selected object")},
        {'r', ".split view vert", ("Split view vertically")},
        {'s', ".cycle star style", ("Cycle the star style between:\n"
                                    "- fuzzy discs\n- points\n- scaled discs")},
        {'u', ".split view horiz", ("Split view horizontally")},
        {'v', ".cycle OpenGL render parent", _("Cycle between supported OpenGL render paths")},
        {'w', ".toggle wireframe", _("Toggle wireframe mode")},
        {'x', ".toggle antialias", _("Toggle antialias lines")},
        {'y', ".toggle automag", _("Toggle AutoMag = auto adaptation of star visibility to field of view")},
        {NULL}
    };

    int i = 0;
    char param[2];
    param[1]='\0';
    while (std_cel_key[i].funname != NULL )
    {
        param[0]=std_cel_key[i].key;
        gc->registerFunction(GCFunc(".!charEnter",
                                    param,
                                    std_cel_key[i].descript),
                             std_cel_key[i].funname);
        i++;
    }

    i = 0;
    while (std_ctrl_cel_key[i].funname != NULL )
    {
        param[0]=std_ctrl_cel_key[i].key;
        gc->registerFunction(GCFunc(".!charEnterCtrl",
                                    param,
                                    std_ctrl_cel_key[i].descript),
                             std_ctrl_cel_key[i].funname);
        i++;
    }

}

void initGCStdCelBinds(GeekConsole *gc, const char *bindspace)
{
    struct std_cel_key_t {
        const char *key;
        const char *fun;
    } std_cel_key[] = {
        {"a",       ".increase velocity"}, // Increase velocity
        {"b #toggle objects#labels#Star", ""}, // Toggle star labels
        {"S-b #toggle objects#labels#Star", ""}, // Toggle star labels
        {"c",       ".center on selected"}, // Center on selected object
        {"d",       ".run demo.cel"}, // Run demo script (demo.cel)
        {"S-d",     ".run demo.cel"}, // Run demo script (demo.cel)
        {"e #toggle objects#labels#Galaxy", ""}, // Toggle galaxy labels
        {"S-e #toggle objects#labels#Galaxy", ""}, // Toggle galaxy labels
        {"g", ""}, // Goto selected object
        {"f",       ".follow selected object"}, // Follow selected object
        {"S-f",     ".follow selected object"}, // Follow selected object
        {"g",       "goto object"}, // Go to selected object
        {"h",       ".select our sun"}, // Select our sun (Home)
        {"S-h",     ".select our sun"}, // Select our sun (Home)
        {"i #toggle objects#objects#Cloud Maps", ""}, // Toggle cloud textures
        {"S-i #toggle objects#objects#Cloud Maps", ""}, // Toggle cloud textures
        {"j",       ".toggle forw/rev time"}, // Toggle Forward/Reverse time
        {"S-j",     ".toggle forw/rev time"}, // Toggle Forward/Reverse time
        {"SPACE",   ".pause/restart time and script"}, // Pause/Resume the flow of time and scripts (toggle)
        {"BACKSPACE", ".select parent"}, // Selects the parent
        {"k #.time scale increment#0.1", ""}, // Time 10x slower
        {"l #.time scale increment#10", ""}, // Time 10x faster
        {"m #toggle objects#labels#Moon", ""}, // Toggle moon labels
        {"S-m #toggle objects#labels#MinorMoon", ""}, // Toggle moon labels
        {"n #toggle objects#labels#Spacecraft", ""}, // Toggle spacecraft labels
        {"S-n #toggle objects#labels#Spacecraft", ""}, // Toggle spacecraft labels
        {"o",       ".toggle orbits"}, // Toggle planet orbits
        {"S-o",     ".toggle orbits"}, // Toggle planet orbits
        {"p #toggle objects#labels#Planet", ""}, // Toggle planet labels
        {"S-p #toggle objects#labels#Planet", ""}, // Toggle planet labels
        {"q",       ".reverse direction"}, // Reverse direction
        {"S-q",     ".reverse direction"}, // Reverse direction
        {"r",       ".lower texture res"}, // Lower texture resolution
        {"s",       ".stop motion"}, // Stop motion
        {"S-s",     ".stop motion"}, // Stop motion
        {"t",       ".track selected"}, // Track selected object (keep selected object centered in view)
        {"S-t",     ".track selected"}, // Track selected object (keep selected object centered in view)
        {"u #toggle objects#objects#Galaxies", ""}, // Toggle galaxy rendering
        {"S-u #toggle objects#objects#Galaxies", ""}, // Toggle galaxy rendering
        {"v",       ".toggle verbosity"}, // Toggle verbosity of information text
        {"S-v",     ".toggle verbosity"}, // Toggle verbosity of information text
        {"w #toggle objects#labels#Asteroid", ""}, // Toggle asteroid labels
        {"x",       ".movement toward center"}, // Set movement direction toward center of screen
        {"S-x",     ".movement toward center"}, // Set movement direction toward center of screen
        {"y",       ".sync orbit"}, // Sync Orbit the selected object, at a rate synced to its rotation
        {"S-y",     ".sync orbit"}, // Sync Orbit the selected object, at a rate synced to its rotation
        {"z",       ".decrease velocity"}, // Decrease velocity
        {"S-c",     ".center on selected CO"}, // Center/orbit--center the selected object without changing the position of the reference object.
        {"S-k #.time scale increment#0.5", ""}, // Time 2x slower
        {"S-l #.time scale increment#2", ""}, // Time 2x faster
        {"S-r",     ".raise texture res"}, // Raise texture resolution
        {"S-w #toggle objects#labels#Comet", ""}, // Toggle comet labels
        {"`",       ".toggle fps"}, // Show frames rendered per second
        {"~",       ".toggle load info"}, // Display file loading info
        {"!",       ".set time to current"}, // Set time to current date and time
        {"@",       ".edit mode"}, // Edit Mode
        {"%",       ".toggle star color tbl"}, // Toggle star color tables
        {"^ #toggle objects#objects#Nebula", ""}, // Toggle nebula rendering
        {"& #toggle objects#labels#Location", ""}, // Toggle location labels
        {"*",       ".look back"}, // Look back
        {"(",       ".decrease galaxy brightness"}, // Decrease galaxy brightness independent of star brightness
        {")",       ".increase galaxy brightness"}, // Increase galaxy brightness independent of star brightness
        {"-",       ".subtract light-travel delay"}, // (hyphen) Subtract light-travel delay from current simulation time
        {"= #toggle objects#labels#Constellation", ""}, // Toggle constellation labels
        {"+",       ".toggle limit of knowledge"}, // Switch between artistic and limit of knowledge planet textures
        {"[",       ".decrease limiting magnitude"}, // Decrease limiting magnitude
        {"]",       ".Increase limiting magnitude"}, // Increase Decrease limiting
        {"{",       ".decrease ambient illumination"}, // Decrease ambient illumination
        {"}",       ".increase ambient illumination"}, // Increase ambient illumination
        {";",       ".earth equat coord sphere"}, // Show an earth-based equatorial coordinate sphere
        {":",       ".lock 2 object as one"}, // Lock two objects together as one (select #1, "f", select #2, ":")
        {"\"",      ".chase selected"}, // Chase selected object (orientation is based on selection's velocity)
        {",",       ".FOV narrow"}, // Narrow field of view (FOV--also Shift+Left Drag)
        {".",       ".FOV widen"}, // Widen field of view (FOV--also Shift+Left Drag)
        {"/ #toggle objects#objects#Diagrams", ""}, // Toggle constellation diagrams
        {"<",       ".brightness increase"}, //
        {">",       ".brightness decrease"}, //
        {"?",       ".toggle light-travel delay"}, // Display light-travel delay between observer and selected object
        {"\\",      ".real time"}, // Real time (cancels x factors and backward time)
        {"|",       ".toggle bloom"}, //
        {"ESC",     ".cancel script and travel"},
        {"TAB",     ".cycle views"},
        {"S-TAB",   ".single view"},
        {"RET",     ".select object (cel)"},
        {"C-a #toggle objects#objects#Atmospheres", ""},//  (BROKEN--increases velocity--see Ctrl+A) Toggle atmospheres
        {"S-C-a #toggle objects#objects#Atmospheres", ""},
        {"C-b #toggle objects#objects#Boundaries", ""}, // Toggle constellation boundaries
        {"S-C-b #toggle objects#objects#Boundaries", ""},
        {"C-d",     ".single view"}, // Single View (Multi-View--also Shift+Tab)
        {"S-C-d",   ".single view"},
        {"C-e #toggle objects#objects#Eclipse Shadows", ""}, //  Toggle eclipse shadow rendering
        {"S-C-e #toggle objects#objects#Eclipse Shadows", ""},
        {"C-f",     ".toggle alt-azimuth mode"}, //  Non-KDE: Toggle Alt-Azimuth mode (used with Ctrl+g Go to surface)
        {"S-C-f",   ".toggle alt-azimuth mode"},
        {"C-g",     ".goto surface"}, //  Non-KDE: Go to surface of selected object
        {"S-C-g",   ".goto surface"},
        {"C-h",     ".select our sun"}, //  DUP (h and H) Select our sun (Home)
        {"S-C-h",   ".select our sun"},
        {"C-j",     ".select object (cel)"}, //  DUP Select a star or planet by typing it's name
        {"S-C-j",   ".select object (cel)"},
        {"C-k",     ".toggle markers"}, //  Toggle display of object markers
        {"S-C-k",   ".toggle markers"},
        {"C-l #toggle objects#objects#Night Maps", ""}, //  Toggle night side planet maps (light pollution)
        {"S-C-l #toggle objects#objects#Night Maps", ""},
// C-m now "select object"
//        {"C-m",     ".select object (cel)"}, //  DUP Select a star or planet by typing it's name
        {"S-C-m",   ".select object (cel)"},
        {"C-p",     ".mark selected"}, //  Non-GLUT: Mark selected object (Marker display must be active)
        {"S-C-p",   ".mark selected"},
        {"C-r",     ".split view vert"}, //  Split view vertically
        {"S-C-u",   ".split view vert"},
        {"C-s",     ".cycle star style"}, //  Cycle the star style between fuzzy discs, points, and scaled discs
        {"C-t #toggle objects#objects#Comet Tails", ""}, //  Toggle rendering of comet tails
        {"C-u",     ".split view horiz"}, //  Split view horizontally
        {"S-C-r",   ".split view horiz"},
        {"C-v",     ".cycle OpenGL render parent"}, //  Cycle between supported OpenGL render paths
        {"S-C-v",   ".cycle OpenGL render parent"},
        {"C-w",     ".toggle wireframe"}, //  Toggle wireframe mode
        {"S-C-w",   ".toggle wireframe"},
// C-x reserved as prefix key!
//        {"C-x",     ".toggle antialias"}, //  Toggle antialias lines
        {"S-C-x",   ".toggle antialias"},
        {"C-y",     ".toggle automag"}, //  Toggle AutoMag = auto adaptation of star visibility to field of view
        {NULL}
    };

    int i = 0;
    while (std_cel_key[i].key != NULL )
    {
        cout << std_cel_key[i].fun << "\n";
        gc->bind(bindspace, std_cel_key[i].key, std_cel_key[i].fun);
        i++;
    }

    // other
    gc->bind(bindspace, "0 #0", ".select planet");
    gc->bind(bindspace, "1 #1", ".select planet");
    gc->bind(bindspace, "2 #2", ".select planet");
    gc->bind(bindspace, "3 #3", ".select planet");
    gc->bind(bindspace, "4 #4", ".select planet");
    gc->bind(bindspace, "5 #5", ".select planet");
    gc->bind(bindspace, "6 #6", ".select planet");
    gc->bind(bindspace, "7 #7", ".select planet");
    gc->bind(bindspace, "8 #8", ".select planet");
    gc->bind(bindspace, "9 #9", ".select planet");

}
