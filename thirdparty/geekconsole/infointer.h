#ifndef _INFOINTER_H_
#define _INFOINTER_H_

#ifndef MAX_NODE_HISTORY_SYZE
#define MAX_NODE_HISTORY_SYZE 64
#endif

#include "gcinfo/shared.h"
#include <map>
#include <vector>
#include <string>

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
    int getBestCompletionSizePx()
        { return -1;}
    string getHelpText();

    NODE *getDynamicNode(char *filename, char *nodename);

    void rebuildDirCache();
    bool addFileToDirCache(std::string filename);

    // register fullpath of info's file name
    void addCachedInfoFile(std::string filename, std::string fullname);
    // get fullpath of info file
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

    typedef struct chunk_s {
        enum Type
        {
            TEXT,
            TEXT2, // highlight text
            LINK,
            NODE, //  info node
            /* horizontal spaces  from whole line, if  it smaller then
               size of line text before add just 1 ' ' */
            HSPACE,
            SEPARATOR1, // horizontal line level1
            SEPARATOR2,
            SEPARATOR3,
        };
        chunk_s(string label, string _filename, string _nodename, int line):
            text(label), hyp(_nodename), filename(_filename),
            linenumber(line), type(NODE){
        };
        chunk_s(const char *_str): text(_str), type(TEXT) {};
        chunk_s(const char *_str, int len):type(TEXT) 
            {
                if (len > 0)
                    text = std::string(_str, len);
            };
        // [[text here][exec:select object#sol][more info here]]
        chunk_s(const char *_str, int len,
                const char *_hyp, int hyplen,
                const char *_tip, int tiplen): type(LINK)
            {
                if (len > 0)
                    text = std::string(_str, len);
                if (hyplen > 0)
                    hyp = std::string(_hyp, hyplen);
                if (tiplen > 0)
                    tip = std::string(_tip, tiplen);
            };
        chunk_s(int _spaces): type(HSPACE), spaces(_spaces){};
        chunk_s(Type _type): type(_type) {};
        std::string text;
        std::string hyp; // for type NODE - node name
        std::string filename; // for type NODE - node's file name
        int linenumber;
        std::string tip;
        Type type;
        int spaces;
    };

private:
    void forward(int);

    void processChar(const char *c_p, int modifiers);
    typedef std::vector<chunk_s> Chunks;

    std::vector<Chunks> lines;
    int pageScrollIdx;
    int leftScrollIdx; //  scroll
    uint scrollSize;
    bool richedEnd;
    int state;
    int width;
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
        bool   dynamic;
    };
    typedef std::vector <dir_item> menus;
    // section, dir entrys
    typedef std::map<std::string, menus> dirSection_t;
    dirSection_t dirCache;
    NODE dirNode;
    std::map<std::string, std::string> cachedInfoFiles;
    // add Dir menu
    void addDirTxt(char *content, int size, bool dynamic);
    bool dirModified;
};

extern InfoInteractive *infoInteractive;
#endif // _INFOINTER_H_
