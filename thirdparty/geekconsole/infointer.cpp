#include "geekconsole.h"
#include "gcinfo/nodes.h"
#include "gcinfo/info-utils.h"
#include "infointer.h"

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

  This interactive  is emulate `less' command  with several navigation
  command for info-specified facility. */

const char *hint_follownode = "S-RET - follow node";

std::string describeChunk(InfoInteractive::chunk_s *chk)
{
    if (chk->type == InfoInteractive::chunk_s::NODE)
    {
        std::string text = "Node: ";
        text += chk->text;
        if (!chk->filename.empty())
        {
            text += "\nFile: ";
            text += chk->filename;
        }
        return text;
    }
}

InfoInteractive::InfoInteractive(std::string name)
    :GCInteractive(name, false),
     pageScrollIdx(0),
     font(NULL),
     selectedY(1),
     selectedX(-1)
{
}

void InfoInteractive::setFont(TextureFont* _font)
{
    if (!gc->isVisible)
        font = _font;
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
    Chunks chunks;

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
        chunks.push_back(_("Node not found."));
        lines.push_back(chunks);
        if (info_recent_file_error)
        {
            chunks.clear();
            chunks.push_back(info_recent_file_error);
            lines.push_back(chunks);
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
    Chunks chunks;

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
        chunks.push_back(chunk_s(_("File: ")));
        chunk_s c(info_parsed_nodename);
        c.type = chunk_s::TEXT2;
        chunks.push_back(c);
    }

    info_node_label_of_line(contents);
    if (info_parsed_nodename)
    {
        chunks.push_back(chunk_s(_(",  Node: ")));
        chunk_s c(info_parsed_nodename);
        c.type = chunk_s::TEXT2;
        chunks.push_back(c);
    }

#define CHECKLABEL(_textm, _nodeset)                                    \
    if (info_parsed_filename || info_parsed_nodename)                   \
    {                                                                   \
        chunks.push_back(chunk_s(_(",  " _textm ": ")));                \
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
        chunks.push_back(chunk_s(label, filename, nodename,             \
                                 info_parsed_line_number));             \
        _nodeset.set(filename, nodename, info_parsed_line_number);      \
    }

    info_next_label_of_line(contents);
    CHECKLABEL("Next", node_next);

    // prev or previous label
    info_prev_label_of_line(contents);
    CHECKLABEL("Previous", node_prev);

    info_up_label_of_line(contents);
    CHECKLABEL("Up", node_up);

    lines.push_back(chunks);

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
    Chunks chunks;

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
                    chunks.push_back(chunk_s(28)); // format comment of menu to right align
                    lstart += skip_whitespace(lstart);
                    state = MENU_DESCR;
                }
            }
            else
            {
                if (chunks.size() == 0 && c - lstart > 0)
                {
                    int i;
                    for (i = 0; i < c - lstart; i++)
                    {
                        if (!(lstart[i] == '-' || lstart[i] == ' ' ||
                              lstart[i] == '*' || lstart[i] == '='))
                            break;
                    }
                    if (i == c - lstart)
                    {
                        if (c[-1] == '*')
                            chunks.push_back(chunk_s(chunk_s::SEPARATOR1));
                        else if (c[-1] == '=')
                            chunks.push_back(chunk_s(chunk_s::SEPARATOR2));
                        else if (c[-1] == '-')
                            chunks.push_back(chunk_s(chunk_s::SEPARATOR3));
                        goto appendline;
                    }
                }
            }
            chunks.push_back(chunk_s(lstart, c - lstart));
        appendline:
            lines.push_back(chunks);
            chunks.clear();
            lstart = c + 1;
        }
        else if (*c == '`' && state == TEXTMODE) // highlight text between `'
        {
            c++; // push '`'also
            chunks.push_back(chunk_s(lstart, c - lstart));
            lstart = c;
            while(c < end && *c != '\'')
            {
                if (*c == '\n')
                {   // push current line and continue search for '\''
                    chunk_s chk(lstart, c - lstart);
                    chk.type = chunk_s::TEXT2;
                    chunks.push_back(chk);
                    lines.push_back(chunks);
                    chunks.clear();
                    lstart = c +1;
                }
                c++;
            }
            chunk_s chk(lstart, c - lstart);
            chk.type = chunk_s::TEXT2;
            chunks.push_back(chk);
            lstart = c;
        }
        else if (*c == '\t') // just convert tab to 8 spaces, TODO: add real tabs
        {
            if (c -1 - lstart > 0)
                chunks.push_back(chunk_s(lstart, c -1 - lstart));
            chunks.push_back("        ");
            lstart = c +1;
        }
        // "^\* "
        else if (*c == '*') // menu or *Note
        {
            // if "* " at line begin - menu item found
            bool search_for_menu = (c[1] == ' ' && lstart == c);
            if (search_for_menu) {
                chunks.push_back("* ");
                c+=2;
            } else {
                // check for XREF
                if (string_in_line(INFO_XREF_LABEL, c) != strlen(INFO_XREF_LABEL)) {
                    c++;
                    continue; // seems no XREF
                }
                // replace *Note to See
                if (c - lstart > 0)
                    chunks.push_back(chunk_s(lstart, c - lstart));
                //chunks.push_back(chunk_s(lstart, c - lstart + strlen(INFO_XREF_LABEL)));
                chunks.push_back(chunk_s("See "));
                c += strlen(INFO_XREF_LABEL);
            }
            lstart = c;
            int offset = string_in_line (":", c);
            // menu item must have ":"
            if (offset < 1  || c[offset] == '\n')
                continue;

            if (search_for_menu)
                state = MENU_HINT;

            std::string label(c, offset -1);
            std::string file_name;
            std::string node_name;
            int         line_number = 0;

            // TODO: need canonicalize whitespace for label

            c += offset;
            /* If this menu entry continues with another ':' then the
               nodename is the same as the label. */
            if (*c == ':')
            {
                node_name = label;
                c++;
            }
            else
            {
                c += skip_whitespace_and_newlines (c);
                c = info_parse_node (c, DONT_SKIP_NEWLINES);

                if (info_parsed_filename)
                    file_name = info_parsed_filename;

                if (info_parsed_nodename)
                    node_name = info_parsed_nodename;

                line_number = info_parsed_line_number -2; // -2 is visible line correction
            }
            chunks.push_back(chunk_s(label, file_name, node_name, line_number));
            if (search_for_menu)
                menu.push_back(node_s(file_name, node_name, line_number));
            lstart = c;
            continue;
        }
        c++;
    }
}

