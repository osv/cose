#include "cgame.h"
#include "geekconsole.h"
#include <celutil/directory.h>
#include <celengine/starbrowser.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <celutil/formatnum.h>
#include <celengine/eigenport.h>

using namespace Eigen;

#ifndef CEL_DESCR_SEP
#define CEL_DESCR_SEP " | "
#endif

Color32 clBackgroundD(100, 100, 250,200);
Color32 *clBackground = &clBackgroundD;
Color32 clBgInteractiveD(0, 0, 0, 150);
Color32 *clBgInteractive = &clBgInteractiveD;
Color32 clBgInteractiveBrdD(255, 255, 255, 200);
Color32 *clBgInteractiveBrd = &clBgInteractiveBrdD;
Color32 clInteractiveFntD(255, 255, 255, 255);
Color32 *clInteractiveFnt =&clInteractiveFntD;
Color32 clInteractivePrefixFntD(255, 155, 255, 255);
Color32 *clInteractivePrefixFnt = &clInteractivePrefixFntD;
Color32 clInteractiveExpandD(255, 200, 200, 255);
Color32 *clInteractiveExpand = &clInteractiveExpandD;
Color32 clDescrFntD(200, 255, 200, 255);
Color32 *clDescrFnt = &clDescrFntD;
Color32 clCompletionFntD(255, 255, 255, 255);
Color32 *clCompletionFnt = &clCompletionFntD;
Color32 clCompletionMatchCharFntD(0, 0, 0, 255);
Color32 *clCompletionMatchCharFnt = &clCompletionMatchCharFntD;
Color32 clCompletionAfterMatchD(0, 255, 0, 255);
Color32 *clCompletionAfterMatch = &clCompletionAfterMatchD;
Color32 clCompletionMatchCharBgD(255, 0, 0, 255);
Color32 *clCompletionMatchCharBg = &clCompletionMatchCharBgD;
Color32 clCompletionExpandBrdD(255, 0, 0, 255);
Color32 *clCompletionExpandBrd = &clCompletionExpandBrdD;

GeekConsole *geekConsole = NULL;
const std::string ctrlZDescr("C-z - Unexpand");
const std::string strUniqMatchedRET("Unique match, RET - complete and finish");

std::string historyDir("history/");

static FormattedNumber SigDigitNum(double v, int digits)
{
    return FormattedNumber(v, digits,
                           FormattedNumber::GroupThousands |
                           FormattedNumber::SignificantDigits);
}

static void distance2Sstream(std::stringstream &ss, double distance)
{
    const char* units = "";

    if (abs(distance) >= astro::parsecsToLightYears(1e+6))
    {
        units = "Mpc";
        distance = astro::lightYearsToParsecs(distance) / 1e+6;
    }
    else if (abs(distance) >= 0.5 * astro::parsecsToLightYears(1e+3))
    {
        units = "Kpc";
        distance = astro::lightYearsToParsecs(distance) / 1e+3;
    }
    else if (abs(distance) >= astro::AUtoLightYears(1000.0f))
    {
        units = _("ly");
    }
    else if (abs(distance) >= astro::kilometersToLightYears(10000000.0))
    {
        units = _("au");
        distance = astro::lightYearsToAU(distance);
    }
    else if (abs(distance) > astro::kilometersToLightYears(1.0f))
    {
        units = "km";
        distance = astro::lightYearsToKilometers(distance);
    }
    else
    {
        units = "m";
        distance = astro::lightYearsToKilometers(distance) * 1000.0f;
    }

    ss << SigDigitNum(distance, 5) << ' ' << units;
}

static void planetocentricCoords2Sstream(std::stringstream& ss,
                                         const Body& body,
                                         double longitude,
                                         double latitude,
                                         double altitude,
                                         bool showAltitude)
{
    char ewHemi = ' ';
    char nsHemi = ' ';
    double lon = 0.0;
    double lat = 0.0;

    // Terrible hack for Earth and Moon longitude conventions.  Fix by
    // adding a field to specify the longitude convention in .ssc files.
    if (body.getName() == "Earth" || body.getName() == "Moon")
    {
        if (latitude < 0.0)
            nsHemi = 'S';
        else if (latitude > 0.0)
            nsHemi = 'N';

        if (longitude < 0.0)
            ewHemi = 'W';
        else if (longitude > 0.0f)
            ewHemi = 'E';

        lon = (float) abs(radToDeg(longitude));
        lat = (float) abs(radToDeg(latitude));
    }
    else
    {
        // Swap hemispheres if the object is a retrograde rotator
        Quaterniond q = body.getEclipticToEquatorial(astro::J2000);
        bool retrograde = (q * Vector3d::UnitY()).y() < 0.0;

        if ((latitude < 0.0) ^ retrograde)
            nsHemi = 'S';
        else if ((latitude > 0.0) ^ retrograde)
            nsHemi = 'N';
        
        if (retrograde)
            ewHemi = 'E';
        else
            ewHemi = 'W';

        lon = -radToDeg(longitude);
        if (lon < 0.0)
            lon += 360.0;
        lat = abs(radToDeg(latitude));
    }

    ss.unsetf(ios::fixed);
    ss << setprecision(6);
    ss << lat << nsHemi << ' ' << lon << ewHemi;
    if (showAltitude)
        ss << ' ' << altitude << _("km");
}

