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

// help func for info node creation, return hyperlink of gc funcs
static string makeHyperFcFunc(string text);

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
    ss << "  Function" << " `" << function << "'\n";
    if (f)
    {
        std::string s = f->getInfo();
        if (s != "")
            ss << "  " << f->getInfo() << "\n";
        if (f->getType() == GCFunc::Alias) {
            ss << "  It is alias to" << " \"" << makeHyperFcFunc(normalLuaStr(f->getAliasFun())) << "\"";
            if (f->getAliasParams() != "")
                ss << " with params \"" << makeHyperFcFunc(normalLuaStr(f->getAliasParams())) << "\"\n";
            else
                ss << '\n';
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
                ss << "  It is bound to:\n";
            bindx ++;
            ss << "Bind space" << " "
                // make url to list of keys in bind space
               << INFO_TAGSTART "node n=\"" + b->getName() + ": (*binds*)Keys " + b->getName() + "\"" INFO_TAGEND << ": "
               << b->getBinds(function) << "\n";

            for (bit = binds.begin(); bit < binds.end(); bit++)
            {
                string res;
                res += (*bit).keyToStr();
                res += INFO_TAGSTART "tab n=\"12\"" INFO_TAGEND;
                res += " " + makeHyperFcFunc(normalLuaStr((*bit).gcFunName));
                if (!(*bit).params.empty())
                {
                    res += INFO_TAGSTART "tab n=\"34\"" INFO_TAGEND;
                    res += makeHyperFcFunc(normalLuaStr((*bit).params));
                }
                ss << res << "\n";
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


static int describeBindKeyExt(GeekConsole *gc, int state, std::string value)
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
            gc->setInteractive(infoInteractive);
            infoInteractive->setNode("*binds*", "Key " + bindspace + "/" + value);
        }
        break;
    }
    default:
        break;
    }
    return state;
}

static string makeHyperFcFunc(string text)
{
    string res;
    vector<string> vstr = splitString(text, "#");
    vector<string>::iterator it;
    GeekConsole *gc = getGeekConsole();
    for (it = vstr.begin();
         it != vstr.end(); it++) {
        if (gc->getFunctionByName(*it))
        {
            if (!res.empty())
                res.push_back('#');
            res += INFO_TAGSTART "node n=\"" + *it + ": (*gcfun*)Function " + *it + "\"" INFO_TAGEND;
        }
        else
        {
            if (!res.empty())
                res.push_back('#');
            res += *it;
        }
    }
    return res;
}

// help fun - get info about key
static string _describeKey(string bindspace, string key)
{
    std::stringstream ss;
    GeekConsole *gc = getGeekConsole();
    GeekBind *gb = gc->getGeekBind(bindspace);
    if (gb && (gb->isBinded(key) == GeekBind::BINDED))
    {
        ss << "In Bind space `" + bindspace + "' ";
        if ((gb)->isActive)
            ss << " (active) ";

        ss << key << " runs the command \"";
        ss << gb->getFunName(key) << "\"\n";

        if (gb->getParams(key) != "")
            ss << "with params \"" << gb->getParams(key)
               << "\"\n";

        std::string function = gb->getFunName(key);
        if (function.empty())
            function = gb->getFirstParam(key);
        ss << "\n" << _describeFunction(gc, function);
    }
    else
    {
        ss << "Key `" << key << "' is not defined in `"
           << bindspace << "' bind space. *Note Keys " << bindspace << "::.";
    }
    return ss.str();
}

/* dynamic info node for gc function */
string funcInfoDynNode(string filename, string nodename)
{
    string res = "File: *gcfun*,\tNode: " + nodename + ", ";

    GeekConsole *gc = getGeekConsole();
    const char *func_node = "Function ";

    if ("Top" == nodename)
    {
        res += "Up: (dir) \n\n"
            "* Menu:\n\n"
            "* All: Function *all*. List of all functions of geekconsole\n";
    }
    else if (nodename.find(func_node) == 0)
    {
        string funcname = nodename.substr(strlen(func_node));
        res += "Up: Top\n\n";
        if (funcname == "*all*")
        {
            res += "* Menu:\n\n";
            vector<string> funcs = gc->getFunctionsNames();
            vector<string> funinfo;
            std::sort(funcs.begin(), funcs.end());
            for (vector<string>::const_iterator it = funcs.begin();
                 it != funcs.end(); it++)
            {
                GCFunc *f = gc->getFunctionByName(*it);
                if (!f)
                    continue;

                res.append("* " + *it + ": Function " + *it + ".");
                funinfo = splitString(f->getInfo(), "\n");
                // add only first 5 lines
                if (funinfo.size() > 0)
                {
                    res.append(" ");
                    vector<string>::const_iterator l = funinfo.begin();
                    int n = 0;
                    for (; l != funinfo.end() && n < 5;
                     l++, n++)
                        res.append(*l + "\n");
                }
                else
                    res.push_back('\n');
            }
        }
        else
            res += _describeFunction(gc, funcname);
    }

    return res;
}

