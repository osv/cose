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

// This file contain base functions for geekconsole.

#include "geekconsole.h"
#include "infointer.h"
#include "gvar.h"

/******************************************************************
 *  Functions
 ******************************************************************/

static int execFunction(GeekConsole *gc, int state, std::string value)
{
    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "exec-function", "M-x", "Exec function");
        std::vector<std::string> v = gc->getFunctionsNames();
        std::sort(v.begin(), v.end());
        listInteractive->setCompletion(v);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case -1:
    {
        // describe key binds for this function
        GCFunc *f = gc->getFunctionByName(value);
        if (!f)
            break;
        gc->setInfoText(f->getInfo());
        const std::vector<GeekBind *>& gbs = gc->getGeekBinds();
        std::string bindstr;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
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
        return gc->execFunction(value);
    }
    return state;
}

static void startMacro()
{
    getGeekConsole()->setMacroRecord(true);
}

static void endMacro()
{
    getGeekConsole()->setMacroRecord(false);
}

static void endAndCallMacro()
{
    getGeekConsole()->callMacro();
}

static int saveMacro(GeekConsole *gc, int state, std::string value)
{
    static string svmFunName; // save fun name here
    static string bindspace; // save name of bind space here

    gc->setMacroRecord(false, true);

    if (state == 0)
    {
        gc->setInteractive(listInteractive, "exec-function", _("Alias name:"), _("Set name for alias"));
        gc->setInfoText("Current macro:\n" + gc->getCurrentMacro());
        listInteractive->setCompletion(gc->getFunctionsNames());
    } else if (state == 1)
    {
        svmFunName = value;
        if (gc->getFunctionByName(value))
        {
            gc->setInteractive(listInteractive, "yes-no", _("Are you sure to overwrite?"), "\"" + value + "\" " + _("already defined."));
            listInteractive->setCompletionFromSemicolonStr("yes;no");
            listInteractive->setMatchCompletion(true);
            gc->setInfoText(string(_("Info about")) + " \"" + value + "\":\n" + gc->getFunctionByName(value)->getInfo());
        }
        else // register function
            state = 3; // skip next state
    }

    if (state == 2)
    {
        if (value != "yes")
        {
            gc->setInteractive(listInteractive, "exec-function", _("Set other alias name:"),
                               string(_("Set name for alias, last was")) + "\"" + svmFunName + "\"");
            gc->setInfoText(string(_("Current macro")) + ":\n" + gc->getCurrentMacro());
            listInteractive->setCompletion(gc->getFunctionsNames());
            return 0; // recall with 0 state
        }
        state = 3;
    }

    if (state == 3) // register function and ask for bind space
    {
        // create alias for archive
        gc->reRegisterFunction(GCFunc("exec function", gc->getCurrentMacro().c_str(), "", true), svmFunName);
        // TODO: maybe good to save macro in cfg file for load it at startup
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Bind key: Select bindspace"));
        gc->setInfoText(svmFunName + "\n" +
                        _("You can continue and assign key or break by ESC or C-g"));
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
    }

    if (state == 4 ) // ask for key
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Key bind"), _("Choose key bind, and parameters. Example: C-c g #param 1#"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(false);
        }
        return 4;
    } else if (state == -1 -4) // describe current bind
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->setInfoText(gb->getBindDescr(value));
    } else if (state == 5) // finish
    {
        // bind key (archive = true)
        gc->bind(bindspace, value, svmFunName, true);
    }

    return state;
}

static int saveMacrosBind(GeekConsole *gc, int state, std::string value)
{

    static string svmFunName; // save fun name here
    static string bindspace; // save name of bind space here

    gc->setMacroRecord(false, true);

    if (state == 0)
    {
        // TODO: maybe good to save macro in cfg file for load it at startup
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Bind key: Select bindspace"));
        gc->setInfoText("Current macro:\n" + gc->getCurrentMacro());
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
    }

    if (state == 1 ) // ask for key
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Key bind"), _("Choose key bind. Example: C-x c"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(false);
        }
        return 1;
    }
    else if (state == -1 -1) // describe current bind
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->setInfoText(gb->getBindDescr(value));
    }
    else if (state == 2) // finish
    {
        // bind key (archive = true)
        gc->bind(bindspace, value + " " + gc->getCurrentMacro(), "", true);
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
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
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
        gc->setInteractive(listInteractive, "bindkey-key", _("Key bind"), _("Choose key bind, and parameters. Example: C-c g #param 1#"));
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
            gc->setInfoText(gb->getBindDescr(value));
        break;
    }
    case 2:
        keybind = value;
        gc->setInteractive(listInteractive, "exec-function", _("Function"), _("Select function to bind ") + keybind );
        listInteractive->setCompletion(gc->getFunctionsNames());
        listInteractive->setMatchCompletion(true);
        break;
    case 3:
        gc->bind(bindspace, keybind, value, true);
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
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
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
            gc->setInfoText(gb->getBindDescr(value));
        break;
    }
    case 2:
        gc->unbind(bindspace, value);
        break;
    default:
        break;
    }
    return state;
}

