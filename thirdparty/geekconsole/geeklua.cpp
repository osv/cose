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
#include "infointer.h"
#include "gvar.h"

/******************************************************************
 *  Lua API for geek console
 ******************************************************************/
static inline std::string sstr(const char *str)
{
    if (str)
        return string(str);
    else
        return string();
}

static void registerLuaFunction(lua_State* l, bool reregister)
{
    const char *errMsg = "Two or 3 arguments expected for "
        "gc.re(Re)gisterFunction(string, string, [string])";
    CelxLua celx(l);
    celx.checkArgs(2, 3, errMsg);
    const char *gcFunName = celx.safeGetString(1, AllErrors, "argument 1 to gc.re(Re)gisterFunction must be a string");
    int callback;
    const char *luaInfo = 0;
    if (l) {
        int argc = lua_gettop(l);
        if (argc >= 2 && argc <= 3)
        {
            if (argc == 3)
                luaInfo = celx.safeGetString(2, WrongType, "argument 2 to gc.re(Re)gisterFunction/3 must be a string or function");
            if (lua_isfunction (l, argc)) // last arg may be function or string
            {
                callback = luaL_ref(l, LUA_REGISTRYINDEX);
                if (!reregister)
                    if (luaInfo)
                        getGeekConsole()->registerFunction(GCFunc(callback, l, luaInfo), gcFunName);
                    else
                        getGeekConsole()->registerFunction(GCFunc(callback, l), gcFunName);
                else
                    if (luaInfo)
                        getGeekConsole()->reRegisterFunction(GCFunc(callback, l, luaInfo), gcFunName);
                    else
                        getGeekConsole()->reRegisterFunction(GCFunc(callback, l), gcFunName);
            } else  {
                const char *luaFunName = celx.safeGetString(argc, AllErrors, "last argument to gc.re(Re)gisterFunction must be a string or function");
                if (!reregister)
                    if (luaInfo)
                        getGeekConsole()->registerFunction(GCFunc(luaFunName, l, luaInfo), gcFunName);
                    else
                        getGeekConsole()->registerFunction(GCFunc(luaFunName, l), gcFunName);
                else
                    if (luaInfo)
                        getGeekConsole()->reRegisterFunction(GCFunc(luaFunName, l, luaInfo), gcFunName);
                    else
                        getGeekConsole()->reRegisterFunction(GCFunc(luaFunName, l), gcFunName);
            }
        }
    }
    return;
}

static int registerFunction(lua_State* l)
{
    registerLuaFunction(l, false);
    return 0;
}

static int reRegisterFunction(lua_State* l)
{
    registerLuaFunction(l, true);
    return 0;
}

static int registerAlias(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 4, "Three arguments expected for gc.registerAlias(string, string, string, [string])");
    const char *gcFunName = celx.safeGetString(1, AllErrors, "argument 1 to gc.registerAlias must be a string");
    const char *aliasName = celx.safeGetString(2, AllErrors, "argument 2 to gc.registerAlias must be a string");
    const char *args = celx.safeGetString(3, AllErrors, "argument 3 to gc.registerAlias must be a string");
    const char *doc = celx.safeGetString(4, WrongType, "argument 4 to gc.registerAlias must be a string");
    if (doc)
        getGeekConsole()->reRegisterFunction(GCFunc(aliasName, args, doc), gcFunName);
    else
        getGeekConsole()->reRegisterFunction(GCFunc(aliasName, args), gcFunName);
    return 0;
}

// create alias (archive)
static int registerAliasA(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 4, "Three arguments expected for gc.registerAliasA(string, string, string, [string])");
    const char *gcFunName = celx.safeGetString(1, AllErrors, "argument 1 to gc.registerAliasA must be a string");
    const char *aliasName = celx.safeGetString(2, AllErrors, "argument 2 to gc.registerAliasA must be a string");
    const char *args = celx.safeGetString(3, AllErrors, "argument 3 to gc.registerAliasA must be a string");
    const char *doc = celx.safeGetString(4, WrongType, "argument 4 to gc.registerAliasA must be a string");
    if (doc)
        getGeekConsole()->reRegisterFunction(GCFunc(aliasName, args, doc, true), gcFunName);
    else
        getGeekConsole()->reRegisterFunction(GCFunc(aliasName, args, "", true), gcFunName);
    return 0;
}

