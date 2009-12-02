#ifndef _GEEKBIND_H_
#define _GEEKBIND_H_

/* Simple container for emacs-style hot-keys
   Hot key are linked to geekconsole function
   example of hot-key binding:

   bind("C-x C-g e @Earth", "select object");

   - Here "C-" is Control key, and as like in emacs here is S-, M- (Alt only, not ESC!)
   - "@Earth" is value passed to function-interactive "select object".

   GeekConsole allow multiple call of interactives by using @EXEC value. Example:

   bind(C-x M-x @select object@Earth/Moon@EXEC@goto, "exec function");
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
    struct KeyBind
    {
        char c[MAX_KEYBIND_LEN];
        int  mod[MAX_KEYBIND_LEN];
        std::string gcFunName;
        std::string params;
        int len;
        std::string keyToStr();
        bool set(const char *bind);
    };
    GeekBind(std::string name);
    ~GeekBind() {};
    bool isBinded(KeyBind);
    // coma sep. list of k. binds for specified fun
    std::string getBinds(std::string funName);
    std::string getBindDescr(std::string keybind);
    std::vector<std::string> getAllBinds();
    const std::vector<KeyBind>& getBinds()
        {return binds;}
    const std::string getName()
        { return name;}
    bool bind(const char *keybind, std::string funName);
    void unbind(const char *keybind);
    // for using in geeckonsole to decide to where dispatch keydown even
    bool isActive;
private:
    std::vector<KeyBind> binds;
    KeyBind curKey;
    std::string name;
};

#endif // _GEEKBIND_H_
