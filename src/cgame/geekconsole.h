#ifndef _GEEKCONSOLE_H_
#define _GEEKCONSOLE_H_

#include <map>
#include <vector>
#include <string>
#include <celengine/overlay.h>
#include <celestia/celestiacore.h>
#include <celx.h>
#include <celx_internal.h>
#include "geekbind.h"

#define MAX_HISTORY_SYZE 128

inline vector<string> splitString( string str, string delim ){
    vector<string> result;
    uint cutAt;
    while( (cutAt = str.find_first_of( delim )) != str.npos ){
        if( cutAt > 0 ){
            result.push_back( str.substr( 0, cutAt ) );
        }
        str = str.substr( cutAt + 1 );
    }
    if( str.length() > 0 ){
        result.push_back( str );
    }
    return result;
}


class GeekConsole;
class GCFunction;
class GCInteractive;

// colors for theming
struct Color32
{
    Color32(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
        {
            rgba[0]= r;
            rgba[1]= g;
            rgba[2]= b;
            rgba[3]= a;
        }
    Color32(): i(0) {};
    union
    {
        GLubyte rgba[4];
        uint32 i;
    };
};

/*
 * Interactive for geek console
 */
class GCInteractive
{
public:
    GCInteractive(std::string name, bool useHistory = true);
    ~GCInteractive();
    /* Reset interactive
     */
    virtual void Interact(GeekConsole *gc, string historyName);
    virtual void charEntered(const wchar_t wc, int modifiers);
    // called by gc
    virtual void cancelInteractive();
    /* You must render Interactive by using GeekConsole's overlay
     * overlay->beginText() called before call renderInteractive.
     */
    virtual void renderInteractive();
    /* render with default font gc->getCompletionFont()
     */
    virtual void renderCompletion(float height, float width);
    /* Update descibe str completion, etc.
       Will called after charEntered
     */
    virtual void update();
    /* set def value in buffer
     */
    virtual void setDefaultValue(std::string v);
    /* set to buffer str from last value in history
     */
    void setLastFromHistory();
protected:
    GeekConsole *gc;
    std::string separatorChars; // additional separator chars

    std::vector<std::string> typedHistoryCompletion;

    //size of buf without expanded part (by history or completion)
    uint bufSizeBeforeHystory;
    void setBufferText(std::string str);
    std::string getBufferText()const
        {return buf;}
private:
    void prepareHistoryCompletion();
    std::string buf;
    std::string interactiveName;
    bool useHistory;
    typedef std::map<std::string, std::vector<std::string>, CompareIgnoringCasePredicate> History;
    History history;
    std::string curHistoryName;
    uint typedHistoryCompletionIdx;
};


// Interactive with autocompletion list
class ListInteractive: public GCInteractive
{
public:
    ListInteractive(std::string name):GCInteractive(name, true), cols(4)
        {};
    void Interact(GeekConsole *_gc, string historyName);
    virtual void setRightText(std::string str);
    virtual std::string getRightText()const; // return text before after 
    bool tryComplete();
    void charEntered(const wchar_t wc, int modifiers);
    virtual void renderCompletion(float height, float width);
    void update();
    void setCompletion(std::vector<std::string> completion);
    void setMatchCompletion(bool mustMatch); // true - result must be matched in completion. Default false
    void setCompletionFromSemicolonStr(std::string); // add completion by str like "yes;no"
    void setColumns(int c = 4)
        {cols = c; if (cols < 1) cols = 1; if (cols > 8) cols = 8;}
protected:
    virtual void updateTextCompletion();
    std::vector<std::string> completionList;
    uint pageScrollIdx;
    uint scrollSize; // number of compl. items to scroll
    uint completedIdx;
    int cols;
    std::vector<std::string> typedTextCompletion;
    bool mustMatch; // if true - on RET key finish promt only if matched in completionList
};

// celestia's object prompter
class CelBodyInteractive: public ListInteractive
{
public:
    CelBodyInteractive(std::string name, CelestiaCore *celApp);
    void Interact(GeekConsole *_gc, string historyName);
    void charEntered(const wchar_t wc, int modifiers);
    void renderCompletion(float height, float width);
    void update();
    void cancelInteractive();
private:
    void updateTextCompletion();
    CelestiaCore *celApp;
    Selection lastCompletionSel;
    Selection firstSelection; // selection before interactive do any sel changes
};


class FlagInteractive: public ListInteractive
{
public:
    FlagInteractive(std::string name): ListInteractive(name), defDelim("/;") {};

