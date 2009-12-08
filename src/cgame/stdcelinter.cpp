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

        gc->setInteractive(listInteractive, "select-star", _("Select star"), "");
        listInteractive->setCompletion(completion);
        break;
    }
    case -2-1: // describe star
    {
        cout << 2 << "\n";
        Selection sel = gc->getCelCore()->getSimulation()->
            findObjectFromPath(value, true);
        std::string descr = describeSelection(sel, gc->getCelCore());
        if (!descr.empty())
            gc->descriptionStr = describeSelection(sel, gc->getCelCore());
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
}