std::string describeSelection(Selection sel, CelestiaCore *celAppCore)
{
    std::stringstream ss;
    if (!sel.empty())
    {
        Vec3d v = sel.getPosition(celAppCore->getSimulation()->getTime()) -
            celAppCore->getSimulation()->getObserver().getPosition();
        double distance;
        Star *star;
        SolarSystem* sys;
        double kmDistance;
        Body *body;
        DeepSkyObject *dso;
        Location *location;
        switch (sel.getType())
        {
        case Selection::Type_Star:
            distance = v.length() * 1e-6;
            star = sel.star();
            ss << "[Star] Dist: ";
            distance2Sstream(ss, distance);
            ss << CEL_DESCR_SEP << _("Radius: ")
               << SigDigitNum(star->getRadius() / 696000.0f, 2)
               << " " << _("Rsun")
               << "  (" << SigDigitNum(star->getRadius(), 3) << " km" << ")"
               << CEL_DESCR_SEP "class ";
            if (star->getSpectralType()[0] == 'Q')
                ss <<  _("Neutron star");
            else if (star->getSpectralType()[0] == 'X')
                ss <<  _("Black hole");
            else
                ss << star->getSpectralType();
            sys = celAppCore->getSimulation()->
                getUniverse()->getSolarSystem(star);
            if (sys != NULL && sys->getPlanets()->getSystemSize() != 0)
            {
                int planetsnum = 0;
                int dwarfsnum = 0;
                int astersnum = 0;
                PlanetarySystem *planets = sys->getPlanets();
                for (int i = 0; i < planets->getSystemSize(); i++)
                {
                    switch (planets->getBody(i)->getClassification())
                    {
                    case Body::Planet:
                        planetsnum++;
                        break;
                    case Body::DwarfPlanet:
                        dwarfsnum++;
                        break;
                    case Body::Asteroid:
                        astersnum++;
                        break;
                    default:
                        break;
                    }
                }
                ss << CEL_DESCR_SEP << planetsnum << " Planets, " << dwarfsnum
                   << " Dwarf, " << astersnum << " Asteroid";
            }
            break;
        case Selection::Type_Body:
            distance = v.length() * 1e-6,
                v * astro::microLightYearsToKilometers(1.0);
            body = sel.body();
            kmDistance = astro::lightYearsToKilometers(distance);
            ss << "[Body] Dist: ";
            distance = astro::kilometersToLightYears(kmDistance - body->getRadius());
            distance2Sstream(ss, distance);
            ss << CEL_DESCR_SEP << _("Radius: ");
            distance = astro::kilometersToLightYears(body->getRadius());
            distance2Sstream(ss, distance);
            break;
        case Selection::Type_DeepSky:
        {
            dso = sel.deepsky();
            distance = v.length() * 1e-6 - dso->getRadius();
            char descBuf[128];
            dso->getDescription(descBuf, sizeof(descBuf));
            ss << "[DSO, "
               << descBuf
               << "] ";
            if (distance >= 0)
            {
                ss << _("Distance: ");
                distance2Sstream(ss, distance);
            }
            else
            {
                ss << _("Distance from center: ");
                distance2Sstream(ss, distance + dso->getRadius());
            }
            ss << CEL_DESCR_SEP << _("Radius: ");
            distance2Sstream(ss, dso->getRadius());
            break;
        }
        case Selection::Type_Location:
        {
            location = sel.location();
            body = location->getParentBody();
            ss << "[Location] "
               << _("Distance: ");
            distance2Sstream(ss, v.length() * 1e-6);
            ss << CEL_DESCR_SEP;
            Vector3f locPos = location->getPosition();
            Vector3d lonLatAlt = body->cartesianToPlanetocentric(locPos.cast<double>());
            planetocentricCoords2Sstream(ss, *body,
                                         lonLatAlt.x(), lonLatAlt.y(), lonLatAlt.z(), false);
            break;
        }
        default:
            break;
        }
    }
    return ss.str();
}

/*
  GeekConsole
 */
GeekConsole::GeekConsole(CelestiaCore *celCore):
    isVisible(false),
    consoleType(Tiny),
    titleFont(NULL),
    font(NULL),
    celCore(celCore),
    overlay(NULL),
    curInteractive(NULL),
    curFun(NULL)
{
    overlay = new Overlay();
    *overlay << setprecision(6);
    *overlay << ios::fixed;
    // create global key bind space
//    geekConsole->createGeekBind("Global");
    GeekBind *gb = new GeekBind("Global", this);
    geekBinds.push_back(gb);

}

GeekConsole::~GeekConsole()
{
    for_each(geekBinds.begin(), geekBinds.end(), deleteFunc<GeekBind*>());;
    delete overlay;
}


void GeekConsole::execFunction(GCFunc *fun)
{
    curFun = fun;
    funState = 0;
    isVisible = true;
    fun->call(this, funState, "");
}

bool GeekConsole::execFunction(std::string funName)
{
    GCFunc *f = getFunctionByName(funName);
    curFun = f;
    funState = 0;
    if (f)
    {
        isVisible = true;
        f->call(this, funState, "");
    }
    else
        isVisible = false;
    return f;
}

bool GeekConsole::execFunction(std::string funName, std::string param)
{
    GCFunc *f = getFunctionByName(funName);
    curFun = f;
    funState = 0;
    if (f)
    {
        isVisible = true;
        std::string val;
        curFun->call(this, funState, "");
        if (!param.empty())
        {
            if (param[param.size() - 1] == '@')
                param.resize(param.size() - 1);
            if (param[0] == '@')
                param.erase(0, 1);
        }
        else
            return true;
        vector<string> res;
        string::size_type next_pos, curr_pos=0;

        for (int i=0; ;i++) {
            next_pos = param.find_first_of("@", curr_pos);
            val = (param.substr(curr_pos, next_pos-curr_pos));
            funState++;
            if (!isVisible)
            {
                if (val == "EXEC")
                {
                    GCFunc *f = getFunctionByName(funName);
                    if (!f)
                    {
                        DPRINTF(1, "GFunction not found: '%s'", val.c_str());
                        return false;
                    }
                    isVisible = true;
                    curFun = f;
                    funState = 0;
                    curFun->call(this, funState, "");
                }
            }
            else
                curFun->call(this, funState, val);

            if (string::npos == next_pos)
                break;
            curr_pos = next_pos+1;
        }
        // int i = 0;
        // size_t pos = 0, end;
        // std::string val;
        // curFun->call(this, funState, "");
        // pos = param.find('@');
        // funState++;
        // while(pos != std::string::npos && isVisible)
        // {
        //     end = param.find('@', pos + 1);
        //     val = param.substr(pos + 1, end - 1);
        //     cout << val << "' --- val state: " << funState << " " << param << "\n";
        //     isVisible = true;
        //     curFun->call(this, funState, val);
        //     pos = param.find('@', end);
        // }
    }
    else
        isVisible = false;
    return f;
}

void GeekConsole::describeCurText(std::string text)
{
    if (curFun)
        curFun->call(this, -funState - 1, text);
}

void GeekConsole::registerFunction(GCFunc fun, std::string name)
{
    Functions::iterator it = functions.find(name);
    if (it == functions.end())
    {
        functions[name] = fun;
        DPRINTF(1, "Registering function for geek console: '%s'\n", name.c_str());
    }
}

void GeekConsole::reRegisterFunction(GCFunc fun, std::string name)
{
    functions[name] = fun;
    DPRINTF(1, "Reregistering function for geek console: '%s'\n", name.c_str());
}