static int describeBindings(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
    {
        gc->setInteractive(pagerInteractive);
        std::stringstream ss;
        ss << _("Key translations:\n");

        std::vector<GeekBind::KeyBind>::iterator it;

        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<GeekBind *>::const_iterator gb;
        std::vector<std::string> temp;
        for (gb = gbs.begin();
             gb != gbs.end(); gb++)
        {
            ss << "---\n"
               << _("Bind space ") << "\"" <<
                (*gb)->getName() << "\" (";
            if ((*gb)->isActive)
                ss << _("Active");
            else
                ss << _("Inactive");
            ss << "):\n";
            ss << "---\n";

            std::vector<GeekBind::KeyBind> binds = (*gb)->getBinds();
            for (it = binds.begin();
                 it != binds.end(); it++)
            {
                ss << pagerInteractive->makeSpace(
                    pagerInteractive->makeSpace((*it).keyToStr(), 12,
                                                (*it).gcFunName),
                    34, (*it).params) << "\n";
            }
            ss << "\n";
        }
        pagerInteractive->setText(ss.str());
    }
    return state;
}

/// return info about function, his binds
static std::string _describeFunction(GeekConsole *gc, std::string function)
{
    std::stringstream ss;

    // all binds of function
    const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
    std::vector<string> completion;

    GCFunc *f = gc->getFunctionByName(function);
    ss << _("  Function") << " `" << function << "'\n\n";
    if (f)
    {
        std::string s = f->getInfo();
        if (s != "")
            ss << f->getInfo() << "\n";
        if (f->getType() == GCFunc::Alias) {
            ss << "---\n"
               << _("It is alias to") << " \"" << f->getAliasFun() << "\"\n";
            if (f->getAliasParams() != "")
                ss << _("with params \"") << f->getAliasParams() << "\"\n";

        }
    }

    int bindx = 0;
    for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
         it != gbs.end(); it++)
    {
        GeekBind *b = *it;
        std::vector<GeekBind::KeyBind> binds= b->getAllBinds(function);
        std::vector<GeekBind::KeyBind>::iterator bit;
        if (!binds.empty())
        {
            if (bindx == 0)
                ss << _("It is bound to:\n");
            bindx ++;
            ss << _("Bind space") << " \"" << b->getName() << "\": "
               << b->getBinds(function) << "\n";

            for (bit = binds.begin(); bit < binds.end(); bit++)
            {
                ss << pagerInteractive->makeSpace(
                    pagerInteractive->makeSpace((*bit).keyToStr(), 12,
                                                (*bit).gcFunName),
                    34, (*bit).params) << "\n";
            }
        }
    }
    ss << "\n";

    return ss.str();
}

static int describeFunction(GeekConsole *gc, int state, std::string value)
{
    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "exec-function", _("Function"));
        std::vector<std::string> v = gc->getFunctionsNames();
        std::sort(v.begin(), v.end());
        listInteractive->setCompletion(v);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case -1:
    {
        // describe key binds for this function
        GCFunc *f = gc->getFunctionByName(value);
        if (!f)
            break;
        gc->setInfoText(f->getInfo());
        const std::vector<GeekBind *>& gbs = gc->getGeekBinds();
        std::string bindstr;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
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
    {
        gc->setInteractive(pagerInteractive);
        pagerInteractive->setText(
            _describeFunction(gc, value));
    }
    }
    return state;
}


static int describeBindKey(GeekConsole *gc, int state, std::string value)
{
    static string bindspace;

    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Describe bind key: Select bindspace"));
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
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
            gc->setInfoText(gb->getBindDescr(value));
        break;
    }
    case 2:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
        {
            std::stringstream ss;
            gc->setInteractive(pagerInteractive, "", _("Describe bind key"));
            ss << value << _(" runs the command \"");
            ss << gb->getFunName(value) << "\"\n";

            if (gb->getParams(value) != "")
                ss << _("with params \"") << gb->getParams(value)
                   << "\"\n";

            std::string function = gb->getFunName(value);
            if (function.empty())
                function = gb->getFirstParam(value);
            ss << "\n" << _describeFunction(gc, function);

            pagerInteractive->setText(ss.str());
        }
        break;
    }
    default:
        break;
    }
    return state;
}