// try to return in @completionlist array of strings
static bool safeGetStrArray(lua_State* l, int index, vector<string> *completionlist)
{
    int argc = lua_gettop(l);
    if (index < 1 || index > argc)
        return false;
    if (lua_istable(l, index))
    {
        int n;
        n = lua_objlen(l, index);
        int i;
        for(i = 0; i < n; i++) {
            lua_rawgeti(l, index, i+1);

            if (lua_isstring(l, -1))
            {
                const char *v = lua_tostring(l, -1);
                completionlist->push_back(sstr(v));
            }

            lua_remove(l, -1);
        }
        return true;
    }
    else
        return false;
}

static int setListInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(4, 5, "Four or three arguments expected for gc.listInteractive(sCompletion, shistory, sPrefix, sDescr, [mustMatch])");
    const char *history = celx.safeGetString(2, AllErrors, "argument 2 to gc.listInteractive must be a string");
    const char *prefix = celx.safeGetString(3, AllErrors, "argument 3 to gc.listInteractive must be a string");
    const char *descr = celx.safeGetString(4, AllErrors, "argument 4 to gc.listInteractive must be a string");
    bool isMustMatch = celx.safeGetBoolean(5, WrongType, "argument 5 to gc.listInteractive must be a number", false);
    vector<string> completionlist;

    // first  param must be array or string
    if (safeGetStrArray(l, 1, &completionlist))
    {
        getGeekConsole()->setInteractive(listInteractive, history, prefix, descr);
        listInteractive->setCompletion(completionlist);
    }
    else
    {
        const char *completion = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string array or string");
        getGeekConsole()->setInteractive(listInteractive, history, prefix, descr);
        if (completion)
            listInteractive->setCompletionFromSemicolonStr(completion);
    }
    listInteractive->setMatchCompletion(isMustMatch);
    return 0;
}

static int setPasswdInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gc.passwdInteractive(sPrefix, sDescr)");
    const char *prefix = celx.safeGetString(1, AllErrors, "argument 1 to gc.passwdInteractive must be a string");
    const char *descr = celx.safeGetString(2, AllErrors, "argument 2 to gc.passwdInteractive must be a string");
    getGeekConsole()->setInteractive(passwordInteractive, "", prefix, descr);
    return 0;
}

static int setCelBodyInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 4, "Three or four arguments expected for gc.celBodyInteractive(shistory, sPrefix, sDescr, [sCompetion]");
    vector<string> completionlist;
    const char *history = celx.safeGetString(1, AllErrors, "argument 1 to gc.celBodyInteractive must be a string");
    const char *prefix = celx.safeGetString(2, AllErrors, "argument 2 to gc.celBodyInteractive must be a string");
    const char *descr = celx.safeGetString(3, AllErrors, "argument 3 to gc.celBodyInteractive must be a string");
    // last 4-st param must be array or string
    if (safeGetStrArray(l, 4, &completionlist))
        getGeekConsole()->setInteractive(celBodyInteractive, history, prefix, descr);
        if (!completionlist.empty())
            celBodyInteractive->setCompletion(completionlist);
    else
    {
        const char *completion = celx.safeGetString(4, WrongType, "argument 4 to gc.celBodyInteractive must be a string array or string");
        getGeekConsole()->setInteractive(celBodyInteractive, history, prefix, descr);
        if (completion)
            celBodyInteractive->setCompletionFromSemicolonStr(completion);
    }
    return 0;
}

static int setColorChooseInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 3, "Three arguments expected for gc.colorChooseInteractive(shistory, sPrefix, sDescr)");
    const char *history = celx.safeGetString(1, AllErrors, "argument 1 to gc.colorChooseInteractive must be a string");
    const char *prefix = celx.safeGetString(2, AllErrors, "argument 2 to gc.colorChooseInteractive must be a string");
    const char *descr = celx.safeGetString(3, AllErrors, "argument 3 to gc.colorChooseInteractive must be a string");
    getGeekConsole()->setInteractive(colorChooserInteractive, history, prefix, descr);
    return 0;
}