GCFunc *GeekConsole::getFunctionByName(std::string name)
{
    Functions::iterator it;
    it = functions.find(name);
    if (it != functions.end())
        return &it->second;
    else return NULL;
}
std::vector<std::string> GeekConsole::getFunctionsNames()
{
    std::vector<std::string> names;
    for (Functions::const_iterator iter = functions.begin();
         iter != functions.end(); iter++)
    {
        const string& alias = iter->first;
        {
            names.push_back(alias);
        }
    }
    return names;
}

bool GeekConsole::charEntered(const char sym, const wchar_t wc, int modifiers)
{
    if (!isVisible)
    {
        bool isPrefix = false;
        for (std::vector<GeekBind *>::iterator it = geekBinds.begin();
             it != geekBinds.end(); it++)
        {
            GeekBind *gb = *it;
            if (gb->isActive)
            {
                GeekBind::GBRes r = gb->charEntered(sym, modifiers);
                if (r == GeekBind::KEYPREFIX)
                    isPrefix = true;
                else if (r == GeekBind::KEYMATCH)
                    return true;
            }
        }
        return isPrefix;
    }
    char C = toupper((char)wc);
    switch (C)
    {
    case '\023':  // Ctrl+U
        if (consoleType == Big)
            consoleType = Tiny;
        else
            consoleType++;
        return true;
    case '\007':  // Ctrl+G
    case '\033':  // ESC
        finish();
    return true;
    }

    if (curInteractive)
        curInteractive->charEntered(wc, modifiers);
    return true;
}

void GeekConsole::render()
{
    if (!isVisible || !curInteractive)
        return;
    if (font == NULL)
        font = getFont(celCore);
    if (titleFont == NULL)
        titleFont = getTitleFont(celCore);
    if (font == NULL || titleFont == NULL)
        return;

    float fontH = font->getHeight();
    float titleFontH = titleFont->getHeight();
    int nb_lines;
    float complH; // height of completion area
    float rectH; // height of console
    switch(consoleType)
    {
    case GeekConsole::Tiny:
        nb_lines = 3;
        break;
    case GeekConsole::Medium:
        nb_lines = (height * 0.4 - 2 * titleFontH) / fontH;
        break;
    case GeekConsole::Big:
    default:
        nb_lines = (height * 0.8 - 2 * titleFontH) / fontH;
        break;
    }
    complH = (nb_lines+1) * fontH + nb_lines;
    rectH = complH + 2 * titleFontH;
    overlay->begin();
    glTranslatef(0.0f, 5.0f, 0.0f); //little margin from bottom

    // background
    glColor4ubv(clBackground->rgba);
    overlay->rect(0.0f, 0.0f, width, rectH);

    // Interactive & description rects
    glColor4ubv(clBgInteractive->rgba);
    overlay->rect(0.0f, 0.0f , width, titleFontH);
    overlay->rect(0.0f, rectH - titleFontH , width, titleFontH);
    glColor4ubv(clBgInteractiveBrd->rgba);
    overlay->rect(0.0f, 0.0f , width-1, titleFontH, false);
    overlay->rect(0.0f, rectH - titleFontH , width-1, titleFontH, false);

    // render Interactive
    glPushMatrix();
    {
        overlay->setFont(titleFont);
    	glTranslatef(2.0f, rectH - titleFontH + 4, 0.0f);
        overlay->beginText();
        glColor4ubv(clInteractivePrefixFnt->rgba);
    	*overlay << InteractivePrefixStr << " ";
        curInteractive->renderInteractive();
        overlay->endText();
    }
    glPopMatrix();

    // compl list
    glPushMatrix();
    {
    	glTranslatef(2.0f, rectH - fontH - titleFontH, 0.0f);
        overlay->setFont(font);
        curInteractive->renderCompletion(complH - fontH, width-1);
    }
    glPopMatrix();

    // description line
    glPushMatrix();
    {
    	glTranslatef(2.0f, 4.0f, 0.0f);
        overlay->setFont(titleFont);
        overlay->beginText();

        glColor4ubv(clDescrFnt->rgba);

    	*overlay << descriptionStr;
        overlay->endText();
    }
    glPopMatrix();
    overlay->end();
}

void GeekConsole::setInteractive(GCInteractive *Interactive, std::string historyName, std::string InteractiveStr, std::string descrStr)
{
    curInteractive = Interactive;
    curInteractive->Interact(this, historyName);
    InteractivePrefixStr = InteractiveStr;
    descriptionStr = descrStr;
}

void GeekConsole::InteractFinished(std::string value)
{
    if(!curFun) return;
    funState++;
    curFun->call(this, funState, value);
}

void GeekConsole::finish()
{
    isVisible = false;
    curFun = NULL;
    if (curInteractive)
        curInteractive->cancelInteractive();
}

GeekBind *GeekConsole::createGeekBind(std::string bindspace)
{
    GeekBind *gb = new GeekBind(bindspace, this);
    geekBinds.push_back(gb);
    return gb;
}

GeekBind *GeekConsole::getGeekBind(std::string bindspace)
{
    for (std::vector<GeekBind *>::iterator it = geekBinds.begin();
		 it != geekBinds.end(); it++)
    {
        GeekBind *gb = *it;
        if (gb->getName() == bindspace)
            return gb;
    }
    return NULL;
}

void GeekConsole::registerAndBind(std::string bindspace, const char *bindkey,
                                  GCFunc fun, const char *funname)
{
    registerFunction(fun, funname);
    if (bindspace.empty())
        bindspace = "Global";
    GeekBind *gb = getGeekBind(bindspace);
    if (gb)
        gb->bind(bindkey, funname);
}

bool GeekConsole::bind(std::string bindspace, std::string bindkey, std::string function)
{
    if (bindspace.empty())
        bindspace = "Global";
    GeekBind *gb = getGeekBind(bindspace);
    if (gb)
        return gb->bind(bindkey.c_str(), function);
    return false;
}

void GeekConsole::unbind(std::string bindspace, std::string bindkey)
{
    if (bindspace.empty())
        bindspace = "Global";
    GeekBind *gb = getGeekBind(bindspace);
    if (gb)
        gb->unbind(bindkey.c_str());
}

/******************************************************************
 *  Interactives
 ******************************************************************/

