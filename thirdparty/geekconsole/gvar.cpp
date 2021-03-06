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

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <sys/stat.h>

#include "gvar.h"
#include "geekconsole.h"
#include "infointer.h"

// For some version of inttypes.h need to define it for macros PRIi32...
// TODO: FIX: Seems windows will have problem with inttypes.h
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

// global variable manager
GeekVar gVar;

using namespace std;

static int64 strToInt64(string str);
static int32 strToInt32(string str);
static uint32 strToUint32(string str);

// convert given text to number by given flag table
static uint32 setFlag(GeekVar::flags32_s *flagtbl, string flagDelim, string text)
{
    uint32 res;
    // try to set as int first
    res = strToUint32(text.c_str());

    if (flagtbl)
    {
        // try proceed flags
        vector <string> flags = splitString(text, flagDelim);

        // need init var with table's vars
        vector<string>::iterator it;
        for (it = flags.begin();
             it != flags.end(); it++)
        {
            int j = 0;
            while(flagtbl[j].name != NULL)
            {
                string s = *it;
                if(s == flagtbl[j].name )
                {
                    res += flagtbl[j].mask;
                    break;
                }
                j++;
            }
        }
    }
    return res;
}

// convert given text to number by given enum table
static uint32 setEnum(GeekVar::flags32_s *flagtbl, string text)
{
    // try to set as int first
    uint32 res;
    res = strToUint32(text.c_str());

    if (flagtbl)
    {
        // need init var with table's vars
        int j = 0;
        while(flagtbl[j].name != NULL)
        {
            if(text == flagtbl[j].name )
            {
                res = flagtbl[j].mask;
                break;
            }
            j++;
        }
    }
    return res;
}

// return color as uint32 from text
static uint32 setColor(string text)
{
    if (text.empty())
        return 0;

    // if color start as digit return number
    if (!text.empty() && isdigit(text[0]))
        return strToUint32(text);

    // color name or color like #RRGGBBAA converted to number
    Color32 c = getColor32FromText(text);
    return c.i;
}

void GeekVar::gvar::reset()
{
    set(resetString);
}

GeekVar::gvar::~gvar() {
    if (freep)
        delete p;
};

string GeekVar::gvar::get() {

    // if not binded return string value
    if (!p)
        return stringVal;

    char buffer [128];
    switch(type)
    {
    case Int32:
        sprintf(buffer, "%" PRIi32, *(int32 *)p);
        break;
    case Bool:
    {
        int32 v = *(bool *) p;
        sprintf(buffer, "%" PRIi32, v);
        break;
    }
    case Double:
        return doubleToString(*(double *) p);
        break;
    case Float:
        return doubleToString(*(float *) p);
        break;
    case Int64:
        sprintf(buffer, "%" PRIi64, *(int64 *) p);
        break;
    case Celbody: // celbody same as string
    case String:
        return *(string *) p;
    case Flags32:
        sprintf(buffer, "%" PRIu32, *(uint32 *) p);
        break;
    case Enum32:
        sprintf(buffer, "%" PRIu32, *(uint32 *) p);
        break;
    case Color:
        sprintf(buffer, "%" PRIu32, *(uint32 *) p);
        break;
    case Unknown:
        return stringVal;
    }
    return string(buffer);
}

void GeekVar::gvar::set(const string &val)
{
    lastVal = get();

    if (!p)
    {
        char buffer[64];
        switch(type)
        {
        case Flags32:
        {
            sprintf(buffer, "%" PRIu32,
                    (uint32) setFlag(flagtbl, flagDelim, val));
            stringVal = buffer;
            break;
        }
        case Enum32:
        {
            sprintf(buffer, "%" PRIu32,
                    (uint32) setEnum(flagtbl, val));
            stringVal = buffer;
            break;
        }
        case Color:
        {
            sprintf(buffer, "%" PRIu32,
                    (uint32) setColor(val));
            stringVal = buffer;
            break;
        }
        default:
            stringVal = val;
            break;
        }
        return;
    }

    switch(type)
    {
    case Flags32:
    {
        *(uint32 *)p = setFlag(flagtbl, flagDelim, val);
        break;
    }
    case Enum32:
    {
        *(uint32 *)p = setEnum(flagtbl, val);
        break;
    }
    case Color:
        *(uint32 *)p = setColor(val);
        break;
    case Int32:
        *(int32 *) p = strToInt32(val.c_str());
        break;
    case Int64:
        *(int32 *) p = strToInt64(val.c_str());
        break;
    case Bool:
        if (val == "true" || val == "yes")
            *(bool *) p = true;
        else
            *(bool *) p = (bool) atoi(val.c_str());
        break;
    case Double:
        *(double *) p = (double) strtod(val.c_str(), NULL);
        break;
    case Float:
        *(float *) p = (float) atof(val.c_str());
        break;
    case Celbody:
    case String:
        *(string *) p = val;
        break;
    case Unknown:
        stringVal = val;
        break;
    }
}

/******************************************************************
GeekVar
*******************************************************************/

GeekVar::GeekVar ()
{
    lock_set_get = false;
};

GeekVar::gvar *GeekVar::getGvar(string name)
{
    vars_t::iterator it = vars.find(name);
    if (it != vars.end())
    {
        gvar *v = &it->second;
        return v;
    }
    return NULL;
}

void GeekVar::Unbind(std::string name)
{
    gvar *oldvar = getGvar(name);
    if (oldvar)
    {
        gvar newvar;
        newvar.set(oldvar->get());
        vars[name] = newvar;
    }
}

#define DECLARE_GVAR_BIND(FunType, VarType)                             \
    bool GeekVar::Bind(string name, VarType *var, const char *resetval, string doc) \
    {                                                                   \
        if (!var)                                                       \
            return false;                                               \
        gvar *oldvar = getGvar(name);                                   \
        if(!oldvar)                                                     \
        {                                                               \
            gvar newvar(var, resetval, doc);                            \
            /* reset with default value  */                             \
            newvar.reset();                                             \
            vars[name] = newvar;                                        \
        }                                                               \
        else                                                            \
        {                                                               \
            /* skip if var is binded or created */                      \
            if (oldvar->type != Unknown)                                \
                return false;                                           \
                                                                        \
            gvar newvar(var, resetval, doc);                            \
            newvar.set(oldvar->get());                                  \
            vars[name] = newvar;                                        \
        }                                                               \
        callCreateHook(name);                                           \
        return true;                                                    \
    }