static int setFlagInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(4, 6, "Two or three arguments expected for gc.flagInteractive(sCompletion, shistory, sPrefix, sDescr, [mustMatch], [sDelim])");
    const char *history = celx.safeGetString(2, AllErrors, "argument 2 to gc.flagInteractive must be a string");
    const char *prefix = celx.safeGetString(3, AllErrors, "argument 3 to gc.flagInteractive must be a string");
    const char *descr = celx.safeGetString(4, AllErrors, "argument 4 to gc.flagInteractive must be a string");
    bool isMustMatch = celx.safeGetBoolean(5, WrongType, "argument 5 to gc.flagInteractive must be a boolean", true);
    const char *delim = celx.safeGetString(6, WrongType, "argument 6 to gc.flagInteractive must be a string");

    // last 4-st param must be array or string
    vector<string> completionlist;
    if (safeGetStrArray(l, 1, &completionlist))
        getGeekConsole()->setInteractive(flagInteractive, history, prefix, descr);
        if (!completionlist.empty())
            flagInteractive->setCompletion(completionlist);
    else
    {
        const char *completion = celx.safeGetString(1, WrongType, "argument 1 to gc.flagInteractive must be a string array or string");
        getGeekConsole()->setInteractive(flagInteractive, history, prefix, descr);
        if (completion)
            flagInteractive->setCompletionFromSemicolonStr(completion);
    }

    flagInteractive->setMatchCompletion(isMustMatch);
    if (delim)
        flagInteractive->setSeparatorChars(delim);
    return 0;
}

static int setFileInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 7, "Three or seven arguments expected for gc.flagInteractive("
                   "shistory, sPrefix, sDescr, [mustMatch], [sDir], [sRootDir], [sFileExt])");
    const char *history = celx.safeGetString(1, AllErrors, "argument 1 to gc.flagInteractive must be a string");
    const char *prefix = celx.safeGetString(2, AllErrors, "argument 2 to gc.flagInteractive must be a string");
    const char *descr = celx.safeGetString(3, AllErrors, "argument 3 to gc.flagInteractive must be a string");
    bool isMustMatch = celx.safeGetBoolean(4, WrongType, "argument 4 to gc.flagInteractive must be a number", false);
    const char *dir = celx.safeGetString(5, WrongType, "argument 5 to gc.flagInteractive must be a string");
    const char *rootDir = celx.safeGetString(6, WrongType, "argument 6 to gc.flagInteractive must be a string");
    const char *fileExt = celx.safeGetString(7, WrongType, "argument 7 to gc.flagInteractive must be a string");
    getGeekConsole()->setInteractive(fileInteractive, history, prefix, descr);
    fileInteractive->setMatchCompletion(isMustMatch);
    if (dir)
        if (rootDir)
            fileInteractive->setDir(dir, rootDir);
        else
            fileInteractive->setDir(dir);
    if (fileExt)
        fileInteractive->setFileExtenstion(fileExt);
    return 0;
}

static int setInfoInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 2, "One or three arguments expected for gc.infoInteractive([sFile], [sNode])");
    const char *file = celx.safeGetString(1, WrongType, "argument 1 to gc.infoInteractive must be a string");
    const char *node = celx.safeGetString(2, WrongType, "argument 2 to gc.infoInteractive must be a string");
    getGeekConsole()->setInteractive(infoInteractive);
    infoInteractive->setNode(sstr(file), sstr(node));
    return 0;
}

/*TODO: get sized str not NULL terminater 'cause node may content ^@^H[ tags. (^@ - \x000)*/
static int showHelpNode(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.showHelpNode(sNodeContent)");
    const char *node_content = celx.safeGetString(1, AllErrors, "argument 1 to gc.showHelpNode must be a string");
    showHelpNode(sstr(node_content));
    return 1;
}

// convert geekconsole's color to #RRGGBB
static int color2HexRGB(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.color2Hext(sColorName)");
    const char *colorname = celx.safeGetString(1, AllErrors, "argument 1 to gc.color2Hex must be a string");
    Color32 c = getColor32FromText(colorname);
    char buf[32];
    sprintf (buf, "#%02X%02X%02X", c.rgba[0], c.rgba[1], c.rgba[2]);
    lua_pushstring(l, buf);
    return 1;
}

static int alphaFromColor(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.alphaFromColor(sColorName)");
    const char *colorname = celx.safeGetString(1, AllErrors, "argument 1 to gc.alphaFromColor must be a string");
    Color c = getColorFromText(colorname);
    lua_pushnumber(l, c.alpha());
    return 1;
}

