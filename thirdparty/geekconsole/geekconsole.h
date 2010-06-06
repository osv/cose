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
    virtual void charEntered(const char *c_p, int modifiers);
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
    virtual void setLastFromHistory();
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
    int typedHistoryCompletionIdx;
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
    void charEntered(const char *c_p, int modifiers);
    virtual void renderCompletion(float height, float width);
    void update();
    void playMatch(); // play match snd if textbuffer is in completion list
    void setCompletion(std::vector<std::string> completion);
    void setMatchCompletion(bool mustMatch); // true - result must be matched in completion. Default false
    void setCompletionFromSemicolonStr(std::string); // add completion by str like "yes;no"
    void setColumns(int c = 4)
        {cols = c; if (cols < 1) cols = 1; if (cols > 8) cols = 8;}
protected:
    virtual void updateTextCompletion();
    std::vector<std::string> completionList;
    int pageScrollIdx;
    uint scrollSize; // number of compl. items to scroll
    int completedIdx;
    uint cols;
    std::vector<std::string> typedTextCompletion;
    bool mustMatch; // if true - on RET key finish promt only if matched in completionList
};

// celestia's object prompter
class CelBodyInteractive: public ListInteractive
{
public:
    CelBodyInteractive(std::string name, CelestiaCore *celApp);
    void Interact(GeekConsole *_gc, string historyName);
    void charEntered(const char *c_p, int modifiers);
    void renderCompletion(float height, float width);
    void update();
    void setCompletion(std::vector<std::string> completion);
    void setCompletionFromSemicolonStr(std::string);
    void cancelInteractive();
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
    // selection before interactive do any sel changes
    std::string firstSelection;
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
    void charEntered(const char *c_p, int modifiers);
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

// pager, text viewer, similar to `less` command
class PagerInteractive: public GCInteractive
{
public:
    PagerInteractive(std::string name);
    void Interact(GeekConsole *_gc, string historyName);
    void charEntered(const char *c_p, int modifiers);
    void renderCompletion(float height, float width);
    void renderInteractive();
    void setText(std::string text);
    void appendText(std::string text); // append with text \n before
    void setText(std::vector<std::string> text);
    void setLastFromHistory() {};
    enum {
        PG_NOP = 0,
        PG_ENTERDIGIT = 1,
        PG_SEARCH_FWD = 2,
        PG_SEARCH_BWD = 3,
        PG_ESC = 4
    };
private:
    void forward(int);

    void processChar(const char *c_p, int modifiers);
    std::vector<std::string> *lines;
    std::vector<std::string> text;
    std::vector<std::string> helpText; // help for display
    int pageScrollIdx;
    int leftScrollIdx; //  scroll
    uint scrollSize;
    bool richedEnd;
    int state;
    int width;
    std::string lastSearchStr;
    int N;

    int chopLine;
    int windowSize;
    int halfWindowSize;
    int getChopLine();
    int getWindowSize();
    int getHalfWindowSize();
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
    GCFunc(const char *aliasTo, const char *parameters, std::string doc = ""): type(Alias),
                                                                               aliasfun(aliasTo),
                                                                               params(parameters),
                                                                               info(doc){};
    int call(GeekConsole *gc, int state, std::string value);

    GCFuncType const getType() { return type; }
    std::string const getAliasFun() { return aliasfun; }
    std::string const getAliasParams() { return params; }
    std::string const getInfo() { return info; }

private:
    GCFuncType type;
    CFunc cFun;
	CFuncVoid vFun; // simple void func()
    string luaFunName;
    int luaCallBack;
    lua_State* lua;
    string aliasfun; // alias to function
    string params; // parameters to alias functions
    string info; // documentation of this function
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

    void addPromter(std::string name,
                    GCInteractive *Interactive);
    int execFunction(std::string funName);
    int execFunction(std::string funName, std::string param);
    // call callback fun for describe interactive text with state (-funstate - 1)
    void describeCurText(string text);
    void registerFunction(GCFunc fun, std::string name);
    void reRegisterFunction(GCFunc fun, std::string name);
    GCFunc *getFunctionByName(std::string);
    std::vector<std::string> getFunctionsNames();
    bool charEntered(const char *c_p, int modifiers);
    void resize(int w, int h)
        {
            overlay->setWindowSize(w, h);
            width = w; height = h;
        };
    int const getWidth() {return width;}
    void render();
    bool isVisible;
    int32 consoleType;

    TextureFont* getInteractiveFont();
    TextureFont* getCompletionFont();

    Overlay* getOverlay() const
        {return overlay;}
    void setInteractive(GCInteractive *Interactive,
                        std::string historyName,  // if empty - dont use history
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
                         GCFunc fun, const char *funname);
    const std::vector<GeekBind *>& getGeekBinds()
        { return geekBinds;}
    bool bind(std::string bindspace, std::string bindkey, std::string function);
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

    void setBeeper(Beep *);
    Beep *getBeeper()
        { return beeper; }
    void beep();
    
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

    std::vector<std::string> infoText; // lines of info text
    uint infoWidth; // max width in pixel of infoText

    std::string lastMacro;
    std::string currentMacro;
    bool isMacroRecording;
    uint maxMacroLevel;
    uint curMacroLevel;
    Beep *beeper;
};

// init geekconsole
extern void initGeekConsole(CelestiaCore *celApp);

extern void shutdownGeekconsole();

extern GeekConsole *getGeekConsole();

extern Color getColorFromText(const string &text);
extern Color32 getColor32FromText(const string &text);
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
extern PagerInteractive *pagerInteractive;

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

#endif // _GEEKCONSOLE_H_
