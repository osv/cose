#include "geekconsole.h"
#include "gcinfo/nodes.h"
#include "gcinfo/filesys.h"
#include "gcinfo/info-utils.h"
#include "gcinfo/shared.h"
#include "infointer.h"
#include "gvar.h"

#include <celutil/directory.h>
#include <celutil/resmanager.h>

#include <fstream>

/*
  InfoInteractive

  Some part of code used from orig.  info source `texinfo-4.13/info/`.
  This code used for info file  read, but in future need rewrite basic
  read API  because almost of code  is unused, also "info"  dir may be
  located  in any  sub-extra dir  for  all addons,  so need  recursive
  search.  All files from `texinfo-4.13/info/` located in `./gcinfo/`:

   search.c
   nodes.c
   info-utils.c
   filesys.c
   dir.c
   tilde.c

  If you want to use system info files just create link:
   %ln -s /usr/local/info ~/temp/celestia/sysinfo
  where ~/temp/celestia/ is your celestia data dir/

  Text  render based  on celesita's  overlay.  Text  is  splitted into
  chunks. Each chunk  may have different height. For  soft scroll each
  line fave fixed height (font height +1). If chunk's height is bigger
  than default  line height - empty  lines will be added,  so if first
  visible line for  render is empty - previous  non-empty line will be
  rendered. This is nod usual way  for rich text render, in future may
  logic be changed.

  This interactive  is emulate `less' command  with several navigation
  command for info-specified facility. */

// TODO: Garbage collect for unused node contents.

const char *hint_follownode = "S-RET - follow node";
const char *dirCache_FileName = "dir.cache";
const char *infoFileListCache_FileName = "info.cache";

// remove \n from str
string normalizeNodeName(string &s)
{
    string res = s;
    for(int i = 0; i < (int) res.size(); i++)
        if (res[i] == '\n')
            res[i] = ' ';
    return res;
}

class InfoDirLoader : public EnumFilesHandler
{
public:
    InfoInteractive *infoInter;
    InfoDirLoader(InfoInteractive *i) :
        infoInter(i){};

    bool process(const string& filename) {
        size_t sz = filename.find(".info");
        if (sz != string::npos)
        {
            string fullname = getPath() + '/' + filename;
            infoInter->addFileToDirCache(fullname);
            if (filename.find(".info-") == string::npos)
                infoInter->addCachedInfoFile(string(filename, 0, sz), fullname);
        }
        return true;
    }
};

/* Texinfo  require  that  image  files  may be  in  .info  file  dir.
   Celestia's  TextureInfo is not  good -  it try  to locate  image in
   texture/[resolution]/ dir, also ignore basedir  */
class GCTextureInfo : public ResourceInfo<Texture>
{
public:
    std::string source;
    std::string path;

    GCTextureInfo(const std::string _source,
                const std::string _path) :
        source(_source), path(_path) {};

    virtual std::string resolve(const std::string&baseDir) {
        if (!path.empty())
            return path + "/" + source;
        else
            return baseDir + source;
    };

    virtual Texture* load(const std::string& name) {
        return LoadTextureFromFile(name,
                                   Texture::EdgeClamp, Texture::NoMipMaps);
    };

};

inline bool operator<(const GCTextureInfo& ti0, const GCTextureInfo& ti1){
    if (ti0.source == ti1.source)
        return ti0.path < ti1.path;
    else
        return ti0.source < ti1.source;
};

typedef ResourceManager<GCTextureInfo> GCTextureManager;

static GCTextureManager* textureManager = NULL;
static GCTextureManager* GetGCTextureManager()
{
    if (textureManager == NULL)
        textureManager = new GCTextureManager("texture");
    return textureManager;
}

float ChkText::render(rcontext *rc)
{
    *rc->ovl << text;
    return 0;
}

float ChkText::getHeight()
{
    return 0;
}

float ChkVar::render(rcontext *rc)
{
    string str;
    GCOverlay *ovl = rc->ovl;
    switch(type)
    {
    case VAR_VALUE:
        str = gVar.GetFlagString(varname); break;
    case DEFAULT_VALUE:
        str = gVar.GetFlagResetString(varname); break;
    case LAST_VALUE:
        str = gVar.GetFlagLastString(varname); break;
    case TYPE_VALUE:
        str = gVar.GetTypeName(gVar.GetType(varname)); break;
    };

    float width = rc->font->getWidth(str);
    // save x offset
    float xoffset = ovl->getXoffset();
    const short spacesz = (spaces == 0 ? 1 : spaces) * rc->font->getAdvance('X');
    if (spaces > 0 && width < spacesz)
        width = spacesz;

    glColor4ubv(clCompletionAfterMatch->rgba);
    ovl->rect(xoffset, -2, width, rc->font->getHeight());
    glColor4ubv(clCompletionExpandBg->rgba);
    *ovl << str;

    if (TYPE_VALUE != type && gVar.GetType(varname) == GeekVar::Color)
    {
        // render 2x2 chess board
        glColor4ub(255,255,255,255);
        float x = xoffset + width + 2;
        float y = -1.0;
        float h = rc->font->getHeight() - 2;
        float w = rc->font->getAdvance('X') * 6;
        float hw = w / 2;
        float hh = h / 2;
        ovl->rect(x, y, w, h);
        glColor3ub(0,0,0);
        ovl->rect(x, y + hh, hw, hh);
        ovl->rect(x + hw, y, hw, hh);

        // now color
        Color32 c = getColor32FromText(gVar.GetFlagString(varname));
        glColor4ubv(c.rgba);
        ovl->rect(x+1, y+1, w-2, h-2);
        // width correction
        width += w + 4;
    }
    ovl->setXoffset(xoffset + width);

    // return to default color
    glColor4ubv(clCompletionFnt->rgba);
    return 0; // 0 - default font height
}

std::string ChkVar::getText()
{
    return varname;
}

float ChkVar::getHeight()
{
    return 0;
}

ChkImage::ChkImage(const std::string& path, const std::string& source)
{
    imagename = source;
    texRes = GetGCTextureManager()->getHandle(GCTextureInfo(path, source));
}

float ChkImage::render(rcontext *rc)
{
    GCOverlay *ovl = rc->ovl;
    if (texRes != InvalidResource)
    {
        texture = GetGCTextureManager()->find(texRes);
    }

    if (texture != NULL)
    {
        glColor4ub(255, 255, 255, 255);
        ovl->drawImage(texture);
        // return to default color
        glColor4ubv(clCompletionFnt->rgba);
        return (float) texture->getHeight();
    }
    else
        *ovl << " [image: " << imagename << "] ";
    return 0;
}

float ChkImage::getHeight()
{
    return 0;
}

float ChkHText::render(rcontext *rc)
{
    rc->ovl->rect(rc->ovl->getXoffset(), -2,
              rc->font->getWidth(text), 1);
    *rc->ovl << text;
    return 0;
}

ChkNode::ChkNode(std::string label,
            std::string f, std::string n, int l):
    ChkText(normalizeNodeName(label)), filename(f), node(normalizeNodeName(n)), linenumber(l)
{
}

float ChkNode::render(rcontext *rc)
{
    if (rc->renderSelected)
    {
        glColor4ubv(clCompletionExpandBg->rgba);
        rc->ovl->rect(rc->ovl->getXoffset(), -2,
                      rc->font->getWidth(text), rc->font->getHeight());
    }

    glColor4ubv(clCompletionAfterMatch->rgba);
    *rc->ovl << text;

    // return to default color
    glColor4ubv(clCompletionFnt->rgba);
    return 0;
}