void InfoInteractive::charEntered(const char *c_p, int modifiers)
{
    char c = *c_p;

    if (pageScrollIdx >= (int) lines.size())
        pageScrollIdx = lines.size() - 1;
    if (pageScrollIdx < 0)
        pageScrollIdx = 0;
    if (selectedY > pageScrollIdx + scrollSize)
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
                for (uint chk = lines[i].size() - 1; chk >= 0; chk--)
                {
                    chunk_s *chunk = &lines[i][chk];
                    if (chunk->text.find(lastSearchStr) != string::npos)
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
            for (int i = pageScrollIdx +1; i < lines.size(); i++)
            {
                for (uint chk = 0; chk < lines[i].size(); chk++)
                {
                    chunk_s *chunk = &lines[i][chk];
                    if (chunk->text.find(lastSearchStr) != string::npos)
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
    if (selectedY > pageScrollIdx + scrollSize)
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
                   c == 'e' || c == CTRL_E || c == 'j' || c == CTRL_J) {
            if (modifiers & GeekBind::SHIFT)
            {
                if (selectedY != -1 && selectedX != -1)
                {
                    chunk_s *chunk = &lines[selectedY][selectedX];
                    if (chunk->type == chunk_s::NODE)
                    {
                        setNode(chunk->filename, chunk->hyp, chunk->linenumber);
                    }
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
            if (selectedY == -1 || selectedY >= lines.size())
                i = pageScrollIdx;
            if (selectedX == -1)
                selectedX = 0;
            else
                selectedX++;
            // try search link in current selected line for hint
            for (int chk = selectedX; chk < lines[i].size(); chk++)
            {
                chunk_s *chunk = &lines[i][chk];
                if (chunk->type == chunk_s::LINK || chunk->type == chunk_s::NODE)
                {
                    selectedY = i;
                    selectedX = chk;
                    gc->setInfoText(describeChunk(chunk));
                    gc->appendDescriptionStr(_(hint_follownode));
                    return;
                }
            }

            i++; // next lines
            vector<Chunks>::const_iterator it = lines.begin();
            while (i < pageScrollIdx + scrollSize && i < lines.size())
            {
                for (int chk = 0; chk < lines[i].size(); chk++)
                {
                    chunk_s *chunk = &lines[i][chk];
                    if (chunk->type == chunk_s::LINK || chunk->type == chunk_s::NODE)
                    {
                        selectedY = i;
                        selectedX = chk;
                        gc->setInfoText(describeChunk(chunk));
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
            if (selectedY == -1 || selectedY >= lines.size())
                i = pageScrollIdx + scrollSize -1;
            if (i >= lines.size())
                i = lines.size() -1;
            if (selectedX == -1)
                selectedX = lines[i].size();
            else
                selectedX--;
            // try back search link in current selected line
            for (int chk = selectedX; chk >= 0; chk--)
            {
                chunk_s *chunk = &lines[i][chk];
                if (chunk->type == chunk_s::LINK || chunk->type == chunk_s::NODE)
                {
                    selectedY = i;
                    selectedX = chk;
                    gc->setInfoText(describeChunk(chunk));
                    gc->appendDescriptionStr(_(hint_follownode));
                    return;
                }
            }

            i--; // next lines
            vector<Chunks>::const_iterator it = lines.begin();
            while (i >= 0)
            {
                for (int chk = lines[i].size(); chk >= 0; chk--)
                {
                    chunk_s *chunk = &lines[i][chk];
                    if (chunk->type == chunk_s::LINK || chunk->type == chunk_s::NODE)
                    {
                        selectedY = i;
                        selectedX = chk;
                        gc->setInfoText(describeChunk(chunk));
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
    scrollSize = nb_lines;
    this->width = width;

    if (pageScrollIdx < 0)
        pageScrollIdx = 0;

    glColor4ubv(clCompletionFnt->rgba);
    glPushMatrix();
    glTranslatef((float) -leftScrollIdx, 0.0f, 0.0f);
    gc->getOverlay()->beginText();

    uint j, line;
    for (j = 0, line = pageScrollIdx;
         line < lines.size() && j < nb_lines; line++, j++)
    {
        for (uint chk = 0; chk < lines[line].size(); chk++)
        {
            chunk_s *chunk = &lines[line][chk];
            if (chunk->type == chunk_s::TEXT)
                *gc->getOverlay() << chunk->text;
            else if (chunk->type == chunk_s::TEXT2)
            {
                gc->getOverlay()->rect(gc->getOverlay()->getXoffset(), -2,
                                       font->getWidth(chunk->text), 1);
                *gc->getOverlay() << chunk->text;

            }
            else if (chunk->type == chunk_s::LINK || chunk->type == chunk_s::NODE)
            {
                if (selectedY == line && selectedX == chk)
                {  // selected item color setup
                    glColor4ubv(clCompletionExpandBg->rgba);
                    gc->getOverlay()->rect(gc->getOverlay()->getXoffset(), -2,
                                           font->getWidth(chunk->text), fh);
                }
                glColor4ubv(clCompletionAfterMatch->rgba);
                *gc->getOverlay() << chunk->text;
                glColor4ubv(clCompletionFnt->rgba);
            }
            else if (chunk->type == chunk_s::HSPACE)
            {
                const float xoffset = gc->getOverlay()->getXoffset();
                short basesz = font->getAdvance('X');
                short spacesz = font->getAdvance(' ');
                int n = ((float)chunk->spaces * basesz - xoffset) / spacesz;
                if (n <= 0)
                    n = 1;
                *gc->getOverlay() << string(n, ' ');
            }
            else if (chunk->type == chunk_s::SEPARATOR1) // bold line
            {
                gc->getOverlay()->rect(0, fh/2 + 1, 100.0 * width, 3);
            }
            else if (chunk->type == chunk_s::SEPARATOR2) // ====
            {
                gc->getOverlay()->rect(0, fh/2 + 1, 100.0 * width, 3, false);
            }
            else if (chunk->type == chunk_s::SEPARATOR3) // ----
                gc->getOverlay()->rect(0, fh/2 + 1, 100.0 * width, 1);
        }
        *gc->getOverlay() << "\n";
    }
    for (; j < nb_lines; j++)
    {
        *gc->getOverlay() << "~";
        *gc->getOverlay() << "\n";
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
            float scrollH = height + fh - 2;
            glTranslatef(width - scrollW - 2.0f, -height, 0.0f);
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

