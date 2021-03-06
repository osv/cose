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

#ifndef _GEEKBIND_H_
#define _GEEKBIND_H_

/* Simple container for emacs-style hot-keys
   Hot key are linked to geekconsole function
   example of hot-key binding:

   bind("C-x C-g e #Earth", "select object");

   or

   bind("C-x C-g e #select object#Earth", "");

   - Here "C-" is Control key, and as like in emacs here is S-, M- (Alt only, not ESC!)
   - "#Earth" is value passed to function-interactive "select object".

   GeekConsole allow  multiple call of interactives  by using #*EXEC*#
   value. Example:

   bind(C-x M-x #select object#Earth/Moon#*EXEC*#goto, "exec function");
 */

#ifndef MAX_KEYBIND_LEN
#define MAX_KEYBIND_LEN 8
#endif

class GeekBind
{
public:
    enum KBMod
    {
        SHIFT  = 1,
        CTRL   = 2,
        META   = 8, // ALT, ESC
    };

    // Results comparison of key sequence
    enum BindStatus
    {
        BINDED,
        IS_PREFIX,
        NOT_BINDED
    };

    struct KeyBind
    {
        char c[MAX_KEYBIND_LEN];
        int  mod[MAX_KEYBIND_LEN];
        /// function name, may be empty and than this.firstParam is function
        std::string gcFunName;
        // first param of splited param by '#' char
        std::string firstParam;
        std::string params;
        int len;
        std::string keyToStr() const;
        bool set(const char *bind);
        bool operator<(const KeyBind&) const;
        bool archive; // does bind must be saved
    };
    GeekBind(std::string name);
    ~GeekBind() {};
    BindStatus isBinded(KeyBind);
    BindStatus isBinded(string &key);
    // coma sep. list of k. binds for specified fun
    std::string getBinds(std::string funName);
    std::string getBindDescr(std::string keybind);
    std::string getFunName(std::string keybind);
    std::string getFirstParam(std::string keybind);
    std::string getParams(std::string keybind);
    std::vector<std::string> getAllBinds();
    /// return bind for function
    std::vector<KeyBind> getAllBinds(std::string funName);
    const std::vector<KeyBind>& getBinds();
    const std::string getName()
        { return name;}
    bool bind(const char *keybind, std::string funName, bool arch = false);
    void unbind(const char *keybind);
    // for using in geeckonsole to decide to where dispatch keydown even
    bool isActive;
private:
    std::vector<KeyBind> binds;
    KeyBind curKey;
    std::string name;
    bool needSort;
};

// keys that can not be shifted like ~!@...
extern const char *nonShiftChars;

#endif // _GEEKBIND_H_