string ChkNode::getHelpTip()
{
    string res = "Node: ";
    res += node;
    if (!filename.empty())
    {
        res += "\nFile: ";
        res += filename;
    }
    return res;
}

void ChkNode::followLink()
{
    infoInteractive->setNode(filename, node, linenumber);
}

float ChkHSpace::render(rcontext *rc)
{
    const float xoffset = rc->ovl->getXoffset();
    const short basesz = rc->font->getAdvance('X');
    const short spacesz = rc->font->getAdvance(' ');
    int n = ceil((m_spaces * basesz - xoffset) / spacesz);
    if (n <= 0)
        n = 1;
    *rc->ovl << string(n, ' ');
    return 0;
}

float ChkSeparator::render(rcontext *rc)
{
    float fh = rc->font->getHeight();

    if (type == SEPARATOR1) // bold line
    {
        rc->ovl->rect(0, fh/2 + 1, 100.0 * rc->width, 3);
    }
    else if (type == SEPARATOR2) // ====
    {
        rc->ovl->rect(0, fh/2 + 1, 100.0 * rc->width, 3, false);
    }
    else if (type == SEPARATOR3) // ----
        rc->ovl->rect(0, fh/2 + 1, 100.0 * rc->width, 2);
    else if (type == SEPARATOR4) // ....
        rc->ovl->rect(0, fh/2 + 1, 100.0 * rc->width, 1);
    return 0;
}

ChkGCFun::ChkGCFun(std::string fun, std::string text, std::string tip):
    function(normalizeNodeName(fun)), text(normalizeNodeName(text)), helpTip(tip)
{

}

float ChkGCFun::render(rcontext *rc)
{
    string str = text;
    if (str.empty())
        str = function;

    GCOverlay *ovl = rc->ovl;
    *ovl << "[";
    if (rc->renderSelected)
    {
        glColor4ubv(clCompletionExpandBg->rgba);
        ovl->rect(ovl->getXoffset(), -2,
                   rc->font->getWidth(str), rc->font->getHeight());
    }

    glColor4ubv(clCompletionAfterMatch->rgba);
    *ovl << str;

    // return to default color
    glColor4ubv(clCompletionFnt->rgba);
    *ovl << "]";
    return 0;
}

string ChkGCFun::getHelpTip()
{
    string res = helpTip;
    if (!res.empty())
        res+="\n";
    res+= "*EXEC*#" + function;
    return res;
}

void ChkGCFun::followLink()
{
    if (!function.empty())
    {
        getGeekConsole()->setReturnToLastInfoNode();
        getGeekConsole()->execFunction("exec function", function);
    }
}

// InfoInteractive //

void InfoInteractive::line_s::free()
{
    Chunks::iterator it;
    for (it = chunks.begin(); it != chunks.end(); it++)
        delete *it;
}

InfoInteractive::InfoInteractive(std::string name)
    :GCInteractive(name, false),
     pageScrollIdx(0),
     font(NULL),
     selectedY(1),
     selectedX(-1),
     dirModified(true)
{
    dirNode.contents = NULL;
    dynNode.contents = NULL;

    if (!loadDirCache())
        rebuildDirCache();
}

InfoInteractive::~InfoInteractive()
{
    maybe_free(dirNode.contents);
    maybe_free(dynNode.contents);
}

void InfoInteractive::setFont(TextureFont* _font)
{
    if (!gc->isVisible)
        font = _font;
}

string InfoInteractive::getHelpText()
{
    return GCInteractive::getHelpText() +
        _("\n---\n               SUMMARY OF PAGER COMMANDS\n\n"
          " \n"
          "q  Q                    Exit.\n\n"
          "                            MOVING\n"
          "e  ^E  j  ^N  CR        Forward  one line   (or N lines).\n"
          "y  ^Y  k  ^K  ^P        Backward one line   (or N lines).\n"
          "f  ^F  ^V  SPACE        Forward  one window (or N lines).\n"
          "b  ^B                   Backward one window (or N lines).\n"
          "z                       Forward  one window (and set window to N).\n"
          "w                       Backward one window (and set window to N).\n"
          "d  ^D                   Forward  one half-window (and set half-window to N).\n"
          "u  ^U                   Backward one half-window (and set half-window to N).\n"
          "l                       Left  one half screen width (or N positions).\n"
          "r                       Right one half screen width (or N positions).\n"
          " \n"
          "Default \"window\" is the screen height.\n"
          "Default \"half-window\" is half of the screen height.\n"
          " \n                            SEARCHING\n"
          "/pattern                Search forward for (N-th) matching line.\n"
          "?pattern                Search backward for (N-th) matching line.\n"
          "n                       Repeat previous search (for N-th occurrence).\n"
          "N                       Repeat previous search in reverse direction.\n"
          " \n                             JUMPING\n"
          "g  <                    Go to first line in file (or line N).\n"
          "G  >                    Go to last line in file (or line N).\n"
          "p  %                    Go to beginning of file (or N percent into file).\n"
          " \n                          INFO NAVIGATION\n"
          "E  S-^E  J  S-^N  S-CR  Follow the selected hypertext link\n"
          "]                       Move forwards or down through node structure\n"
          "[                       Move backwards or up through node structure\n"
          "}                       Select the Next node\n"
          "}                       Select the Prev node\n"
          "t                       Select the node `Top' in this file\n"
          "M-/                     Select next hypertext\n"
          "M-?                     Select prev hypertext");
}

void InfoInteractive::Interact(GeekConsole *_gc, string historyName)
{
    GCInteractive::Interact(_gc, historyName);
    leftScrollIdx = 0;
    scrollSize = 0;
    richedEnd = false;
    state = PG_NOP;
    lastSearchStr = "";

    chopLine = -1;
    windowSize = -1;
    halfWindowSize = -1;

    if (!font)
        font = gc->getCompletionFont();

    N = -1;
    // selectedY = -1;
    // selectedX = -1;
    // pageScrollIdx = 0;
}

// forward scrollIdx
// Dont increment if end of view is visible
void InfoInteractive::forward(int fwdby)
{
    if (fwdby > 0)
    {
        if (pageScrollIdx < (int) lines.size() - (int) scrollSize)
        {
            pageScrollIdx += fwdby;
            if (pageScrollIdx > (int) lines.size() - (int) scrollSize)
            { // end exceed
                pageScrollIdx = lines.size() - scrollSize;
            }
        } else
            gc->beep();
    } else
        if (pageScrollIdx == 0)
            gc->beep();
        else
            pageScrollIdx += fwdby;
}

int InfoInteractive::getChopLine()
{
    if (chopLine == -1)
        // by default scroll 1/4 screen width
        return width / 4 * gc->getCompletionFont()->getWidth("X");
    else
        return chopLine;
}

int InfoInteractive::getWindowSize()
{
    if (windowSize == -1)
        return scrollSize;
    else
        return windowSize;
}

int InfoInteractive::getHalfWindowSize()
{
    if (halfWindowSize == -1)
        return scrollSize / 2;
    else
        return halfWindowSize;
}

void InfoInteractive::setNode(node_s node)
{
    setNode(node.filename, node.nodename, node.line);
    selectedX = node.selectedX;
    selectedY = node.selectedY;
}

