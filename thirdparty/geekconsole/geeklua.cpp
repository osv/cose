// Copyright (C) 2010 Olexandr Sydorchuk <olexandr_syd [at] users.sourceforge.net>
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

/******************************************************************
 *  Lua API for geek console
 ******************************************************************/
std::string sstr(const char *str)
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

static int setListInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(4, 5, "Four or three arguments expected for gc.listInteractive(sCompletion, shistory, sPrefix, sDescr, [mustMatch])");
    const char *completion = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    const char *history = celx.safeGetString(2, AllErrors, "argument 2 to gc.listInteractive must be a string");
    const char *prefix = celx.safeGetString(3, AllErrors, "argument 3 to gc.listInteractive must be a string");
    const char *descr = celx.safeGetString(4, AllErrors, "argument 4 to gc.listInteractive must be a string");
    bool isMustMatch = celx.safeGetBoolean(5, WrongType, "argument 5 to gc.listInteractive must be a number", false);
    getGeekConsole()->setInteractive(listInteractive, history, prefix, descr);
    listInteractive->setCompletionFromSemicolonStr(completion);
    listInteractive->setMatchCompletion(isMustMatch);
    return 0;
}

static int setPasswdInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gc.listInteractive(sPrefix, sDescr)");
    const char *prefix = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    const char *descr = celx.safeGetString(2, AllErrors, "argument 2 to gc.listInteractive must be a string");
    getGeekConsole()->setInteractive(passwordInteractive, "", prefix, descr);
    return 0;
}

static int setCelBodyInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 4, "Three or four arguments expected for gc.celBodyInteractive(shistory, sPrefix, sDescr, [sCompetion]");
    const char *history = celx.safeGetString(1, AllErrors, "argument 1 to gc.celBodyInteractive must be a string");
    const char *prefix = celx.safeGetString(2, AllErrors, "argument 2 to gc.celBodyInteractive must be a string");
    const char *descr = celx.safeGetString(3, AllErrors, "argument 3 to gc.celBodyInteractive must be a string");
    const char *completion = celx.safeGetString(4, WrongType, "argument 4 to gc.celBodyInteractive must be a string");
    getGeekConsole()->setInteractive(celBodyInteractive, history, prefix, descr);
    if (completion)
        celBodyInteractive->setCompletionFromSemicolonStr(completion);
    return 0;
}

static int setColorChooseInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 3, "Three arguments expected for gc.celBodyInteractive(shistory, sPrefix, sDescr)");
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
    const char *completion = celx.safeGetString(1, AllErrors, "argument 1 to gc.flagInteractive must be a string");
    const char *history = celx.safeGetString(2, AllErrors, "argument 2 to gc.flagInteractive must be a string");
    const char *prefix = celx.safeGetString(3, AllErrors, "argument 3 to gc.flagInteractive must be a string");
    const char *descr = celx.safeGetString(4, AllErrors, "argument 4 to gc.flagInteractive must be a string");
    bool isMustMatch = celx.safeGetBoolean(5, WrongType, "argument 5 to gc.flagInteractive must be a boolean", true);
    const char *delim = celx.safeGetString(6, WrongType, "argument 6 to gc.flagInteractive must be a string");
    getGeekConsole()->setInteractive(flagInteractive, history, prefix, descr);
    flagInteractive->setCompletionFromSemicolonStr(completion);
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

static int setPagerInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 3, "One or three arguments expected for gc.pagerInteractive(sText, sPrefix, sDescr)");
    const char *text = celx.safeGetString(1, AllErrors, "argument 1 to gc.pagerInteractive must be a string");
    const char *prefix = celx.safeGetString(2, WrongType, "argument 2 to gc.pagerInteractive must be a string");
    const char *descr = celx.safeGetString(3, WrongType, "argument 3 to gc.pagerInteractive must be a string");
    getGeekConsole()->setInteractive(pagerInteractive, "", sstr(prefix), sstr(descr));
    pagerInteractive->setText(text);
    return 0;
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
        return 0;

    // append macro
    getGeekConsole()->appendCurrentMacro(string("@*EXEC*@") + funName);
    if (args)
        getGeekConsole()->appendCurrentMacro(args);

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


static int gcUnBindKey(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "One or two arguments expected for gc.unbind(sKeyAndARGS, [sNamseSpace])");
    const char *key = celx.safeGetString(1, AllErrors, "argument 1 to gc.bind must be a string");
    const char *namesp = celx.safeGetString(2, WrongType, "argument 2 to gc.bind must be a string");
    if (namesp)
        getGeekConsole()->unbind(namesp, key);
    else
        getGeekConsole()->unbind("", key);
    return 0;
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
    celx.registerMethod("listInteractive", setListInteractive);
    celx.registerMethod("passwdInteractive", setPasswdInteractive);
    celx.registerMethod("celBodyInteractive", setCelBodyInteractive);
    celx.registerMethod("colorChooseInteractive", setColorChooseInteractive);
    celx.registerMethod("pagerInteractive", setPagerInteractive);
    celx.registerMethod("flagInteractive", setFlagInteractive);
    celx.registerMethod("fileInteractive", setFileInteractive);
    celx.registerMethod("color2HexRGB", color2HexRGB);
    celx.registerMethod("alphaFromColor", alphaFromColor);
    celx.registerMethod("finish", finishGConsole);
    celx.registerMethod("call", callFun);
    celx.registerMethod("setColumns", setColumns);
    celx.registerMethod("setLastFromHistory", setLastFromHistory);
    celx.registerMethod("bind", gcBindKey);
    celx.registerMethod("unbind", gcUnBindKey);
    lua_settable(l, LUA_GLOBALSINDEX);
}