GCInteractive::GCInteractive(std::string _InteractiveName, bool _useHistory)
{
    InteractiveName = _InteractiveName;
    useHistory = _useHistory;
    if (!useHistory)
        return;
    Directory* dir = OpenDirectory(historyDir);
    std::string filename;
    if (dir != NULL)
        while (dir->nextFile(filename))
        {
            if (filename[0] == '.')
                continue;
            int extPos = filename.rfind('.');
            if (extPos == (int)string::npos)
                continue;
            std::string ext = string(filename, extPos, filename.length() - extPos + 1);
            if (compareIgnoringCase("." + InteractiveName, ext) == 0)
            {
                std::string line;
                std::ifstream infile((historyDir + filename).c_str(), ifstream::in);
                std::string histName = string(filename, 0, extPos);
                while (getline(infile, line, '\n'))
                {
                    history[histName].push_back(line);
                }
                infile.close();
            }
        }
}

GCInteractive::~GCInteractive()
{
    if (!useHistory)
        return;
    // save history
    Directory* dir = OpenDirectory(historyDir);
    std::string filename;
    if (dir != NULL)
    {
        History::iterator it1;
        std::vector<std::string>::iterator it2;
        for (it1 = history.begin();
             it1 != history.end(); it1++)
        {
            std::string name = it1->first;
            std::ofstream outfile(string(historyDir + name + "." + InteractiveName).c_str(), ofstream::out);

            for (it2 = history[name].begin();
                 it2 != history[name].end(); it2++)
            {
                outfile << *it2 << "\n";
            }
            outfile.close();
        }
    }
    else
        cout << "Cann't locate '" << historyDir <<"' dir. GeekConsole's history not saved." << "\n";
}

void GCInteractive::Interact(GeekConsole *_gc, string historyName)
{
    gc = _gc;
    buf = "";
    curHistoryName = historyName;
    typedHistoryCompletionIdx = 0;
    bufSizeBeforeHystory = 0;
    prepareHistoryCompletion();
}

void GCInteractive::charEntered(const wchar_t wc, int modifiers)
{
    char C = toupper((char)wc);
    std::vector<std::string>::iterator it;
    std::vector<std::string>::reverse_iterator rit;
    switch (C)
    {
    case '\n':
    case '\r':
        {
            if (useHistory && !curHistoryName.empty())
            {
                // append history only if buf not empty and not equal to oprev hist
                if (!buf.empty())
                    if (!history[curHistoryName].empty())
                    {
                        rit = history[curHistoryName].rbegin();
                        if (*rit != buf)
                            history[curHistoryName].push_back(buf);
                    }
                    else
                        history[curHistoryName].push_back(buf);
                if (history[curHistoryName].size() > MAX_HISTORY_SYZE)
                    history[curHistoryName].erase(history[curHistoryName].begin(),
                                                  history[curHistoryName].begin()+1);
            }
            gc->InteractFinished(buf);
            return;
        }
    case '\020':  // Ctrl+P prev history
        if (typedHistoryCompletion.empty())
            return;
        if (typedHistoryCompletionIdx > typedHistoryCompletion.size()-1)
            typedHistoryCompletionIdx = 0;
        rit = typedHistoryCompletion.rbegin() + typedHistoryCompletionIdx;
        typedHistoryCompletionIdx++;
        buf = *rit;
        gc->descriptionStr = ctrlZDescr;
        gc->describeCurText(getBufferText());
        return;
    case '\016':  // Ctrl+N forw history
        if (typedHistoryCompletion.empty())
            return;
        if (typedHistoryCompletionIdx > typedHistoryCompletion.size()-1)
            typedHistoryCompletionIdx = 0;
        it = typedHistoryCompletion.begin() + typedHistoryCompletionIdx;
        typedHistoryCompletionIdx--;
        buf = *it;
        gc->descriptionStr = ctrlZDescr;
        gc->describeCurText(getBufferText());
        return;
    case '\032':  // Ctrl+Z
        if (bufSizeBeforeHystory == buf.size())
            return;
        setBufferText(string(buf, 0, bufSizeBeforeHystory));
        prepareHistoryCompletion();
        return;
    case '\025': // Ctrl+U
        setBufferText("");
        prepareHistoryCompletion();
        break;
    case '\027':  // Ctrl+W
    case '\b':
        if ((modifiers & GeekBind::CTRL) != 0)
            while (buf.size() && !strchr(" /:,;", buf[buf.size() - 1])) {
                setBufferText(string(buf, 0, buf.size() - 1));
            }
    setBufferText(string(buf, 0, buf.size() - 1));
    prepareHistoryCompletion();
    default:
        break;
    }
#ifdef TARGET_OS_MAC
    if ( wc && (!iscntrl(wc)) )
#else
        if ( wc && (!iswcntrl(wc)) )
#endif
        {
            buf += wc;
            prepareHistoryCompletion();
            bufSizeBeforeHystory = buf.size();
        }
    gc->descriptionStr.clear();
}

void GCInteractive::cancelInteractive()
{
}

void GCInteractive::setBufferText(std::string str)
{
    buf = str;
    bufSizeBeforeHystory = buf.size();
}

void GCInteractive::renderInteractive()
{
    glColor4ubv(clInteractiveFnt->rgba);
    *gc->getOverlay() << string(buf, 0, bufSizeBeforeHystory);
    if (bufSizeBeforeHystory <= buf.size())
    {
        glColor4ubv(clInteractiveExpand->rgba);
    	std::string s = string(buf, bufSizeBeforeHystory, buf.size() - bufSizeBeforeHystory);
    	*gc->getOverlay() << s;
        glColor4ubv(clInteractiveFnt->rgba);
        *gc->getOverlay() << "|";
    }
}

void GCInteractive::prepareHistoryCompletion()
{
    std::vector<std::string>::iterator it;
    typedHistoryCompletion.clear();
    int buf_length = UTF8Length(buf);
    // Search through all string in history
    for (it = history[curHistoryName].begin();
         it != history[curHistoryName].end(); it++)
    {
        if (buf_length == 0 ||
            (UTF8StringCompare(*it, buf, buf_length) == 0))
            typedHistoryCompletion.push_back(*it);
    }
    typedHistoryCompletionIdx = 0;
}

void GCInteractive::renderCompletion(float height, float width)
{

}

void PasswordInteractive::renderInteractive()
{
    glColor4ubv(clInteractiveFnt->rgba);
    for (uint i = 0; i < getBufferText().length(); i++)
        *gc->getOverlay() << "*";
    *gc->getOverlay() << "|";
}
// ListInteractive