void InfoInteractive::setNode(string f, string n, int linenumber)
{
    line_s line;

    // save history before all
    nodeHistory.push_back(node_s(m_filename, m_nodename, pageScrollIdx, selectedX, selectedY));
    if (nodeHistory.size() > MAX_NODE_HISTORY_SYZE)
        nodeHistory.pop_front();
    selectedX = -1;
    selectedY = 1;
    pageScrollIdx = 0;
    leftScrollIdx = 0;

    if (f.empty())
        f = m_filename;
    if (n.empty())
        n = "Top";

    // read node
    NODE *node = info_get_node ((char *)f.c_str(), (char *)n.c_str());

    // clear old lines
    std::vector<line_s>::iterator it;
    for (it = lines.begin(); it != lines.end(); it++)
        (*it).free();
    lines.clear();

    node_next.clear();
    node_prev.clear();
    node_up.clear();
    menu.clear();

    if (node)
    {
        m_filename = f;
        m_nodename = n;

        setNodeText(node->contents, node->nodelen);
    }
    else
    {
        line.add(new ChkText(_("Node not found.")));
        lines.push_back(line);
        if (info_recent_file_error)
        {
            line.clear();
            line.add(new ChkText(info_recent_file_error));
            lines.push_back(line);
        }
    }

    // XXX: check line_number corection
    pageScrollIdx = linenumber;
    if (pageScrollIdx < 0)
        pageScrollIdx = 0;
}

void InfoInteractive::setNodeText(char *contents, int size)
{
    char *c = contents;
    char *end = contents + size;
    char *lstart; // line start
    line_s line;

    // first line is header, skip it
    while(c < end && *c != '\n')
        c++;
    c++;

    lstart = c;

    // find "Prev:", "Next:", "Up:", "File:", or "Node:" labels
    // by using modified info-utils's func

    info_file_label_of_line(contents);
    if (info_parsed_nodename)
    {
        line.add(new ChkText(_("File: ")));
        line.add(new ChkHText(info_parsed_nodename));
    }

    info_node_label_of_line(contents);
    if (info_parsed_nodename)
    {
        line.add(new ChkText(_(",  Node: ")));
        line.add(new ChkHText(info_parsed_nodename));
    }

#define CHECKLABEL(_textm, _nodeset)                                    \
    if (info_parsed_filename || info_parsed_nodename)                   \
    {                                                                   \
        line.add(new ChkText(_(",  " _textm ": ")));                    \
                                                                        \
        string filename;                                                \
        string nodename;                                                \
        string label;                                                   \
        if (info_parsed_filename)                                       \
        {                                                               \
            filename = info_parsed_filename;                            \
            label = string("(") + info_parsed_filename + string(")");   \
        }                                                               \
        if (info_parsed_nodename)                                       \
        {                                                               \
            label += info_parsed_nodename;                              \
            nodename = info_parsed_nodename;                            \
        }                                                               \
        line.add(new ChkNode(label, filename, nodename,                 \
                         info_parsed_line_number));                     \
        _nodeset.set(filename, nodename, info_parsed_line_number);      \
    }

    info_next_label_of_line(contents);
    CHECKLABEL("Next", node_next);

    // prev or previous label
    info_prev_label_of_line(contents);
    CHECKLABEL("Previous", node_prev);

    info_up_label_of_line(contents);
    CHECKLABEL("Up", node_up);

    lines.push_back(line);

    // parse rest
    addNodeText(lstart, end - lstart);
}

