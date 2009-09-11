#ifndef _GEEKBIND_H_
#define _GEEKBIND_H_

#ifndef MAX_KEYBIND_LEN
#define MAX_KEYBIND_LEN 8
#endif

class GeekConsole;

class GeekBind
{
public:
    enum KBMod
    {
		SHIFT  = 1,
		CTRL   = 2,
		META   = 8, // ALT, ESC
    };
    enum GBRes
    {
        NOTMATCHED  = 0,
        KEYPREFIX = 1,
        KEYMATCH  = 2
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
	GeekBind(std::string name, GeekConsole *gc);
    ~GeekBind() {};
	GBRes charEntered(char sym, int modifiers);
	bool isBinded(KeyBind);
    // coma sep. list of k. binds for specified fun
	std::string getBinds(std::string funName);
	std::string getBindDescr(std::string keybind);
    std::vector<std::string> getAllBinds();
    const std::string getName()
        { return name;}
	bool bind(const char *keybind, std::string funName);
    void unbind(const char *keybind);
    // for using in geeckonsole to decide to where dispatch keydown even
    bool isActive;
private:
	GeekConsole *gc;
	std::vector<KeyBind> binds;
	KeyBind curKey;
    std::string name;
};

#endif // _GEEKBIND_H_