void ListInteractive::Interact(GeekConsole *_gc, string historyName)
{
    GCInteractive::Interact(_gc, historyName);
    setMatchCompletion(false);
    typedTextCompletion.clear();
    completionList.clear();
    pageScrollIdx = 0;
    completedIdx = -1;
}

void ListInteractive::updateTextCompletion()
{
    std::string buftext = string(getBufferText(), 0, bufSizeBeforeHystory);
    std::vector<std::string>::iterator it;
    typedTextCompletion.clear();
    int buf_length = UTF8Length(buftext);
    // Search through all string in base completion list
    for (it = completionList.begin();
         it != completionList.end(); it++)
    {
        if (buf_length == 0 ||
            (UTF8StringCompare(*it, buftext, buf_length) == 0))
            typedTextCompletion.push_back(*it);
    }
}

bool ListInteractive::tryComplete()
{
    int i = 0; // match size
    bool match = true;
    int buf_length = UTF8Length(getBufferText());
    vector<std::string>::const_iterator it1, it2;
    if (!typedTextCompletion.empty())
        if (typedTextCompletion.size() == 1) // only 1 item to expand
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            setBufferText(*it);
        }
        else
        {
            while(match) // find size of matched chars to expand
            {
                uint compareIdx = buf_length + i;
                it1 = typedTextCompletion.begin();
                for (it2 = typedTextCompletion.begin()+1;
                     it2 != typedTextCompletion.end() && match; it2++)
                {
                    std::string s1 = *it1;
                    std::string s2 = *it2;
                    if (s1.length() < compareIdx ||
                        s2.length() <= compareIdx ||
                        s1[compareIdx] != s2[compareIdx])
                        match = false;
                }
                if (match)
                    i++;
            }
        }
    if (i > 0)
    {
        vector<std::string>::const_iterator it = typedTextCompletion.begin();
        setBufferText(string(*it, 0, buf_length + i));
        return true;
    }
    return false;
}

void ListInteractive::charEntered(const wchar_t wc, int modifiers)
{
    char C = toupper((char)wc);
    if  (C == '\t') // TAB - expand and scroll if many items to expand found
    {
        if (!tryComplete())
        {
            // scroll
            pageScrollIdx += scrollSize;
            if (pageScrollIdx > typedTextCompletion.size())
                pageScrollIdx = 0;
            char buff[32];
            sprintf(buff,"%i of %i pages, M-/ next expand, M-? prev expand, C-z unexpand", (pageScrollIdx / scrollSize) + 1,
                    (typedTextCompletion.size() / scrollSize) + 1);
            gc->descriptionStr = buff;
            gc->describeCurText(getBufferText());
            completedIdx = pageScrollIdx;
            return;
        }

    }
    // expand completion on M-/
    else if (((modifiers & GeekBind::META) != 0) && C == '/')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            completedIdx++;
            if (completedIdx >= typedTextCompletion.size())
                completedIdx = pageScrollIdx;
            if (completedIdx > pageScrollIdx + scrollSize - 1)
                completedIdx = pageScrollIdx;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setBufferText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            gc->descriptionStr = ctrlZDescr;
            gc->describeCurText(getBufferText());
            return;
        }
    }
    // revers expand completion on M-? ( i.e. S-M-/)
    else if (((modifiers & GeekBind::META ) != 0) && C == '?')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (completedIdx == 0)
                completedIdx = pageScrollIdx + scrollSize;
            completedIdx--;
            if (completedIdx < pageScrollIdx)
                completedIdx = pageScrollIdx + scrollSize;
            if (completedIdx > typedTextCompletion.size())
                completedIdx = typedTextCompletion.size() - 1;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setBufferText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            gc->descriptionStr = ctrlZDescr;
            gc->describeCurText(getBufferText());
            return;
        }
    }
    else if ((C == '\n' || C == '\r'))
    {
        tryComplete();
        if(mustMatch)
        {
            std::string buftext = getBufferText();
            std::vector<std::string>::iterator it;
            for (it = typedTextCompletion.begin();
                 it != typedTextCompletion.end(); it++)
            {
                if (*it == buftext)
                {
                    GCInteractive::charEntered(wc, modifiers);
                    return;
                }
            }
            return;
        }
    }
    std::string oldBufText = getBufferText();

    GCInteractive::charEntered(wc, modifiers);

    std::string buftext = getBufferText();
    // reset page scroll only if text changed
    if (oldBufText != buftext)
        pageScrollIdx = 0;

    // if must_always_match with completion items - try complete

    if(mustMatch && buftext.length() > oldBufText.length())
        tryComplete();

    // Refresh completion (text to complete is only first bufSizeBeforeHystory chars
    // of buf text)
    updateTextCompletion();

    // if match on - complete again
    // and if no any item to expand than revert to old text when text increased
    if(mustMatch && buftext.length() > oldBufText.length())
    {
        tryComplete();
        if (typedTextCompletion.empty())
        {
            setBufferText(oldBufText);
            updateTextCompletion();
        }
    }

    if (typedTextCompletion.size() == 1)
    {
        gc->descriptionStr = strUniqMatchedRET;
	    gc->describeCurText(getBufferText());
    }
}

void ListInteractive::renderCompletion(float height, float width)
{
    TextureFont *font = gc->getCompletionFont();
    float fh = font->getHeight();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel
    uint nb_cols = 4;
    scrollSize = nb_cols * nb_lines;
    std::string buftext(getBufferText(), 0, bufSizeBeforeHystory);
    uint buf_length = buftext.size();
    vector<std::string>::const_iterator it = typedTextCompletion.begin();
    it += pageScrollIdx;
    for (uint i=0; it < typedTextCompletion.end() && i < nb_cols; i++)
    {
        glPushMatrix();
        gc->getOverlay()->beginText();
        for (uint j = 0; it < typedTextCompletion.end() && j < nb_lines; it++, j++)
        {
            std::string s = *it;
            glColor4ubv(clCompletionFnt->rgba);
            if (i * nb_lines + j == completedIdx - pageScrollIdx)
            {
                // border of current completopn
                glColor4ubv(clCompletionExpandBrd->rgba);
                gc->getOverlay()->rect(0.0f, 0.0f - 2,
                                       font->getWidth(s), font->getHeight(), false);
            }
            // completion item (only text before match char)
            glColor4ubv(clCompletionFnt->rgba);
            *gc->getOverlay() << std::string(s, 0, buf_length);
            // match char background
            // match char background
            if (s.size() >= buf_length)
            {
                if (clCompletionMatchCharBg->rgba[3] > 0)
                {
                    std::string text(s, buf_length, 1);
                    glColor4ubv(clCompletionMatchCharBg->rgba);
                    gc->getOverlay()->rect(font->getWidth(string(s, 0, buf_length)), 0.0f - 2,
                                           font->getWidth(text), font->getHeight());
                    glColor4ubv(clCompletionMatchCharFnt->rgba);
                    *gc->getOverlay() << text;
                }
                // rest text of item from match char
                glColor4ubv(clCompletionAfterMatch->rgba);
                if (s.size() > buf_length)
                    *gc->getOverlay() << string(s, buf_length + 1, 255);
                *gc->getOverlay() << "\n";
            }
        }
        gc->getOverlay()->endText();
        glPopMatrix();
        int shiftx = width/nb_cols;
        glTranslatef((float) (shiftx), 0.0f, 0.0f);
    }
}