/* Parse text of info body */
void InfoInteractive::addNodeText(char *contents, int size)
{
    enum State
    {
        TEXTMODE,
        MENU_HINT,  // some text after "* foo: (bar).     Hint here"
        MENU_DESCR, // text after menu hint, that need to be right aligned
    };

    State state = TEXTMODE;
    line_s line;

    char *c = contents;
    char *end = contents + size;
    char *lstart = c; // line start

    while(c < end)
    {
        if (*c == '\n')
        {
            if (MENU_HINT == state || MENU_DESCR == state)
            {
                if (c - lstart == 0)
                    if (MENU_HINT == state)
                        state = MENU_DESCR;
                    else
                        state = TEXTMODE; // if empty line - reset state to def.
                else
                {
                    line.add(new ChkHSpace(28)); // format comment of menu to right align
                    lstart += skip_whitespace(lstart);
                    state = MENU_DESCR;
                }
            }
            else
            {
                if (line.chunks.size() == 0 && c - lstart > 0)
                {
                    int i;
                    for (i = 0; i < c - lstart; i++)
                    {
                        if (!(lstart[i] == '-' || lstart[i] == ' ' ||
                              lstart[i] == '*' || lstart[i] == '=' ||
                              lstart[i] == '.'))
                            break;
                    }
                    if (i == c - lstart)
                    {
                        if (c[-1] == '*')
                            line.add(new ChkSeparator(ChkSeparator::SEPARATOR1));
                        else if (c[-1] == '=')
                            line.add(new ChkSeparator(ChkSeparator::SEPARATOR2));
                        else if (c[-1] == '-')
                            line.add(new ChkSeparator(ChkSeparator::SEPARATOR3));
                        else if (c[-1] == '.' && i > 3) // make sure not just "..."
                            line.add(new ChkSeparator(ChkSeparator::SEPARATOR4));
                        goto appendline;
                    }
                }
            }
            line.add(new ChkText(lstart, c - lstart));
        appendline:
            lines.push_back(line);
            line.clear();
            lstart = c + 1;
        }
        else if (*c == '`' && state == TEXTMODE) // highlight text between `'
        {
            c++; // push '`'also
            line.add(new ChkText(lstart, c - lstart));
            lstart = c;
            while(c < end && *c != '\'')
            {
                if (*c == '\n')
                {   // push current line and continue search for '\''
                    line.add(new ChkHText(lstart, c - lstart));
                    lines.push_back(line);
                    line.clear();
                    lstart = c +1;
                }
                c++;
            }
            line.add(new ChkHText(lstart, c - lstart));
            lstart = c;
        }
        else if (*c == '\t') // just convert tab to 8 spaces, TODO: add real tabs
        {
            if (c -1 - lstart > 0)
                line.add(new ChkText(lstart, c -1 - lstart));
            line.add(new ChkText("        "));
            lstart = c +1;
        }
        // "^\* "
        else if (*c == '*') // menu or *Note
        {
            // if "* " at line begin - menu item found
            bool search_for_menu = (c[1] == ' ' && lstart == c);
            if (search_for_menu) {
                line.add(new ChkText("* "));
                c+=2;
            } else {
                // check for XREF
                if (string_in_line(INFO_XREF_LABEL, c) != strlen(INFO_XREF_LABEL)) {
                    c++;
                    continue; // seems no XREF
                }
                // replace *Note to See
                if (c - lstart > 0)
                    line.add(new ChkText(lstart, c - lstart));

                line.add(new ChkText("See "));
                c += strlen(INFO_XREF_LABEL);
            }
            lstart = c;
            int offset = string_in_line (":", c);
            // menu item must have ":"
            if (offset < 1)
            {
                if (search_for_menu)
                    continue;
                else
                {
                    int temp;

                    temp = skip_line (c);
                    offset = string_in_line (":", c + temp);
                    if (offset == -1)
                        continue;       /* Give up? */
                    else
                        offset += temp;
                }
            }

            std::string label(c, offset -1);
            std::string file_name;
            std::string node_name;
            int         line_number = 0;

            c += offset;
            /* If this menu entry continues with another ':' then the
               nodename is the same as the label. */
            if (*c == '\n')
                continue;

            if (search_for_menu)
                state = MENU_HINT;

            if (*c == ':')
            {
                node_name = label;
                c++;
            }
            else
            {
                c += skip_whitespace_and_newlines (c);
                if (search_for_menu)
                    c = info_parse_node (c, DONT_SKIP_NEWLINES);
                else
                    c = info_parse_node (c, SKIP_NEWLINES);

                if (info_parsed_filename)
                    file_name = info_parsed_filename;

                if (info_parsed_nodename)
                    node_name = info_parsed_nodename;

                line_number = info_parsed_line_number -2; // -2 is visible line correction
            }
            line.add(new ChkNode(label, file_name, node_name, line_number));
            if (search_for_menu)
                menu.push_back(node_s(file_name, node_name, line_number));
            lstart = c;
            continue;
            // ^@^H[...^@^H]
        } else if (c < end -2 &&
                   c[0] == 0 && c[1] == 8 && c[2] == '[') // ^@^H[
        {
            if (c - lstart > 0)
                line.add(new ChkText(lstart, c - lstart));

            c += 3; // skip ^@^H[
            char *esc_start = c;
            //search for ^@^H]
            while (c < end -2 &&
                   !(c[0] == 0 && c[1] == 8 && c[2] == ']'))
                c++;

            lstart = c +3; // skip ^@^H]
            char *esc_end = c;

            int tagsz = skip_non_whitespace(esc_start);
            string tagname;
            if (tagsz > 0)
                tagname = string(esc_start, tagsz);
            SEARCH_BINDING search;
            search.buffer = esc_start;
            search.start = tagsz;
            search.end = esc_end - esc_start;


            // parse  rest params like  foo="xyz" or  bar=[[zyx]] into
            // map
            map<string,string> params;

# define CURC search.buffer + search.start
            while(1)
            {
                search.start += skip_whitespace_and_newlines(CURC);
                search.flags = 0;
                long param_name_start = search.start;
                long param_name_end = search_forward("=", &search);
                if (-1 == param_name_end)
                    break;

                int strsz;
                strsz = param_name_end - param_name_start;
                if (strsz <= 0)
                    break;
                string param_name(CURC, strsz);
                search.start = param_name_end +1; // +1 -  "=" offset

                string value;

                // value maybe in "foobar" or in [[foobar]]

                if (search.buffer[search.start] == '"')
                {
                    search.start++;
                    search.flags = 0;
                    long value_start = search.start;
                    long value_end = search_forward("\"", &search);
                    if (-1 == param_name_end) // valid end?
                        break;

                    strsz = value_end - value_start;
                    if (strsz <= 0)
                        break;

                    value = string(CURC, strsz);
                    search.start = value_end + 1;
                }
                else if (search.buffer[search.start] == '[' &&
                         search.buffer[search.start +1] == '[')
                {
                    search.start += 2; // skip "[["
                    search.flags = 0;
                    long value_start = search.start;
                    long value_end = search_forward("]]", &search);
                    if (-1 == param_name_end) // valid end?
                        break;

                    strsz = value_end - value_start;
                    if (strsz <= 0)
                        break;

                    value = string(CURC, strsz);
                    search.start = value_end + 2; // 2 - size of "]]"
                }
                else
                    break;
                params[param_name] = value;
            }
#undef CURC
            map<string,string>::const_iterator it;
            if (tagname == "var" || tagname == "descvar"
                || tagname == "editvar" || tagname == "editvar2")
            {
                it = params.find("name");
                if (it != params.end())
                {
                    string varname = it->second;
                    it = params.find("spaces");
                    int spaces = 16;
                    if (it != params.end())
                        spaces = atoi((it->second).c_str());

                    if (tagname == "descvar")
                    {
                        line.add(new ChkGCFun(string("set variable#") + varname, varname,
                                              _("Edit variable")));
                        line.add(new ChkHSpace(18));
                        line.add(new ChkVar(varname, spaces));
                        line.add(new ChkText(" "));
                        lines.push_back(line);
                        line.clear();
                        line.add(new ChkText("  Type "));
                        line.add(new ChkVar(varname, 8, ChkVar::TYPE_VALUE));
                        if (gVar.IsBinded(varname))
                            line.add(new ChkText(" (Binded)"));
                        lines.push_back(line);
                        line.clear();
                        line.add(new ChkGCFun(string("reset variable#") + varname, _("Default:")));
                        line.add(new ChkText(" "));
                        line.add(new ChkVar(varname, 1, ChkVar::DEFAULT_VALUE));
                        line.add(new ChkHSpace(26));
                        line.add(new ChkGCFun(string("set variable with last#") + varname, _("Last:")));
                        line.add(new ChkText(" "));
                        line.add(new ChkVar(varname, 1, ChkVar::LAST_VALUE));
                        lines.push_back(line);
                        line.clear();

                        // var doc
                        string doc = gVar.GetDoc(varname);
                        if (!doc.empty())
                            addNodeText(const_cast<char *>(doc.c_str()), doc.size());
                    }
                    else if (tagname == "editvar" || tagname == "editvar2")
                    {
                        line.add(new ChkVar(varname, spaces));
                        line.add(new ChkText(" "));
                        line.add(new ChkGCFun(string("set variable#") + varname, "...",
                                              string(_("Edit variable:\n")) + varname));
                        line.add(new ChkText(" "));
                        line.add(new ChkGCFun(string("reset variable#") + varname, "R",
                                              string(_("Reset variable to:\n")) + gVar.GetFlagResetString(varname)));
                        line.add(new ChkText(" "));
                        line.add(new ChkGCFun(string("set variable with last#") + varname, "L",
                                              string(_("Set to last value:\n")) + gVar.GetFlagLastString(varname)));
                        if (tagname == "editvar2")
                        {
                            line.add(new ChkHSpace(32));
                            line.add(new ChkText(varname));
                        }
                    }
                    else
                        line.add(new ChkVar(varname, spaces));
                }
            }
            else if (tagname == "gcfun")
            {
                it = params.find("f");
                if (it != params.end())
                {
                    string funname = it->second;
                    string text;
                    string helpTip;
                    it = params.find("text");
                    if (it != params.end())
                        text = it->second;
                    it = params.find("help");
                    if (it != params.end())
                        helpTip = it->second;
                    line.add(new ChkGCFun(funname, text, helpTip));
                }
            }
            else if (tagname == "image")
            {
                it = params.find("src");
                if (it != params.end())
                {
                    string path(getInfoFullPath(m_filename));
                    size_t found = path.find_last_of("/\\");
                    path = path.substr(0,found);
                    line.add(new ChkImage(it->second, path));
                }
            }
        }
        c++;
    }
    if (c - lstart > 0)
        line.add(new ChkText(lstart, c - lstart));
    if (!line.chunks.empty())
        lines.push_back(line);
}

