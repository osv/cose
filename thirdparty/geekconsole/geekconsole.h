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

#ifndef _GEEKCONSOLE_H_
#define _GEEKCONSOLE_H_

#include <map>
#include <vector>
#include <string>
#include <celestia/celestiacore.h>
#include <celx.h>
#include <celx_internal.h>
#include "gcoverlay.h"
#include "geekbind.h"

#define MAX_HISTORY_SYZE 128

#define CTRL_A '\001'
#define CTRL_B '\002'
#define CTRL_C '\003'
#define CTRL_D '\004'
#define CTRL_E '\005'
#define CTRL_F '\006'
#define CTRL_G '\007'
#define CTRL_H '\010'
#define CTRL_I '\011'
#define CTRL_J '\012'
#define CTRL_K '\013'
#define CTRL_L '\014'
#define CTRL_M '\015'
#define CTRL_N '\016'
#define CTRL_O '\017'
#define CTRL_P '\020'
#define CTRL_Q '\021'
#define CTRL_R '\022'
#define CTRL_S '\023'
#define CTRL_T '\024'
#define CTRL_U '\025'
#define CTRL_V '\026'
#define CTRL_W '\027'
#define CTRL_X '\030'
#define CTRL_Y '\031'
#define CTRL_Z '\032'

inline std::vector<std::string> splitString(std::string str, std::string delim ){
    std::vector<std::string> result;
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
    Color32(GLubyte r, GLubyte g, GLubyte b)
        {
            rgba[0]= r;
            rgba[1]= g;
            rgba[2]= b;
            rgba[3]= 255;
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
    virtual void Interact(GeekConsole *gc, std::string historyName);
    virtual void charEntered(const char *c_p, int modifiers);
    virtual void mouseWheel(float motion, int modifiers);
    virtual void mouseButtonDown(float x, float y, int button);
    virtual void mouseButtonUp(float x, float y, int button);
    virtual void mouseMove(float dx, float dy, int modifiers);
    virtual void mouseMove(float x, float y);

    // called by gc
    virtual void cancelInteractive();
    /* You must render Interactive by using GeekConsole's overlay
     * overlay->beginText() called before call renderInteractive.
     */
    virtual void renderInteractive();
    /* render with default font gc->getCompletionFont()
     */
    virtual void renderCompletion(float height, float width);
    /* Update: describe current buftext.
       Will called after charEntered
     */
    virtual void update(const std::string &buftext);
    /* set def value in buffer
     */
    virtual void setDefaultValue(std::string v);
    /* set to buffer str from last value in history
     */
    virtual void setLastFromHistory();
    std::string getBufferText()const
        {return buf;}

    // get size of completion, -1 for need max as possible
    virtual int getBestCompletionSizePx();

    virtual std::string getHelpText();

    virtual std::string getStateHelpTip();

    // default value that can be set by C-r
    std::string defaultValue;
protected:
    GeekConsole *gc;
    std::string separatorChars; // additional separator chars

    std::vector<std::string> typedHistoryCompletion;

    //size of buf without expanded part (by history or completion)
    uint bufSizeBeforeHystory;
    void setBufferText(std::string str);
private:
    void prepareHistoryCompletion();
    std::string buf;
    std::string interactiveName;
    bool useHistory;
    typedef std::map<std::string, std::vector<std::string>, CompareIgnoringCasePredicate> History;
    History history;
    std::string curHistoryName;
    int typedHistoryCompletionIdx;
};


// Interactive with autocompletion list
class ListInteractive: public GCInteractive
{
public:
    enum CompletionType
    {
        Standart,
        Fast,
        Filter
    };

    ListInteractive(std::string name):GCInteractive(name, true), cols(4)
        {};
    void Interact(GeekConsole *_gc, std::string historyName);
    virtual void setRightText(std::string str);
    // return text that start after last separator chars
    virtual std::string getRightText()const;
    // return text before last separator chars
    virtual std::string getLeftText()const;
    bool tryComplete();
    void charEntered(const char *c_p, int modifiers);
    void charEnteredFilter(const char *c_p, int modifiers);
    void mouseWheel(float motion, int modifiers);
    void mouseButtonDown(float x, float y, int button);
    void mouseMove(float x, float y);
    // return index of picked completion item, -1 - no item
    int  pick(float x, float y);
    virtual void renderInteractive();
    virtual void renderCompletion(float height, float width);
    void update(const std::string &buftext);
    void playMatch(); // play match snd if textbuffer is in completion list
    void setCompletion(std::vector<std::string> completion);
    void setMatchCompletion(bool mustMatch); // true - result must be matched in completion. Default false
    void setCompletionFromSemicolonStr(std::string); // add completion by str like "yes;no"
    void setColumns(int c = 4)
        {
            cols = c; if (cols < 1) cols = 1; if (cols > 8) cols = 8;
            maxColumns = cols;}
    // set auto calc size for columns based on num of chars @sz
    void setColumnsCh(int sz, int _maxColumns = 8)
        {columnSizeInChar = sz; maxColumns = _maxColumns;}
    void setColumnsMax(int _maxColumns = 8)
        {maxColumns = _maxColumns;}
    int getBestCompletionSizePx();
    std::string getHelpText();
    std::string getStateHelpTip();

protected:
    virtual void updateTextCompletion();

    std::vector<std::string> completionList;
    int pageScrollIdx;
    uint scrollSize; // number of compl. items to scroll
    int completedIdx;
    uint cols; // num of columns of completion (cached)
    std::vector<std::string> typedTextCompletion;
    bool mustMatch; // if true - on RET key finish promt only if matched in completionList
    uint nb_lines; // cached number of lines of completion (recalc when render)
private:
    // render completion: standart incremental
    void renderCompletionStandart(float height, float width);
    // filtered renderer of completion
    void renderCompletionFilter(float height, float width);
    // only for Filter type, indicate that inter may be finished by RET key
    bool canFinish;

    // if > 0 calculate cols each time on render completion, cols can't be > 8
    int columnSizeInChar;
    int maxColumns; // used with columnSizeInChar as max cols
    int lastWidth;
};

// celestia's object prompter
class CelBodyInteractive: public ListInteractive
{
public:
    CelBodyInteractive(std::string name, CelestiaCore *celApp);
    void Interact(GeekConsole *_gc, std::string historyName);
    void charEntered(const char *c_p, int modifiers);
    void mouseButtonDown(float, float, int);
    void mouseMove(float, float);
    void update(const std::string &buftext);
    void setCompletion(std::vector<std::string> completion);
    void setCompletionFromSemicolonStr(std::string);
    void cancelInteractive();
    int getBestCompletionSizePx()
        { return -1;}
    std::string getHelpText();
private:
    void updateTextCompletion();
    CelestiaCore *celApp;

    // Selesction  is  string name  not  class  Selection, because  in
    // future in  game, objects  may be destroyed  so it  will prevent
    // segfault.

    // TODO: seems that celestia will have dynamic load/unload addons,
    // maybe  this will  be not  problem with  shared  pointers.  Need
    // optimisation for code replacing to Selection instead string.

    // name of last selection
    std::string lastCompletionSel;
    void _updateMarkSelection(const std::string &text);
    // selection before interactive do any sel changes
    std::string firstSelection;
};


class FlagInteractive: public ListInteractive
{
public:
    FlagInteractive(std::string name): ListInteractive(name), defDelim("/;") {};

    void Interact(GeekConsole *_gc, std::string historyName);
    void setSeparatorChars(std::string);
    void charEntered(const char *c_p, int modifiers);
    std::string getDefaultDelim() const
        { return defDelim; }
    std::string getHelpText();

private:
    void updateTextCompletion();
    std::string defDelim;
};

class KeyInteractive: public GCInteractive
{
public:
    enum {
        ACTVE_KEYS      = 2, // only math keys from active bind spaces
        NOT             = 4,
        ANY             = 8,
    };

    KeyInteractive(std::string name): GCInteractive(name, false) {};

    void Interact(GeekConsole *_gc, std::string historyName);
    void charEntered(const char *c_p, int modifiers);
    void update(const std::string &buftext);
    void setKeyMatching(int);
    std::string getHelpText();
    std::string getStateHelpTip();
private:
    GeekBind::KeyBind curKey; // current key sequence
    int key_match;
};

class FileInteractive: public ListInteractive
{
public:
    FileInteractive(std::string name): ListInteractive(name) {};
    void Interact(GeekConsole *_gc, std::string historyName);
    void setFileExtenstion(std::string);
    void setRightText(std::string str);
    void update(const std::string &buftext);
    void charEntered(const char *c_p, int modifiers);
    int getBestCompletionSizePx()
        { return -1;}
    string getHelpText();
    /* Set dir, entire - root dir; use it in inter-callback */
    void setDir(std::string dir, std::string entire = "./");
private:
    void updateTextCompletion();
    std::vector<std::string> fileExt;
    std::string dirEntire; // root dir entire (not show for user)
    std::vector<std::string> dirCache;
    std::string lastPath;
    bool rebuildDirCache;
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
    string getHelpText();
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
        LuaNamed, // str of lua fun name
        Alias,
        Nill
    };
    GCFunc():
        type(Nill) {};
    // constructor for c function
    GCFunc(CFunc cfun, std::string doc = ""): type(C), cFun(cfun), vFun(NULL),  info(doc){};
    // simple c void callback
	GCFunc(CFuncVoid vfun, std::string doc = ""): type(Cvoid), cFun(NULL), vFun(vfun), info(doc){};
    // constructor for name of lua function
    GCFunc(const char *name, lua_State* l, std::string doc = ""): type(LuaNamed), luaFunName(name),
                                                                  lua(l), info(doc){};
    // constructor for lua callback
    GCFunc(const int callBackRef, lua_State* l, std::string doc = ""): type(Lua), luaCallBack(callBackRef),
                                                                       lua(l), info(doc){};
    // constructor for alias to function with some preset parameters
    GCFunc(const char *aliasTo, const char *parameters, std::string doc = "", bool arch = false):
        type(Alias),
        aliasfun(aliasTo),
        params(parameters),
        info(doc),
        archive(arch){};
    int call(GeekConsole *gc, int state, std::string value);

    GCFuncType const getType() { return type; }
    std::string const getAliasFun() { return aliasfun; }
    std::string const getAliasParams() { return params; }
    std::string const getInfo() { return info; }
    bool const isAliasArchive () { return archive; }

private:
    GCFuncType type;
    CFunc cFun;
	CFuncVoid vFun; // simple void func()
    std::string luaFunName;
    int luaCallBack;
    lua_State* lua;
    std::string aliasfun; // alias to function
    std::string params; // parameters to alias functions
    std::string info; // documentation of this function
    bool archive; // alias may be saved by GeekConsole::createAutogen
};