void ListInteractive::setCompletion(std::vector<std::string> completion)
{
    completionList = completion;
    pageScrollIdx = 0;
    typedTextCompletion =  completion;
}

void ListInteractive::setMatchCompletion(bool _mustMatch)
{
    mustMatch = _mustMatch;
}

void ListInteractive::setCompletionFromSemicolonStr(std::string str)
{
    completionList.clear();
    uint begin = 0;
    uint f;
    while(1)
    {
        f = str.find(';', begin);
        if (f != string::npos)
        {
            std::string item(str, begin, f - begin);
            completionList.push_back(item);
            begin = f + 1;
        }
        else
        {
            if (str.size() > begin)
                completionList.push_back(std::string(str, begin, str.size() - begin));
            break;
        }
    }
    pageScrollIdx = 0;
    typedTextCompletion = completionList;
}

CelBodyInteractive::CelBodyInteractive(std::string name, CelestiaCore *core):
    GCInteractive(name), celApp(core)
{

}

void CelBodyInteractive::updateTextCompletion()
{
    string str = GCInteractive::getBufferText();

    //skip update for some history and expand action
    if (str.length() != bufSizeBeforeHystory)
        return;
    typedTextCompletion = celApp->getSimulation()->
        getObjectCompletion(str,
                            (celApp->getRenderer()->getLabelMode() & Renderer::LocationLabels) != 0);
}

std::string CelBodyInteractive::getRightText() const
{
    string str = GCInteractive::getBufferText();
    string::size_type pos = str.rfind('/', str.length());
    if (pos != string::npos)
        return string(str, pos + 1);
    else
        return str;
}

void CelBodyInteractive::setRightText(std::string text)
{
    string str = GCInteractive::getBufferText();
    string::size_type pos = str.rfind('/', str.length());
    if (pos != string::npos)
    {
        GCInteractive::setBufferText(string(str, 0, pos + 1) + text);
    }
    else
        GCInteractive::setBufferText(text);
}

void CelBodyInteractive::Interact(GeekConsole *_gc, string historyName)
{
    GCInteractive::Interact(_gc, historyName);
    updateTextCompletion();
    pageScrollIdx = 0;
    completedIdx = -1;
    lastCompletionSel = Selection();
}

bool CelBodyInteractive::tryComplete()
{
    int i = 0; // match size
    bool match = true;
    int buf_length = UTF8Length(getRightText());
    vector<std::string>::const_iterator it1, it2;
    if (!typedTextCompletion.empty())
        if (typedTextCompletion.size() == 1) // only 1 item to expand
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            setRightText(*it);
        }
        else
        {
            while(match) // find size of matched chars to expand
            {
                uint compareIdx = buf_length + i;
                it1 = typedTextCompletion.begin();
                for (it2 = typedTextCompletion.begin()+1;
                     it2 != typedTextCompletion.end() && match; it2++)
                {
                    std::string s1 = *it1;
                    std::string s2 = *it2;
                    if (s1.length() < compareIdx ||
                        s2.length() <= compareIdx ||
                        s1[compareIdx] != s2[compareIdx])
                        match = false;
                }
                if (match)
                    i++;
            }
        }
    if (i > 0)
    {
        vector<std::string>::const_iterator it = typedTextCompletion.begin();
        setRightText(string(*it, 0, buf_length + i));
        return true;
    }
    return false;
}

void CelBodyInteractive::charEntered(const wchar_t wc, int modifiers)
{
    char C = toupper((char)wc);
    if  (C == '\t') // TAB - expand and scroll if many items to expand found
    {
        if (!tryComplete())
        {
            // scroll
            pageScrollIdx += scrollSize;
            if (pageScrollIdx > typedTextCompletion.size())
                pageScrollIdx = 0;
            char buff[32];
            sprintf(buff,"%i of %i pages, M-/ next expand, M-? prev expand, C-z unexpand", (pageScrollIdx / scrollSize) + 1,
                    (typedTextCompletion.size() / scrollSize) + 1);
            gc->descriptionStr = buff;
            completedIdx = pageScrollIdx;
            updateDescrStr();
            return;
        }

    }
    // expand completion on M-/
    else if (((modifiers & GeekBind::META) != 0) && C == '/')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            completedIdx++;
            if (completedIdx >= typedTextCompletion.size())
                completedIdx = pageScrollIdx;
            if (completedIdx > pageScrollIdx + scrollSize - 1)
                completedIdx = pageScrollIdx;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setRightText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            updateDescrStr();
            gc->describeCurText(getRightText());
            return;
        }
    }
    // revers expand completion on M-? ( i.e. S-M-/)
    else if (((modifiers & GeekBind::META ) != 0) && C == '?')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (completedIdx == 0)
                completedIdx = pageScrollIdx + scrollSize;
            completedIdx--;
            if (completedIdx < pageScrollIdx)
                completedIdx = pageScrollIdx + scrollSize;
            if (completedIdx > typedTextCompletion.size())
                completedIdx = typedTextCompletion.size() - 1;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setRightText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            updateDescrStr();
            gc->describeCurText(getRightText());
            return;
        }
    }
    // M-c
    else if (((modifiers & GeekBind::META ) != 0) && C == 'C')
    {
        celApp->getSimulation()->setSelection(lastCompletionSel);
        celApp->getSimulation()->centerSelection();
        return;
    }
    else if ((C == '\n' || C == '\r'))
    {
        Selection sel = celApp->getSimulation()->findObjectFromPath(GCInteractive::getBufferText(), true);
        if (sel.empty()) // dont continue if not match any obj
            return;
        GCInteractive::charEntered(wc, modifiers);
        return;
    }

    std::string oldBufText = GCInteractive::getBufferText();

    GCInteractive::charEntered(wc, modifiers);

    std::string buftext = GCInteractive::getBufferText();

    // reset page scroll only if text changed
    if (oldBufText != buftext)
        pageScrollIdx = 0;

    updateTextCompletion();

    updateDescrStr();
}