void InfoInteractive::charEntered(const char *c_p, int modifiers)
{
    char c = *c_p;

    if (pageScrollIdx >= (int) lines.size())
        pageScrollIdx = lines.size() - 1;
    if (pageScrollIdx < 0)
        pageScrollIdx = 0;
    if (selectedY > pageScrollIdx + (int)scrollSize)
        selectedY = -1;
    if (selectedY < pageScrollIdx)
        selectedY = -1;

    switch (state)
    {
    case PG_NOP:
        if ((c >= '0' && c <= '9')) {
            state = PG_ENTERDIGIT;
            GCInteractive::charEntered(c_p, modifiers);
            break;
        } else if (c == '/' && !(modifiers & GeekBind::META)) {
            state = PG_SEARCH_FWD;
            return;
        } else if (c == '?' && !(modifiers & GeekBind::META)) {
            state = PG_SEARCH_BWD;
            return;
        }
        else {
            processChar(c_p, modifiers);
        }
        break;
    case PG_ENTERDIGIT:
        // only promt for digits, otherwise dispatch with resulted N
        if ((c >= '0' && c <= '9') || c == '\b') {
            GCInteractive::charEntered(c_p, modifiers);
            break;
        } else {
            state = PG_NOP;
            N = atoi(getBufferText().c_str());
            charEntered(c_p, modifiers);
        }
        setBufferText("");
        break;
    case PG_SEARCH_BWD:
    {
        if (c == '\n' || c == '\r') {
            state = PG_NOP;
            if (getBufferText() != "")
            {
                lastSearchStr = getBufferText();
            }
            setBufferText("");
            for (int i = pageScrollIdx -1; i >= 0; i--)
            {
                for (int chk = lines[i].chunks.size() - 1; chk >= 0; chk--)
                {
                    Chunk *chunk = lines[i].chunks[chk];
                    if (chunk->getText().find(lastSearchStr) != string::npos)
                    {
                        if (--N < 1) { // skip N matches
                            pageScrollIdx = i;
                            gc->descriptionStr = _("Search backward");
                            gc->descriptionStr += ": " + lastSearchStr;
                            return;
                        }
                        break; // only one match for 1 line
                    }
                }
            }
            N = -1;
            gc->descriptionStr = _("Pattern not found: ") + lastSearchStr;
            return;
        } else {
            // backspace when empty buffer - cancel search
            if (c == '\b' && getBufferText().empty())
                state = PG_NOP;
            else {
                GCInteractive::charEntered(c_p, modifiers);
                gc->descriptionStr = _("Search backward");
            }
        }
        break;
    }
    case PG_SEARCH_FWD:
    {
        if (c == '\n' || c == '\r') {
            state = PG_NOP;
            if (getBufferText() != "")
            {
                lastSearchStr = getBufferText();
            }
            setBufferText("");
            for (int i = pageScrollIdx +1; i < (int)lines.size(); i++)
            {
                for (uint chk = 0; chk < lines[i].chunks.size(); chk++)
                {
                    Chunk *chunk = lines[i].chunks[chk];
                    if (chunk->getText().find(lastSearchStr) != string::npos)
                    {
                        if (--N < 1) { // skip N matches
                            pageScrollIdx = i;
                            gc->descriptionStr = _("Search forward");
                            gc->descriptionStr += ": " + lastSearchStr;
                            return;
                        }
                        break; // only one match for 1 line
                    }
                }
            }
            N = -1;
            gc->descriptionStr = _("Pattern not found: ") + lastSearchStr;
            return;
        } else {
            // backspace when empty buffer - cancel search
            if (c == '\b' && getBufferText().empty())
                state = PG_NOP;
            else {
                GCInteractive::charEntered(c_p, modifiers);
                gc->descriptionStr = _("Search forward");
            }
        }
        break;
    }
    } // switch

    if (leftScrollIdx < 0)
        leftScrollIdx = 0;
    if (pageScrollIdx >= (int) lines.size())
        pageScrollIdx = lines.size() - 1;
    if (pageScrollIdx < 0)
        pageScrollIdx = 0;
    if (selectedY > pageScrollIdx + (int)scrollSize)
        selectedY = -1;
    if (selectedY < pageScrollIdx)
        selectedY = -1;
}

void InfoInteractive::mouseWheel(float motion, int modifiers)
{
    if (motion > 0.0)
        charEntered("d", 0);
    else
        charEntered("u", GeekBind::CTRL);

}