static int isFunctionRegistered(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.isFunction(sName)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gc.isFunction must be a string");
    if (getGeekConsole()->getFunctionByName(name))
        lua_pushnumber(l, 1);
    else
        lua_pushnumber(l, 0);
    return 1;
}

static int setInfoText(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.setInfoText(sText)");
    const char *text = celx.safeGetString(1, AllErrors, "argument 1 to gc.setInfoText must be a string");
    getGeekConsole()->setInfoText(text);
    return 0;
}

static int finishGConsole(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 0, "No arguments expected for gc.finish");
    getGeekConsole()->finish();
    return 0;
}


static int callFun(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "One or two arguments expected for gc.call(sFunName, [sArgs])");
    const char *funName = celx.safeGetString(1, AllErrors, "argument 1 to gc.call must be a string");
    const char *args = celx.safeGetString(2, WrongType, "argument 2 to gc.call must be a string");

    if (!getGeekConsole()->getFunctionByName(funName))
    {
        getGeekConsole()->showText(string("function `") + funName + "` is undefined");
        return 0;
    }

    // append macro
    getGeekConsole()->appendCurrentMacro(string("#*EXEC*#") + funName);
    if (args)
        getGeekConsole()->appendCurrentMacro(string("#") + args);

    if (args)
        getGeekConsole()->execFunction(funName, args);
    else
        getGeekConsole()->execFunction(funName, "");
    return 0;
}

static int setColumns(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.setColumns(nCols)");
    int num = celx.safeGetNumber(1, AllErrors, "arguments 1 to gc.setColumns must be a number");
    if (getGeekConsole()->getCurrentInteractive() == listInteractive)
        listInteractive->setColumns(num);
    else if (getGeekConsole()->getCurrentInteractive() == celBodyInteractive)
        celBodyInteractive->setColumns(num);
    else if (getGeekConsole()->getCurrentInteractive() == colorChooserInteractive)
        colorChooserInteractive->setColumns(num);
    else if (getGeekConsole()->getCurrentInteractive() == flagInteractive)
        flagInteractive->setColumns(num);
    else if (getGeekConsole()->getCurrentInteractive() == fileInteractive)
        fileInteractive->setColumns(num);
    return 0;
}

static int setColumnsCh(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 2, "One or two arguments expected for gc.setColumnsCh(nNum, [nMaxCols])");
    int num = celx.safeGetNumber(1, AllErrors, "argument 1 to gc.setColumnsCh must be a number");
    int max = celx.safeGetNumber(2, WrongType, "argument 2 to gc.setColumnsCh must be a number");
    if (getGeekConsole()->getCurrentInteractive() == listInteractive)
        listInteractive->setColumnsCh(num, max);
    else if (getGeekConsole()->getCurrentInteractive() == celBodyInteractive)
        celBodyInteractive->setColumnsCh(num, max);
    else if (getGeekConsole()->getCurrentInteractive() == colorChooserInteractive)
        colorChooserInteractive->setColumnsCh(num, max);
    else if (getGeekConsole()->getCurrentInteractive() == flagInteractive)
        flagInteractive->setColumnsCh(num, max);
    else if (getGeekConsole()->getCurrentInteractive() == fileInteractive)
        fileInteractive->setColumnsCh(num, max);
    return 0;
}

static int setColumnsMax(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.setColumnsMax(nCols)");
    int max = celx.safeGetNumber(1, AllErrors, "arguments 1 to gc.setColumnsMax must be a number");
    if (getGeekConsole()->getCurrentInteractive() == listInteractive)
        listInteractive->setColumnsMax(max);
    else if (getGeekConsole()->getCurrentInteractive() == celBodyInteractive)
        celBodyInteractive->setColumnsMax(max);
    else if (getGeekConsole()->getCurrentInteractive() == colorChooserInteractive)
        colorChooserInteractive->setColumnsMax(max);
    else if (getGeekConsole()->getCurrentInteractive() == flagInteractive)
        flagInteractive->setColumnsMax(max);
    else if (getGeekConsole()->getCurrentInteractive() == fileInteractive)
        fileInteractive->setColumnsMax(max);
    return 0;
}

static int setLastFromHistory(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 0, "No arguments expected for gc.setLastFromHistory");
    GCInteractive *inter = getGeekConsole()->getCurrentInteractive();
    if (inter)
        inter->setLastFromHistory();
    return 0;
}

