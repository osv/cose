#include "geekconsole.h"
#include "geekbind.h"
#include <cstdlib>

int skipWhiteSpace(const char *str, int offset)
{
	while (str[offset] && str[offset] == ' ')
		offset++;
	return offset;
}

int skipWord(const char *str, int offset)
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
	{NULL, NULL}
};

std::string GeekBind::KeyBind::keyToStr()
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
        if (keybind[i] == '@') //is params
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
		}
		else // key name
		{
			int ii=0;
			const char *str = keybind + i;
			while(keyNames[ii].keyName)
			{
   				if (!strncmp(keyNames[ii].keyName, str, strlen(keyNames[ii].keyName)))
				{
					c[len] = keyNames[ii].ckey;
					break;
				}
				ii++;
			}
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

GeekBind::GeekBind(std::string _name,GeekConsole *_gc):
    isActive(true),
	gc(_gc),
    name(_name)
{
	curKey.len = 0;
}

GeekBind::GBRes GeekBind::charEntered(char sym, int modifiers)
{
    if (!gc)
        return NOTMATCHED;
	bool renderKeyHint = true;
	curKey.c[curKey.len] = sym;
	curKey.mod[curKey.len] = modifiers;
	curKey.len++;
	if (modifiers & SHIFT)
		curKey.mod[curKey.len] |= SHIFT;
	if (modifiers & CTRL)
		curKey.mod[curKey.len] |= CTRL;
	if (modifiers & META)
		curKey.mod[curKey.len] |= META;
	bool isKeyPrefix = false;
	std::vector<KeyBind>::iterator it;
	for (it = binds.begin();
		 it != binds.end(); it++)
	{
		if (curKey.len > it->len)
			continue;
		bool eq = true;
		for (int i = 0; i < curKey.len; i++)
		{
			if (curKey.mod[i] != it->mod[i])
			{
				eq = false;
				break;
			}
			if (curKey.c[i] != it->c[i])
			{
				eq = false;
				break;
			}
		}
		if (eq)
			if (curKey.len == it->len)
			{
				if (gc->execFunction(it->gcFunName, it->params))
				{
                    if (curKey.len > 1)
                        gc->getCelCore()->flash(curKey.keyToStr() + " (" + it->gcFunName + ") " + it->params, 2.5);
					curKey.len = 0;
				}
				else
				{
					gc->getCelCore()->flash(curKey.keyToStr() + " (" + it->gcFunName + ") not defined", 1.5);
					curKey.len = 0;
				}
				return KEYMATCH;
			}
			else
				isKeyPrefix = true;
	}
	if (!isKeyPrefix || curKey.len == MAX_KEYBIND_LEN)
		if (curKey.len > 1)
		{
			gc->getCelCore()->flash(curKey.keyToStr() + " is undefined");
			curKey.len = 0;
			return KEYPREFIX;
		}
		else
		{
			renderKeyHint = false;
			curKey.len = 0;
			return NOTMATCHED;
		}
	if (renderKeyHint)
		gc->getCelCore()->flash(curKey.keyToStr(), 2.5);
	return KEYPREFIX;
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
        if (funName == it->gcFunName && it->params.empty())
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

bool GeekBind::bind(const char *keybind, std::string funName)
{
	KeyBind k;
	if (!k.set(keybind))
		return false;
	k.gcFunName = funName;
    unbind(keybind);
	binds.push_back(k);
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