class GeekConsole
{
    friend class GCFunc;

public:
    // beep callbacks
    struct Beep {
        virtual void beep() {};
        virtual void match() {};
    };

    enum ConsoleType
    {
        Tiny = 1,
        Medium = 2,
        Big = 3
    };

    GeekConsole(CelestiaCore *celCore);
    ~GeekConsole();
    void createAutogen(const char *filename = "gcautogen.celx", bool update = true);
    int execFunction(std::string funName);
    int execFunction(std::string funName, std::string param);
    // call callback fun for describe interactive text with state (-funstate - 1)
    void describeCurText(std::string text);
    void registerFunction(GCFunc fun, std::string name);
    void reRegisterFunction(GCFunc fun, std::string name);
    GCFunc *getFunctionByName(std::string);
    std::vector<std::string> getFunctionsNames();
    bool charEntered(const char *c_p, int modifiers);
    bool keyDown(int key, int modifiers);
    bool keyUp(int key, int modifiers);
    bool mouseWheel(float motion, int modifiers);
    bool mouseButtonDown(float x, float y, int button);
    bool mouseButtonUp(float x, float y, int button);
    bool mouseMove(float dx, float dy, int modifiers);
    bool mouseMove(float x, float y);
    float mouseYofInter(float y); // return y for interactive
    void resize(int w, int h)
        {
            overlay->setWindowSize(w, h);
            width = w; height = h;
            cachedCompletionH = -1;
        };
    void recalcCompletionHeight()
        {cachedCompletionH = -1;}
    int const getWidth() {return width;}
    int const getHeight() {return height;}
    void render(double time);
    bool isVisible;
    int32 consoleType;