    void Interact(GeekConsole *_gc, string historyName);
    void setSeparatorChars(std::string);
    string getDefaultDelim() const
        { return defDelim; }

private:
    void updateTextCompletion();
    string defDelim;
};

class FileInteractive: public ListInteractive
{
public:
    FileInteractive(std::string name): ListInteractive(name) {};
    void Interact(GeekConsole *_gc, string historyName);
    void setFileExtenstion(std::string);
    void setRightText(std::string str);
    void update();
    void charEntered(const wchar_t wc, int modifiers);
    /* Set dir, entire - root dir; use it in inter-callback */
    void setDir(std::string dir, std::string entire = "./");
private:
    void updateTextCompletion();
    std::vector<std::string> fileExt;
    std::string dirEntire; // root dir entire (not show for user)
    std::vector<std::string> dirCache;
};
// no history + ****** Interactive
class PasswordInteractive: public GCInteractive
{
public:
    PasswordInteractive(std::string name):GCInteractive(name, false)
        {};
    ~PasswordInteractive()
        {};
    void renderInteractive();
};

// color interactive
class ColorChooserInteractive: public ListInteractive
{
public:
    ColorChooserInteractive(std::string name):ListInteractive(name)
        {cols = 2;};
    void Interact(GeekConsole *_gc, string historyName);
    void renderInteractive();
    void renderCompletion(float height, float width);
};

// container for C and lua callback
typedef int (* CFunc)(GeekConsole *gc, int state, std::string value);
typedef void (* CFuncVoid)();

class GCFunc
{
public:
    enum GCFuncType
    {
        C,
        Cvoid,
        Lua,
        Alias,
        Nill
    };
    GCFunc():
        type(Nill) {};
    // constructor for c function
    GCFunc(CFunc cfun): type(C), cFun(cfun), vFun(NULL){};
    // simple c void callback
	GCFunc(CFuncVoid vfun): type(Cvoid), cFun(NULL), vFun(vfun){};
    // constructor for name of lua function
    GCFunc(const char *name, lua_State* l): type(Lua), luaFunName(name),
                                            lua(l){};
    // constructor for alias to function with some preset parameters
    GCFunc(const char *aliasTo, const char *parameters): type(Alias),
                                                       aliasfun(aliasTo), params(parameters){};
    int call(GeekConsole *gc, int state, std::string value);

    std::string const getAliasParams() { return params;
}
private:
    GCFuncType type;
    CFunc cFun;
	CFuncVoid vFun; // simple void func()
    string luaFunName;
    bool isNil;
    lua_State* lua;
    string aliasfun; // alias to function
    string params; // parameters to alias functions
};


class GeekConsole
{
public:
    enum ConsoleType
    {
        Tiny = 1,
        Medium = 2,
        Big = 3
    };

    GeekConsole(CelestiaCore *celCore);
    ~GeekConsole();

    void addPromter(std::string name,
                    GCInteractive *Interactive);
    int execFunction(GCFunc *fun);
    int execFunction(std::string funName);
    int execFunction(std::string funName, std::string param);
    // call callback fun for describe interactive text with state (-funstate - 1)
    void describeCurText(string text);
    void registerFunction(GCFunc fun, std::string name);
    void reRegisterFunction(GCFunc fun, std::string name);
    GCFunc *getFunctionByName(std::string);
    std::vector<std::string> getFunctionsNames();
    bool charEntered(const char sym, const wchar_t wc, int modifiers);
    void resize(int w, int h)
        {
            overlay->setWindowSize(w, h);
            width = w; height = h;
        };
    int const getWidth() {return width;}
    void render();
    bool isVisible;
    int32 consoleType;

