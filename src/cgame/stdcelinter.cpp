#include "geekconsole.h"
#include <celengine/starbrowser.h>
#include <celengine/marker.h>
#include <celengine/eigenport.h>
using namespace Eigen;

static int gotoBody(GeekConsole *gc, int state, std::string value)
{
    gc->getCelCore()->getSimulation()->
        gotoSelection(5.0, Vector3f::UnitY(), ObserverFrame::ObserverLocal);
    gc->finish();
    return state;
}

static int gotoBodyGC(GeekConsole *gc, int state, std::string value)
{
    gc->getCelCore()->getSimulation()->getObserver().gotoSelectionGC(
        gc->getCelCore()->getSimulation()->getSelection(),
        5.0, 0.0, 0.5,
        Vector3f::UnitY(), ObserverFrame::ObserverLocal);
    gc->finish();
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
            gc->getCelCore()->getSimulation()->setSelection(sel);
        }
    }
    gc->finish();
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
            gc->finish();
            break;
        }
        int currentLength = (*stars).size();
        if (currentLength == 0)
        {
            gc->finish();
            break;
        }
        sim->setSelection(Selection((Star *)(*stars)[0]));

        for (int i = 0; i < currentLength; i++)
        {
            const Star *star=(*stars)[i];
            completion.push_back(stardb->getStarName(*star));
        }

        gc->setInteractive(celBodyInteractive, "select-star", _("Select star"), "");
        listInteractive->setCompletion(completion);
        listInteractive->setLastFromHistory();
        break;
    }
    case 3: // finish
    {
        Selection sel = gc->getCelCore()->getSimulation()->findObjectFromPath(value, true);
        if (!sel.empty())
        {
            gc->getCelCore()->getSimulation()->setSelection(sel);
        }
        gc->finish();
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
    gc->finish();
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
        gc->finish();
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
    {"Nebulae",     0x40000},
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
    static bool setUnset; // true set flag, flase - unset
    std::stringstream ss;

    switch(state)
    {
    case 0:
        gc->setInteractive(listInteractive, "set-unset", _("Show/Hide objects"));
        listInteractive->setCompletionFromSemicolonStr("show;hide");
        listInteractive->setMatchCompletion(true);
        break;
    case 1:
        if (value == "show")
            setUnset = true;
        else
            setUnset = false;
        ss << (setUnset ? _("Show") : _("Hide"));
        gc->setInteractive(listInteractive, "flag-type", ss.str(),
                           setUnset ?
                           _("Which type of flag to set") : _("Which type of flag to unset"));
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
            if (setUnset)
            {
                if (!(var & flagsToSet[j].mask))
                    completion.push_back(flagsToSet[j].name);
            } else
                if ((var & flagsToSet[j].mask))
                    completion.push_back(flagsToSet[j].name);
            j++;
        }

        ss << (setUnset ? _("Show") : _("Hide")) << " " << value;
        gc->setInteractive(flagInteractive, "flag-" + value, ss.str());
        flagInteractive->setCompletion(completion);
        flagInteractive->setColumns(5);
        flagInteractive->setMatchCompletion(true);
        break;
    }
    case 3:
    {
        Renderer *r = gc->getCelCore()->getRenderer();
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
                    if (setUnset)
                        var |= flagsToSet[j].mask;
                    else
                        var &= ~ flagsToSet[j].mask;
                    break;
                }
                j++;
            }
        }
        if (flagsToSet == renderflags)
            r->setRenderFlags(var);
        else if (flagsToSet == labelflags)
            r->setLabelMode(var);
        else if (flagsToSet == orbitflag)
            r->setOrbitMask(var);
        }
        gc->finish();
        break;
    }
    return state;
}

/* init */

void initGCStdInteractivsFunctions(GeekConsole *gc)
{
    gc->registerAndBind("", "C-RET",
                        GCFunc(selectBody), "select object");
    gc->registerFunction(GCFunc(gotoBody), "goto object");
    gc->registerAndBind("", "C-c g",
                        GCFunc(gotoBodyGC), "goto object gc");
    gc->registerFunction(GCFunc(selectStar), "select star");
    gc->registerFunction(GCFunc(unmarkAll), "unmark all");
    gc->registerFunction(GCFunc(markObject), "mark object");
    gc->registerAndBind("", "C-x 4 f",
                        GCFunc(openScript), "open script");
    gc->registerFunction(GCFunc(setFlag), "celestia options");
    // aliases
    gc->registerFunction(GCFunc("celestia options", "@show"),
                         "show objects");
    gc->registerFunction(GCFunc("celestia options", "@hide"),
                         "hide objects");
}