void CelBodyInteractive::updateDescrStr()
{
    Selection sel = celApp->getSimulation()->findObjectFromPath(GCInteractive::getBufferText(), true);
    std::string desc = describeSelection(sel, celApp);
    if (!desc.empty())
    {
        gc->descriptionStr = desc + _(", M-c - Select");
    }
    // mark selected
    MarkerRepresentation markerRep(MarkerRepresentation::Crosshair);
    markerRep.setSize(10.0f);
    markerRep.setColor(Color(0.0f, 1.0f, 0.0f, 0.9f));
    celApp->getSimulation()->
        getUniverse()->unmarkObject(lastCompletionSel, 3);
    celApp->getSimulation()->
        getUniverse()->markObject(sel, markerRep, 3);
    lastCompletionSel = sel;
}

void CelBodyInteractive::renderCompletion(float height, float width)
{
    if (typedTextCompletion.empty())
        return;
    TextureFont *font = gc->getCompletionFont();
    float fh = font->getHeight();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel
    uint nb_cols = 4;
    scrollSize = nb_cols * nb_lines;
    std::string buftext(GCInteractive::getBufferText(), 0, bufSizeBeforeHystory);
    string::size_type pos = buftext.rfind('/', buftext.length());
    if (pos != string::npos)
        buftext = string(buftext, pos + 1);

    uint buf_length = buftext.size();
    vector<std::string>::const_iterator it = typedTextCompletion.begin();
    it += pageScrollIdx;
    for (uint i=0; it < typedTextCompletion.end() && i < nb_cols; i++)
    {
        glPushMatrix();
        gc->getOverlay()->beginText();
        for (uint j = 0; it < typedTextCompletion.end() && j < nb_lines; it++, j++)
        {
            std::string s = *it;
            glColor4ubv(clCompletionFnt->rgba);
            if (i * nb_lines + j == completedIdx - pageScrollIdx)
            {
                // border of current completopn
                glColor4ubv(clCompletionExpandBrd->rgba);
                gc->getOverlay()->rect(0.0f, 0.0f - 2,
                                       font->getWidth(s), font->getHeight(), false);
            }
            // completion item (only text before match char)
            glColor4ubv(clCompletionFnt->rgba);
            *gc->getOverlay() << std::string(s, 0, buf_length);
            // match char background
            if (s.size() >= buf_length)
            {
                if (clCompletionMatchCharBg->rgba[3] > 0)
                {
                    std::string text(s, buf_length, 1);
                    glColor4ubv(clCompletionMatchCharBg->rgba);
                    gc->getOverlay()->rect(font->getWidth(string(s, 0, buf_length)), 0.0f - 2,
                                           font->getWidth(text), font->getHeight());
                    glColor4ubv(clCompletionMatchCharFnt->rgba);
                    *gc->getOverlay() << text;
                }
                // rest text of item from match char
                glColor4ubv(clCompletionAfterMatch->rgba);
                if (s.size() > buf_length)
                    *gc->getOverlay() << string(s, buf_length + 1, 255);
                *gc->getOverlay() << "\n";
            }
        }
        gc->getOverlay()->endText();
        glPopMatrix();
        int shiftx = width/nb_cols;
        glTranslatef((float) (shiftx), 0.0f, 0.0f);
    }
}

void CelBodyInteractive::cancelInteractive()
{
    celApp->getSimulation()->
        getUniverse()->unmarkObject(lastCompletionSel, 3);
    lastCompletionSel = Selection();
}

ListInteractive *listInteractive;
PasswordInteractive *passwordInteractive;
CelBodyInteractive *celBodyInteractive;

/******************************************************************
 *  Functions
 ******************************************************************/

int GCFunc::call(GeekConsole *gc, int state, std::string value)
{
    if (!isLuaFunc)
        if (vFun)
        { vFun(); gc->finish(); return 1;}
        else
            return cFun(gc, state, value);
    else
    {
        lua_pcall       ( lua, 0, LUA_MULTRET, 0 );
        lua_getfield    ( lua, LUA_GLOBALSINDEX, luaFunName.c_str());   // push global function on stack
        lua_pushinteger ( lua, state );                     // push second argument on stack
        lua_pushstring  ( lua, value.c_str());              // push first argument on stack
        lua_pcall       ( lua, 2, 1, 0 );                   // call function taking 2 argsuments and getting one return value
        return lua_tointeger ( lua, -1 );
    }
}

static int execFunction(GeekConsole *gc, int state, std::string value)
{
    GCFunc *f;
    switch (state)
    {
    case 0:
        gc->setInteractive(listInteractive, "exec-function", "M-x", "Exec function");
        listInteractive->setCompletion(gc->getFunctionsNames());
        listInteractive->setMatchCompletion(true);
        break;
    case -1:
    {
        // describe key binds for this function
        std::vector<GeekBind *>* gbs = gc->getGeekBinds();
        std::string bindstr;
        for (std::vector<GeekBind *>::iterator it = gbs->begin();
             it != gbs->end(); it++)
        {
            GeekBind *gb = *it;
            std::string str = gb->getBinds(value);
            if (!str.empty())
                bindstr += ' ' + gb->getName() + ": " + str;
        }
        if (!bindstr.empty())
            gc->descriptionStr += " [Matched;" + bindstr + ']';
        break;
    }
    case 1:
        f = gc->getFunctionByName(value);
        if (f)
        {
            gc->execFunction(f);
            return 0;
        }
        break;
    }
    return state;
}

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

static int bindKey(GeekConsole *gc, int state, std::string value)
{
    static string bindspace;
    static string keybind;
    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Bind key: Select bindspace"));
        std::vector<GeekBind *> *gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::iterator it = gbs->begin();
             it != gbs->end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case 1:
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Key bind"), _("Choose key bind, and parameters. Example: C-c g @param 1@"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(false);
        }
        break;
    }
    case -1-1:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->descriptionStr = gb->getBindDescr(value);
        break;
    }
    case 2:
        keybind = value;
        gc->setInteractive(listInteractive, "exec-function", _("Function"), _("Select function to bind ") + keybind );
        listInteractive->setCompletion(gc->getFunctionsNames());
        listInteractive->setMatchCompletion(true);
        break;
        break;
    case 3:
        gc->bind(bindspace, keybind, value);
        gc->finish();
        break;
    default:
        break;
    }
    return state;
}