static int info(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
    {
        gc->setInteractive(infoInteractive, "Info");
        infoInteractive->setNode("Dir");
    }
    return state;
}

static int info_last_node(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
    {
        gc->setInteractive(infoInteractive, "Info");
    }
    return state;
}

static void info_rebuild_dir()
{
    infoInteractive->rebuildDirCache();
}

void initGCInteractives(GeekConsole *gc)
{
    gc->registerAndBind("", "M-x",
                        GCFunc(execFunction), "exec function");
    gc->registerFunction(GCFunc(bindKey), "bind");
    gc->registerFunction(GCFunc(unBindKey), "unbind");
    gc->registerAndBind("", "C-h b",
                        GCFunc(describeBindings, _("Display all key bindings")),
                        "describe bindings");
    gc->registerAndBind("", "C-h f",
                        GCFunc(describeFunction, _("Display documentation for the given function.")),
                        "describe function");
    gc->registerAndBind("", "C-h k",
                        GCFunc(describeBindKey, _("Display the documentation for the key sequence.")),
                        "describe key");

    gc->registerAndBind("", "C-x C-k s",
                        GCFunc(startMacro, _("Define macro.\n"
                                             "Record subsequent keyboard input (only for geekconsole binds),\n"
                                             "record interactive action of geekconsole.\n"
                                             "To end record use \"macro end\",\n"
                                             "to call macro use \"macro end and call\"")),
                        "macro start");
    gc->registerAndBind("", "C-x C-k e",
                        GCFunc(endMacro, _("End macro.\n"
                                             "To call macro use \"macro end and call\"\n"
                                             "To save macro as alias and define key use \"macro save\"")),
                        "macro end");
    gc->registerAndBind("", "C-x C-k n",
                        GCFunc(saveMacro, _("Assign name to last macro.\n"
                                            "Also bind you can bind key")),
                        "macro save");
    gc->registerAndBind("", "C-x C-k S-n",
                        GCFunc(saveMacrosBind, _("Bind key for current macro.\n"
                                                 "If you wish register name for this macro\n"
                                                 "use \"macro save\" instead.")),
                        "macro save as bind");
    gc->registerAndBind("", "C-x e",
                        GCFunc(endAndCallMacro, _("Call last macro, "
                                                  "ending it first if currently being defined.\n"
                                                  "Also bind to key")),
                        "macro end and call");
    gc->registerFunction(GCFunc(info, _("Read Info documents")), "info");
    gc->registerFunction(GCFunc(info_last_node), "info, last visited node");
    gc->registerFunction(GCFunc(info_rebuild_dir, _("Rebuild directory node - the top of the INFO tree")),
                                       "info, rebuild dir node");

    // bind colors
    typedef struct gconsole_color_vars_t {
        const char *name;
        const char *descr;
        Color32 *c;
    };

    gconsole_color_vars_t gconsole_color_vars[] = {
        {"Bg", "Background color of geek console.", clBackground},
        {"Bg Interactive", "Background color for entry field.", clBgInteractive},
        {"Bg Interactive Brd", "Border color for entry field.", clBgInteractiveBrd},
        {"Interactive Fnt", "Font color for entry.", clInteractiveFnt},
        {"Interactive Prefix Fnt", "Font color for prefix of entry field.", clInteractivePrefixFnt},
        {"Interactive Expand", "Font color fot expanded part of entry field.", clInteractiveExpand},
        {"Description", "Font color of status (bottom) filed.", clDescrFnt},
        {"Completion", "Completion text color.", clCompletionFnt},
        {"Completion Match", "Completion font color for matched text.", clCompletionMatchCharFnt},
        {"Completion Match Bg", "Completion background color for matched text.", clCompletionMatchCharBg},
        {"Completion After Match", "Completion font color for text after matched text.", clCompletionAfterMatch},
        {"Completion Expand Bg", "Background color for selected item.", clCompletionExpandBg},
        {"Infotext Fnt", "Font color of help tip box.", clInfoTextFnt},
        {"Infotext Bg", "Background color of help tip box.", clInfoTextBg},
        {"Infotext Brd", "Border color of help tip box.", clInfoTextBrd},
        {NULL}
    };

    int i = 0;
    std::string cname("render/color/gconsole/");
    while (gconsole_color_vars[i].name) {
        // bind cfg var
        gVar.BindColor(cname + gconsole_color_vars[i].name,
                       &gconsole_color_vars[i].c->i,
                       gconsole_color_vars[i].c->i,
                       gconsole_color_vars[i].descr);
        i++;
    }

}