static int gcBindKey(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3, "Two or three arguments expected for gc.bind(sKeyAndARGS, sFunName, [sNameSpace])");
    const char *key = celx.safeGetString(1, AllErrors, "argument 1 to gc.bind must be a string");
    const char *fun = celx.safeGetString(2, AllErrors, "argument 2 to gc.bind must be a string");
    const char *namesp = celx.safeGetString(3, WrongType, "argument 3 to gc.bind must be a string");
    if (namesp)
        getGeekConsole()->bind(namesp, key, fun);
    else
        getGeekConsole()->bind("", key, fun);
    return 0;
}

// create bind (archive)
static int gcBindKeyA(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3, "Two or three arguments expected for gc.bind(sKeyAndARGS, sFunName, [sNameSpace])");
    const char *key = celx.safeGetString(1, AllErrors, "argument 1 to gc.bind must be a string");
    const char *fun = celx.safeGetString(2, AllErrors, "argument 2 to gc.bind must be a string");
    const char *namesp = celx.safeGetString(3, WrongType, "argument 3 to gc.bind must be a string");
    if (namesp)
        getGeekConsole()->bind(namesp, key, fun, true);
    else
        getGeekConsole()->bind("", key, fun, true);
    return 0;
}


static int gcUnBindKey(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "One or two arguments expected for gc.unbind(sKeyAndARGS, [sNamseSpace])");
    const char *key = celx.safeGetString(1, AllErrors, "argument 1 to gc.unbind must be a string");
    const char *namesp = celx.safeGetString(2, WrongType, "argument 2 to gc.unbind must be a string");
    if (namesp)
        getGeekConsole()->unbind(namesp, key);
    else
        getGeekConsole()->unbind("", key);
    return 0;
}

static int gcFlash(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "One or two arguments expected for gc.flash(sText, [nDuration])");
    const char *text = celx.safeGetString(1, AllErrors, "argument 1 to gc.flash must be a string");
    double duration = celx.safeGetNumber(2, WrongType, "argument 2 to gc.flash must be a number", 2.5);
    getGeekConsole()->showText(sstr(text), duration);
    return 0;
}

static int varNewString(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "1, 2, 3 arguments expected for gvar.NewString(sName, sValue, sDoc)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewString must be a string");
    const char *val = celx.safeGetString(2, WrongType, "argument 2 to gvar.NewString must be a number");
    const char *doc = celx.safeGetString(3, WrongType, "argument 3 to gvar.NewString must be a string");
    gVar.New(name, sstr(val), sstr(doc));
    return 0;
}

static int varNewInt(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "1, 2, 3 arguments expected for gvar.NewInt(sName, iValue, sDoc)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewInt must be a string");
    const char *doc = celx.safeGetString(3, WrongType, "argument 3 to gvar.NewInt must be a string");
    int val = celx.safeGetNumber(2, WrongType, "argument 2 to gvar.NewInt must be a number");
    gVar.New(name, val, sstr(doc));
    return 0;
}

static int varNewIntHex(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "1, 2, 3 arguments expected for gvar.NewIntHex(sName, iValue, sDoc)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewIntHex must be a string");
    const char *doc = celx.safeGetString(3, WrongType, "argument 3 to gvar.NewIntHex must be a string");
    int val = celx.safeGetNumber(2, WrongType, "argument 2 to gvar.NewIntHex must be a number");
    gVar.NewHex(name, val, sstr(doc));
    return 0;
}

static int varNewBool(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "1, 2, 3 arguments expected for gvar.NewBool(sName, dValue, sDoc)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewBool must be a string");
    const char *doc = celx.safeGetString(3, WrongType, "argument 3 to gvar.NewBool must be a string");
    bool val = celx.safeGetBoolean(2, WrongType, "argument 2 to gvar.NewBool must be a boolean");
    gVar.New(name, val, sstr(doc));
    return 0;
}

static int varNewFloat(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "1, 2, 3 arguments expected for gvar.NewFloat(sName, fValue, sDoc)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewFloat must be a string");
    const char *doc = celx.safeGetString(3, WrongType, "argument 3 to gvar.NewBool must be a string");
    float val = celx.safeGetNumber(2, WrongType, "argument 2 to gvar.NewFloat must be a number");
    gVar.New(name, val, sstr(doc));
    return 0;
}

