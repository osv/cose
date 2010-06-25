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
#include "geekbind.h"
#include <cstdlib>

const char *nonShiftChars="~!@#$%^&*()_+|{}><?:\"";

static int skipWhiteSpace(const char *str, int offset)
{
    while (str[offset] && str[offset] == ' ')
        offset++;
    return offset;
}

static int skipWord(const char *str, int offset)
{
    while (str[offset] && str[offset] != ' ')
        offset++;
    return offset;
}

static const struct key_names
{
    char *keyName;
    char ckey;
} keyNames[] =
{
    {"SPACE", ' '},
    {"ESC", 27},
    {"BACKSPACE", 8},
    {"RET", 13},
    {"TAB", '\t'},
    {"DEL", 127},
    {"DELELETE", 127},
    {NULL, NULL}
};

std::string GeekBind::KeyBind::keyToStr() const
{
    std::string str;
    std::string key;
    for (int i = 0; i < len; i++)
    {
        if (mod[i] & CTRL)
            str += "C-";
        if (mod[i] & META)
            str += "M-";
        if (mod[i] & SHIFT)
            str += "S-";
        int ii=0;
        key = c[i];
        while(keyNames[ii].keyName)
        {
            if (c[i] == keyNames[ii].ckey)
            {
                key = keyNames[ii].keyName;
                break;
            }
            ii++;
        }
        str += key;
        if (i < len - 1)
            str += ' ';
    }
    return str;
}

bool GeekBind::KeyBind::set(const char *keybind)
{
    int i = 0;
    len = 0;
    i = skipWhiteSpace(keybind, i);
    while(keybind[i])
    {
        mod[len] = 0;
        while (keybind[i] && keybind[i+1] == '-')
        {
            switch (keybind[i])
            {
            case 'C':
            case 'c':
                mod[len] |= GeekBind::CTRL;
            break;
            case 'M':
            case 'm':
                mod[len] |= GeekBind::META;
            break;
            case 'S':
            case 's':
                mod[len] |= GeekBind::SHIFT;
            break;
            default:
                return false;
            };
            i+=2;
        }
        i = skipWhiteSpace(keybind, i);
        if (keybind[i] == '#' && //is params
            keybind[i+1] != ' ' && // no '#'
            keybind[i+1] != '\0')
        {
            if (len == 0) // params w/o keybind
                return false;
            const char *str = keybind + i;
            params = std::string(str);
            return true;
        }
        int wordend = skipWord(keybind, i);
        if (wordend - i == 1) // char
        {
            c[len] = keybind[i];
            if (isupper(keybind[i])) // no upper case, just S-
            {
                c[len] = tolower(c[len]);
                mod[len] |= SHIFT;
            }
            // clear shit for some spec chars
            if (strchr(nonShiftChars, c[len]))
                mod[len] &= ~GeekBind::SHIFT;
        }
        else // key name
        {
            int ii=0;
            const char *str = keybind + i;
            bool found = false;
            while(keyNames[ii].keyName)
            {
                if (!strncmp(keyNames[ii].keyName, str, strlen(keyNames[ii].keyName)))
                {
                    c[len] = keyNames[ii].ckey;
                    found = true;
                    break;
                }
                ii++;
            }
            if (!found)
                return false;
        }
        if (len == MAX_KEYBIND_LEN)
            break;
        len++;

        i = wordend;
        if (!keybind[i])
            break;

        i = skipWhiteSpace(keybind, i+1);
        if (!keybind[i])
            break;
    }
    return true;
}

bool GeekBind::KeyBind::operator<(const GeekBind::KeyBind& a) const
{
    return keyToStr() < a.keyToStr();
}

GeekBind::GeekBind(std::string _name):
    isActive(true),
    name(_name)
{
    curKey.len = 0;
    needSort = false;
}