    TextureFont* getInteractiveFont() const
        {return titleFont;}
    TextureFont* getCompletionFont() const
        {return font;}
    Overlay* getOverlay() const
        {return overlay;}
    void setInteractive(GCInteractive *Interactive,
                        std::string historyName,  // if empty - dont use history
                        std::string InteractiveStr = "", // str before prompt
                        std::string descrStr = ""); // describe str (bottom)

    void InteractFinished(std::string value);
    void finish();
    CelestiaCore *getCelCore() const
        {return celCore;}
    GeekBind *createGeekBind(std::string bindspace);
    GeekBind *getGeekBind(std::string bindspace);
    void registerAndBind(std::string bindspace, const char *bindkey,
                         GCFunc fun, const char *funname);
    const std::vector<GeekBind *>& getGeekBinds()
        { return geekBinds;}
    bool bind(std::string bindspace, std::string bindkey, std::string function);
    void unbind(std::string bindspace, std::string bindkey);
    GCInteractive *getCurrentInteractive() const
        {return curInteractive;}
    // descript. prefix in before interactive prompt
    std::string InteractivePrefixStr;
    // bottom description str
    std::string descriptionStr;
private:
    // call current fun, and recall if state returned by fun less
    int call(const std::string &value);
    TextureFont* titleFont;
    TextureFont* font;

    CelestiaCore *celCore;
    Overlay* overlay;
    int width; // width of viewport
    int height; // height of viewport

    GCInteractive *curInteractive;
    GCFunc *curFun;
    std::string curFunName;
    int funState;
    typedef std::map<std::string, GCFunc>  Functions;
    Functions functions;
    std::vector<GeekBind *> geekBinds; // list of key bind spaces
    GeekBind::KeyBind curKey; // current key sequence
};

extern Color getColorFromText(const string &text);

/* Init/destroy interactives */

extern void initGCInteractives(GeekConsole *gc);
extern void destroyGCInteractives();

/* Setup celestia's interactive's functions */
extern void initGCStdInteractivsFunctions(GeekConsole *gc);

/* Setup lua api for working with geekconsole */
extern void LoadLuaGeekConsoleLibrary(lua_State* l);

/* geekConsole sample */
extern GeekConsole *geekConsole;

/* Interactives */

extern ListInteractive *listInteractive;
extern PasswordInteractive *passwordInteractive;
extern CelBodyInteractive *celBodyInteractive;
extern ColorChooserInteractive *colorChooserInteractive;
extern FlagInteractive *flagInteractive;
extern FileInteractive *fileInteractive;

/* describeselection is used celestia body interactive.
   You can setup own describer
*/
typedef std::string (*CustomDescribeSelection)(Selection sel,
                                               CelestiaCore *celAppCore);

/* Describe selection.
   doCustomDescribe - use custom describer if exist*/
extern std::string describeSelection(Selection sel, CelestiaCore *celAppCore, bool doCustomDescribe = true);

extern CustomDescribeSelection customDescribeSelection;

// directory to store history (default = "history/")
extern std::string historyDir;

// color theming of gc

extern Color32 *clBackground;
extern Color32 *clBgInteractive;
extern Color32 *clBgInteractiveBrd;
extern Color32 *clInteractiveFnt;
extern Color32 *clInteractivePrefixFnt;
extern Color32 *clInteractiveExpand;
extern Color32 *clDescrFnt;
extern Color32 *clCompletionFnt;
extern Color32 *clCompletionMatchCharFnt;
extern Color32 *clCompletionAfterMatch;
extern Color32 *clCompletionMatchCharBg;
extern Color32 *clCompletionExpandBg;

#endif // _GEEKCONSOLE_H_