static int varNewDouble(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "1, 2, 3 arguments expected for gvar.NewDouble(sName, dValue, sDoc)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewDouble must be a string");
    const char *doc = celx.safeGetString(3, WrongType, "argument 3 to gvar.NewDouble must be a string");
    double val = celx.safeGetNumber(2, WrongType, "argument 2 to gvar.NewDouble must be a number");
    gVar.New(name, val, sstr(doc));
    return 0;
}

static int varNewColor(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "1, 2, 3 arguments expected for gvar.NewColor(sName, dValue, sDoc)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewColor must be a string");
    const char *doc = celx.safeGetString(3, WrongType, "argument 3 to gvar.NewColor must be a string");
    const char *val = celx.safeGetString(2, WrongType, "argument 2 to gvar.NewDouble must be a string");
    gVar.NewColor(name, sstr(val).c_str(), sstr(doc));
    return 0;
}

static int varNewBody(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "1, 2, 3 arguments expected for gvar.NewBody(sName, dValue, sDoc)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewBody must be a string");
    const char *doc = celx.safeGetString(3, WrongType, "argument 3 to gvar.NewBody must be a string");
    const char *val = celx.safeGetString(2, WrongType, "argument 2 to gvar.NewBody must be a string");
    gVar.NewCelBodyName(name, sstr(val).c_str(), sstr(doc));
    return 0;
}

// manage flag tables that passed from lua for using it in c
class FlagTableMng {
public:
    typedef GeekVar::flags32_s* flagtable;
    FlagTableMng():num(0), tables(0){};
    ~FlagTableMng();
    GeekVar::flags32_s *getFlags(lua_State* l, int index);
private:
    int num;
    flagtable *tables;
};

static FlagTableMng flagTableMng;

FlagTableMng::~FlagTableMng()
{
    for (int i = 0; i < num; i++)
    {
        int j = 0;
        flagtable tbl = tables[i];
        while(tbl[j].name)
        {
            free((void *)(tbl[j].name));
            if (tbl[j].helptip)
                free((void *)(tbl[j].helptip));
            j++;
        }
    }
    free(tables);
}

// return allocated flag table flags32_s
GeekVar::flags32_s *FlagTableMng::getFlags(lua_State* l, int tableIndex)
{
    int argc = lua_gettop(l);
    if (tableIndex < 1 || tableIndex > argc)
        return false;
    if (lua_istable(l, tableIndex))
    {
        int n;
        n = lua_objlen(l, tableIndex);
        if (n < 1)
            return NULL;

        num++;
        tables = (flagtable *)realloc(tables, num * sizeof(flagtable));
        // alloc +1 - last for NULL
        flagtable tbl = (flagtable)malloc((n +1) * sizeof(GeekVar::flags32_s));
        tables[num - 1] = tbl;

        int i = 0;
        for (int ti = 1; ti <= n; ti++)
        {
            lua_rawgeti(l, tableIndex, ti);
            if (lua_istable(l, -1)) {
                int subn = lua_objlen(l, -1);
                if (subn > 1) {
                    tbl[i].name = NULL;
                    tbl[i].helptip = NULL;
                    tbl[i].mask = 0;
                    lua_rawgeti(l, -1, 1);
                    if (lua_isstring(l, -1))
                    {
                        const char *name = lua_tostring(l, -1);
                        tbl[i].name = strdup(name);
                    }
                    lua_remove(l, -1);

                    lua_rawgeti(l, -1, 2);
                    if (lua_isnumber(l, -1) && tbl[i].name)
                        tbl[i].mask = lua_tonumber(l, -1);
                    lua_remove(l, -1);

                    // 3rd item may be doc
                    if (subn > 2 && tbl[i].name)
                    {
                        lua_rawgeti(l, -1, 3);
                        if (lua_isstring(l, -1))
                            tbl[i].helptip = strdup(lua_tostring(l, -1));
                        lua_remove(l, -1);
                    }
                    i++;
                }
            }
            lua_remove(l, -1);
        }

        tbl[i].name = NULL;
        return tbl;
    }
    else
        return NULL;
}