// Commands are based on both more and vi. Commands may be preceded by
// a decimal number, called N in.
// N = -1 is default
void InfoInteractive::processChar(const char *c_p, int modifiers)
{
    char c = *c_p;
    char C = toupper(c);
    setBufferText("");
    switch (state)
    {
    case PG_NOP:
        if (c == '<' || c == 'g') {
            // Go to line N in the file, default 1
            if (N < 0) // default
                pageScrollIdx = 0;
            else
                pageScrollIdx = N;
            leftScrollIdx = 0;
        } else if (c == '>' || c == 'G') {
            // Go to line N in the file, default the end of the file.
            if (N < 0) // default
                pageScrollIdx = lines.size() - scrollSize;
            else
                pageScrollIdx = lines.size() - scrollSize - N;
            leftScrollIdx = 0;
        } else if (c == 'p' || c == '%') {
            // Go to a position N percent
            if (N < 0) // default
                pageScrollIdx = 0;
            else
                pageScrollIdx = lines.size() * N / 100;
            leftScrollIdx = 0;
        } else if (c == ' ' || c == 'f' || c == CTRL_V) {
            // if M-SPC and end of node than goto next node
            if (modifiers == GeekBind::META && c == ' ' &&
                pageScrollIdx >= (int) lines.size() - (int) scrollSize)
                if (node_next.empty())
                {
                    gc->descriptionStr = _("No `Next' pointer for this node.");
                    gc->beep();
                    return;
                }
                else
                {
                    setNode(node_next.filename, node_next.nodename,
                            node_next.line);
                    return;
                }
            // Scroll forward N lines, default one window
            if (N > 0)
                forward(N);
            else
                forward(getWindowSize());
        } else if (c == 'z') {
            // Scroll  forward N  lines,  but if  N  is specified,  it
            // becomes the new window size.
            if (N > 0)
                windowSize = N;
            forward(getWindowSize());
        } else if (c == '\r' || c == '\n' || c == CTRL_N ||
                   C == 'E' || c == CTRL_E || C == 'J' || c == CTRL_J) {
            if (modifiers & GeekBind::SHIFT)
            {
                if (selectedY != -1 && selectedX != -1)
                {
                    Chunk *chunk = lines[selectedY].chunks[selectedX];
                    chunk->followLink();
                }
            }
            else
            {
                // Scroll forward  N lines, default 1. The  entire N lines
                // are displayed, even if N is more than the screen size.
                if (N > 0)
                    forward(N);
                else
                    forward(1);
            }
        } else if (c == 'd' || c == CTRL_D) {
            //Scroll forward  N lines, default one half  of the screen
            //size. If N is specified,  it becomes the new default for
            //subsequent d and u commands.
            if (N > 0)
                halfWindowSize = N;
            forward(getHalfWindowSize());
        } else if (c == 'b' || c == CTRL_B) {
            // Scroll backward N lines, default one window
            if (N > 0)
                forward(-N);
            else
                forward(-getWindowSize());
        } else if (c == 'w') {
            // Scroll  backward N  lines, but  if N  is  specified, it
            // becomes the new window size.
            if (N > 0)
                windowSize = N;
            forward(-getWindowSize());
        } else if (c == 'y' || c == 'k' || c == 'e' ||
                   c == CTRL_Y || c == CTRL_P || c == CTRL_K) {
            // Scroll backward N lines,  default 1. The entire N lines
            // are displayed, even if N is more than the screen size.
            if (N > 0)
                forward(-N);
            else
                forward(-1);
        } else if (c == 'u' || c == CTRL_U) {
            //Scroll backward N lines,  default one half of the screen
            //size. If N is specified,  it becomes the new default for
            //subsequent d and u commands.
            if (N > 0)
                halfWindowSize = N;
            forward(-getHalfWindowSize());
        } else if (c == '(') {
            if (N > 0)
                chopLine = N;
            leftScrollIdx -= getChopLine() * font->getAdvance('X');
        } else if (c == ')') {
            if (N > 0)
                chopLine = N;
            leftScrollIdx += getChopLine() * font->getAdvance('X');
        } else if (c == 'l') {
            // history mode
            if (nodeHistory.size()) {
                node_s node = nodeHistory.back();
                nodeHistory.pop_back();
                setNode(node);
                nodeHistory.pop_back();
            } else {
                gc->descriptionStr = _("No more recent nodes");
                gc->beep();
            }
        } else if (c == 't') {
            // goto top of current file
            setNode(m_filename, "Top");
        } else if (c == ']') {
            // Move to 1st menu item, Next, Up/Next, or error.
            if (!menu.empty()) {
                setNode(menu[0].filename, menu[0].nodename, menu[0].line);
            } else {
                /* This node does not contain a menu.  If it contains a
                   "Next:" pointer, use that. */
                if (!node_next.empty()) {
                    setNode(node_next);
                } else {
                    /* There  wasn't a  "Next:" for  this  node.  Move
                       "Up:" until we can move "Next:".  If that isn't
                       possible,  complain  that  there  are  no  more
                       nodes. */

                    while (1) {
                        if (!node_up.empty()) {
                            setNode(node_up);

                            /* If no "Next" pointer, keep backing up. */
                            if (node_next.empty())
                                continue;

                            /* If this  node's first menu  item is the
                               same as this  node's Next pointer, keep
                               backing up. */
                            if (node_next.filename.empty() &&
                                !menu.empty() &&
                                menu[0].filename == node_next.filename &&
                                menu[0].nodename == node_next.nodename)
                                continue;

                            /* This node has  a "Next" pointer, and it
                               is not the same  as the first menu item
                               found in this node. */
                            setNode(node_next);
                            break;
                        } else {
                            gc->descriptionStr = _("No more nodes within this document.");
                            gc->beep();
                            break;
                        }
                    }
                }
            }
        } else if (c == '[') {
            /* Move Prev, Up or error. */
            if (node_prev.empty()) {
                if (node_up.empty()) {
                    gc->descriptionStr = _("No `Prev' or `Up' for this node within this document.");
                    gc->beep();
                    return;
                } else {
                    setNode(node_up);
                }
            } else {

                /* Watch out!  If this node's  Prev is the same as the
                   Up, then  move Up.  Otherwise, we  could move Prev,
                   and then to  the last menu item in  the Prev.  This
                   would cause  the user to loop  through a subsection
                   of the info file. */
                if (node_prev.filename == node_up.filename &&
                    node_prev.nodename == node_up.nodename)
                    setNode(node_up);
                else {

                    /* Move to  the previous  node.  If this  node now
                       contains  a  menu, and  we  have not  inhibited
                       movement to it,  move to the node corresponding
                       to the last menu item. */
                    setNode(node_prev);
                    if (!menu.empty())
                        setNode(menu[menu.size()-1]);
                }
            }
        } else if (c == '}') {
            if (!node_next.empty())
                setNode(node_next);
            else {
                    gc->descriptionStr = _("No `Next' for this node within this document.");
                    gc->beep();
            }
        } else if (c == '{') {
            if (!node_prev.empty())
                setNode(node_prev);
            else {
                    gc->descriptionStr = _("No `Prev' for this node within this document.");
                    gc->beep();
            }
        } else if (c == 'q' || c == 'Q') {
                GCInteractive::charEntered("\r", 0);
        } else if (c == 'h' || c == 'H') { // help
            // TODO: helptip
        } else if (c == 'n') {
            // Repeat previous search (for N-th occurrence)
            state = PG_SEARCH_FWD;
            charEntered("\n", 0);
            return; // dont continue (N must be not reset)
        } else if (c == 'N') { // search prev
            state = PG_SEARCH_BWD;
            charEntered("\n", 0);
            return;
        } else if (c == '/' && (modifiers == GeekBind::META)) { // M-/ select next
            if (lines.empty())
                return;
            int i = selectedY;
            if (selectedY == -1 || selectedY >= (int)lines.size())
                i = pageScrollIdx;
            if (selectedX == -1)
                selectedX = 0;
            else
                selectedX++;
            // try search link in current selected line for hint
            for (int chk = selectedX; chk < (int)lines[i].chunks.size(); chk++)
            {
                Chunk *chunk = lines[i].chunks[chk];
                if (chunk->isLink())
                {
                    selectedY = i;
                    selectedX = chk;
                    gc->setInfoText(chunk->getHelpTip());
                    gc->appendDescriptionStr(_(hint_follownode));
                    return;
                }
            }

            i++; // next lines
            vector<line_s>::const_iterator it = lines.begin();
            while (i < pageScrollIdx + (int)scrollSize && i < (int)lines.size())
            {
                for (int chk = 0; chk < (int)lines[i].chunks.size(); chk++)
                {
                    Chunk *chunk = lines[i].chunks[chk];
                    if (chunk->isLink())
                    {
                        selectedY = i;
                        selectedX = chk;
                        gc->setInfoText(chunk->getHelpTip());
                        gc->appendDescriptionStr(_(hint_follownode));
                        return;
                    }
                }
                i++;
            }
            selectedY = -1;
            selectedX = -1;
        } else if (c == '?' && (modifiers & GeekBind::META)) { // M-? select prev
            if (lines.empty())
                return;
            int i = selectedY;
            if (selectedY == -1 || selectedY >= (int)lines.size())
                i = pageScrollIdx + scrollSize -1;
            if (i >= (int)lines.size())
                i = lines.size() -1;
            if (selectedX == -1)
                selectedX = lines[i].chunks.size();
            else
                selectedX--;
            // try back search link in current selected line
            for (int chk = selectedX;
                 chk >= 0 && chk < (int)lines[i].chunks.size(); chk--)
            {
                Chunk *chunk = lines[i].chunks[chk];
                if (chunk->isLink())
                {
                    selectedY = i;
                    selectedX = chk;
                    gc->setInfoText(chunk->getHelpTip());
                    gc->appendDescriptionStr(_(hint_follownode));
                    return;
                }
            }

            i--; // next lines
            vector<line_s>::const_iterator it = lines.begin();
            while (i >= 0)
            {
                for (int chk = lines[i].chunks.size() -1; chk >= 0; chk--)
                {
                    Chunk *chunk = lines[i].chunks[chk];
                    if (chunk->isLink())
                    {
                        selectedY = i;
                        selectedX = chk;
                        gc->setInfoText(chunk->getHelpTip());
                        gc->appendDescriptionStr(_(hint_follownode));
                        return;
                    }
                }
                i--;
            }
            selectedY = -1;
            selectedX = -1;
        } else {
            gc->beep();
        }
        break;
    }
    N = -1;
}