static int unBindKey(GeekConsole *gc, int state, std::string value)
{
    static string bindspace;

    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Unbind key: Select bindspace"));
        std::vector<GeekBind *> *gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::iterator it = gbs->begin();
             it != gbs->end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case 1:
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Un bind key"), _("Choose key bind"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(true);
        }
        break;
    }
    case -1-1:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->descriptionStr = gb->getBindDescr(value);
        break;
    }
    case 2:
        gc->unbind(bindspace, value);
        gc->finish();
        break;
    default:
        break;
    }
    return state;
}

static int describebindKey(GeekConsole *gc, int state, std::string value)
{
    static string bindspace;

    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Describe bind key: Select bindspace"));
        std::vector<GeekBind *> *gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::iterator it = gbs->begin();
             it != gbs->end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case 1:
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Key bind for describing"), _("Choose key bind"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(true);
        }
        break;
    }
    case -1-1:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->descriptionStr = gb->getBindDescr(value);
        break;
    }
    case 2:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
        {
            gc->getCelCore()->flash(gb->getBindDescr(value), 6);
            gc->finish();
        }
        break;
    }
    default:
        break;
    }
    return state;
}

static StarBrowser starBrowser;
static int selectStar(GeekConsole *gc, int state, std::string value)
{
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
        starBrowser.setSimulation(gc->getCelCore()->getSimulation());
        StarDatabase* stardb = celAppCore->getSimulation()->
            getUniverse()->getStarCatalog();
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
        celAppCore->getSimulation()->setSelection(Selection((Star *)(*stars)[0]));

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

static bool isInteractsInit = false;

void initGCInteractivesAndFunctions(GeekConsole *gc)
{
    if (!isInteractsInit)
    {
        listInteractive = new ListInteractive("list");
        passwordInteractive = new PasswordInteractive("passwd");
        celBodyInteractive = new CelBodyInteractive("cbody", gc->getCelCore());
        isInteractsInit = true;
    }
    gc->registerAndBind("", "M-x",
                        GCFunc(execFunction), "exec function");
    gc->registerAndBind("", "C-RET",
                        GCFunc(selectBody), "select object");
    gc->registerFunction(GCFunc(gotoBody), "goto object");
    gc->registerAndBind("", "C-c g",
                        GCFunc(gotoBodyGC), "goto object gc");
    gc->registerFunction(GCFunc(bindKey), "bind");
    gc->registerFunction(GCFunc(unBindKey), "unbind");
    gc->registerFunction(GCFunc(describebindKey), "describe key");
    gc->registerFunction(GCFunc(selectStar), "select star");
}

void destroyGCInteractivesAndFunctions()
{
    delete listInteractive;
    delete passwordInteractive;
    delete celBodyInteractive;
}

/******************************************************************
 *  Lua API for geek console
 ******************************************************************/

static int registerFunction(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gc.registerFunction(string, string)");
    const char *gcFunName = celx.safeGetString(1, AllErrors, "argument 1 to gc.reRegisterFunction must be a string");
    const char *luaFunName = celx.safeGetString(2, AllErrors, "argument 2 to gc.reRegisterFunction must be a string");
    geekConsole->registerFunction(GCFunc(luaFunName, l), gcFunName);
    return 0;
}

static int reRegisterFunction(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gc.reRegisterFunction(string, string)");
    const char *gcFunName = celx.safeGetString(1, AllErrors, "argument 1 to gc.reRegisterFunction must be a string");
    const char *luaFunName = celx.safeGetString(2, AllErrors, "argument 2 to gc.reRegisterFunction must be a string");
    geekConsole->reRegisterFunction(GCFunc(luaFunName, l), gcFunName);
    return 0;
}

static int setListInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(5, 5, "Two or three arguments expected for gc.listInteractive(sCompletion, shistory, sPrefix, sDescr, mustMatch)");
    const char *completion = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    const char *history = celx.safeGetString(2, AllErrors, "argument 2 to gc.listInteractive must be a string");
    const char *prefix = celx.safeGetString(3, AllErrors, "argument 3 to gc.listInteractive must be a string");
    const char *descr = celx.safeGetString(4, AllErrors, "argument 4 to gc.listInteractive must be a string");
    int isMustMatch = celx.safeGetNumber(5, WrongType, "argument 5 to gc.listInteractive must be a number", 0);
    geekConsole->setInteractive(listInteractive, history, prefix, descr);
    listInteractive->setCompletionFromSemicolonStr(completion);
    listInteractive->setMatchCompletion(isMustMatch);
    return 0;
}

static int setPasswdInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two or three arguments expected for gc.listInteractive(sPrefix, sDescr)");
    const char *prefix = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    const char *descr = celx.safeGetString(2, AllErrors, "argument 2 to gc.listInteractive must be a string");
    geekConsole->setInteractive(passwordInteractive, "", prefix, descr);
    return 0;
}

static int setCelBodyInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 3, "Two or three arguments expected for gc.celBodyInteractive(shistory, sPrefix, sDescr)");
    const char *history = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    const char *prefix = celx.safeGetString(2, AllErrors, "argument 2 to gc.listInteractive must be a string");
    const char *descr = celx.safeGetString(3, AllErrors, "argument 3 to gc.listInteractive must be a string");
    geekConsole->setInteractive(celBodyInteractive, history, prefix, descr);
    return 0;
}

static int finishGConsole(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 0, "No arguments expected for gc.finish");
    geekConsole->finish();
    return 0;
}


static int callFun(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.call(sFunName)");
    const char *funName = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    geekConsole->execFunction(funName);
    return 0;
}

void LoadLuaGeekConsoleLibrary(lua_State* l)
{
    CelxLua celx(l);

    lua_pushstring(l, "gc");
    lua_newtable(l);
    celx.registerMethod("registerFunction", registerFunction);
    celx.registerMethod("reRegisterFunction", reRegisterFunction);
    celx.registerMethod("listInteractive", setListInteractive);
    celx.registerMethod("passwdInteractive", setPasswdInteractive);
    celx.registerMethod("celBodyInteractive", setCelBodyInteractive);
    celx.registerMethod("finish", finishGConsole);
    celx.registerMethod("call", callFun);
    lua_settable(l, LUA_GLOBALSINDEX);
}