static int varNewFlag(lua_State *l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 5, "2, 3, 4 or 5 arguments expected for gvar.NewFlag(sName, tFlags, [nValue], [sDelimiters], [sDoc])");
    GeekVar::flags32_s *fl = flagTableMng.getFlags(l, 2);
    if (!fl)
        return 0;
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewFlag must be a string");
    double val = celx.safeGetNumber(3, WrongType, "argument 3 to gvar.NewFlag must be a number", 0.0);
    string delims = sstr(celx.safeGetString(4, WrongType, "argument 4 to gvar.NewFlag must be a string"));
    if (delims == "")
        delims = "/";
    const char *doc = celx.safeGetString(5, WrongType, "argument 5 to gvar.NewFlag must be a string");
    gVar.NewFlag(name, fl, delims, (uint32)val, sstr(doc));
    return 0;
}

static int varNewEnum(lua_State *l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 4, "2, 3, or 4 arguments expected for gvar.NewEnum(sName, tFlags, [nValue], [sDoc])");
    GeekVar::flags32_s *fl = flagTableMng.getFlags(l, 2);
    if (!fl)
        return 0;
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.NewFlag must be a string");
    double val = celx.safeGetNumber(3, WrongType, "argument 3 to gvar.NewFlag must be a number", 0.0);
    const char *doc = celx.safeGetString(4, WrongType, "argument 4 to gvar.NewFlag must be a string");
    gVar.NewEnum(name, fl, (uint32)val, sstr(doc));
    return 0;
}

static int varSet(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "1 or 2 arguments expected for gvar.Set(table)|(sName, dValue)");

    if (lua_istable(l, 1))
    {
        for (int ti = 1; ti <= lua_objlen(l, 1); ti++)
        {
            lua_rawgeti(l, 1, ti);
            if (lua_istable(l, -1)) {
                int subn = lua_objlen(l, -1);
                if (subn > 1) {
                    const char *name = NULL;

                    lua_rawgeti(l, -1, 1);
                    if (lua_isstring(l, -1))
                        name = lua_tostring(l, -1);
                    lua_remove(l, -1);

                    lua_rawgeti(l, -1, 2);
                    if (name)
                        if (lua_isboolean (l, -1))
                        {
                            gVar.Set(name, (bool)lua_toboolean(l, -1));
                        }
                        else
                            if (lua_isnumber (l, -1))
                            {
                                gVar.Set(name, lua_tonumber(l, -1));
                            }
                            else
                                if (lua_isstring (l, -1))
                                {
                                    gVar.Set(name, sstr(lua_tostring(l, -1)));
                                }
                    lua_remove(l, -1);
                }
            }
            lua_remove(l, -1);
        }
    }
    else // 1 param is not table
    {
        const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.Set must be a string");
        if (lua_isboolean (l, 1))
        {
            bool val = celx.safeGetBoolean(2, AllErrors, "argument 2 to gvar.Set must be a number/boolean/string");
            gVar.Set(name, val);
        }
        else
            if (lua_isnumber (l, 1))
            {
                double val = celx.safeGetNumber(2, AllErrors, "argument 2 to gvar.Set must be a number/boolean/string");
                gVar.Set(name, val);
            }
            else
                if (lua_isstring (l, 1))
                {
                    const char *val = celx.safeGetString(2, AllErrors, "argument 2 to gvar.Set must be a number/boolean/string");
                    gVar.Set(name, sstr(val));
                }
    }
    return 0;
}

static int varGetNumber(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "1 argument expected for gvar.AsNum(sName)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.AsNum must be a string");
    lua_pushnumber(l, gVar.GetDouble(sstr(name)));
    return 1;
}

static int varGetBool(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "1 argument expected for gvar.AsBool(sName)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.AsBool must be a string");
    lua_pushboolean(l, gVar.GetBool(sstr(name)));
    return 1;
}

static int varGetStr(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "1 argument expected for gvar.AsStr(sName)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.AsStr must be a string");
    lua_pushstring(l, gVar.GetString(sstr(name)).c_str());
    return 1;
}

static int varReset(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "1 argument expected for gvar.Reset(sName)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.Reset must be a string");
    gVar.Reset(sstr(name));
    return 0;
}

static int varGetReset(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "1 argument expected for gvar.GetReset(sName)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.GetReset must be a string");
    lua_pushstring(l, gVar.GetResetValue(sstr(name)).c_str());
    return 1;
}

static int varGetLast(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "1 argument expected for gvar.GetLast(sName)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.GetLast must be a string");
    lua_pushstring(l, gVar.GetLastValue(sstr(name)).c_str());
    return 1;
}

