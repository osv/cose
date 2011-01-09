#ifndef _INFOINTER_H_
#define _INFOINTER_H_

#ifndef MAX_NODE_HISTORY_SYZE
#define MAX_NODE_HISTORY_SYZE 64
#endif

#include "gcinfo/shared.h"
#include <map>
#include <vector>
#include <string>

class Chunk
{
public:
    typedef struct rcontext {
        GCOverlay *ovl;
        TextureFont *font;
        float height;
        float width;
        bool renderSelected; // item selected?
    };

    Chunk() {};
    virtual float render(rcontext *rc) {return 0;};
    virtual std::string getText() {
        return ""; };
    virtual std::string getHelpTip() {
        return ""; };
    virtual float getHeight() {return 0;};
    virtual bool isLink() {
        return false; };
    virtual void followLink() {};
};

/* horizontal spaces  from whole line, if  it smaller then
   size of line text before add just 1 ' ' */
class ChkHSpace : public Chunk
{
public:
    ChkHSpace(uint spaces): m_spaces(spaces) {};
    float render(rcontext *rc);
private:
    float m_spaces;
};

class ChkSeparator : public Chunk
{
public:

    enum Separator{
        SEPARATOR1,
        SEPARATOR2,
        SEPARATOR3,
        SEPARATOR4,
    };

    ChkSeparator(Separator s) : type(s) {};
    float render(rcontext *rc);
private:
    Separator type;
};

class ChkText : public Chunk
{
public:
    ChkText(std::string t): text(t) {};
    ChkText(const char *str, int len)
        {
            if (len > 0)
                text = std::string(str, len);
        };

    float render(rcontext *rc);
    std::string getText() {
        return text; };
    float getHeight();
protected:
    std::string text;
};

class ChkVar : public Chunk
{
public:
    enum Type{
        VAR_VALUE,
        DEFAULT_VALUE,
        LAST_VALUE,
        TYPE_VALUE,
    };
    ChkVar(std::string t, int spaces, Type type = VAR_VALUE):
        varname(t), spaces(spaces), type(type) {};
    float render(rcontext *rc);
    std::string getText();
    float getHeight();
protected:
    std::string varname;
    int spaces;
    Type type;
};

class ChkImage : public Chunk
{
public:
    ChkImage(const std::string& path, const std::string& source);
    float render(rcontext *rc);
    float getHeight();
protected:
    std::string imagename;
    Texture *texture;
    ResourceHandle texRes;
};

// highlight text
class ChkHText : public ChkText
{
public:
    ChkHText(std::string t): ChkText(t) {};
    ChkHText(const char *str, int len): ChkText(str, len) {};
    float render(rcontext *rc);
};

// info node link
class ChkNode : public ChkText
{
public:
    ChkNode(std::string label,
            std::string f, std::string n, int l);
    float render(rcontext *rc);
    std::string filename;
    std::string node;
    int linenumber;
    bool isLink() {
        return true;};
    std::string getHelpTip();
    void followLink();
};

class ChkGCFun : public Chunk
{
public:
    ChkGCFun(std::string fun, std::string text = "", std::string tip = "");
    float render(rcontext *rc);
    bool isLink() { return true;};
    std::string getHelpTip();
    void followLink();
private:
    std::string function;
    std::string text;
    std::string helpTip;
};

// pager, text viewer, similar to `less` and `info` command
class InfoInteractive: public GCInteractive
{
public:
    // help node struct
    typedef struct node_s {
        node_s () {};
        node_s (std::string &_filename, std::string &_nodename, int _line,
                int _selectedX = -1, int _selectedY = 1) :
            filename(_filename), nodename(_nodename), line(_line),
            selectedX(_selectedX), selectedY(_selectedY) {};
        void set(std::string &_filename, std::string &_nodename, int _line = 0,
                 int _selectedX = -1, int _selectedY = 1) {
            filename = _filename;
            nodename = _nodename;
            line = _line;
            selectedX = _selectedX;
            selectedY = _selectedY;
        }
        void clear() {
            filename.clear();
            nodename.clear();
            line = 0;
            selectedX = -1;
            selectedY = 1; // skip first header line
        }
        bool empty() {
            return filename.empty() && nodename.empty();
        }

        std::string filename;
        std::string nodename;
        int line;
        int selectedX;
        int selectedY;
    };