void InfoInteractive::renderCompletion(float height, float width)
{
    float fwidth = font->getAdvance('X');
    float fh = font->getHeight();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel
    if (!nb_lines)
        return;

    scrollSize = nb_lines;
    if (scrollSize == 0)
        scrollSize = 1;

    this->width = width;

    if (pageScrollIdx < 0)
        pageScrollIdx = 0;

    // Default  color, each  chunk->render  must not  change GL  color
    // agter return.
    glColor4ubv(clCompletionFnt->rgba);

    glPushMatrix();
    glTranslatef((float) -leftScrollIdx, 0.0f, 0.0f);
    GCOverlay *overlay =  gc->getOverlay();
    overlay->setFont(font);
    overlay->beginText();

    Chunk::rcontext rc;
    rc.ovl = overlay;
    rc.font = font;
    rc.height = height;
    rc.width = width;
    rc.renderSelected = false;

    // need clip test for proper image clip
    GLdouble p0[4] = { 0, 1.0, 0.0, height - fh };
    GLdouble p1[4] = { 0, -1.0, 0.0, fh -1 };
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glClipPlane(GL_CLIP_PLANE0, p0);
    glClipPlane(GL_CLIP_PLANE1, p1);

    uint j, line;

    // first search non empty line (before pageScrollIdx) because that
    // line may containe image, and render it
    for (line = pageScrollIdx -1; line < lines.size() && line > 0; line--)
    {
        if (lines[line].chunks.empty())
            continue;
        glPushMatrix();
        glTranslatef(0.0f, (pageScrollIdx - line) * (fh +1), 0.0f);

        for (uint chk = 0; chk < lines[line].chunks.size(); chk++)
        {
            Chunk *chunk = lines[line].chunks[chk];
            chunk->render(&rc);
        }
        overlay->setXoffset(0);
        glPopMatrix();
        break;
    }

    // now render other visible range of lines
    // lines that ha
    std::vector<line_s>::iterator it;
    float totalHeight = 0.0f; // collected heights of rendered lines
    for (j = 0, line = pageScrollIdx;
         line < lines.size() && j < nb_lines; line++, j++)
    {
        float chunkHeight;
        float lineHeight = fh +1;
        for (uint chk = 0; chk < lines[line].chunks.size(); chk++)
        {
            Chunk *chunk = lines[line].chunks[chk];

            if (!(selectedY == (int)line && selectedX == (int)chk))
                chunkHeight = chunk->render(&rc);
            else
            {
                rc.renderSelected = true;
                chunkHeight = chunk->render(&rc);
                rc.renderSelected = false;
            }

            if (chunkHeight > lineHeight)
                lineHeight = chunkHeight;
        }

        overlay->endl();

        // insert empty lines if current line height was more than standart line height (fh +1)
        if (lineHeight > lines[line].height + fh + 1.1f) // 0.1 is epsil
        {
            int lines_to_insert = floor(((float)(lineHeight - lines[line].height)) / (fh +1));
            if (lines_to_insert > 0)
            {
                it = lines.begin() + line +1;
                lines[line].height = (lines_to_insert +1) * (fh +1);
                line_s l;
                lines.insert(it, lines_to_insert, l);
                line += lines_to_insert;
                j += lines_to_insert;
            }
        }
    }

    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);

    totalHeight += fh +1;
    while (j < nb_lines)
    {
        *overlay << "~";
        overlay->endl();
        totalHeight += fh +1;
        j++;
    }

    gc->getOverlay()->endText();
    glPopMatrix();
    // render scroll
    uint scrolled_perc = 0;
    if ((lines.size() - nb_lines) > 0)
        scrolled_perc = 100 * pageScrollIdx / (lines.size() - nb_lines);
    if (scrolled_perc > 100)
        scrolled_perc = 100;

    gc->getOverlay()->beginText();
    {
        glPushMatrix();
        const float scrollW = 5;
        glColor4ubv(clCompletionExpandBg->rgba);
        glTranslatef((float) (width - 5 * fwidth) - scrollW - 4, 0.0f, 0.0f);
        gc->getOverlay()->rect(0, 0.0f - 2, 5 * fwidth, fh);
        glColor4ubv(clCompletionFnt->rgba);
        *gc->getOverlay() << scrolled_perc << "%";
        glPopMatrix();

        // scrollbar
        if (lines.size() > nb_lines)
        {
            float scrollH = height - 2;
            glTranslatef(width - scrollW - 2.0f, -height + fh, 0.0f);
            glColor4ubv(clCompletionExpandBg->rgba);
            gc->getOverlay()->rect(0.0f, 0.0f, scrollW, scrollH);
            // bar
            float scrollHInner = scrollH - 2;
            float scrollSize = scrollHInner / ((float)lines.size() / nb_lines);
            if (scrollSize < 2.0f)
                scrollSize = 2.0f;
            float offsetY = scrollHInner -
                pageScrollIdx * scrollHInner / lines.size() - scrollSize + 1;
            glColor4ubv(clCompletionFnt->rgba);
            gc->getOverlay()->rect(1.0f, offsetY, scrollW - 2.0f, scrollSize);
        }
    }
    gc->getOverlay()->endText();
}

void InfoInteractive::renderInteractive()
{
    glColor4ubv(clInteractiveFnt->rgba);
    if (state == PG_SEARCH_FWD)
        *gc->getOverlay() << "/";
    else if (state == PG_SEARCH_BWD)
        *gc->getOverlay() << "?";
    else
        *gc->getOverlay() << ":";
    *gc->getOverlay() << getBufferText();
}

void InfoInteractive::rebuildDirCache()
{
    dirCache.clear();
    cachedInfoFiles.clear();

    Directory* dir = OpenDirectory(".");

    InfoDirLoader loader(this);
    loader.pushDir(".");
    dir->enumFiles(loader, true);

    delete dir;

    saveDirCache();
}

bool InfoInteractive::addFileToDirCache(string filename)
{
    struct stat fstat;
    long filesize;
    int compressed;

    cout << _("Get dir from info: ") << filename << '\n';

    stat(filename.c_str(), &fstat);
    char *contents = filesys_read_info_file ((char *)filename.c_str(), &filesize, &fstat, &compressed);
    if (!contents)
        return false;

    addDirTxt(contents, filesize, false);

    free(contents);
    return true;
}