DECLARE_GVAR_BIND(Int32, int32);
DECLARE_GVAR_BIND(Int64, int64);
DECLARE_GVAR_BIND(Double, double);
DECLARE_GVAR_BIND(Float, float);
DECLARE_GVAR_BIND(Bool, bool);
DECLARE_GVAR_BIND(String, string);

bool GeekVar::Bind(std::string name, std::string *var,
              std::string resetval, std::string doc)
{
    return Bind(name, var, resetval.c_str(), doc);
}

bool GeekVar::BindHex(string name, int32 *var, const int32 resetval, string doc)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi32, resetval);
    Bind(name, var, buffer, doc);
    gvar *v = getGvar(name);
    if (v)
    {
        v->saveAsHex = true;;
    }
    return v;
}

bool GeekVar::BindHex(string name, int64 *var, const int64 resetval, string doc)
{
    char buffer[128];
    sprintf(buffer, "%" PRIi64, resetval);
    Bind(name, var, buffer, doc);
    gvar *v = getGvar(name);
    if (v)
    {
        v->saveAsHex = true;;
    }
    return v;
}

bool GeekVar::Bind(string name, int32 *var, const int32 resetval, string doc)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi32, resetval);
    return Bind(name, var, buffer, doc);
}

bool GeekVar::Bind(string name, int64 *var, const int64 resetval, string doc)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi64, resetval);
    return Bind(name, var, buffer);
}

bool GeekVar::Bind(string name, double *var, const double resetval, string doc)
{
    return Bind(name, var, doubleToString(resetval).c_str(), doc);
}

bool GeekVar::Bind(string name, float *var, const float resetval, string doc)
{
    return Bind(name, var, doubleToString(resetval).c_str(), doc);
}


bool GeekVar::Bind(string name, bool *var, const bool resetval, string doc)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi32, (int32) resetval);
    return Bind(name, var, buffer, doc);
}

bool GeekVar::BindFlag(string name, flags32_s *flagtbl, string _flagDelim, uint32 *var, const char *resetval, string doc)
{
    if (!var)
        return false;
    gvar *oldvar = getGvar(name);
    if (!oldvar)
    {
        gvar newvar(flagtbl, _flagDelim, var, resetval, doc);
        // reset with default value
        newvar.reset();
        vars[name] = newvar;
    }
    else
    {
        /* skip if var is binded or created */
        if (oldvar->type != Unknown)
            return false;

        *var = setFlag(flagtbl, _flagDelim, oldvar->get());
        gvar newvar(flagtbl, _flagDelim, var, resetval, doc);
        vars[name] = newvar;
    }
    callCreateHook(name);
    return true;
}

bool GeekVar::BindFlag(string name, flags32_s *flagtbl, string delim, uint32 *var, const uint32 resetval, string doc)
{
    char buffer[64];
    sprintf(buffer, "%" PRIu32, resetval);
    return BindFlag(name, flagtbl, delim, var, buffer, doc);
}

bool GeekVar::BindEnum(string name, flags32_s *flagtbl, uint32 *var, const char *resetval, string doc)
{
    if (!var)
        return false;
    gvar *oldvar = getGvar(name);
    if (!oldvar)
    {
        gvar newvar(flagtbl, var, resetval, doc);
        // reset with default value
        newvar.reset();
        vars[name] = newvar;
    }
    else
    {
        /* skip if var is binded or created */
        if (oldvar->type != Unknown)
            return false;

        *var = setEnum(flagtbl, oldvar->get());
        gvar newvar(flagtbl, var, resetval, doc);
        vars[name] = newvar;
    }
    callCreateHook(name);
    return true;
}

bool GeekVar::BindEnum(string name, flags32_s *flagtbl, uint32 *var, const uint32 resetval, string doc)
{
    char buffer[64];
    sprintf(buffer, "%" PRIu32, resetval);
    return BindEnum(name, flagtbl, var, buffer, doc);
}

bool GeekVar::BindColor(string name, uint32 *var, const char *resetval, string doc)
{
    if (!var)
        return false;
    gvar *oldvar = getGvar(name);
    if (!oldvar)
    {
        gvar newvar(Color, resetval, doc);
        newvar.p = var;
        // reset with default value
        newvar.reset();
        vars[name] = newvar;
    }
    else
    {
        /* skip if var is binded or created */
        if (oldvar->type != Unknown)
            return false;

        *var = setColor(oldvar->get());
        gvar newvar(Color, resetval, doc);
        newvar.p = var;
        vars[name] = newvar;
    }
    callCreateHook(name);
    return true;
}

bool GeekVar::BindCelBodyName(std::string name, std::string *var,
                              const char *resetval, std::string doc)
{
    if (!Bind(name, var, resetval, doc))
        return false;
    gvar *v = getGvar(name);
    if (v)
    {
        v->type = Celbody;
    }
    return v;
}

bool GeekVar::BindCelBodyName(std::string name, std::string *var,
                     string resetval, std::string doc)
{
    return BindCelBodyName(name, var, resetval.c_str(), doc);
}

bool GeekVar::BindColor(string name, uint32 *var, const uint32 resetval, string doc)
{
    char buffer[64];
    sprintf(buffer, "%" PRIu32, resetval);
    return BindColor(name, var, buffer, doc);
}

/******************************************************************
 *  New
 ******************************************************************/

