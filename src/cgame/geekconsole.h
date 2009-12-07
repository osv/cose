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

/*
  GeekConsole

  GeekConsole used for interacting using callback function:

  int _somefun(GeekConsole *gc, int state, std::string value)
  {
  switch (state)
  {
  case 1:
  if (value == "yes")
  exit(1);
  else
  gc->finish(); // *finish* interacting and hide console
  case 0:
  // setup interactive
  gc->setInteractive(listInteract, "quit", "Are You Sure?", "Quit from game");
  listInteractive->setCompletionFromSemicolonStr("yes;no");
  listInteractive->setMatchCompletion(true); // default false
  }
  return state;
  }

  So you can call console's interactive:

  GCFunc somefun(_somefun);
  geekConsole->execFunction(&_somefun);

  Function _somefun will be called by geekconsole, and you must call
  some interacter in this.

  You can create simple "void" callback:

  void toggleFullScreen()
  {
  // ... just toggle, no need geekconsole.finish() or other.
  }

  and register this
  geekConsole->registerFunction(GCFunc(ToggleFullscreen), "toggle fullscreen");

  To set key bind for some func use bind:

  geekConsole->bind("Global", "M-RET",
  "toggle fullscreen");

  or register and bind at once:

  geekConsole->registerAndBind("Global", "C-x f",
  GCFunc(ToggleFullscreen), "toggle fullscreen");

  you can use void bindspace that's mean "Global":

  geekConsole->bind("", "M-RET",
  "toggle fullscreen");

  For key bind there are some emacs's suggestion:

  M- alt (not true meta key)
  C- Ctrl
  S- Shift
  RET enter
  etc

  Special keys (F1, etc) not processed.

  For first exec of fun state is 0.
  For describe current entered text callback is called with state (-currentState - 1)
  State will inc by 1 before GCInteractive finished.

  Key binds:
  C-s............... Change size of console
  C-g............... Cancel interactive
  Esc............... Cancel interactive
  Ctrl+p............ Prev in history
  Ctrl+n............ Next in history
  Ctrl+z............ Remove expanded part from input
  Ctrl+w............ Kill backword
  Ctrl+u............ Kill whole line
  TAB............... Try complete or scroll completion
  Alt+/............. Expand next
  Alt+?............. Expand prev
  Alt+c............. cel object promt: select & center view
*/

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
    virtual void Interact(GeekConsole *gc, string historyName);
    virtual void charEntered(const wchar_t wc, int modifiers);
    virtual void cancelInteractive();
    /* You must render Interactive by using GeekConsole's overlay
     * because overlay->beginText() called before call renderInteractive.
     */
    virtual void renderInteractive();
    /* render with default font gc->getCompletionFont()
     */
    virtual void renderCompletion(float height, float width);
    GeekConsole *gc;
    typedef std::map<std::string, std::vector<std::string>, CompareIgnoringCasePredicate> History;
    History history;
    std::string curHistoryName;
    uint typedHistoryCompletionIdx;
    void prepareHistoryCompletion();
    std::vector<std::string> typedHistoryCompletion;
    uint bufSizeBeforeHystory; //size of buf befor apply mathed str from hist. need for restore prev buf
    void setBufferText(std::string str);
    std::string getBufferText()const
        {return buf;}
private:
    std::string buf;
    std::string InteractiveName;
    bool useHistory;
};


// Interactive with autocompletion list
class ListInteractive: public GCInteractive
{
public:
    ListInteractive(std::string name):GCInteractive(name, true), cols(4)
        {};
    void Interact(GeekConsole *_gc, string historyName);
    bool tryComplete();
    void charEntered(const wchar_t wc, int modifiers);
    void renderCompletion(float height, float width);
    void setCompletion(std::vector<std::string> completion);
    void setMatchCompletion(bool mustMatch); // true - result must be matched in completion. Default false
    void setCompletionFromSemicolonStr(std::string); // add completion by str like "yes;no"
    void setColumns(int c = 4)
        {if (c >0 && c < 8) cols = c;}
protected:
    void updateTextCompletion();
    std::vector<std::string> completionList;
    uint pageScrollIdx;
    uint scrollSize; // number of compl. items to scroll
    uint completedIdx;
    int cols;
    std::vector<std::string> typedTextCompletion;
    bool mustMatch; // if true - on RET key finish promt only if matched in completionList
};

class CelBodyInteractive: public GCInteractive
{
public:
    CelBodyInteractive(std::string name, CelestiaCore *celApp);
    void Interact(GeekConsole *_gc, string historyName);
    bool tryComplete();
    void charEntered(const wchar_t wc, int modifiers);
    void renderCompletion(float height, float width);
    void setRightText(std::string str);
    std::string getRightText()const; // return text before after "/"
    void updateDescrStr(); // update descr str of console with info about body
    void cancelInteractive();
    void setColumns(int c = 4)
        {if (c >0 && c < 8) cols = c;}
private:
    void updateTextCompletion();
    std::vector<std::string> completionList;
    uint pageScrollIdx;
    uint scrollSize; // number of compl. items to scroll
    uint completedIdx;
    int cols;
    std::vector<std::string> typedTextCompletion;
    CelestiaCore *celApp;
    Selection lastCompletionSel;
};

// no history + ****** Interactive
class PasswordInteractive: public GCInteractive
{
public:
    PasswordInteractive(std::string name):GCInteractive(name, true)
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
        {};
    ~ColorChooserInteractive()
        {};
    void Interact(GeekConsole *_gc, string historyName);
    void renderInteractive();
    void renderCompletion(float height, float width);
};

// container for C and lua function
typedef int (* CFunc)(GeekConsole *gc, int state, std::string value);
typedef void (* CFuncVoid)();

class GCFunc
{
public:
    GCFunc():
        isNil(true) {};
    // constructor for c function
    GCFunc(CFunc cfun): isLuaFunc(false), cFun(cfun), vFun(NULL), isNil(false){};
	GCFunc(CFuncVoid vfun): isLuaFunc(false), cFun(NULL), vFun(vfun), isNil(false){};
    // constructor for name of lua function
    GCFunc(const char *name, lua_State* l): isLuaFunc(true), luaFunName(name),
                                            isNil(false), lua(l){};
    int call(GeekConsole *gc, int state, std::string value);
private:
    bool isLuaFunc;
    CFunc cFun;
	CFuncVoid vFun; // simple void func()
    string luaFunName;
    bool isNil;
    lua_State* lua;
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
    void execFunction(GCFunc *fun);
    bool execFunction(std::string funName);
    bool execFunction(std::string funName, std::string param);
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
                        std::string InteractiveStr, // str before prompt
                        std::string descrStr); // describe str (bottom)

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
    // descript. prefix in before interactive prompt
    std::string InteractivePrefixStr;
    // bottom description str
    std::string descriptionStr;
private:
    // call current fun, and recall if state returned by fun less
    void call(const std::string &value);
    TextureFont* titleFont;
    TextureFont* font;

    CelestiaCore *celCore;
    Overlay* overlay;
    int width; // width of viewport
    int height; // height of viewport

    GCInteractive *curInteractive;
    GCFunc *curFun;
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

/* describeselection is used celestia body interactive.
   You can setup own describer
*/
typedef std::string (*CustomDescribeSelection)(Selection sel,
                                               CelestiaCore *celAppCore);

extern std::string describeSelection(Selection sel, CelestiaCore *celAppCore, bool doCustomDescribe = true);

extern CustomDescribeSelection customDescribeSelection;

// direcory to store history (default = "history/")
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