bool GeekBind::isBinded(KeyBind b)
{
    std::vector<KeyBind>::iterator it;
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        if (b.len != it->len)
            continue;
        bool eq = true;
        for (int i = 0; i < b.len; i++)
        {
            if (b.mod[i] != it->mod[i])
            {
                eq = false;
                break;
            }
            if (b.c[i] != it->c[i])
            {
                eq = false;
                break;
            }
        }
        if (eq)
            return true;
    }
    return false;
}

std::string GeekBind::getBinds(std::string funName)
{
    std::vector<KeyBind>::iterator it;
    std::string str;
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        if (funName == it->gcFunName ||
            (funName == it->firstParam && it->gcFunName.empty()))
            if (str.empty())
                str = it->keyToStr();
            else
                str += ", " + it->keyToStr();
    }
    return str;
}

std::string GeekBind::getBindDescr(std::string keybind)
{
    std::vector<KeyBind>::iterator it;
    KeyBind k;
    k.set(keybind.c_str());
    std::string str = k.keyToStr();
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        if (str == it->keyToStr())
            return it->keyToStr() + " (" + it->gcFunName
                + ") " + it->params ;
    }
    return "";
}

std::string GeekBind::getFunName(std::string keybind)
{
    std::vector<KeyBind>::iterator it;
    KeyBind k;

    k.set(keybind.c_str());
    std::string str = k.keyToStr();
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        if (str == it->keyToStr())
            return it->gcFunName;
    }
    return "";
}

std::string GeekBind::getFirstParam(std::string keybind)
{
    std::vector<KeyBind>::iterator it;
    KeyBind k;

    k.set(keybind.c_str());
    std::string str = k.keyToStr();
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        if (str == it->keyToStr())
            return it->firstParam;
    }
    return "";
}

std::string GeekBind::getParams(std::string keybind)
{
    std::vector<KeyBind>::iterator it;
    KeyBind k;
    k.set(keybind.c_str());
    std::string str = k.keyToStr();
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        if (str == it->keyToStr())
            return it->params;
    }
    return "";
}

std::vector<std::string> GeekBind::getAllBinds()
{
    std::vector<std::string> kb;
    std::vector<KeyBind>::iterator it;
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        kb.push_back(it->keyToStr());
    }
    return kb;
}

std::vector<GeekBind::KeyBind> GeekBind::getAllBinds(std::string funName)
{
    std::vector<GeekBind::KeyBind> kb;
    std::vector<GeekBind::KeyBind>::iterator it;
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        if (funName == it->gcFunName ||
            (funName == it->firstParam && it->gcFunName.empty()))
            kb.push_back(*it);
    }
    return kb;
}

const std::vector<GeekBind::KeyBind>& GeekBind::getBinds()
{
    if (needSort)
        sort(binds.begin(), binds.end());
    needSort = false;
    return binds;
}

bool GeekBind::bind(const char *keybind, std::string funName)
{
    KeyBind k;
    if (!k.set(keybind))
        return false;
    k.gcFunName = funName;

    // get first param of params of keybind
    string::size_type startpos = 0, endpos;
    if (!k.params.empty())
    {
        if (k.params[0] == '#')
            startpos = k.params.find_first_not_of('#');
        if (startpos != string::npos)
            endpos = k.params.find_first_of('#', startpos);
        k.firstParam = string(k.params, startpos, endpos - 1);
    }

    unbind(keybind);
    binds.push_back(k);
    needSort = true;
    return true;
}

void GeekBind::unbind(const char *keybind)
{
    std::vector<KeyBind>::iterator it;
    KeyBind b;
    b.set(keybind);
    for (it = binds.begin();
         it != binds.end(); it++)
    {
        if (b.len != it->len)
            continue;
        bool eq = true;
        for (int i = 0; i < b.len; i++)
        {
            if (b.mod[i] != it->mod[i])
            {
                eq = false;
                break;
            }
            if (b.c[i] != it->c[i])
            {
                eq = false;
                break;
            }
        }
        if (eq)
        {
            binds.erase(it);
            break;
        }
    }
    return;
}