    TextureFont* getInteractiveFont();
    TextureFont* getCompletionFont();

    GCOverlay* getOverlay() const
        {return overlay;}
    void setInteractive(GCInteractive *Interactive,
                        std::string historyName = "",  // if empty - dont use history
                        std::string InteractiveStr = "", // str before prompt
                        std::string descrStr = ""); // describe str (bottom)
    // on "ENTER" press event
    void InteractFinished(std::string value);
    void finish(); // hide console
    CelestiaCore *getCelCore() const
        {return celCore;}
    GeekBind *createGeekBind(std::string bindspace); // return existen or new binder
    GeekBind *getGeekBind(std::string bindspace);
    void registerAndBind(std::string bindspace, const char *bindkey,
                         GCFunc fun, const char *funname, bool archive = false);
    const std::vector<GeekBind *>& getGeekBinds()
        { return geekBinds;}
    bool bind(std::string bindspace, std::string bindkey, std::string function, bool archive = false);
    void unbind(std::string bindspace, std::string bindkey);
    GCInteractive *getCurrentInteractive() const
        {return curInteractive;}
    void setInfoText(std::string);
    void clearInfoText();

    // enable/disable macro recording
    void setMacroRecord(bool enable, bool quiet = false);
    bool isMacroRecord() const
        { return isMacroRecording; }
    void callMacro();
    std::string getCurrentMacro() const
        { return currentMacro; }
    void appendCurrentMacro(std::string);
    void appendDescriptionStr(std::string);
    GCInteractive *getCurInteractive()
        { return curInteractive; }
    void setBeeper(Beep *);
    Beep *getBeeper()
        { return beeper; }
    void beep();
    void showText(std::string s, double duration = 2.5);
    // last real time
    double getLastTick() const
        { return lastTickTime; }