#define DECLARE_GVAR_NEW(FunType, VarType)                               \
    bool GeekVar::New##FunType(string name, const VarType resetval, string doc)  \
    {                                                                   \
        VarType *p = new VarType;                                       \
        *p = resetval;                                                  \
        if (Bind##FunType(name, p, resetval, doc))                      \
        {                                                               \
            gvar *v = getGvar(name);                                    \
            if (v)                                                      \
            {                                                           \
                v->freep = true;;                                       \
            }                                                           \
            return true;                                                \
        }                                                               \
        return false;                                                   \
    }

DECLARE_GVAR_NEW(Hex, int32);
DECLARE_GVAR_NEW(Hex, int64);
DECLARE_GVAR_NEW(, int32);
DECLARE_GVAR_NEW(, int64);
DECLARE_GVAR_NEW(, double);
DECLARE_GVAR_NEW(, float);
DECLARE_GVAR_NEW(, bool);
DECLARE_GVAR_NEW(, string);
DECLARE_GVAR_NEW(Color, uint32);
DECLARE_GVAR_NEW(CelBodyName, string);

bool GeekVar::NewFlag(std::string name, flags32_s *flagtbl, std::string delim,
                      const char *resetval, std::string doc)
{
    uint32 *p = new uint32;
    *p = strToUint32(resetval);
    if (BindFlag(name, flagtbl, delim, p, resetval, doc))
    {
        gvar *v = getGvar(name);
        if (v)
        {
            v->freep = true;;
        }
        return true;
    }
    return false;
}

bool GeekVar::NewFlag(string name, flags32_s *flagtbl, string delim, const uint32 resetval, string doc)
{
    uint32 *p = new uint32;
    *p = resetval;
    if (BindFlag(name, flagtbl, delim, p, resetval, doc))
    {
        gvar *v = getGvar(name);
        if (v)
        {
            v->freep = true;;
        }
        return true;
    }
    return false;
}

bool GeekVar::NewEnum(std::string name, flags32_s *flagtbl,
                      const char *resetval, std::string doc)
{
    uint32 *p = new uint32;
    *p = strToUint32(resetval);
    if (BindEnum(name, flagtbl, p, resetval, doc))
    {
        gvar *v = getGvar(name);
        if (v)
        {
            v->freep = true;;
        }
        return true;
    }
    return false;
}

bool GeekVar::NewEnum(string name, flags32_s *flagtbl, const uint32 resetval, string doc)
{
    uint32 *p = new uint32;
    *p = resetval;
    if (BindEnum(name, flagtbl, p, resetval, doc))
    {
        gvar *v = getGvar(name);
        if (v)
        {
            v->freep = true;;
        }
        return true;
    }
    return false;
}

bool GeekVar::NewColor(string name, const char *resetval, string doc)
{
    return NewColor(name, strToUint32(resetval), doc);
}


/******************************************************************
 *  Set
 ******************************************************************/

void GeekVar::Set(string name, int32 val)
{
    char buffer[64];
    sprintf(buffer, "%" PRIu32, val);

    gvar *v = getGvar(name);
    if(v)
    {
        v->set(buffer);
        if (!lock_set_get && v->setHook)
        {
            lock_set_get = true;
            v->setHook(name);
            lock_set_get = false;
        }
    }
    else
    {
        gvar v;
        v.set(buffer);
        vars[name] = v;
        callCreateHook(name);
    }
}

void GeekVar::Set(string name, int64 val)
{
    char buffer[255];
    sprintf(buffer, "%" PRIi64, val);

    gvar *v = getGvar(name);
    if(v)
    {
        v->set(buffer);
        if (!lock_set_get && v->setHook)
        {
            lock_set_get = true;
            v->setHook(name);
            lock_set_get = false;
        }
    }
    else
    {
        gvar v;
        v.set(buffer);
        vars[name] = v;
        callCreateHook(name);
    }
}

void GeekVar::Set(string name, bool val)
{
    char buffer[64];
    sprintf(buffer, "%" PRIi32, (int32) val);

    gvar *v = getGvar(name);
    if(v)
    {
        v->set(buffer);
        if (!lock_set_get && v->setHook)
        {
            lock_set_get = true;
            v->setHook(name);
            lock_set_get = false;
        }
    }
    else
    {
        gvar v;
        v.set(buffer);
        vars[name] = v;
        callCreateHook(name);
    }
}

void GeekVar::Set(string name, double val)
{
    gvar *v = getGvar(name);
    if(v)
    {
        v->set(doubleToString(val));
        if (!lock_set_get && v->setHook)
        {
            lock_set_get = true;
            v->setHook(name);
            lock_set_get = false;
        }
    }
    else
    {
        gvar v;
        v.set(doubleToString(val));
        vars[name] = v;
        callCreateHook(name);
    }
}

void GeekVar::Set(string name, float val)
{
    gvar *v = getGvar(name);
    if(v)
    {
        v->set(doubleToString(val));
        if (!lock_set_get && v->setHook)
        {
            lock_set_get = true;
            v->setHook(name);
            lock_set_get = false;
        }
    }
    else
    {
        gvar v;
        v.set(doubleToString(val));
        vars[name] = v;
        callCreateHook(name);
    }
}

void GeekVar::Set(string name, string val)
{
    gvar *v = getGvar(name);
    if(v)
    {
        v->set(val);
        if (!lock_set_get && v->setHook)
        {
            lock_set_get = true;
            v->setHook(name);
            lock_set_get = false;
        }
    }
    else
    {
        gvar v;
        v.set(val);
        vars[name] = v;
        callCreateHook(name);
    }
}

void GeekVar::Set(string name, const char *val)
{
    Set(name, string(val));
}

/******************************************************************
 *  Get
 ******************************************************************/

#define RETURN_BINDED(v)                        \
    if (v->p)                                   \
        if (v->type == Int32)                   \
            return *(int32 *)v->p;              \
        else if (v->type == Int64)              \
            return *(int64 *)v->p;              \
        else if (v->type == Float)              \
            return *(float *)v->p;              \
        else if (v->type == Double)             \
            return *(double *)v->p;             \
        else if (v->type == Flags32)            \
            return *(uint32 *)v->p;             \
        else if (v->type == Enum32)             \
            return *(uint32 *)v->p;             \
        else if (v->type == Color)              \
            return *(uint32 *)v->p;

int32 GeekVar::GetI32(string name)
{
    gvar *v = getGvar(name);
    if(v)
    {
        if (!lock_set_get && v->getHook)
        {
            lock_set_get = true;
            v->getHook(name);
            lock_set_get = false;
        }
        RETURN_BINDED(v);
        return strToInt32(v->get().c_str());
    }
    else
        return 0;
}

uint32 GeekVar::GetUI32(std::string name)
{
    gvar *v = getGvar(name);
    if(v)
    {
        if (!lock_set_get && v->getHook)
        {
            lock_set_get = true;
            v->getHook(name);
            lock_set_get = false;
        }
        if (v->p)
        {
            RETURN_BINDED(v);
        }
        else
            if (v->type == Flags32)
                return setFlag(v->flagtbl, v->flagDelim, v->get());
            else if (v->type == Enum32)
                return setEnum(v->flagtbl, v->get());
        return strToUint32(v->get().c_str());
    }
    else
        return 0;
}

int64 GeekVar::GetI64(string name)
{
    gvar *v = getGvar(name);
    if(v)
    {
        if (!lock_set_get && v->getHook)
        {
            lock_set_get = true;
            v->getHook(name);
            lock_set_get = false;
        }
        RETURN_BINDED(v);
        return strToInt64(v->get().c_str());
    }
    else
        return 0;
}

double GeekVar::GetDouble(string name)
{
    gvar *v = getGvar(name);
    if(v)
    {
        if (!lock_set_get && v->getHook)
        {
            lock_set_get = true;
            v->getHook(name);
            lock_set_get = false;
        }
        RETURN_BINDED(v);
        return atof(v->get().c_str());
    }
    else
        return 0;
}

float GeekVar::GetFloat(string name)
{
    gvar *v = getGvar(name);
    if(v)
    {
        if (!lock_set_get && v->getHook)
        {
            lock_set_get = true;
            v->getHook(name);
            lock_set_get = false;
        }
        RETURN_BINDED(v);
        return atof(v->get().c_str());
    }
    else
        return 0;
}

bool GeekVar::GetBool(string name)
{
    gvar *v = getGvar(name);
    if (v)
    {
        if (!lock_set_get && v->getHook)
        {
            lock_set_get = true;
            v->getHook(name);
            lock_set_get = false;
        }
        if (v->get() == "true")
            return true;
        else
            return (bool) GetI32(name);
    }
    else
        return false;
}

string GeekVar::GetString(string name)
{
    gvar *v = getGvar(name);
    if(v)
    {
        if (!lock_set_get && v->getHook)
        {
            lock_set_get = true;
            v->getHook(name);
            lock_set_get = false;
        }
        return v->get();
    }
    else
        return "";
}

/* return text of flags, enum or color
   @force_string use given text instead of binded var of var
 */
static string _getFlagStr(GeekVar::gvar *v, string text, bool force_string = false)
{
    if (!v)
        return "";
    if (v->flagtbl)
    {
        uint32 flags = 0;
        if (!force_string && v->p) // if binded
        {
            if (v->type == GeekVar::Flags32 || v->type == GeekVar::Enum32)
                flags = *(uint32 *) v->p;
        }
        else
        {
            if (v->type == GeekVar::Flags32)
                flags = setFlag(v->flagtbl, v->flagDelim, text);
            else if(v->type == GeekVar::Enum32)
                flags = setEnum(v->flagtbl, text);
        }

        string res;
        int j = 0;
        if (v->type == GeekVar::Enum32)
        {
            while(v->flagtbl[j].name != NULL)
            {
                if (flags == v->flagtbl[j].mask)
                    return v->flagtbl[j].name;
                j++;
            }
        }
        else // flags
            if (!v->flagDelim.empty())
                while(v->flagtbl[j].name != NULL)
                {
                    if (flags & v->flagtbl[j].mask)
                    {
                        flags &= ~v->flagtbl[j].mask;
                        if (res.empty())
                            res = v->flagtbl[j].name;
                        else
                        {
                            res += v->flagDelim[0];
                            res += v->flagtbl[j].name;
                        }
                    }
                    j++;
                }
        return res;
    }
    if (v->type == GeekVar::Color)
    {
        Color32 c;
        if (!force_string && v->p) // if binded
        {
            c.i = *(uint32 *) v->p;
        }
        else
            c.i = setColor(text);
        return getColorName(c);
    }
    return text;
}

string GeekVar::GetFlagString(string name)
{
    gvar *v = getGvar(name);
    if (!v)
        return "";
    if (!lock_set_get && v->getHook)
    {
        lock_set_get = true;
        v->getHook(name);
        lock_set_get = false;
    }
    return _getFlagStr(v, v->get());
}

string GeekVar::GetFlagResetString(string name)
{
    gvar *v = getGvar(name);
    if (!v)
        return "";
    return _getFlagStr(v, v->resetString, true);
}

string GeekVar::GetFlagLastString(string name)
{
    gvar *v = getGvar(name);
    if (!v)
        return "";
    return _getFlagStr(v, v->lastVal, true);
}

/******************************************************************
 *  Misc
 ******************************************************************/

void GeekVar::Reset(string name)
{
    gvar *v = getGvar(name);
    if (v)
    {
        v->reset();
        if (!lock_set_get && v->setHook)
        {
            lock_set_get = true;
            v->setHook(name);
            lock_set_get = false;
        }
    }
}

string GeekVar::GetResetValue(string name)
{
    gvar *v = getGvar(name);
    if (v)
    {
        return v->resetString;
    }
    else
        return "";
}

std::string GeekVar::GetLastValue(std::string name)
{
    gvar *v = getGvar(name);
    if (v)
    {
        return v->lastVal;
    }
    else
        return "";
}

GeekVar::gvar_type GeekVar::GetType(string name)
{
    gvar *v = getGvar(name);
    if (v)
        return v->type;
    else
        return Unknown;
}

std::string GeekVar::GetTypeName(gvar_type type)
{
    switch (type)
    {
    case Int32: return "integer 32 bit";
    case Int64: return "integer 64 bit";
    case Bool: return "boolean";
    case Double: return "long number";
    case Float: return "number";
    case Unknown: return "unknown";
    case String: return "string";
    case Flags32: return "flags";
    case Enum32: return "enum";
    case Color: return "color";
    case Celbody: return "celestia body";
    }
    return "";
}

GeekVar::flags32_s *GeekVar::GetFlagTbl(string name)
{
    gvar *v = getGvar(name);
    if (v)
        return v->flagtbl;
    else
        return NULL;
}

string GeekVar::GetFlagDelim(string name)
{
    gvar *v = getGvar(name);
    if (v)
        return v->flagDelim;
    else
        return "";
}

string GeekVar::GetDoc(string name)
{
    gvar *v = getGvar(name);
    if (v)
        return v->doc;
    else
        return "";
}

vector<string> GeekVar::GetVarNames(string path)
{
    vector<string> res;
    vars_t::iterator it,itlow,itup;

    itlow=vars.lower_bound (path);
    for ( it=itlow ; it != vars.end(); it++ )
    {
        if ((*it).first.compare(0, path.size(), path) != 0)
            break;
        res.push_back((*it).first);
    }
    return res;
}

void GeekVar::CopyVars(std::string srcgroup, std::string dstgroup)
{
    vars_t::iterator it,itlow,itup;

    itlow=vars.lower_bound (srcgroup);
    for ( it=itlow ; it != vars.end(); it++ )
    {
        if ((*it).first.compare(0, srcgroup.size(), srcgroup) != 0)
            break;
        if (srcgroup.size() < ((*it).first).size())
        {
            string dstvar = dstgroup + ((*it).first).substr(srcgroup.size());
            // if dest var exist than just set value
            gvar *oldv = getGvar(dstvar);
            if (oldv)
            {
                Set(dstvar, GetString((*it).first));
            }
            else // otherwice create same, but not binded or hooked
            {
                gvar v = (*it).second;
                v.p = NULL;
                v.setHook = NULL;
                v.getHook = NULL;
                v.set(((*it).second).get());
                vars[dstvar] = v;
            }
        }
    }
}

void GeekVar::CopyVarsTypes(std::string srcgroup, std::string dstgroup)
{
    vars_t::iterator it,itlow,itup;
    itlow=vars.lower_bound (srcgroup);
    for ( it=itlow; it != vars.end(); it++ )
    {
        if ((*it).first.compare(0, srcgroup.size(), srcgroup) != 0)
            break;
        if (srcgroup.size() <= ((*it).first).size())
        {
            string dstvar = dstgroup + ((*it).first).substr(srcgroup.size());
            // if destvar exist than copy all types, doc, flagtables

            gvar *oldv = getGvar(dstvar);
            if (oldv)
            {
                oldv->type = (*it).second.type;
                oldv->doc = (*it).second.doc;
                oldv->flagDelim = (*it).second.flagDelim;
                oldv->flagtbl = (*it).second.flagtbl;
                oldv->saveAsHex = (*it).second.saveAsHex;
                if (!oldv->p)
                    oldv->resetString = oldv->get();
            }
        }
    }
}

bool GeekVar::IsBinded(std::string name)
{
    gvar *v = getGvar(name);
    // freep - true for created bu New* vars
    if (v && v->p && !v->freep)
        return true;
    else
        return false;
}

void GeekVar::SetGetHook(std::string name, HookFunc hook)
{
    gvar *v = getGvar(name);
    if (v)
        v->getHook = hook;
}
void GeekVar::SetSetHook(std::string name, HookFunc hook)
{
    gvar *v = getGvar(name);
    if (v)
        v->setHook = hook;
}

void GeekVar::AddCreateHook(std::string groupname, HookFunc hook)
{
    createHooks.insert(pair<std::string, HookFunc>(groupname, hook));
}

void GeekVar::callCreateHook(std::string var)
{
    createHook_t::iterator it;
    for ( it=createHooks.begin() ; it != createHooks.end(); it++ )
    {
        if (var.compare(0, (*it).first.size(), (*it).first) == 0)
        {
            ((*it).second)(var);
        }
    }
}

/******************************************************************
 *  help
 ******************************************************************/

static uint32 strToUint32(string str)
{
    // test for float
    if (str.find('e') == string::npos)
    {
        return strtoul(str.c_str(), NULL, 0);
    }
    else
        // first cast to double than to int
        return (uint32) atof(str.c_str());
}


static int32 strToInt32(string str)
{
    // test for float
    if (str.find('e') == string::npos)
    {
        return atol(str.c_str());
    }
    else
        // first cast to double than to int
        return (int32) atof(str.c_str());
}

static int64 strToInt64(string str)
{
    // test for float
    if (str.find('e') == string::npos)
    {
        int64 res;
        sscanf(str.c_str(), "%" PRIi64, &res);
        return res;
    }
    else
    {
        // first cast to double than to int64
        return (int64) atof(str.c_str());
    }
}

/******************************************************************
 *  Interactives
 ******************************************************************/


// help func, return known info about variable
static string describeVar(string varname)
{
    // first doc
    string res = gVar.GetDoc(varname) + "\n";
    // values
    res.append(_("Current value: "));
    res.append(gVar.GetFlagString(varname) + "\n");
    res.append(_("Previous value: "));
    res.append(gVar.GetFlagLastString(varname) + "\n");
    res.append(_("Default: "));
    res.append(gVar.GetFlagResetString(varname) + "\n");
    res.append(_("Variable type: "));
    res.append(gVar.GetTypeName(gVar.GetType(varname)));
    return res;
}

static int setVarFlag(GeekConsole *gc, int state, std::string value)
{
    static string varname;
    static GeekVar::gvar_type vartype;
    static GeekVar::flags32_s *flags;

    enum v_action {
        SET = 1,
        UNSET = 2,
        TOGGLE =3
    };
    static v_action setUnset;
    std::stringstream ss;

    switch(state)
    {
    case 0:
        gc->setInteractive(listInteractive, "select-gvar", _("Select variable"));
        listInteractive->setCompletion(gVar.GetVarNames(""));
        listInteractive->setMatchCompletion(true);
        break;
    case -1:
        gc->setInfoText(describeVar(value));
        break;
    case 1: // select action: set/unset
        varname = value;
        vartype = gVar.GetType(varname);

        gc->setInteractive(listInteractive, "set-unset", _("Set/Unset/Toggle flags"));
        listInteractive->setCompletionFromSemicolonStr("set;unset;toggle");
        listInteractive->setMatchCompletion(true);
        break;
    case 2: // create completion list for variable
    {
        vector<string> completion;
        switch (vartype)
        {
        case GeekVar::Unknown:
        case GeekVar::Int32:
        case GeekVar::Double:
        case GeekVar::Float:
        case GeekVar::Int64:
        case GeekVar::String:
            gc->setInteractive(listInteractive, "set-gvar-val.str",
                               string(_("Set ")) + varname + " (" + gVar.GetTypeName(vartype) + ")");
            completion.push_back(gVar.GetResetValue(varname));
            completion.push_back(gVar.GetLastValue(varname));
            completion.push_back(gVar.GetString(varname));
            listInteractive->setCompletion(completion);
            listInteractive->setDefaultValue(gVar.GetString(varname));
            break;
        case GeekVar::Bool:
            gc->setInteractive(listInteractive, "",
                               string(_("Set ")) + varname + " (" + gVar.GetTypeName(vartype) + ")");
            completion.push_back("yes");
            completion.push_back("no");
            completion.push_back("true");
            completion.push_back("false");
            listInteractive->setCompletion(completion);
            listInteractive->setMatchCompletion(true);
            listInteractive->setDefaultValue(gVar.GetString(varname));
            break;
        case GeekVar::Flags32:
        {
            if (value == "set")
            {
                ss << _("Set flags");
                setUnset = SET;
            } else if (value == "unset")
            {
                ss << _("Unset flags");
                setUnset = UNSET;
            } else
            {
                ss << _("Toggle flags");
                setUnset = TOGGLE;
            }

            flags = gVar.GetFlagTbl(varname);
            uint32 var = gVar.GetUI32(varname);
            int i = 0;
            while(flags[i].name != NULL)
            {
                // add only if flag not set currently if need to set flag and so on
                if (setUnset == SET)
                {
                    if (!(var & flags[i].mask))
                        completion.push_back(flags[i].name);
                } else if (setUnset == UNSET)
                {
                    if ((var & flags[i].mask))
                        completion.push_back(flags[i].name);
                } else
                    completion.push_back(flags[i].name);
                i++;
            }

            ss << " " << varname;
            gc->setInteractive(flagInteractive, "", ss.str());
            flagInteractive->setCompletion(completion);
            flagInteractive->setMatchCompletion(true);
            flagInteractive->setSeparatorChars(gVar.GetFlagDelim(varname));
            break;
        }
        case GeekVar::Enum32:
        {
            gc->setInteractive(listInteractive, "",
                               string(_("Set ")) + varname + " (enum)");
            listInteractive->setMatchCompletion(true);

            flags = gVar.GetFlagTbl(varname);
            int i = 0;
            while(flags && flags[i].name != NULL)
            {
                completion.push_back(flags[i].name);
                i++;
            }
            listInteractive->setCompletion(completion);
            listInteractive->setDefaultValue(gVar.GetFlagString(varname));
            break;
        }
        case GeekVar::Color:
            gc->setInteractive(colorChooserInteractive, "select-color",
                               string(_("Select color ")) + varname);
            colorChooserInteractive->setDefaultValue(gVar.GetFlagString(varname));
            break;
        case GeekVar::Celbody:
            gc->setInteractive(celBodyInteractive, "select-body",
                               string(_("Select body ")) + varname);
            celBodyInteractive->setDefaultValue(gVar.GetFlagString(varname));
            break;
        }
        break;
    } // case 2:
    // hint for suitable value of variable
    case -2-1:
    {
        switch(vartype)
        {
        case GeekVar::Flags32:
        case GeekVar::Enum32:
        {
            // find doc of value from table
            int i = 0;
            while(flags && flags[i].name != NULL)
            {
                if (flags[i].name == value)
                {
                    if (flags[i].helptip)
                        gc->setInfoText(_(flags[i].helptip));
                    break;
                }
                i++;
            }
            break;
        }
        case GeekVar::Celbody:
            break;
        default:
            // for non flag vars just hint of variable
            gc->setInfoText(describeVar(varname));
            break;
        }
        break;
    }

    case 3:
    {
        if (GeekVar::Celbody == vartype)
        {
            Selection sel = gc->getCelCore()->getSimulation()->findObjectFromPath(value, true);
            if (!sel.empty())
                value = getSelectionName(sel);
        }
        else if (GeekVar::Flags32 == vartype)
        {
            uint32 var = gVar.GetUI32(varname);
            // split flags str
            std::vector<std::string> strArray =
                splitString(value, gVar.GetFlagDelim(varname));
            std::vector<string>::iterator it;

            // now setup flags
            for (it = strArray.begin();
                 it != strArray.end(); it++)
            {
                int i = 0;
                while(flags[i].name != NULL)
                {
                    string s = *it;
                    if(s == flags[i].name )
                    {
                        if (setUnset == SET)
                            var |= flags[i].mask;
                        else if (setUnset == UNSET)
                            var &= ~ flags[i].mask;
                        else // toggle
                            var ^= flags[i].mask;
                        break;
                    }
                    i++;
                }
            }
            gVar.Set(varname, (int32)var);
            break;
        }
        gVar.Set(varname, value);
        break;
    } // case 3:
    }
    return state;
}



static int setVar(GeekConsole *gc, int state, std::string value)
{
    static string varname;
    static GeekVar::gvar_type vartype;
    static GeekVar::flags32_s *flags;
    switch(state)
    {
        // select var
    case 0:
        gc->setInteractive(listInteractive, "select-gvar", _("Select variable"));
        listInteractive->setCompletion(gVar.GetVarNames(""));
        listInteractive->setMatchCompletion(true);
        break;
    case -1:
        gc->setInfoText(describeVar(value));
        break;
        // select value
    case 1:
    {
        varname = value;
        vartype = gVar.GetType(varname);
        vector<string> completion;

        switch (vartype)
        {
        case GeekVar::Unknown:
        case GeekVar::Int32:
        case GeekVar::Double:
        case GeekVar::Float:
        case GeekVar::Int64:
        case GeekVar::String:
            gc->setInteractive(listInteractive, "set-gvar-val.str",
                               varname + " (" + gVar.GetTypeName(vartype) + ")");
            completion.push_back(gVar.GetResetValue(varname));
            completion.push_back(gVar.GetLastValue(varname));
            completion.push_back(gVar.GetString(varname));
            listInteractive->setCompletion(completion);
            listInteractive->setDefaultValue(gVar.GetString(varname));
            break;
        case GeekVar::Bool:
            gc->setInteractive(listInteractive, "",
                               varname + " (" + gVar.GetTypeName(vartype) + ")");
            completion.push_back("yes");
            completion.push_back("no");
            completion.push_back("true");
            completion.push_back("false");
            listInteractive->setCompletion(completion);
            listInteractive->setMatchCompletion(true);
            listInteractive->setDefaultValue(gVar.GetString(varname));
            break;
        case GeekVar::Flags32:
        {
            gc->setInteractive(flagInteractive, "",
                               varname + " (flags)");
            flagInteractive->setMatchCompletion(true);
            flags = gVar.GetFlagTbl(varname);
            int i = 0;
            while(flags[i].name != NULL)
            {
                completion.push_back(flags[i].name);
                i++;
            }
            flagInteractive->setCompletion(completion);
            flagInteractive->setDefaultValue(gVar.GetFlagString(varname));
            flagInteractive->setSeparatorChars(gVar.GetFlagDelim(varname));
            break;
        }
        case GeekVar::Enum32:
        {
            gc->setInteractive(listInteractive, "",
                               varname + " (enum)");
            listInteractive->setMatchCompletion(true);

            flags = gVar.GetFlagTbl(varname);
            int i = 0;
            while(flags && flags[i].name != NULL)
            {
                completion.push_back(flags[i].name);
                i++;
            }
            listInteractive->setCompletion(completion);
            listInteractive->setDefaultValue(gVar.GetFlagString(varname));
            break;
        }
        case GeekVar::Color:
            gc->setInteractive(colorChooserInteractive, "select-color",
                               string(_("Select color ")) + varname);
            colorChooserInteractive->setDefaultValue(gVar.GetFlagString(varname));
            break;
        case GeekVar::Celbody:
            gc->setInteractive(celBodyInteractive, "select-body",
                               string(_("Select body ")) + varname);
            celBodyInteractive->setDefaultValue(gVar.GetFlagString(varname));
            break;
        }
        break;
    }
    // hint for suitable value of variable
    case -1-1:
    {
        switch(vartype)
        {
        case GeekVar::Flags32:
        case GeekVar::Enum32:
        {
            // find doc of value from table
            int i = 0;
            while(flags && flags[i].name != NULL)
            {
                if (flags[i].name == value)
                {
                    if (flags[i].helptip)
                    gc->setInfoText(_(flags[i].helptip));
                    break;
                }
                i++;
            }
            break;
        }
        case GeekVar::Celbody:
            break;
        default:
            // for non flag vars just hint of variable
            gc->setInfoText(describeVar(varname));
            break;
        }
        break;
    }
    // set
    case 2:
    {
        // normalize body name for type Celbody

        if (vartype == GeekVar::Celbody)
        {
            Selection sel = gc->getCelCore()->getSimulation()->findObjectFromPath(value, true);
            if (!sel.empty())
                value = getSelectionName(sel);
        }
        gVar.Set(varname, value);
        break;
    }
    }
    return state;
}

static int resetVar(GeekConsole *gc, int state, std::string value)
{
    switch(state)
    {
        // select var
    case 0:
        gc->setInteractive(listInteractive, "select-gvar", _("Select variable for reset"));
        listInteractive->setCompletion(gVar.GetVarNames(""));
        listInteractive->setMatchCompletion(true);
        break;
    case -1:
        gc->setInfoText(describeVar(value));
        break;
        // select value
    case 1:
        gVar.Reset(value);
        break;
    }
    return state;
}

static int varSetLastVal(GeekConsole *gc, int state, std::string value)
{
    switch(state)
    {
        // select var
    case 0:
        gc->setInteractive(listInteractive, "select-gvar", _("Select variable"));
        listInteractive->setCompletion(gVar.GetVarNames(""));
        listInteractive->setMatchCompletion(true);
        break;
    case -1:
        gc->setInfoText(describeVar(value));
        break;
        // select value
    case 1:
        gVar.Set(value, gVar.GetLastValue(value));
        break;
    }
    return state;
}

static int varSave(GeekConsole *gc, int state, std::string value)
{
    static string varnames;
    static string filename;
    if (state == 0)
    {
        gc->setInteractive(listInteractive, "gvar-path", _("Variable(s) to save"));
        listInteractive->setCompletion(gVar.GetVarNames(""));
    }
    else if (state == -1)
        gc->setInfoText(describeVar(value));
    else if (state == 1)
    {
        varnames = value;
        gc->setInteractive(fileInteractive, "gvar-file-savename", string(_("Select file to save")) + " " + varnames);
        fileInteractive->setLastFromHistory();
        fileInteractive->setFileExtenstion(".celx;.cel");
    } else if (state == 2)
    {
        struct stat stFileInfo;
        filename = value;
        // if file exist
        if (!stat(value.c_str(),&stFileInfo))
        {
            // ask for overwrite
            gc->setInteractive(listInteractive, "yes-no-gfs", _("Are you sure to overwrite?"), "File: \"" + value + "\"");
            listInteractive->setCompletionFromSemicolonStr("yes;no");
            listInteractive->setMatchCompletion(true);
        }
        else
            state = 4; // ok, we can write file, so skip yes-no check
    } else if (state == 3) // yes-no
        if (value != "yes")
        {
            gc->setInteractive(fileInteractive, "gvar-file-savename", string(_("Select file to save")) + " " + varnames);
            fileInteractive->setLastFromHistory();
            fileInteractive->setFileExtenstion(".celx;.cel");
            return 1; // recall with 1 state
        }
        else
            state = 4;

    if (state == 4)
    {
        //save
        std::ofstream file(filename.c_str(), ofstream::out);
        if (!file.good())
        {
            gc->beep();
            gc->showText("File not created " + filename);
            return state;
        }

        file << "-- -*- mode: lua -*-\n\ngvar.Set({\n";
        vector<string> vars = gVar.GetVarNames(varnames);
        vector<string>::iterator itv;
        for (itv = vars.begin(); itv != vars.end(); itv++)
        {
            if ((*itv).empty() || ((*itv)[0] == '.'))
                continue;

            file << "   {\"" << normalLuaStr((*itv)) << "\",\t";
            switch(gVar.GetType((*itv)))
            {
            case GeekVar::Int32:
            case GeekVar::Double:
            case GeekVar::Float:
            case GeekVar::Int64:
                file << doubleToString(gVar.GetDouble(*itv));
                break;
            case GeekVar::Bool:
            {
                bool b = gVar.GetBool(*itv);
                if (b)
                    file << "true";
                else
                    file << "false";
                break;
            }
            default:
                file << "\"" << normalLuaStr(gVar.GetFlagString(*itv)) << "\"";
                break;
            }
            file << "},\n";
        }
        file << "})";
        file.close();
        gc->showText("Saved in " + filename);
    }
    return state;
}

static int aproposVarCust(GeekConsole *gc, int state, std::string value)
{
    switch(state)
    {
    case 0:
        gc->setInteractive(listInteractive, "apropos-gvar", _("Pattern"));
        break;
    case 1:
        if (value.empty())
            value = "root";
        gc->setInteractive(infoInteractive, "Info");
        infoInteractive->setNode("*customize*", "Apropos customize " + value);
        break;
    }
    return state;
}

static int aproposVarSum(GeekConsole *gc, int state, std::string value)
{
    switch(state)
    {
    case 0:
        gc->setInteractive(listInteractive, "apropos-gvar", _("Pattern"));
        break;
    case 1:
        if (value.empty())
            value = "root";
        gc->setInteractive(infoInteractive, "Info");
        infoInteractive->setNode("*customize*", "Apropos summary " + value);
        break;

    }
    return state;
}


static std::vector<std::string> getGroupsOfVarNames(std::vector<std::string> l, int text_start)
{
    vector<string> res;
    vector<string>::iterator it;

    size_t found;
    string lastAdded;
    for (it = l.begin(); it != l.end(); it++)
    {
        found  = (*it).find_first_of('/', text_start);
        if (found != string::npos)
        {
            found++;
            string grp = (*it).substr(0, found);

            if (grp != lastAdded)
            {
                res.push_back(grp);
                lastAdded = grp;
            }
        }
    }
    return res;
}

static std::vector<std::string> getNamesOfVarNames(std::vector<std::string> l, int text_start)
{
    vector<string> res;
    vector<string>::iterator it;

    size_t found;
    string lastAdded;
    for (it = l.begin(); it != l.end(); it++)
    {
        found  = (*it).find_first_of('/', text_start);
        if (found == string::npos)
        {
            res.push_back(*it);
            lastAdded = *it;
        }
    }
    return res;
}

/* Info file generator for customize variables
 */
string customizeVar(string filename, string nodename)
{
    string res = "File: *customize*,\tNode: " + nodename + ", ";// Top,  Up: (dir)\n\n";

    const char *custom_node = "Customize ";
    const char *summary_node = "Summary ";
    const char *apropos_custom = "Apropos customize ";
    const char *apropos_sum = "Apropos summary ";
    string groupName;

    if ("Top" == nodename)
    {
        res += "Up: (dir)\n\n"
            "   Here you can overview and customize all variables.\n\n"
            "* Menu:\n"
            "* Customize: Customize root.       Customize variables.\n"
            "* Summary: Summary root.           Customize without details.\n"
            "* All: Apropos summary root.       Overview all variables.\n\n";
        return res;
    }

    if (nodename.find(apropos_custom) == 0)
    {
        res += "Up: Top\n\n"
            "Customization of all variables that matches pattern.\n***\n";

        string pattern = nodename.substr(strlen(apropos_custom));
        if (pattern == "root")
            pattern = "";
        int buf_length = UTF8Length(pattern);

        vector<string> vars;
        filterVector(gVar.GetVarNames(groupName), vars, pattern);
        vector<string>::iterator it;
        for (it = vars.begin(); it != vars.end(); it++)
        {
            res += INFO_TAGSTART "descvar name=[[";
            res += *it;
            res += "]]" INFO_TAGEND "....\n";
        }
        return res;
    }

    if (nodename.find(apropos_sum) == 0)
    {
        res += "Up: Top\n\n"
            "Summary of all variables that matches pattern.\n***\n";

        string pattern = nodename.substr(strlen(apropos_sum));
        if (pattern == "root")
            pattern = "";
        int buf_length = UTF8Length(pattern);

        vector<string> vars;
        filterVector(gVar.GetVarNames(groupName), vars, pattern);
        vector<string>::iterator it;
        for (it = vars.begin(); it != vars.end(); it++)
        {
            res += INFO_TAGSTART "editvar2 name=[[";
            res += *it;
            res += "]]" INFO_TAGEND "\n";
        }
        res += "\n....\n\nMore information about variables *Note ";
        if (pattern.empty())
            pattern = "root";
        res += apropos_custom + pattern + "::.\n";
        return res;
    }

    // check for "Customize ..." or "Summary ..." node name
    const char *nodename_pfx = NULL;
    if (nodename.find(custom_node) == 0)
    {
        nodename_pfx = custom_node;
    }
    if (nodename.find(summary_node) == 0)
    {
        nodename_pfx = summary_node;
    }

    if (nodename_pfx)
    {
        groupName = nodename.substr(strlen(nodename_pfx));
        // special case: root group name, clearit for math vars
        if (groupName == "root")
            groupName = "";

        string upNode = "Top";
        if (!groupName.empty())
        {
            size_t f = groupName.find_last_of('/');
            if (f != string::npos)
            {
                upNode = string(nodename_pfx) + " root";
                f = groupName.substr(0, f).find_last_of('/');
            }
            if (f != string::npos)
                upNode = nodename_pfx + groupName.substr(0, f +1);
        }
        res += ", Up: " + upNode + "\n\n";

        vector<string> vars = gVar.GetVarNames(groupName);
        vector<string> groups = getGroupsOfVarNames(vars, groupName.size());
        vector<string> varnames = getNamesOfVarNames(vars, groupName.size());
        vector<string>::iterator it;

        res += "Customization of the \"";
        res += groupName + "\" group.\n***\n\n";

        if (!groups.empty())
        {
            res += "* Sub groups:\n\n";
            for (it = groups.begin(); it != groups.end(); it++)
            {
                res += "* ";
                res += *it + ": (*customize*)";
                res += nodename_pfx;
                res += *it + ".\n";
            }
            res += "\n---\n";
        }
        res.push_back('\n');

        if (custom_node == nodename_pfx)
        {
            for (it = varnames.begin(); it != varnames.end(); it++)
            {
                res += INFO_TAGSTART "descvar name=[[";
                res += *it;
                res += "]]" INFO_TAGEND "....\n";
            }
        }
        else if (summary_node == nodename_pfx)
        {
            for (it = varnames.begin(); it != varnames.end(); it++)
            {
                res += INFO_TAGSTART "editvar2 name=[[";
                res += *it;
                res += "]]" INFO_TAGEND "\n";
            }

            res += "\n....\n\nMore information about variable *Note ";
            if (groupName.empty())
                groupName = "root";
            res += custom_node + groupName + "::.\n";
        }
    }

    return res;
}


void initGCVarInteractivsFunctions()
{
    GeekConsole *gc = getGeekConsole();
    gc->registerFunction(GCFunc(setVar, _("Set variable")),
                         "set variable");
    gc->registerFunction(GCFunc(setVarFlag, _("Set, unset or toggle variable's flags\n"
                                    "or just set variable")),
                         "set flag");
    gc->registerFunction(GCFunc(resetVar, _("Reset variable")),
                         "reset variable");
    gc->registerFunction(GCFunc(varSetLastVal, _("Set variable with the last set value")),
                         "set variable with last");
    gc->registerFunction(GCFunc(aproposVarCust, _("Show all meaningful variables "
                                                  "whose names match")),
                         "apropos variable");
    gc->registerFunction(GCFunc(aproposVarSum, _("Show summary of all meaningful variables "
                                                  "whose names match")),
                         "apropos variable summary");
    gc->registerFunction(GCFunc(varSave, _("Save variables as celx (lua) file.\n"
                                                  "You can load saved file by `open script'.")),
                         "save variables");
    gc->registerFunction(GCFunc("save variables", "render/color/", _("Save color theme.")),
                         "save color theme");

    char dir[] = "INFO-DIR-SECTION Customize\n"
        "START-INFO-DIR-ENTRY\n"
        "* Customize: (*customize*)Customize root.  Customize variables.\n"
        "* Summary: (*customize*)Summary root.      Customize without details.\n"
        "     describe.\n"
        "END-INFO-DIR-ENTRY\n"
        "INFO-DIR-SECTION GeekConsole\n"
        "START-INFO-DIR-ENTRY\n"
        "* Variables: (*customize*)Top.   Customize variables.\n"
        "END-INFO-DIR-ENTRY";
    infoInteractive->addDirTxt(dir, sizeof(dir), true);
    infoInteractive->registerDynamicNode("*customize*", customizeVar);
}