    InfoInteractive(std::string name);
    ~InfoInteractive();
    void Interact(GeekConsole *_gc, string historyName);
    /* set node to view, also compile text with hyper links
       FILENAME can be passed as NULL, in which case the filename of "dir" is used.
       NODENAME can be passed as NULL, in which case the nodename of "Top" is used.
       If the node cannot be found, return a NULL pointer. */
    void setNode(string filename, string nodename = "", int linenumber = 0);
    void setNode(node_s node);
    void setNodeText(char *contents, int size);
    void addNodeText(char *contents, int size);
    void charEntered(const char *c_p, int modifiers);
    void mouseWheel(float motion, int modifiers);
    void renderCompletion(float height, float width);
    void renderInteractive();
    void setLastFromHistory() {};
    void setFont(TextureFont* _font);
    TextureFont* getFont() const {
        return font;}
    int getBestCompletionSizePx()
        { return -1;}
    string getHelpText();

    NODE *getDynamicNode(char *filename, char *nodename);

    void rebuildDirCache();
    bool addFileToDirCache(std::string filename);

    // register fullpath of info's file name
    void addCachedInfoFile(std::string filename, std::string fullname);
    // get fullpath of info file
    const char *getInfoFullPath(string &filename);
    const char *getInfoFullPath(char *filename);

    void saveDirCache();
    bool loadDirCache();
    // pager state
    enum {
        PG_NOP = 0,
        PG_ENTERDIGIT = 1,
        PG_SEARCH_FWD = 2,
        PG_SEARCH_BWD = 3,
    };

    // add Dir menu
    void addDirTxt(char *content, int size, bool dynamic);
    // dynamic node callback
    typedef std::string (* GCDynNode)(std::string filename, std::string nodename);
    void registerDynamicNode(std::string filename, GCDynNode dynNode);

private:
    void forward(int);

    void processChar(const char *c_p, int modifiers);

    typedef std::vector<Chunk*> Chunks;

    typedef struct line_s {
        Chunks chunks;
        float height;
        inline void add(Chunk *chk) {
            chunks.push_back(chk);
            float h = chk->getHeight();
            if (h > height)
                height = h;
        };
        void clear() {
            chunks.clear();
            height = 0;
        };
        // free ptr for Chunks
        void free();
    };

    std::vector<line_s> lines;

    int pageScrollIdx;
    int leftScrollIdx; //  scroll
    uint scrollSize;
    bool richedEnd;
    int state;
    float width;
    float height;
    std::string lastSearchStr;
    int N;
    TextureFont* font;
    int chopLine;
    int windowSize;
    int halfWindowSize;
    int getChopLine();
    int getWindowSize();
    int getHalfWindowSize();
    int selectedY;
    int selectedX;

    std::string m_filename; // current file
    std::string m_nodename; // current node name
    node_s node_next; // parsed next node for current node
    node_s node_prev; // parsed prev node for current node
    node_s node_up; // parsed up node for current node
    std::vector <node_s> menu; // menu of this node
    std::list <node_s> nodeHistory; // history of visited nodes

    // cached dir entry

    struct dir_item
    {
        string menu;
        string descr;
        bool   dynamic; // dynamic - don't save as cache
    };
    typedef std::vector <dir_item> menus;
    // section, dir entrys
    typedef std::map<std::string, menus> dirSection_t;
    dirSection_t dirCache;
    NODE dirNode;
    std::map<std::string, std::string> cachedInfoFiles;
    bool dirModified;

    // dynamic node callback names
    typedef std::map <std::string, GCDynNode> dynNodes_t;
    dynNodes_t dynNodes;
    NODE dynNode;
};

extern InfoInteractive *infoInteractive;
#endif // _INFOINTER_H_