static int varCopyVars(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "2 arguments expected for gvar.CopyVars(sName, sSrcGroup, sDstGroup)");
    const char *src = celx.safeGetString(1, AllErrors, "argument 1 to gvar.CopyVars must be a string");
    const char *dst = celx.safeGetString(2, AllErrors, "argument 2 to gvar.CopyVars must be a string");
    gVar.CopyVars(sstr(src), sstr(dst));
    return 0;
}

static int varCopyVarsTypes(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "2 arguments expected for gvar.CopyVarsTypes(sName, sSrcGroup, sDstGroup)");
    const char *src = celx.safeGetString(1, AllErrors, "argument 1 to gvar.CopyVarsTypes must be a string");
    const char *dst = celx.safeGetString(2, AllErrors, "argument 2 to gvar.CopyVarsTypes must be a string");
    gVar.CopyVarsTypes(sstr(src), sstr(dst));
    return 0;
}

static int varGetType(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "1 argument expected for gvar.GetType(sName)");
    const char *name = celx.safeGetString(1, AllErrors, "argument 1 to gvar.GetType must be a string");
    lua_pushstring(l, gVar.GetTypeName(gVar.GetType(sstr(name))).c_str());
    return 1;
}

void LoadLuaGeekConsoleLibrary(lua_State* l)
{
    CelxLua celx(l);

    lua_pushstring(l, "gc");
    lua_newtable(l);
    celx.registerMethod("registerFunction", registerFunction);
    celx.registerMethod("reRegisterFunction", reRegisterFunction);
    celx.registerMethod("isFunction", isFunctionRegistered);
    celx.registerMethod("setInfoText", setInfoText);
    celx.registerMethod("registerAlias", registerAlias);
    celx.registerMethod("registerAliasA", registerAliasA);
    celx.registerMethod("listInteractive", setListInteractive);
    celx.registerMethod("passwdInteractive", setPasswdInteractive);
    celx.registerMethod("celBodyInteractive", setCelBodyInteractive);
    celx.registerMethod("colorChooseInteractive", setColorChooseInteractive);
    celx.registerMethod("flagInteractive", setFlagInteractive);
    celx.registerMethod("fileInteractive", setFileInteractive);
    celx.registerMethod("infoInteractive", setInfoInteractive);
    celx.registerMethod("showHelpNode", showHelpNode);
    celx.registerMethod("color2HexRGB", color2HexRGB);
    celx.registerMethod("alphaFromColor", alphaFromColor);
    celx.registerMethod("finish", finishGConsole);
    celx.registerMethod("call", callFun);
    celx.registerMethod("setColumns", setColumns);
    celx.registerMethod("setColumnsCh", setColumnsCh);
    celx.registerMethod("setColumnsMax", setColumnsMax);
    celx.registerMethod("setLastFromHistory", setLastFromHistory);
    celx.registerMethod("bind", gcBindKey);
    celx.registerMethod("bindA", gcBindKeyA);
    celx.registerMethod("unbind", gcUnBindKey);
    celx.registerMethod("flash", gcFlash);
    lua_settable(l, LUA_GLOBALSINDEX);

    lua_pushstring(l, "gvar");
    lua_newtable(l);
    // new
    celx.registerMethod("NewString", varNewString);
    celx.registerMethod("NewBool", varNewBool);
    celx.registerMethod("NewInt", varNewInt);
    celx.registerMethod("NewIntHex", varNewIntHex);
    celx.registerMethod("NewFloat", varNewFloat);
    celx.registerMethod("NewDouble", varNewDouble);
    celx.registerMethod("NewNumber", varNewDouble);
    celx.registerMethod("NewColor", varNewColor);
    celx.registerMethod("NewBody", varNewBody);
    celx.registerMethod("NewFlag", varNewFlag);
    celx.registerMethod("NewEnum", varNewEnum);
    // set
    celx.registerMethod("Set", varSet);
    // get
    celx.registerMethod("AsNum", varGetNumber);
    celx.registerMethod("AsBool", varGetBool);
    celx.registerMethod("AsStr", varGetStr);
    // misc
    celx.registerMethod("Reset", varReset);
    celx.registerMethod("GetReset", varGetReset);
    celx.registerMethod("GetLast", varGetLast);
    celx.registerMethod("CopyVars", varCopyVars);
    celx.registerMethod("CopyVarsTypes", varCopyVarsTypes);
    celx.registerMethod("GetType", varGetType);

    lua_settable(l, LUA_GLOBALSINDEX);
}