    // after finish interactive need to return to last info node
    void setReturnToLastInfoNode(bool r = true)
        { isInfoInterCall = r; }
    // descript. prefix in before interactive prompt
    std::string InteractivePrefixStr;
    // bottom description str
    std::string descriptionStr;
private:
    int execFunction(GCFunc *fun);

    // call current fun, and recall if state returned by fun less
    int call(const std::string &value, bool params = false);
    TextureFont* titleFont;
    TextureFont* font;

    CelestiaCore *celCore;
    GCOverlay* overlay;
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

    std::vector<std::string> infoText; // lines of info text
    uint infoWidth; // max width in pixel of infoText

    std::string lastMacro;
    std::string currentMacro;
    bool isMacroRecording;
    uint maxMacroLevel;
    uint curMacroLevel;
    Beep *beeper;
    bool mouseDown;

    int cachedCompletionH; // Height for renderer of completion
    int cachedCompletionRectH; // Real completion rect height
    int lastMX, lastMY; // last mouse coord

    std::vector<std::string> helpText; // lines of info helpText
    uint helpTextWidth; // max width in pixel of helpText
    void setHelpText(std::string);

    std::string messageText;
    double messageStart;
    double lastTickTime;
    double messageDuration; // < 0 for infinity

    // true if  function is called from  infoInteractive. After finish
    // this need to return back to info
    bool isInfoInterCall;
};

// convert control, '"', '\' chars to \X or \DDD i.e. to readable lua string
extern std::string normalLuaStr(std::string);

/* Selection::getName  return  number of  DSO  or  star, this  version
  return full name of selection */
extern std::string getSelectionName(Selection sel);

// Split @filter by ' ' and return only those items that have all splited parts.
extern void filterVector(const std::vector<std::string> &src,
                         std::vector<std::string> &dst, const std::string filter);

// init geekconsole
extern void initGeekConsole(CelestiaCore *celApp);

extern void shutdownGeekconsole();

extern GeekConsole *getGeekConsole();

// set and show *help* info node
extern void showHelpNode(std::string node_content);

extern Color getColorFromText(const std::string &text);
extern Color32 getColor32FromText(const std::string &text);
extern std::string getColorName(const Color32 &color);
extern char removeCtrl(char ckey);
extern char toCtrl(char key);

/* Init/destroy interactives */

extern void initGCInteractives(GeekConsole *gc);
extern void destroyGCInteractives();

/* Setup celestia's interactive's functions */
extern void initGCStdInteractivsFunctions(GeekConsole *gc);
/* Setup default celestia binds */
extern void initGCStdCelBinds(GeekConsole *gc, const char *bindspace = "global");

/* Setup lua api for working with geekconsole */
extern void LoadLuaGeekConsoleLibrary(lua_State* l);

/* Interactives */

extern ListInteractive *listInteractive;
extern PasswordInteractive *passwordInteractive;
extern CelBodyInteractive *celBodyInteractive;
extern ColorChooserInteractive *colorChooserInteractive;
extern FlagInteractive *flagInteractive;
extern FileInteractive *fileInteractive;
extern KeyInteractive *keyInteractive;

/* describeselection is used celestia body interactive.
   You can setup own describer
*/
typedef std::string (*CustomDescribeSelection)(Selection sel,
                                               CelestiaCore *celAppCore);

/* Describe selection.
   @param doCustomDescribe use custom describer if exist */
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
extern Color32 *clInfoTextFnt;
extern Color32 *clInfoTextBg;
extern Color32 *clInfoTextBrd;

// global completion style for Listinteractive and other
extern int completionStyle;

#endif // _GEEKCONSOLE_H_