/* dynamic info node for key binds */
string bindsInfoDynNode(string filename, string nodename)
{
    string res = "File: *binds*,\tNode: " + nodename + ", ";

    GeekConsole *gc = getGeekConsole();
    const char *keys_node = "Keys ";
    const char *key_node = "Key ";

    if ("Top" == nodename)
    {
        res += "Up: (dir) \n\n"
            "* Menu:\n\n"
            "Describe keys in bind spaces:\n"
            "* All: Keys *all*.\n";
        const std::vector<GeekBind *>& gbs = gc->getGeekBinds();
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
        {
            res += "* " + (*it)->getName() + ": Keys " + (*it)->getName() + '.';
            if ((*it)->isActive)
                res += " Bindspace is active";
            res += '\n';
        }
        res += "\n"
            "   You can request full describe for key sequence by using\n"
            "node `Key key_name' or `Key bindspace/key_name' for example *Note Key M-x::.";
        return res;
    }
    else if (nodename.find(key_node) == 0)
    {
        res += "Up: Top\n\n";
        string bindspace;
        string key = nodename.substr(strlen(key_node));
        GeekBind::KeyBind kb;
        kb.set(key.c_str());
        key = kb.keyToStr();
        size_t found = key.find('/');
        if (found != string::npos)
        {
            bindspace = key.substr(0, found);
            key = key.substr(found +1);
        }
        if (bindspace.empty())
        {
            res += "Describe key `" + key + "' in all bind spaces.\n***\n";
            const std::vector<GeekBind *>& gbs = gc->getGeekBinds();
            for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
                 it != gbs.end(); it++)
                if ((*it)->isBinded(key) == GeekBind::BINDED)
                    res += _describeKey((*it)->getName(), key);
        }
        else
        {
            res += "Describe key " + key + " in `"+ bindspace + "' bind space.\n***\n";
            res += _describeKey(bindspace, key);
        }
    }
    else if (nodename.find(keys_node) == 0)
    {
        string bindspace = nodename.substr(strlen(keys_node));
        res += "Up: Top\n\n"
            "Describe keys in " + bindspace + " bind space(s).\n***\n";
        std::vector<GeekBind::KeyBind>::const_iterator it;
        std::vector<GeekBind *> gbs = gc->getGeekBinds();
        std::vector<GeekBind *>::iterator gb;
        for (gb  = gbs.begin();
             gb != gbs.end(); gb++)
        {
            if (bindspace == (*gb)->getName() || "*all*" == bindspace)
            {
                res += '\n' + (*gb)->getName();
                if ((*gb)->isActive)
                    res += " (Active)";
                res += "\n===\n";

                std::vector<GeekBind::KeyBind> binds = (*gb)->getBinds();
                for (it = binds.begin();
                     it != binds.end(); it++)
                {
                    res += it->keyToStr();
                    res += INFO_TAGSTART "tab n=\"12\"" INFO_TAGEND;
                    res += " " + makeHyperFcFunc(it->gcFunName);
                    if (!it->params.empty())
                    {
                        res += INFO_TAGSTART "tab n=\"34\"" INFO_TAGEND;
                        res += makeHyperFcFunc(it->params);
                    }
                    res += "\n";
                }
            }
        }
    }
    return res;
}

static int describeBindKey(GeekConsole *gc, int state, std::string value)
{
    if (state == 0)
    {
        gc->setInteractive(keyInteractive, "key-test");
        keyInteractive->setKeyMatching(KeyInteractive::ACTVE_KEYS);
    }
    else if (state == 1)
    {
        gc->setInteractive(infoInteractive);
        infoInteractive->setNode("*binds*", "Key " + value);
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
    gc->registerAndBind("", "C-h S-k",
                        GCFunc(describeBindKeyExt, _("Display the documentation for the key sequence"
                                                  "\nby selecting bindspace.")),
                        "describe key Ext");

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
                                            "Also you can bind key")),
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

    char dir[] = "INFO-DIR-SECTION GeekConsole\n"
        "START-INFO-DIR-ENTRY\n"
        "* Binds: (*binds*)Top.   Key bind configuration.\n"
        "* Function: (*gcfun*)Top.  GeekConsole's function.\n"
        "END-INFO-DIR-ENTRY";
    infoInteractive->addDirTxt(dir, sizeof(dir), true);
    infoInteractive->registerDynamicNode("*binds*", bindsInfoDynNode);
    infoInteractive->registerDynamicNode("*gcfun*", funcInfoDynNode);

}