// pars content and insert node netry to *dirCache
void InfoInteractive::addDirTxt(char *content, int size, bool dynamic)
{
    dirModified = true;

    SEARCH_BINDING search;
    search.buffer = content;
    search.start = 0;
    search.end = size;

    // only first node contain dir section
    search.end = get_node_length(&search);

# define CURC search.buffer + search.start

    dirSection_t dirSection;

    std::vector <string> sections;

    while(1)
    {
        sections.clear();
        search.flags = S_SkipDest;
        long pos = search_forward("INFO-DIR-SECTION", &search);
        if (-1 == pos)
            break;
        search.start = pos;

        search.start += skip_whitespace(CURC);
        int sec_end = skip_line(CURC);
        if (sec_end > 1)
        {
            sections.push_back(string(CURC, sec_end -1));
        }
        else
        {
            // section w/o name, invalid node
            break;
        }

        // START-INFO-DIR-ENTRY without INFO-DIR-SECTION?
        if (sections.empty())
            break;

        search.start += sec_end;

        // look what tag is nearest
        int next_sec_pos = search_forward("INFO-DIR-SECTION", &search);
        int dir_entry_start = search_forward("START-INFO-DIR-ENTRY", &search);

        // if dir section - continue for add it to section list
        if (next_sec_pos < dir_entry_start &&
            -1 != next_sec_pos)
        {
            //search.start = next_sec_pos;
            continue;
        }

        // dir section w/o dir entry?
        if (-1 == dir_entry_start)
            break;
        search.start = dir_entry_start;

        // search for end of dir entry
        search.flags = 0;
        long dir_entry_end = search_forward("END-INFO-DIR-ENTRY", &search);

        // check for valid
        if (dir_entry_end < 1)
            break;

        // now parse entry menus
        char *c = CURC;
        char *end = search.buffer + dir_entry_end;
        while(c < end)
        {
            if ('*' == *c)
            {
                dir_item mitem;
                mitem.dynamic = dynamic;

                c += 2; // skip "* "
                char *menu = c;
                // search end of menu name i.e. str like: "* Cpp: (cpp)."
                while(c < end && !(*c == '\t' ||
                                   *c == ','  ||
                                   *c == INFO_TAGSEP ||
                                   (*c == '.' &&
                                    (
                                        (whitespace_or_newline (*(c+1))) ||
                                        (*(c+1) == ')')))))
                    c++;
                if (c > menu)
                    mitem.menu = string(menu, c - menu);
                c++;

                // rest text is description of menu until next menu found
                char *descr = c;
                while(c < end && *c != '\n' && *(c+1) != '*')
                    c++;

                if (c > descr)
                    mitem.descr = string(descr, c - descr);

                // add to dirSection mitem
                for (vector<string>::const_iterator it = sections.begin();
                     it < sections.end(); it++)
                {
                    dirSection_t::iterator ds = dirSection.find(*it);
                    if (ds != dirSection.end())
                    {
                        menus *m = &ds->second;

                        bool found = false;
                        for (menus::const_iterator itm = m->begin();
                             itm != m->end(); itm++)
                        {
                            if ((*itm).menu == mitem.menu)
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                            m->push_back(mitem);
                    }
                    else
                    {
                        menus m;
                        m.push_back(mitem);
                        dirSection[*it] = m;
                    }
                }
            }
            c++;
        }

        search.start = dir_entry_end;
        // try search next section
    }

    // concat file current dir section with global dircache
    for (dirSection_t::iterator itl = dirSection.begin();
         itl != dirSection.end(); itl++ )
    {
        dirSection_t::iterator ds = dirCache.find((*itl).first);
        if (ds != dirCache.end())
        {
            menus ml = (*itl).second;
            menus *mg = &ds->second;
            for (menus::const_iterator itml = ml.begin();
                 itml != ml.end(); itml++)
                {
                    bool found = false;
                    for (menus::const_iterator itmg = mg->begin();
                         itmg != mg->end(); itmg++)
                    {
                        if ((*itmg).menu == (*itml).menu)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        mg->push_back(*itml);
                }
        }
        else
        {
            dirCache[(*itl).first] = (*itl).second;
        }

    }
}

void InfoInteractive::saveDirCache()
{
    fstream file;
    file.open(dirCache_FileName, fstream::out);
    if (!file.good())
        return;

    file << " Auto generated" << endl;
    for (dirSection_t::const_iterator it = dirCache.begin();
         it != dirCache.end(); it++ )
    {
        menus ml = (*it).second;

        file <<  "INFO-DIR-SECTION "
             << (*it).first << endl;

        file << "START-INFO-DIR-ENTRY" << endl;

        for (menus::const_iterator itml = ml.begin();
             itml != ml.end(); itml++)
            if (!(*itml).dynamic)
                file << "* " << (*itml).menu << ".       " << (*itml).descr << endl;
        file << "END-INFO-DIR-ENTRY" << endl << endl;
    }

    file.close();

    // info file list
    file.open(infoFileListCache_FileName, fstream::out);
    if (!file.good())
        return;

    map<string, string>::const_iterator it;
    for (it = cachedInfoFiles.begin(); it != cachedInfoFiles.end(); it++)
        file << (*it).first << endl << (*it).second << endl;
    file.close();
}

bool InfoInteractive::loadDirCache()
{
    dirCache.clear();
    cachedInfoFiles.clear();

    fstream file;
    file.open(infoFileListCache_FileName, fstream::in);
    if (!file.good())
        return false;

    while(!file.eof())
    {
        string name;
        string fullname;
        file >> name >> fullname;
        cachedInfoFiles[name] = fullname;
    }

    file.close();

    return addFileToDirCache(dirCache_FileName);
}

void InfoInteractive::addCachedInfoFile(std::string filename, std::string fullname)
{
    cachedInfoFiles[filename] = fullname;
}

void InfoInteractive::registerDynamicNode(std::string filename, GCDynNode dynNode)
{
    dynNodes[filename] = dynNode;
}

NODE *InfoInteractive::getDynamicNode(char *filename, char *nodename)
{
    // create dir node from cache
    if (is_dir_name(filename))
    {
        if (dirNode.contents && !dirModified)
            return &dirNode;

        dirModified = false;

        maybe_free(dirNode.contents);

        dirNode.filename = "dir";
        dirNode.parent = NULL;
        dirNode.nodename = "Top";
        dirNode.flags = 0;
        dirNode.display_pos = 0;
        std::stringstream ss;
        ss << "File: dir,\tNode: Top\t"
           << endl << endl;
        ss << _(" This (the Directory node) gives a menu of major topics.\n");
        ss << "\n\n* Menu:\n";

        for (dirSection_t::const_iterator it=dirCache.begin();
             it != dirCache.end(); it++ )
        {
            menus ml = (*it).second;

            ss <<  endl << (*it).first << endl;

            for (menus::const_iterator itml = ml.begin();
                 itml != ml.end(); itml++)
                ss << "* " << (*itml).menu << ".       " << (*itml).descr << endl;
        }

        dirNode.nodelen = ss.str().size();
        dirNode.contents = (char *)malloc(dirNode.nodelen);
        memcpy(dirNode.contents, ss.str().c_str(), dirNode.nodelen);
        return &dirNode;
    }

    dynNodes_t::iterator it = dynNodes.find(filename);
    if (it != dynNodes.end())
    {
        maybe_free(dynNode.contents);
        GCDynNode dn = it->second;
        string nodetext = dn(filename, nodename);
        dynNode.filename = filename;
        dynNode.parent = NULL;
        dynNode.nodename = nodename;
        dynNode.flags = 0;
        dynNode.display_pos = 0;
        dynNode.nodelen = nodetext.size();
        dynNode.contents = (char *)malloc(dynNode.nodelen);
        memcpy(dynNode.contents, nodetext.data(), dynNode.nodelen);
        return &dynNode;
    }

    return NULL;
}

const char *InfoInteractive::getInfoFullPath(string &filename)
{
    return getInfoFullPath(const_cast<char *>(filename.c_str()));
}

const char *InfoInteractive::getInfoFullPath(char *filename)
{
    map<string, string>::const_iterator it =
        cachedInfoFiles.find(filename);
    if (it != cachedInfoFiles.end())
        return (&it->second)->c_str();
    else
        return "";
}

NODE *getDynamicNode(char *filename, char *nodename)
{
    return infoInteractive->getDynamicNode(filename, nodename);
}

char *getInfoFullPath(char *filename)
{
    return const_cast<char *>(infoInteractive->getInfoFullPath(filename));
}
