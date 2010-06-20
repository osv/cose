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

/*
  GeekConsole

  GeekConsole used for interacting using callback function like this:

int _somefun(GeekConsole *gc, int state, std::string value)
{
    switch (state)
    {
    case 1:
        if (value == "yes")
            exit(1);
    case 0:
        // setup interactive
        gc->setInteractive(listInteract, "quit", "Are You Sure?", "Quit from game");
        listInteractive->setCompletionFromSemicolonStr("yes;no");
        listInteractive->setMatchCompletion(true); // default false
    }
    return state;
}

geekConsole->registerFunction(GCFunc(_somefun), "quit");

  Or you can create simple "void" callback:

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

eekConsole->registerAndBind("Global", "C-x f",
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

  You can pass same parameter to interactive using keybind or alias function
  Example:

geekConsole->bind("", "C-x C-g e @Sol/Earth@*EXEC*@goto object@",
                    "select object");

  Here binded key "C-x C-g e" to select Sol/Earth end execute other
  interactive "goto object" (by passing parameter *EXEC* before).
  Alias example:

geekConsole->registerFunction(GCFunc("quit", "@yes@"), "force quit");

  Special parameters:
  - *ESC* stop current interactive;
  - *NILL* pass empty value into interactive's promt;
  - *EXEC* finish current interactive and start other.

  For first exec of fun state is 0.
  To describe current entered text, callback is called with state (-currentState - 1)
  State will inc by 1 when GCInteractive is finished

  Key binds:
  C-s C-S-s......... Change size of console
  C-g............... Cancel interactive
  C-s-g............. Break intractive and finish macro record
  Esc............... Cancel interactive
  C-h............... Show documentation about current interactive function
  C-p............... Prev in history
  C-n............... Next in history
  C-z............... Remove expanded part from input
  C-w............... Kill backword
  C-u............... Kill whole line
  TAB............... Try complete or scroll completion
  C-TAB............. Scroll back completion
  M-/............... Expand next
  M-?............... Expand prev
  M-c............... cel object promt: select & center view
*/

#include "geekconsole.h"
#include <celutil/directory.h>
#include <celengine/starbrowser.h>
#include <celengine/marker.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <celutil/formatnum.h>
#include <celengine/eigenport.h>

using namespace Eigen;

#ifndef CEL_DESCR_SEP
#define CEL_DESCR_SEP "; "
#endif

// margin from bottom
#define BOTTOM_MARGIN 5.0

// lines of info text
#define MAX_LINES_INFO 6

Color32 clBackgroundD(100, 100, 250,200);
Color32 *clBackground = &clBackgroundD;
Color32 clBgInteractiveD(0, 0, 0, 150);
Color32 *clBgInteractive = &clBgInteractiveD;
Color32 clBgInteractiveBrdD(255, 255, 255, 200);
Color32 *clBgInteractiveBrd = &clBgInteractiveBrdD;
Color32 clInteractiveFntD(255, 255, 255, 255);
Color32 *clInteractiveFnt =&clInteractiveFntD;
Color32 clInteractivePrefixFntD(255, 155, 255, 255);
Color32 *clInteractivePrefixFnt = &clInteractivePrefixFntD;
Color32 clInteractiveExpandD(255, 200, 200, 255);
Color32 *clInteractiveExpand = &clInteractiveExpandD;
Color32 clDescrFntD(200, 255, 200, 255);
Color32 *clDescrFnt = &clDescrFntD;
Color32 clCompletionFntD(255, 255, 255, 255);
Color32 *clCompletionFnt = &clCompletionFntD;
Color32 clCompletionMatchCharFntD(0, 0, 0, 255);
Color32 *clCompletionMatchCharFnt = &clCompletionMatchCharFntD;
Color32 clCompletionAfterMatchD(0, 255, 0, 255);
Color32 *clCompletionAfterMatch = &clCompletionAfterMatchD;
Color32 clCompletionMatchCharBgD(255, 0, 0, 255);
Color32 *clCompletionMatchCharBg = &clCompletionMatchCharBgD;
Color32 clCompletionExpandBgD(0, 0, 0, 200);
Color32 *clCompletionExpandBg = &clCompletionExpandBgD;
Color32 clInfoTextFntD(0, 0, 0, 255);
Color32 *clInfoTextFnt = &clInfoTextFntD;
Color32 clInfoTextBgD(255, 255, 50, 210);
Color32 *clInfoTextBg = &clInfoTextBgD;
Color32 clInfoTextBrdD(255, 255, 255, 200);
Color32 *clInfoTextBrd = &clInfoTextBrdD;

GeekConsole *geekConsole = NULL;
const char *ctrlZDescr = "C-z Unexpand";
const char *applySelected = "S-RET - set completion";
const char *ret4Finish = "RET - apply";
const char *scrollPages = "%i of %i pages, M-/ next expand, M-? prev expand";

std::string historyDir("history/");

int completionStyle = ListInteractive::Fast;

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

CustomDescribeSelection customDescribeSelection = NULL;

typedef struct colortbl {
    const char *colorName;
    const char *colorHexName;
};

static colortbl colorTable[] = {
    {"black", "#000000"},
    {"dim gray", "#696969"},
    {"dark grey", "#a9a9a9"},
    {"gray", "#bebebe"},
    {"light grey", "#d3d3d3"},
    {"gainsboro", "#dcdcdc"},
    {"white smoke", "#f5f5f5"},
    {"white", "#ffffff"},
    {"red", "#ff0000"},
    {"orange red", "#ff4500"},
    {"dark orange", "#ff8c00"},
    {"orange", "#ffa500"},
    {"gold", "#ffd700"},
    {"yellow", "#ffff00"},
    {"chartreuse", "#7fff00"},
    {"lawn green", "#7cfc00"},
    {"green", "#00ff00"},
    {"spring green", "#00ff7f"},
    {"medium spring green", "#00fa9a"},
    {"cyan", "#00ffff"},
    {"deep sky blue", "#00bfff"},
    {"blue", "#0000ff"},
    {"medium blue", "#0000cd"},
    {"dark violet", "#9400d3"},
    {"dark magenta", "#8b008b"},
    {"magenta", "#ff00ff"},
    {"dark red", "#8b0000"},
    {"brown", "#a52a2a"},
    {"firebrick", "#b22222"},
    {"indian red", "#cd5c5c"},
    {"light coral", "#f08080"},
    {"salmon", "#fa8072"},
    {"light salmon", "#ffa07a"},
    {"tomato", "#ff6347"},
    {"coral", "#ff7f50"},
    {"dark salmon", "#e9967a"},
    {"rosy brown", "#bc8f8f"},
    {"sienna", "#a0522d"},
    {"saddle brown", "#8b4513"},
    {"chocolate", "#d2691e"},
    {"peru", "#cd853f"},
    {"sandy brown", "#f4a460"},
    {"burlywood", "#deb887"},
    {"tan", "#d2b48c"},
    {"navajo white", "#ffdead"},
    {"wheat", "#f5deb3"},
    {"dark goldenrod", "#b8860b"},
    {"goldenrod", "#daa520"},
    {"light goldenrod", "#eedd82"},
    {"pale goldenrod", "#eee8aa"},
    {"cornsilk", "#fff8dc"},
    {"dark khaki", "#bdb76b"},
    {"khaki", "#f0e68c"},
    {"lemon chiffon", "#fffacd"},
    {"dark olive green", "#556b2f"},
    {"olive drab", "#6b8e23"},
    {"yellow green", "#9acd32"},
    {"green yellow", "#adff2f"},
    {"light green", "#90ee90"},
    {"forest green", "#228b22"},
    {"lime green", "#32cd32"},
    {"pale green", "#98fb98"},
    {"dark sea green", "#8fbc8f"},
    {"sea green", "#2e8b57"},
    {"medium sea green", "#3cb371"},
    {"light sea green", "#20b2aa"},
    {"medium aquamarine", "#66cdaa"},
    {"aquamarine", "#7fffd4"},
    {"dark cyan", "#008b8b"},
    {"medium turquoise", "#48d1cc"},
    {"turquoise", "#40e0d0"},
    {"pale turquoise", "#afeeee"},
    {"powder blue", "#b0e0e6"},
    {"light blue", "#add8e6"},
    {"sky blue", "#87ceeb"},
    {"light sky blue", "#87cefa"},
    {"cadet blue", "#5f9ea0"},
    {"steel blue", "#4682b4"},
    {"dark slate gray", "#2f4f4f"},
    {"slate gray", "#708090"},
    {"light slate gray", "#778899"},
    {"royal blue", "#4169e1"},
    {"dodger blue", "#1e90ff"},
    {"cornflower blue", "#6495ed"},
    {"light steel blue", "#b0c4de"},
    {"dark blue", "#00008b"},
    {"navy", "#000080"},
    {"midnight blue", "#191970"},
    {"dark slate blue", "#483d8b"},
    {"slate blue", "#6a5acd"},
    {"medium slate blue", "#7b68ee"},
    {"light slate blue", "#8470ff"},
    {"medium purple", "#9370db"},
    {"blue violet", "#8a2be2"},
    {"purple", "#a020f0"},
    {"dark orchid", "#9932cc"},
    {"medium orchid", "#ba55d3"},
    {"orchid", "#da70d6"},
    {"thistle", "#d8bfd8"},
    {"plum", "#dda0dd"},
    {"violet", "#ee82ee"},
    {"medium violet red", "#c71585"},
    {"violet red", "#d02090"},
    {"pale violet red", "#db7093"},
    {"maroon", "#b03060"},
    {"deep pink", "#ff1493"},
    {"hot pink", "#ff69b4"},
    {"pink", "#ffc0cb"},
    {"light pink", "#ffb6c1"},
    {"snow", "#fffafa"},
    {"misty rose", "#ffe4e1"},
    {"seashell", "#fff5ee"},
    {"peach puff", "#ffdab9"},
    {"linen", "#faf0e6"},
    {"antique white", "#faebd7"},
    {"bisque", "#ffe4c4"},
    {"papaya whip", "#ffefd5"},
    {"moccasin", "#ffe4b5"},
    {"blanched almond", "#ffebcd"},
    {"old lace", "#fdf5e6"},
    {"floral white", "#fffaf0"},
    {"beige", "#f5f5dc"},
    {"light yellow", "#ffffe0"},
    {"light goldenrod ye", "fafad2"},
    {"ivory", "#fffff0"},
    {"honeydew", "#f0fff0"},
    {"mint cream", "#f5fffa"},
    {"light cyan", "#e0ffff"},
    {"azure", "#f0ffff"},
    {"alice blue", "#f0f8ff"},
    {"lavender", "#e6e6fa"},
    {"ghost white", "#f8f8ff"},
    {"lavender blush", "#fff0f5"},
    {"red4", "#8b0000"},
    {"red3", "#cd0000"},
    {"red2", "#ee0000"},
    {"red1", "#ff0000"},
    {"OrangeRed4", "#8b2500"},
    {"OrangeRed3", "#cd3700"},
    {"OrangeRed2", "#ee4000"},
    {"OrangeRed1", "#ff4500"},
    {"DarkOrange4", "#8b4500"},
    {"DarkOrange3", "#cd6600"},
    {"DarkOrange2", "#ee7600"},
    {"DarkOrange1", "#ff7f00"},
    {"orange4", "#8b5a00"},
    {"orange3", "#cd8500"},
    {"orange2", "#ee9a00"},
    {"orange1", "#ffa500"},
    {"gold4", "#8b7500"},
    {"gold3", "#cdad00"},
    {"gold2", "#eec900"},
    {"gold1", "#ffd700"},
    {"yellow4", "#8b8b00"},
    {"yellow3", "#cdcd00"},
    {"yellow2", "#eeee00"},
    {"yellow1", "#ffff00"},
    {"chartreuse4", "#458b00"},
    {"chartreuse3", "#66cd00"},
    {"chartreuse2", "#76ee00"},
    {"chartreuse1", "#7fff00"},
    {"green4", "#008b00"},
    {"green3", "#00cd00"},
    {"green2", "#00ee00"},
    {"green1", "#00ff00"},
    {"SpringGreen4", "#008b45"},
    {"SpringGreen3", "#00cd66"},
    {"SpringGreen2", "#00ee76"},
    {"SpringGreen1", "#00ff7f"},
    {"cyan4", "#008b8b"},
    {"cyan3", "#00cdcd"},
    {"cyan2", "#00eeee"},
    {"cyan1", "#00ffff"},
    {"turquoise4", "#00868b"},
    {"turquoise3", "#00c5cd"},
    {"turquoise2", "#00e5ee"},
    {"turquoise1", "#00f5ff"},
    {"DeepSkyBlue4", "#00688b"},
    {"DeepSkyBlue3", "#009acd"},
    {"DeepSkyBlue2", "#00b2ee"},
    {"DeepSkyBlue1", "#00bfff"},
    {"blue4", "#00008b"},
    {"blue3", "#0000cd"},
    {"blue2", "#0000ee"},
    {"blue1", "#0000ff"},
    {"magenta4", "#8b008b"},
    {"magenta3", "#cd00cd"},
    {"magenta2", "#ee00ee"},
    {"magenta1", "#ff00ff"},
    {"brown4", "#8b2323"},
    {"brown3", "#cd3333"},
    {"brown2", "#ee3b3b"},
    {"brown1", "#ff4040"},
    {"firebrick4", "#8b1a1a"},
    {"firebrick3", "#cd2626"},
    {"firebrick2", "#ee2c2c"},
    {"firebrick1", "#ff3030"},
    {"IndianRed4", "#8b3a3a"},
    {"IndianRed3", "#cd5555"},
    {"IndianRed2", "#ee6363"},
    {"IndianRed1", "#ff6a6a"},
    {"RosyBrown4", "#8b6969"},
    {"RosyBrown3", "#cd9b9b"},
    {"RosyBrown2", "#eeb4b4"},
    {"RosyBrown1", "#ffc1c1"},
    {"snow4", "#8b8989"},
    {"snow3", "#cdc9c9"},
    {"snow2", "#eee9e9"},
    {"snow1", "#fffafa"},
    {"MistyRose4", "#8b7d7b"},
    {"MistyRose3", "#cdb7b5"},
    {"MistyRose2", "#eed5d2"},
    {"MistyRose1", "#ffe4e1"},
    {"tomato4", "#8b3626"},
    {"tomato3", "#cd4f39"},
    {"tomato2", "#ee5c42"},
    {"tomato1", "#ff6347"},
    {"coral4", "#8b3e2f"},
    {"coral3", "#cd5b45"},
    {"coral2", "#ee6a50"},
    {"coral1", "#ff7256"},
    {"salmon4", "#8b4c39"},
    {"salmon3", "#cd7054"},
    {"salmon2", "#ee8262"},
    {"salmon1", "#ff8c69"},
    {"LightSalmon4", "#8b5742"},
    {"LightSalmon3", "#cd8162"},
    {"LightSalmon2", "#ee9572"},
    {"LightSalmon1", "#ffa07a"},
    {"sienna4", "#8b4726"},
    {"sienna3", "#cd6839"},
    {"sienna2", "#ee7942"},
    {"sienna1", "#ff8247"},
    {"chocolate4", "#8b4513"},
    {"chocolate3", "#cd661d"},
    {"chocolate2", "#ee7621"},
    {"chocolate1", "#ff7f24"},
    {"seashell4", "#8b8682"},
    {"seashell3", "#cdc5bf"},
    {"seashell2", "#eee5de"},
    {"seashell1", "#fff5ee"},
    {"PeachPuff4", "#8b7765"},
    {"PeachPuff3", "#cdaf95"},
    {"PeachPuff2", "#eecbad"},
    {"PeachPuff1", "#ffdab9"},
    {"tan4", "#8b5a2b"},
    {"tan3", "#cd853f"},
    {"tan2", "#ee9a49"},
    {"tan1", "#ffa54f"},
    {"bisque4", "#8b7d6b"},
    {"bisque3", "#cdb79e"},
    {"bisque2", "#eed5b7"},
    {"bisque1", "#ffe4c4"},
    {"AntiqueWhite4", "#8b8378"},
    {"AntiqueWhite3", "#cdc0b0"},
    {"AntiqueWhite2", "#eedfcc"},
    {"AntiqueWhite1", "#ffefdb"},
    {"burlywood4", "#8b7355"},
    {"burlywood3", "#cdaa7d"},
    {"burlywood2", "#eec591"},
    {"burlywood1", "#ffd39b"},
    {"NavajoWhite4", "#8b795e"},
    {"NavajoWhite3", "#cdb38b"},
    {"NavajoWhite2", "#eecfa1"},
    {"NavajoWhite1", "#ffdead"},
    {"wheat4", "#8b7e66"},
    {"wheat3", "#cdba96"},
    {"wheat2", "#eed8ae"},
    {"wheat1", "#ffe7ba"},
    {"DarkGoldenrod4", "#8b6508"},
    {"DarkGoldenrod3", "#cd950c"},
    {"DarkGoldenrod2", "#eead0e"},
    {"DarkGoldenrod1", "#ffb90f"},
    {"goldenrod4", "#8b6914"},
    {"goldenrod3", "#cd9b1d"},
    {"goldenrod2", "#eeb422"},
    {"goldenrod1", "#ffc125"},
    {"cornsilk4", "#8b8878"},
    {"cornsilk3", "#cdc8b1"},
    {"cornsilk2", "#eee8cd"},
    {"cornsilk1", "#fff8dc"},
    {"LightGoldenrod4", "#8b814c"},
    {"LightGoldenrod3", "#cdbe70"},
    {"LightGoldenrod2", "#eedc82"},
    {"LightGoldenrod1", "#ffec8b"},
    {"LemonChiffon4", "#8b8970"},
    {"LemonChiffon3", "#cdc9a5"},
    {"LemonChiffon2", "#eee9bf"},
    {"LemonChiffon1", "#fffacd"},
    {"khaki4", "#8b864e"},
    {"khaki3", "#cdc673"},
    {"khaki2", "#eee685"},
    {"khaki1", "#fff68f"},
    {"LightYellow4", "#8b8b7a"},
    {"LightYellow3", "#cdcdb4"},
    {"LightYellow2", "#eeeed1"},
    {"LightYellow1", "#ffffe0"},
    {"ivory4", "#8b8b83"},
    {"ivory3", "#cdcdc1"},
    {"ivory2", "#eeeee0"},
    {"ivory1", "#fffff0"},
    {"OliveDrab4", "#698b22"},
    {"OliveDrab3", "#9acd32"},
    {"OliveDrab2", "#b3ee3a"},
    {"OliveDrab1", "#c0ff3e"},
    {"DarkOliveGreen4", "#6e8b3d"},
    {"DarkOliveGreen3", "#a2cd5a"},
    {"DarkOliveGreen2", "#bcee68"},
    {"DarkOliveGreen1", "#caff70"},
    {"PaleGreen4", "#548b54"},
    {"PaleGreen3", "#7ccd7c"},
    {"PaleGreen2", "#90ee90"},
    {"PaleGreen1", "#9aff9a"},
    {"DarkSeaGreen4", "#698b69"},
    {"DarkSeaGreen3", "#9bcd9b"},
    {"DarkSeaGreen2", "#b4eeb4"},
    {"DarkSeaGreen1", "#c1ffc1"},
    {"honeydew4", "#838b83"},
    {"honeydew3", "#c1cdc1"},
    {"honeydew2", "#e0eee0"},
    {"honeydew1", "#f0fff0"},
    {"SeaGreen4", "#2e8b57"},
    {"SeaGreen3", "#43cd80"},
    {"SeaGreen2", "#4eee94"},
    {"SeaGreen1", "#54ff9f"},
    {"aquamarine4", "#458b74"},
    {"aquamarine3", "#66cdaa"},
    {"aquamarine2", "#76eec6"},
    {"aquamarine1", "#7fffd4"},
    {"DarkSlateGray4", "#528b8b"},
    {"DarkSlateGray3", "#79cdcd"},
    {"DarkSlateGray2", "#8deeee"},
    {"DarkSlateGray1", "#97ffff"},
    {"PaleTurquoise4", "#668b8b"},
    {"PaleTurquoise3", "#96cdcd"},
    {"PaleTurquoise2", "#aeeeee"},
    {"PaleTurquoise1", "#bbffff"},
    {"LightCyan4", "#7a8b8b"},
    {"LightCyan3", "#b4cdcd"},
    {"LightCyan2", "#d1eeee"},
    {"LightCyan1", "#e0ffff"},
    {"azure4", "#838b8b"},
    {"azure3", "#c1cdcd"},
    {"azure2", "#e0eeee"},
    {"azure1", "#f0ffff"},
    {"CadetBlue4", "#53868b"},
    {"CadetBlue3", "#7ac5cd"},
    {"CadetBlue2", "#8ee5ee"},
    {"CadetBlue1", "#98f5ff"},
    {"LightBlue4", "#68838b"},
    {"LightBlue3", "#9ac0cd"},
    {"LightBlue2", "#b2dfee"},
    {"LightBlue1", "#bfefff"},
    {"LightSkyBlue4", "#607b8b"},
    {"LightSkyBlue3", "#8db6cd"},
    {"LightSkyBlue2", "#a4d3ee"},
    {"LightSkyBlue1", "#b0e2ff"},
    {"SkyBlue4", "#4a708b"},
    {"SkyBlue3", "#6ca6cd"},
    {"SkyBlue2", "#7ec0ee"},
    {"SkyBlue1", "#87ceff"},
    {"SteelBlue4", "#36648b"},
    {"SteelBlue3", "#4f94cd"},
    {"SteelBlue2", "#5cacee"},
    {"SteelBlue1", "#63b8ff"},
    {"DodgerBlue4", "#104e8b"},
    {"DodgerBlue3", "#1874cd"},
    {"DodgerBlue2", "#1c86ee"},
    {"DodgerBlue1", "#1e90ff"},
    {"SlateGray4", "#6c7b8b"},
    {"SlateGray3", "#9fb6cd"},
    {"SlateGray2", "#b9d3ee"},
    {"SlateGray1", "#c6e2ff"},
    {"LightSteelBlue4", "#6e7b8b"},
    {"LightSteelBlue3", "#a2b5cd"},
    {"LightSteelBlue2", "#bcd2ee"},
    {"LightSteelBlue1", "#cae1ff"},
    {"RoyalBlue4", "#27408b"},
    {"RoyalBlue3", "#3a5fcd"},
    {"RoyalBlue2", "#436eee"},
    {"RoyalBlue1", "#4876ff"},
    {"SlateBlue4", "#473c8b"},
    {"SlateBlue3", "#6959cd"},
    {"SlateBlue2", "#7a67ee"},
    {"SlateBlue1", "#836fff"},
    {"MediumPurple4", "#5d478b"},
    {"MediumPurple3", "#8968cd"},
    {"MediumPurple2", "#9f79ee"},
    {"MediumPurple1", "#ab82ff"},
    {"purple4", "#551a8b"},
    {"purple3", "#7d26cd"},
    {"purple2", "#912cee"},
    {"purple1", "#9b30ff"},
    {"DarkOrchid4", "#68228b"},
    {"DarkOrchid3", "#9a32cd"},
    {"DarkOrchid2", "#b23aee"},
    {"DarkOrchid1", "#bf3eff"},
    {"MediumOrchid4", "#7a378b"},
    {"MediumOrchid3", "#b452cd"},
    {"MediumOrchid2", "#d15fee"},
    {"MediumOrchid1", "#e066ff"},
    {"thistle4", "#8b7b8b"},
    {"thistle3", "#cdb5cd"},
    {"thistle2", "#eed2ee"},
    {"thistle1", "#ffe1ff"},
    {"plum4", "#8b668b"},
    {"plum3", "#cd96cd"},
    {"plum2", "#eeaeee"},
    {"plum1", "#ffbbff"},
    {"orchid4", "#8b4789"},
    {"orchid3", "#cd69c9"},
    {"orchid2", "#ee7ae9"},
    {"orchid1", "#ff83fa"},
    {"maroon4", "#8b1c62"},
    {"maroon3", "#cd2990"},
    {"maroon2", "#ee30a7"},
    {"maroon1", "#ff34b3"},
    {"DeepPink4", "#8b0a50"},
    {"DeepPink3", "#cd1076"},
    {"DeepPink2", "#ee1289"},
    {"DeepPink1", "#ff1493"},
    {"HotPink4", "#8b3a62"},
    {"HotPink3", "#cd6090"},
    {"HotPink2", "#ee6aa7"},
    {"HotPink1", "#ff6eb4"},
    {"VioletRed4", "#8b2252"},
    {"VioletRed3", "#cd3278"},
    {"VioletRed2", "#ee3a8c"},
    {"VioletRed1", "#ff3e96"},
    {"LavenderBlush4", "#8b8386"},
    {"LavenderBlush3", "#cdc1c5"},
    {"LavenderBlush2", "#eee0e5"},
    {"LavenderBlush1", "#fff0f5"},
    {"PaleVioletRed4", "#8b475d"},
    {"PaleVioletRed3", "#cd6889"},
    {"PaleVioletRed2", "#ee799f"},
    {"PaleVioletRed1", "#ff82ab"},
    {"pink4", "#8b636c"},
    {"pink3", "#cd919e"},
    {"pink2", "#eea9b8"},
    {"pink1", "#ffb5c5"},
    {"LightPink4", "#8b5f65"},
    {"LightPink3", "#cd8c95"},
    {"LightPink2", "#eea2ad"},
    {"LightPink1", "#ffaeb9"},
    {"gray0", "#000000"},
    {"gray1", "#030303"},
    {"gray2", "#050505"},
    {"gray3", "#080808"},
    {"gray4", "#0a0a0a"},
    {"gray5", "#0d0d0d"},
    {"gray6", "#0f0f0f"},
    {"gray7", "#121212"},
    {"gray8", "#141414"},
    {"gray9", "#171717"},
    {"gray10", "#1a1a1a"},
    {"gray11", "#1c1c1c"},
    {"gray12", "#1f1f1f"},
    {"gray13", "#212121"},
    {"gray14", "#242424"},
    {"gray15", "#262626"},
    {"gray16", "#292929"},
    {"gray17", "#2b2b2b"},
    {"gray18", "#2e2e2e"},
    {"gray19", "#303030"},
    {"gray20", "#333333"},
    {"gray21", "#363636"},
    {"gray22", "#383838"},
    {"gray23", "#3b3b3b"},
    {"gray24", "#3d3d3d"},
    {"gray25", "#404040"},
    {"gray26", "#424242"},
    {"gray27", "#454545"},
    {"gray28", "#474747"},
    {"gray29", "#4a4a4a"},
    {"gray30", "#4d4d4d"},
    {"gray31", "#4f4f4f"},
    {"gray32", "#525252"},
    {"gray33", "#545454"},
    {"gray34", "#575757"},
    {"gray35", "#595959"},
    {"gray36", "#5c5c5c"},
    {"gray37", "#5e5e5e"},
    {"gray38", "#616161"},
    {"gray39", "#636363"},
    {"gray40", "#666666"},
    {"gray41", "#696969"},
    {"gray42", "#6b6b6b"},
    {"gray43", "#6e6e6e"},
    {"gray44", "#707070"},
    {"gray45", "#737373"},
    {"gray46", "#757575"},
    {"gray47", "#787878"},
    {"gray48", "#7a7a7a"},
    {"gray49", "#7d7d7d"},
    {"gray50", "#7f7f7f"},
    {"gray51", "#828282"},
    {"gray52", "#858585"},
    {"gray53", "#878787"},
    {"gray54", "#8a8a8a"},
    {"gray55", "#8c8c8c"},
    {"gray56", "#8f8f8f"},
    {"gray57", "#919191"},
    {"gray58", "#949494"},
    {"gray59", "#969696"},
    {"gray60", "#999999"},
    {"gray61", "#9c9c9c"},
    {"gray62", "#9e9e9e"},
    {"gray63", "#a1a1a1"},
    {"gray64", "#a3a3a3"},
    {"gray65", "#a6a6a6"},
    {"gray66", "#a8a8a8"},
    {"gray67", "#ababab"},
    {"gray68", "#adadad"},
    {"gray69", "#b0b0b0"},
    {"gray70", "#b3b3b3"},
    {"gray71", "#b5b5b5"},
    {"gray72", "#b8b8b8"},
    {"gray73", "#bababa"},
    {"gray74", "#bdbdbd"},
    {"gray75", "#bfbfbf"},
    {"gray76", "#c2c2c2"},
    {"gray77", "#c4c4c4"},
    {"gray78", "#c7c7c7"},
    {"gray79", "#c9c9c9"},
    {"gray80", "#cccccc"},
    {"gray81", "#cfcfcf"},
    {"gray82", "#d1d1d1"},
    {"gray83", "#d4d4d4"},
    {"gray84", "#d6d6d6"},
    {"gray85", "#d9d9d9"},
    {"gray86", "#dbdbdb"},
    {"gray87", "#dedede"},
    {"gray88", "#e0e0e0"},
    {"gray89", "#e3e3e3"},
    {"gray90", "#e5e5e5"},
    {"gray91", "#e8e8e8"},
    {"gray92", "#ebebeb"},
    {"gray93", "#ededed"},
    {"gray94", "#f0f0f0"},
    {"gray95", "#f2f2f2"},
    {"gray96", "#f5f5f5"},
    {"gray97", "#f7f7f7"},
    {"gray98", "#fafafa"},
    {"gray99", "#fcfcfc"},
    {"gray100", "#ffffff"},
    NULL
};

Color32 getColor32FromHexText(const string &text)
{
    Color32 c(0,0,0,255);
    if (text.empty())
        return c;
    const char *str = text.c_str();
    int d = (text.length() - 1) / 2;
    if (d > 4) d = 4;
    for (int i = 0; i < d; i++)
    {
        uint r;
        sscanf(&str[i*2 + 1],"%02X",&r);
        c.rgba[i] = r;
    }
    return c;
}

// Text like "green,0.5" or "green,128"return green color with alpha 0.5
// Text maybe hex "#00FF00AA"
Color32 getColor32FromText(const string &text)
{
    Color32 c(0,0,0,0); //default if bad text parse
    if (text.empty())
        return c;
    if (text[0] =='#') // hex
    {
        return getColor32FromHexText(text);
    }
    else
    {
        uint cut = text.find_first_of(',');
        string color = text;
        string alphacolor;
        if (cut != string::npos)
        {
            color = text.substr(0,cut);
            alphacolor = text.substr(cut + 1);
        }
        int i = 0;
        while (colorTable[i].colorName)
        {
            if (UTF8StringCompare(color, colorTable[i].colorName) == 0)
            {
                c = getColor32FromHexText(colorTable[i].colorHexName);
                c.rgba[3] = 255;
                // parse alpha
                if (!alphacolor.empty())
                {// check for float (otherwice it be 0-255 int)
                    cut = alphacolor.find_first_of('.');
                    if (cut != string::npos)
                    {
                        float a = atof(alphacolor.c_str());
                        if (a > 1.0f) a = 1.0f;
                        if (a < 0.0f) a = 0.0f;
                        c.rgba[3] = a * 255;
                    } else {
                        int a = atoi(alphacolor.c_str());
                        if (a > 255) a = 255;
                        if (a < 0) a = 0;
                        c.rgba[3] = a;
                    }
                }
                break;
            }
            i++;
        }
    }
    return c;
}

Color getColorFromText(const string &text)
{
    Color32 c = getColor32FromText(text);
    return Color((float)c.rgba[0] / 255, (float)c.rgba[1] / 255,
                 (float)c.rgba[2] / 255, (float)c.rgba[3] / 255);
}

static FormattedNumber SigDigitNum(double v, int digits)
{
    return FormattedNumber(v, digits,
                           FormattedNumber::GroupThousands |
                           FormattedNumber::SignificantDigits);
}

// Convert Ctrl-Key to key, I can't find same in std.
char removeCtrl(char ckey)
{
    switch (ckey)
    {
    case CTRL_A: return 'a';
    case CTRL_B: return 'b';
    case CTRL_C: return 'c';
    case CTRL_D: return 'd';
    case CTRL_E: return 'e';
    case CTRL_F: return 'f';
    case CTRL_G: return 'g';
    case CTRL_H: return 'h';
    case CTRL_I: return 'i';
    case CTRL_J: return 'j';
    case CTRL_K: return 'k';
    case CTRL_L: return 'l';
    case CTRL_M: return 'm';
    case CTRL_N: return 'n';
    case CTRL_O: return 'o';
    case CTRL_P: return 'p';
    case CTRL_Q: return 'q';
    case CTRL_R: return 'r';
    case CTRL_S: return 's';
    case CTRL_T: return 't';
    case CTRL_U: return 'u';
    case CTRL_V: return 'v';
    case CTRL_W: return 'w';
    case CTRL_X: return 'x';
    case CTRL_Y: return 'y';
    case CTRL_Z: return 'z';
    default:
        break;
    }
    return ckey;
}

// Convert to ctrl key
char toCtrl(char key)
{
    switch (key)
    {
    case 'a': return CTRL_A;
    case 'b': return CTRL_B;
    case 'c': return CTRL_C;
    case 'd': return CTRL_D;
    case 'e': return CTRL_E;
    case 'f': return CTRL_F;
    case 'g': return CTRL_G;
    case 'h': return CTRL_H;
    case 'i': return CTRL_I;
    case 'j': return CTRL_J;
    case 'k': return CTRL_K;
    case 'l': return CTRL_L;
    case 'm': return CTRL_M;
    case 'n': return CTRL_N;
    case 'o': return CTRL_O;
    case 'p': return CTRL_P;
    case 'q': return CTRL_Q;
    case 'r': return CTRL_R;
    case 's': return CTRL_S;
    case 't': return CTRL_T;
    case 'u': return CTRL_U;
    case 'v': return CTRL_V;
    case 'w': return CTRL_W;
    case 'x': return CTRL_X;
    case 'y': return CTRL_Y;
    case 'z': return CTRL_Z;
    default:
        break;
    }
    return key;
}

static void distance2Sstream(std::stringstream &ss, double distance)
{
    const char* units = "";

    if (abs(distance) >= astro::parsecsToLightYears(1e+6))
    {
        units = "Mpc";
        distance = astro::lightYearsToParsecs(distance) / 1e+6;
    }
    else if (abs(distance) >= 0.5 * astro::parsecsToLightYears(1e+3))
    {
        units = "Kpc";
        distance = astro::lightYearsToParsecs(distance) / 1e+3;
    }
    else if (abs(distance) >= astro::AUtoLightYears(1000.0f))
    {
        units = _("ly");
    }
    else if (abs(distance) >= astro::kilometersToLightYears(10000000.0))
    {
        units = _("au");
        distance = astro::lightYearsToAU(distance);
    }
    else if (abs(distance) > astro::kilometersToLightYears(1.0f))
    {
        units = "km";
        distance = astro::lightYearsToKilometers(distance);
    }
    else
    {
        units = "m";
        distance = astro::lightYearsToKilometers(distance) * 1000.0f;
    }

    ss << SigDigitNum(distance, 5) << ' ' << units;
}

static void planetocentricCoords2Sstream(std::stringstream& ss,
                                         const Body& body,
                                         double longitude,
                                         double latitude,
                                         double altitude,
                                         bool showAltitude)
{
    char ewHemi = ' ';
    char nsHemi = ' ';
    double lon = 0.0;
    double lat = 0.0;

    // Terrible hack for Earth and Moon longitude conventions.  Fix by
    // adding a field to specify the longitude convention in .ssc files.
    if (body.getName() == "Earth" || body.getName() == "Moon")
    {
        if (latitude < 0.0)
            nsHemi = 'S';
        else if (latitude > 0.0)
            nsHemi = 'N';

        if (longitude < 0.0)
            ewHemi = 'W';
        else if (longitude > 0.0f)
            ewHemi = 'E';

        lon = (float) abs(radToDeg(longitude));
        lat = (float) abs(radToDeg(latitude));
    }
    else
    {
        // Swap hemispheres if the object is a retrograde rotator
        Quaterniond q = body.getEclipticToEquatorial(astro::J2000);
        bool retrograde = (q * Vector3d::UnitY()).y() < 0.0;

        if ((latitude < 0.0) ^ retrograde)
            nsHemi = 'S';
        else if ((latitude > 0.0) ^ retrograde)
            nsHemi = 'N';
        
        if (retrograde)
            ewHemi = 'E';
        else
            ewHemi = 'W';

        lon = -radToDeg(longitude);
        if (lon < 0.0)
            lon += 360.0;
        lat = abs(radToDeg(latitude));
    }

    ss.unsetf(ios::fixed);
    ss << setprecision(6);
    ss << lat << nsHemi << ' ' << lon << ewHemi;
    if (showAltitude)
        ss << ' ' << altitude << _("km");
}

std::string describeSelection(Selection sel, CelestiaCore *celAppCore, bool doCustomDescribe)
{
    std::stringstream ss;
    char buf[128];
    if (!sel.empty())
    {
        if (doCustomDescribe && customDescribeSelection)
            return customDescribeSelection(sel, celAppCore);

        Vec3d v = sel.getPosition(celAppCore->getSimulation()->getTime()) -
            celAppCore->getSimulation()->getObserver().getPosition();
        double distance;
        Star *star;
        SolarSystem* sys;
        double kmDistance;
        Body *body;
        DeepSkyObject *dso;
        Location *location;

        switch (sel.getType())
        {
        case Selection::Type_Star:
            distance = v.length() * 1e-6;
            star = sel.star();
            if (!star->getVisibility())
            {
                ss << _("Star system barycenter\n");
                break;
            } else
                ss << "Star" << CEL_DESCR_SEP;

            ss << _("Distance: ");
            distance2Sstream(ss, distance);
            if (!star->getVisibility())
                break;
            ss << "\n" << _("Radius: ")
               << SigDigitNum(star->getRadius() / 696000.0f, 2)
               << " " << _("Rsun")
               << "  (" << SigDigitNum(star->getRadius(), 3) << " km" << ")"
               << "\n" << _("Class: ");
            if (star->getSpectralType()[0] == 'Q')
                ss <<  _("Neutron star");
            else if (star->getSpectralType()[0] == 'X')
                ss <<  _("Black hole");
            else
                ss << star->getSpectralType();
            ss << CEL_DESCR_SEP;
            sprintf(buf, _("Abs (app) mag: %.2f (%.2f)\n"),
                    star->getAbsoluteMagnitude(),
                    astro::absToAppMag(star->getAbsoluteMagnitude(),
                                       (float) distance));
            ss << buf;
            if (star->getLuminosity() > 1.0e-10f)
                ss << _("Luminosity: ") << SigDigitNum(star->getLuminosity(), 3) << _("x Sun") << CEL_DESCR_SEP;
            ss << _("Surface temp: ") << SigDigitNum(star->getTemperature(), 3) << " K\n";
            sys = celAppCore->getSimulation()->
                getUniverse()->getSolarSystem(star);
            if (sys != NULL && sys->getPlanets()->getSystemSize() != 0)
            {
                int planetsnum = 0;
                int dwarfsnum = 0;
                int astersnum = 0;
                PlanetarySystem *planets = sys->getPlanets();
                for (int i = 0; i < planets->getSystemSize(); i++)
                {
                    switch (planets->getBody(i)->getClassification())
                    {
                    case Body::Planet:
                        planetsnum++;
                        break;
                    case Body::DwarfPlanet:
                        dwarfsnum++;
                        break;
                    case Body::Asteroid:
                        astersnum++;
                        break;
                    default:
                        break;
                    }
                }
                ss << "\n" << planetsnum << "Planets, " << dwarfsnum
                   << " Dwarf, " << astersnum << " Asteroid";
            }
            break;
        case Selection::Type_Body:
            distance = v.length() * 1e-6,
                v * astro::microLightYearsToKilometers(1.0);
            body = sel.body();
            kmDistance = astro::lightYearsToKilometers(distance);
            ss << "Body" << CEL_DESCR_SEP << _("Distance: ");
            distance = astro::kilometersToLightYears(kmDistance - body->getRadius());
            distance2Sstream(ss, distance);
            ss << "\n" << _("Radius: ");
            distance = astro::kilometersToLightYears(body->getRadius());
            distance2Sstream(ss, distance);
            break;
        case Selection::Type_DeepSky:
        {
            dso = sel.deepsky();
            distance = v.length() * 1e-6 - dso->getRadius();
            char descBuf[128];
            dso->getDescription(descBuf, sizeof(descBuf));
            ss << "DSO" << CEL_DESCR_SEP
               << descBuf
               << "\n";
            if (distance >= 0)
            {
                ss << _("Distance: ");
                distance2Sstream(ss, distance);
            }
            else
            {
                ss << _("Distance from center: ");
                distance2Sstream(ss, distance + dso->getRadius());
            }
            ss << "\n" << _("Radius: ");
            distance2Sstream(ss, dso->getRadius());
            break;
        }
        case Selection::Type_Location:
        {
            location = sel.location();
            body = location->getParentBody();
            ss << "Location" << CEL_DESCR_SEP
               << _("Distance: ");
            distance2Sstream(ss, v.length() * 1e-6);
            ss << CEL_DESCR_SEP;
            Vector3f locPos = location->getPosition();
            Vector3d lonLatAlt = body->cartesianToPlanetocentric(locPos.cast<double>());
            planetocentricCoords2Sstream(ss, *body,
                                         lonLatAlt.x(), lonLatAlt.y(), lonLatAlt.z(), false);
            break;
        }
        default:
            break;
        }
    }
    return ss.str();
}

/*
  GeekConsole
 */
GeekConsole::GeekConsole(CelestiaCore *celCore):
    isVisible(false),
    consoleType(Tiny),
    titleFont(NULL),
    font(NULL),
    celCore(celCore),
    overlay(NULL),
    curInteractive(NULL),
    curFun(NULL),
    isMacroRecording(false),
    maxMacroLevel(64),
    beeper(NULL)
{
    overlay = new Overlay();
    *overlay << setprecision(6);
    *overlay << ios::fixed;
    // create global key bind space
    GeekBind *gb = new GeekBind("Global");
    geekBinds.push_back(gb);
    curKey.len = 0;
}

GeekConsole::~GeekConsole()
{
    for_each(geekBinds.begin(), geekBinds.end(), deleteFunc<GeekBind*>());;
    delete overlay;
}

int GeekConsole::execFunction(GCFunc *fun)
{
    finish(); //need clean old interactive
    curFun = fun;
    curFunName.clear();
    funState = 0;
    isVisible = true;
    return call("");
}

int GeekConsole::execFunction(std::string funName)
{
    GCFunc *f = getFunctionByName(funName);
    // if not called from other function
    if (f && isMacroRecording && !curFun)
    {
        appendCurrentMacro("@*EXEC*@" + funName);
    }
    finish();
    curFun = f;
    funState = 0;
    if (f)
    {
        curFunName = funName;
        isVisible = true;
        return call("");
    }
    else
        isVisible = false;
    return 0;
}

int GeekConsole::execFunction(std::string funName, std::string param)
{
    GCFunc *f = getFunctionByName(funName);
    curFun = f;
    funState = 0;
    if (f)
    {
        curFunName = funName;
        isVisible = true;
        call("", true);

        if (param[param.size() - 1] == '@')
            param.resize(param.size() - 1);
        if (param[0] == '@')
            param.erase(0, 1);

        vector<string> params;
        uint cutAt;
        while( (cutAt = param.find_first_of( "@" )) != param.npos )
        {
            if( cutAt > 0 )
            {
                params.push_back( param.substr( 0, cutAt ) );
            }
            param = param.substr( cutAt + 1 );
        }
        if( param.length() > 0 )
        {
            params.push_back( param );
        }

        std::vector<string>::iterator it;
        for (it = params.begin();
             it != params.end(); it++)
        {
            funState++;
            if (*it == "*ESC*")
            {
                finish();
                return funState;
            }
            else if (*it == "*EXEC*")
            {
                finish();
                it++;
                if (it != params.end())
                {
                    GCFunc *f = getFunctionByName(*it);
                    if (!f)
                    {
                        DPRINTF(1, "GFunction not found: '%s'", (*it).c_str());
                        return funState;
                    }
                    isVisible = true;
                    curFun = f;
                    curFunName = *it;
                    funState = 0;
                    call("", true);
                }
                else
                {
                    return funState;
                }
            }
            else if (*it == "*NILL*")
            {
                call("", true);
            }
            else
                call(*it, true);
        }
    }
    else
        isVisible = false;
    return funState;
}

void GeekConsole::describeCurText(std::string text)
{
    if (curFun)
        curFun->call(this, -funState - 1, text);
}

void GeekConsole::registerFunction(GCFunc fun, std::string name)
{
    Functions::iterator it = functions.find(name);
    if (it == functions.end())
    {
        functions[name] = fun;
        DPRINTF(1, "Registering function for geek console: '%s'\n", name.c_str());
    }
}

void GeekConsole::reRegisterFunction(GCFunc fun, std::string name)
{
    functions[name] = fun;
    DPRINTF(1, "Reregistering function for geek console: '%s'\n", name.c_str());
}

GCFunc *GeekConsole::getFunctionByName(std::string name)
{
    Functions::iterator it;
    it = functions.find(name);
    if (it != functions.end())
        return &it->second;
    else return NULL;
}
std::vector<std::string> GeekConsole::getFunctionsNames()
{
    std::vector<std::string> names;
    for (Functions::const_iterator iter = functions.begin();
         iter != functions.end(); iter++)
    {
        const string& alias = iter->first;
        {
            names.push_back(alias);
        }
    }
    return names;
}


/**
   Process key event for interactive if visible console, if not, than
   check hot keys
 */
bool GeekConsole::charEntered(const char *c_p, int modifiers)
{
    char sym;
    if (modifiers & GeekBind::CTRL)
        sym = removeCtrl(*c_p);
    else
        sym = *c_p;

    if (!sym)
        return false;

    if (!isVisible)
    {
        curKey.c[curKey.len] = tolower(sym);
        curKey.mod[curKey.len] = modifiers;
        // clear shit for some spec chars
        if (strchr(nonShiftChars, curKey.c[curKey.len]))
            curKey.mod[curKey.len] &= ~GeekBind::SHIFT;
        curKey.len++;
        if (modifiers & GeekBind::SHIFT)
            curKey.mod[curKey.len] |= GeekBind::SHIFT;
        if (modifiers & GeekBind::CTRL)
            curKey.mod[curKey.len] |= GeekBind::CTRL;
        if (modifiers & GeekBind::META)
            curKey.mod[curKey.len] |= GeekBind::META;
        bool isKeyPrefix = false;
        std::vector<GeekBind::KeyBind>::const_iterator it;
        std::vector<GeekBind *>::iterator gb;
        for (gb  = geekBinds.begin();
             gb != geekBinds.end(); gb++)
        {
            if (!(*gb)->isActive)
                continue;

            std::vector<GeekBind::KeyBind> binds = (*gb)->getBinds();
            for (it = binds.begin();
                 it != binds.end(); it++)
            {
                if (curKey.len > it->len)
                    continue;
                bool eq = true;
                // compare hot-keys
                for (int i = 0; i < curKey.len; i++)
                {
                    if (curKey.mod[i] != it->mod[i] ||
                        curKey.c[i] != it->c[i])
                    {
                        eq = false;
                        break;
                    }
                }
                if (eq)
                    if (curKey.len == it->len)
                    {
                        std::string fun = it->gcFunName;
                        if(fun.empty())
                            fun = "exec function";
                        if (getFunctionByName(fun))
                        {
                            if (curKey.len > 1)
                                getCelCore()->flash(curKey.keyToStr() + " (" + fun +
                                                    ") " + it->params, 2.5);
                            if (isMacroRecording)
                            {
                                appendCurrentMacro("@*EXEC*@" + fun);
                                if (!it->params.empty())
                                    appendCurrentMacro(it->params);
                            }
                            execFunction(fun, it->params);
                        }
                        else
                        {
                            getCelCore()->flash(curKey.keyToStr() + " (" + fun +
                                                    ") not defined", 1.5);
                        }
                        curKey.len = 0;
                        return true;
                    }
                    else
                        isKeyPrefix = true;
            }
        }
        if (!isKeyPrefix || curKey.len >= MAX_KEYBIND_LEN)
        {
            // for key length 1 dont flash messg.
            if (curKey.len > 1)
            {
                getCelCore()->flash(curKey.keyToStr() + " is undefined");
                curKey.len = 0;
                // true because we dont want to continue passing key event
                return true;
            }
            curKey.len = 0;
            return false;
        }
        else
            getCelCore()->flash(curKey.keyToStr() + "-", 15.0);
        return true;
    } // !isVisible

    if (isMacroRecording)
        descriptionStr = _("C-S-g Stop macro here");
    else
        descriptionStr = _("ESC or C-g Cancel");
    clearInfoText();

    bool isCtrl = modifiers & GeekBind::CTRL;
    bool isShift = modifiers & GeekBind::SHIFT;
    char c = tolower(sym);
    if (isCtrl && c == 'h')
    {
        if (curFun)
            setInfoText(curFun->getInfo());
        return true;
    } else if (isCtrl && c == 's') // C-s
    {
        if (isShift)
        {
            if (consoleType <= Tiny)
                consoleType = Big;
            else
                consoleType--;
            return true;
        } else
        {
            if (consoleType >= Big)
                consoleType = Tiny;
            else
                consoleType++;
            return true;
        }
    } else if ((isCtrl && c == 'g') || // C-g
               sym == '\033') // ESC
    {
        if (isMacroRecording) // append macro
            if (isShift && c == 'g') // C-S-g
            { // last macro command is cancel command
                appendCurrentMacro("@*EXEC*@end macro");
                setMacroRecord(false);
                descriptionStr = _("Macro defining stopped");
            }
            else
                appendCurrentMacro("@*ESC*");
        finish();
    }

    if (curInteractive)
    {
        curInteractive->charEntered(c_p, modifiers);
        if (isVisible && curInteractive)
            curInteractive->update(curInteractive->getBufferText());
    }
    return true;
}

TextureFont* GeekConsole::getInteractiveFont()
{
    if (titleFont == NULL)
        titleFont = getTitleFont(celCore);
    return titleFont;
}

TextureFont* GeekConsole::getCompletionFont()
{
    if (font == NULL)
        font = getFont(celCore);
    return font;
}


void GeekConsole::render()
{
    if (!isVisible || !curInteractive)
        return;
    TextureFont* font = getCompletionFont();
    TextureFont* titleFont = getInteractiveFont();
    if (font == NULL || titleFont == NULL)
        return;

    float fontH = font->getHeight();
    float titleFontH = titleFont->getHeight();
    int nb_lines;
    float complH; // height of completion area
    float rectH; // height of console
    switch(consoleType)
    {
    case GeekConsole::Tiny:
        nb_lines = 3;
        break;
    case GeekConsole::Medium:
        nb_lines = (height * 0.5 - 3 * titleFontH - 10) / (fontH + 1);
        break;
    case GeekConsole::Big:
    default:
        nb_lines = ((height - 3 * titleFontH) / (fontH + 1)) - MAX_LINES_INFO - 1;
        break;
    }

    if (nb_lines < 3)
        nb_lines = 3;

    complH = (nb_lines+1) * (fontH + 1);
    rectH = complH + 2 * titleFontH;
    overlay->begin();
    glTranslatef(0.0f, BOTTOM_MARGIN, 0.0f); //little margin from bottom

    float funNameWidth = titleFont->getWidth(curFunName) + 10.0;

    // background
    glColor4ubv(clBackground->rgba);
    overlay->rect(0.0f, 0.0f, width, rectH);
    overlay->rect(0.0f, rectH, funNameWidth, titleFontH);

    // Interactive & description rects

    glColor4ubv(clBgInteractive->rgba);
    overlay->rect(0.0f, 0.0f , width, titleFontH);
    overlay->rect(0.0f, rectH - titleFontH , width, titleFontH);
    overlay->rect(0.0f, rectH, funNameWidth, titleFontH);

    glColor4ubv(clBgInteractiveBrd->rgba);
    overlay->rect(0.0f, 0.0f , width-1, titleFontH, false);
    overlay->rect(0.0f, rectH - titleFontH , width-1, titleFontH, false);
    overlay->rect(0.0f, rectH, funNameWidth, titleFontH, false);

    // info text
    if (infoText.size()) {
        float y = rectH + titleFontH;
        glColor4ubv(clInfoTextBg->rgba);
        overlay->rect(0.0f, y,
                      infoWidth, fontH * infoText.size() + 2);

        glColor4ubv(clInfoTextBrd->rgba);
        overlay->rect(0.0f, y,
                      infoWidth, fontH * infoText.size() + 2, false);

        // render info text
        glPushMatrix();
        {
            overlay->setFont(font);
            glTranslatef(2.0f, y + 6
                         + (infoText.size() - 1) * fontH, 0.0f);
            overlay->beginText();
            glColor4ubv(clInfoTextFnt->rgba);
            std::vector<std::string>::iterator it;
            for (it = infoText.begin();
                 it != infoText.end(); it++) {
                *overlay << *it << "\n";
            }
            overlay->endText();
        }
        glPopMatrix();
    }

    // render function name
    glPushMatrix();
    {
        overlay->setFont(titleFont);
        glTranslatef(2.0f, rectH + 4, 0.0f);
        overlay->beginText();
        glColor4ubv(clInteractivePrefixFnt->rgba);
        *overlay << curFunName << ":";
        overlay->endText();
    }
    glPopMatrix();

    // render Interactive
    glPushMatrix();
    {
        overlay->setFont(titleFont);
    	glTranslatef(2.0f, rectH - titleFontH + 4, 0.0f);
        overlay->beginText();
        glColor4ubv(clInteractivePrefixFnt->rgba);
    	*overlay << InteractivePrefixStr << " ";
        curInteractive->renderInteractive();
        overlay->endText();
    }
    glPopMatrix();

    // compl list
    glPushMatrix();
    {
    	glTranslatef(2.0f, rectH - fontH - titleFontH, 0.0f);
        overlay->setFont(font);
        curInteractive->renderCompletion(complH - fontH, width-1);
    }
    glPopMatrix();

    // description line
    glPushMatrix();
    {
    	glTranslatef(2.0f, 4.0f, 0.0f);
        overlay->setFont(titleFont);
        overlay->beginText();

        glColor4ubv(clDescrFnt->rgba);

    	*overlay << descriptionStr;
        overlay->endText();
    }
    glPopMatrix();
    overlay->end();
}

void GeekConsole::setInteractive(GCInteractive *Interactive, std::string historyName, std::string InteractiveStr, std::string descrStr)
{
    if (curInteractive)
        curInteractive->cancelInteractive();
    curInteractive = Interactive;
    curInteractive->Interact(this, historyName);
    InteractivePrefixStr = InteractiveStr;
    descriptionStr = descrStr;
    clearInfoText();
    isVisible = true;
}

int GeekConsole::call(const std::string &value, bool params)
{
    if(!curFun) return 0;

    finish();

    if (isMacroRecording && !params)
    {
        // zero state must be skipped
        // <0 skip, because its a describe state
        if (funState > 0)
        {
            if (value == "")
            {
                appendCurrentMacro("@*NILL*");
            }
            else
            {
                appendCurrentMacro("@" + value);
            }
        }
    }
    GCFunc *lastFun = curFun;
    int state = curFun->call(this, funState, value);
    // set new state only if no new function called
    if (lastFun == curFun)
        funState = state;
    return funState;
}

void GeekConsole::InteractFinished(std::string value)
{
    if(!curFun) return;
    funState++;

    // some interactives may need do some clean
    finish();
    call(value);
}

/** Hide console and clear interactive */
void GeekConsole::finish()
{
    isVisible = false;
    if (curInteractive)
        curInteractive->cancelInteractive();
    curInteractive = NULL;
}

GeekBind *GeekConsole::createGeekBind(std::string bindspace)
{
    GeekBind *gb = getGeekBind(bindspace);
    if (gb)
        return gb;
    gb = new GeekBind(bindspace);
    geekBinds.push_back(gb);
    return gb;
}

GeekBind *GeekConsole::getGeekBind(std::string bindspace)
{
    for (std::vector<GeekBind *>::iterator it = geekBinds.begin();
		 it != geekBinds.end(); it++)
    {
        GeekBind *gb = *it;
        if (gb->getName() == bindspace)
            return gb;
    }
    return NULL;
}

void GeekConsole::registerAndBind(std::string bindspace, const char *bindkey,
                                  GCFunc fun, const char *funname)
{
    registerFunction(fun, funname);
    if (bindspace.empty())
        bindspace = "Global";
    GeekBind *gb = getGeekBind(bindspace);
    if (gb)
        gb->bind(bindkey, funname);
}

bool GeekConsole::bind(std::string bindspace, std::string bindkey, std::string function)
{
    if (bindspace.empty())
        bindspace = "Global";
    GeekBind *gb = getGeekBind(bindspace);
    if (!gb)
        gb = createGeekBind(bindspace);
    if(gb)
        return gb->bind(bindkey.c_str(), function);
    return false;
}

void GeekConsole::unbind(std::string bindspace, std::string bindkey)
{
    if (bindspace.empty())
        bindspace = "Global";
    GeekBind *gb = getGeekBind(bindspace);
    if (gb)
        gb->unbind(bindkey.c_str());
}

void GeekConsole::setInfoText(std::string info)
{
    infoText = splitString(info, "\n");
    if (infoText.size() > MAX_LINES_INFO)
        infoText.resize(MAX_LINES_INFO);

    std::vector<std::string>::iterator it;
    infoWidth = 0;
    TextureFont* font = getCompletionFont();
    if (!font)
        return;
    for (it = infoText.begin();
         it != infoText.end(); it++) {
        if (!(*it).empty())
        {
            uint s = font->getWidth(*it);
            if (s > infoWidth)
                infoWidth = s + 4;
        }
    }
}

void GeekConsole::clearInfoText()
{
    infoWidth = 0;
    infoText.clear();
}

void GeekConsole::setMacroRecord(bool enable, bool quiet)
{
    std::string execkey = "@*EXEC*";

    if (enable)
    {
        if (isMacroRecording)
        {
            if (!quiet)
                celCore->flash(_("Already defining macro"));

            size_t found = currentMacro.rfind(execkey);
            if( found != string::npos )
                currentMacro = currentMacro.substr(0, found);
            return;
        }
        currentMacro = "";
        curMacroLevel = 0;
        isMacroRecording = true;
    } else
    {
        if (!isMacroRecording)
        {
            if (!quiet)
                celCore->flash(_("Not defining macro"));
            return;
        }

        size_t found = currentMacro.rfind(execkey);
        if( found != string::npos )
            currentMacro = currentMacro.substr(0, found);

        // replace unusefull *EXEC*@exec function into *EXEC*
        std::string key = "@*EXEC*@exec function";
        found = currentMacro.find(key);
        while (found != string::npos) {
            currentMacro.replace(found, key.length(), execkey);
            found = currentMacro.find(key);
        }

        // remove all nill execs
        key = "@*EXEC*@*ESC*";
        found = currentMacro.find(key);
        while (found != string::npos) {
            currentMacro.replace(found, key.length(), execkey);
            found = currentMacro.find(key);
        }

        if (currentMacro.find(execkey) == 0)
            currentMacro.replace(0, execkey.length(), "");
        isMacroRecording = false;
        lastMacro = currentMacro;
    }
}

void GeekConsole::callMacro()
{
    setMacroRecord(false);
    celCore->flash(_("Call macro"));
    execFunction("exec function", lastMacro);
}

void GeekConsole::appendCurrentMacro(std::string macro)
{
    if (!isMacroRecording)
        return;
    if ( curMacroLevel >= maxMacroLevel)
    {
        setMacroRecord(false, true);
        celCore->flash(_("Max level of macro, defining stopped."), 8);
    }
    else
    {
        currentMacro += macro;
        curMacroLevel++;
    }
}

void GeekConsole::appendDescriptionStr(std::string text)
{
    if (descriptionStr.length())
        descriptionStr += ", " + text;
    else
        descriptionStr = text;
}

void GeekConsole::setBeeper(Beep *b)
{
    beeper = b;
}

void GeekConsole::beep()
{
    if (beeper)
        beeper->beep();
}

/******************************************************************
 *  Interactives
 ******************************************************************/

GCInteractive::GCInteractive(std::string _InteractiveName, bool _useHistory)
{
    interactiveName = _InteractiveName;
    useHistory = _useHistory;
    if (!useHistory)
        return;

    // load history
    Directory* dir = OpenDirectory(historyDir);
    std::string filename;
    if (dir != NULL)
        while (dir->nextFile(filename))
        {
            if (filename[0] == '.')
                continue;
            int extPos = filename.rfind('.');
            if (extPos == (int)string::npos)
                continue;
            std::string ext = string(filename, extPos, filename.length() - extPos + 1);
            if (compareIgnoringCase("." + interactiveName, ext) == 0)
            {
                std::string line;
                std::ifstream infile((historyDir + filename).c_str(), ifstream::in);
                std::string histName = string(filename, 0, extPos);
                while (getline(infile, line, '\n'))
                {
                    history[histName].push_back(line);
                }
                infile.close();
            }
        }
}

GCInteractive::~GCInteractive()
{
    if (!useHistory)
        return;
    // save history
    Directory* dir = OpenDirectory(historyDir);
    std::string filename;
    if (dir != NULL)
    {
        History::iterator it1;
        std::vector<std::string>::iterator it2;
        for (it1 = history.begin();
             it1 != history.end(); it1++)
        {
            std::string name = it1->first;
            std::ofstream outfile(string(historyDir + name + "." + interactiveName).c_str(), ofstream::out);

            for (it2 = history[name].begin();
                 it2 != history[name].end(); it2++)
            {
                outfile << *it2 << "\n";
            }
            outfile.close();
        }
    }
    else
        cout << "Cann't locate '" << historyDir <<"' dir. GeekConsole's history not saved." << "\n";
}

void GCInteractive::Interact(GeekConsole *_gc, string historyName)
{
    gc = _gc;
    buf = "";
    curHistoryName = historyName;
    typedHistoryCompletionIdx = 0;
    bufSizeBeforeHystory = 0;
    prepareHistoryCompletion();
}

void GCInteractive::charEntered(const char *c_p, int modifiers)
{
    wchar_t wc = 0;
    UTF8Decode(c_p, 0, strlen(c_p), wc);

    char C = toupper((char)wc);

    if (useHistory && !curHistoryName.empty())
    {
        gc->descriptionStr += _(", C-p Previous, C-n Next in history");
    }

    std::vector<std::string>::iterator it;
    std::vector<std::string>::reverse_iterator rit;

    switch (C)
    {
    case '\n':
    case '\r':
        {
            if (useHistory && !curHistoryName.empty())
            {
                // append history only if buf not empty and not equal to oprev hist
                if (!buf.empty())
                    if (!history[curHistoryName].empty())
                    {
                        rit = history[curHistoryName].rbegin();
                        if (*rit != buf)
                            history[curHistoryName].push_back(buf);
                    }
                    else
                        history[curHistoryName].push_back(buf);
                if (history[curHistoryName].size() > MAX_HISTORY_SYZE)
                    history[curHistoryName].erase(history[curHistoryName].begin(),
                                                  history[curHistoryName].begin()+1);
            }
            gc->InteractFinished(buf);
            return;
        }
    case '\020':  // Ctrl+P prev history
        if (typedHistoryCompletion.empty())
            return;
        rit = typedHistoryCompletion.rbegin() + typedHistoryCompletionIdx;
        typedHistoryCompletionIdx++;
        if (typedHistoryCompletionIdx > typedHistoryCompletion.size() - 1)
            typedHistoryCompletionIdx = 0;
        buf = *rit;
        gc->appendDescriptionStr(_(ctrlZDescr));
        return;
    case '\016':  // Ctrl+N forw history
        if (typedHistoryCompletion.empty())
            return;
        it = typedHistoryCompletion.begin() + typedHistoryCompletionIdx;
        typedHistoryCompletionIdx--;
        if (typedHistoryCompletionIdx < 0)
            typedHistoryCompletionIdx = typedHistoryCompletion.size() - 1;
        buf = *it;
        gc->appendDescriptionStr(_(ctrlZDescr));
        return;
    case '\032':  // Ctrl+Z
        if (bufSizeBeforeHystory == buf.size())
            return;
        setBufferText(string(buf, 0, bufSizeBeforeHystory));
        prepareHistoryCompletion();
        return;
    case '\025': // Ctrl+U
        setBufferText("");
        prepareHistoryCompletion();
        break;
    case '\027':  // Ctrl+W
    case '\b':
        if ((modifiers & GeekBind::CTRL) != 0)
            while (buf.size() && !strchr(string(separatorChars + " /:,;").c_str(),
                                         buf[buf.size() - 1])) {
                setBufferText(string(buf, 0, buf.size() - 1));
            }
    while (buf.size() && ((buf[buf.size() - 1] & 0xC0) == 0x80)) {
        buf = string(buf, 0, buf.size() - 1);
    }
    setBufferText(string(buf, 0, buf.size() - 1));
    prepareHistoryCompletion();
    default:
        break;
    }
#ifdef TARGET_OS_MAC
    if ( wc && (!iscntrl(wc)) )
#else
        if ( wc && (!iswcntrl(wc)) )
#endif
        {
            if (!(modifiers & GeekBind::CTRL) &&
                !(modifiers & GeekBind::META))
            {
                buf += c_p;
                prepareHistoryCompletion();
                bufSizeBeforeHystory = buf.size();
            }
        }
}

void GCInteractive::cancelInteractive()
{
}

void GCInteractive::setBufferText(std::string str)
{
    buf = str;
    bufSizeBeforeHystory = buf.size();
}

void GCInteractive::renderInteractive()
{
    glColor4ubv(clInteractiveFnt->rgba);
    *gc->getOverlay() << string(buf, 0, bufSizeBeforeHystory);
    if (bufSizeBeforeHystory < buf.size())
    {
        glColor4ubv(clInteractiveExpand->rgba);
    	std::string s = string(buf, bufSizeBeforeHystory, buf.size() - bufSizeBeforeHystory);
    	*gc->getOverlay() << s;
    }
    glColor4ubv(clInteractiveFnt->rgba);
    *gc->getOverlay() << "|";
}

void GCInteractive::prepareHistoryCompletion()
{
    std::vector<std::string>::iterator it;
    typedHistoryCompletion.clear();
    int buf_length = UTF8Length(buf);
    // Search through all string in history
    for (it = history[curHistoryName].begin();
         it != history[curHistoryName].end(); it++)
    {
        if (buf_length == 0 ||
            (UTF8StringCompare(*it, buf, buf_length) == 0))
            typedHistoryCompletion.push_back(*it);
    }
    typedHistoryCompletionIdx = 0;
}

void GCInteractive::renderCompletion(float height, float width)
{

}

void GCInteractive::update(const std::string &buftext)
{
    gc->describeCurText(buftext);
}

void GCInteractive::setDefaultValue(std::string v)
{
    setBufferText(v);
    update(getBufferText());
}

void GCInteractive::setLastFromHistory()
{
    prepareHistoryCompletion();
    if (typedHistoryCompletion.empty())
        return;
    if (typedHistoryCompletionIdx > typedHistoryCompletion.size()-1)
        typedHistoryCompletionIdx = 0;
    std::vector<std::string>::reverse_iterator rit =
        typedHistoryCompletion.rbegin() + typedHistoryCompletionIdx;
    typedHistoryCompletionIdx++;
    buf = *rit;
    gc->appendDescriptionStr(_(ctrlZDescr));
    update(getBufferText());
}

void PasswordInteractive::renderInteractive()
{
    glColor4ubv(clInteractiveFnt->rgba);
    for (uint i = 0; i < getBufferText().length(); i++)
        *gc->getOverlay() << "*";
    *gc->getOverlay() << "|";
}

// Color Chooser interactive
void ColorChooserInteractive::Interact(GeekConsole *_gc, string historyName)
{
    ListInteractive::Interact(_gc, historyName);
    int i = 0;
    while(colorTable[i].colorName)
    {
        completionList.push_back(colorTable[i].colorName);
        i++;
    }
    ListInteractive::setColumns(4);
    typedTextCompletion = completionList;
}

void ColorChooserInteractive::renderInteractive()
{
    glColor4ub(255,255,255,255);
    float fh = gc->getInteractiveFont()->getHeight();
    float x = gc->getWidth() - 90;
    float y = 2.0 - BOTTOM_MARGIN;
    float h = fh - 2;
    gc->getOverlay()->rect(x, y , 80, h-2);
    glColor3ub(0,0,0);
    gc->getOverlay()->rect(x,  y + h/2, 40, h/2);
    gc->getOverlay()->rect(x + 40, y, 40, h/2);

    ListInteractive::renderInteractive();
    Color32 c = getColor32FromText(getBufferText());
    glColor4ubv(c.rgba);
    gc->getOverlay()->rect(x, y , 80, h-2);
}

void ColorChooserInteractive::renderCompletion(float height, float width)
{
    glPushMatrix();
    ListInteractive::renderCompletion(height, width);
    glPopMatrix();

    // render color list
    TextureFont *font = gc->getCompletionFont();
    float fh = font->getHeight();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel

    vector<std::string>::const_iterator it = typedTextCompletion.begin();
    it += pageScrollIdx;
    float marg = font->getWidth("#FFFFFFFF ");
    int shiftx = width/ListInteractive::cols;
    glTranslatef((float) shiftx - marg - 4.0f, 0, 0);
    for (uint i=0; it < typedTextCompletion.end() && i < ListInteractive::cols; i++)
    {
        glPushMatrix();
        gc->getOverlay()->beginText();
        for (uint j = 0; it < typedTextCompletion.end() && j < nb_lines; it++, j++)
        {
            int k = 0;
            while (colorTable[k].colorName)
            {
                if (*it == colorTable[k].colorName)
                {
                    glColor4ubv(getColor32FromHexText(
                                    colorTable[k].colorHexName).rgba);
                    gc->getOverlay()->rect(0, 0, marg, fh);
                    glColor4ub(255, 255, 255, 255);
                    *gc->getOverlay() << colorTable[k].colorHexName << "\n";
                    break;
                }
                k++;
            }
        }
        gc->getOverlay()->endText();
        glPopMatrix();
        glTranslatef((float) shiftx, 0.0f, 0.0f);
    }

}

// ListInteractive

void ListInteractive::Interact(GeekConsole *_gc, string historyName)
{
    GCInteractive::Interact(_gc, historyName);
    setMatchCompletion(false);
    typedTextCompletion.clear();
    completionList.clear();
    pageScrollIdx = 0;
    completedIdx = -1;
    setColumns(4);
    canFinish = false;
}

void ListInteractive::updateTextCompletion()
{
    typedTextCompletion.clear();
    if (completionStyle != Filter) // add items that matched with first chars of buftext
    {
        std::string buftext = string(getBufferText(), 0, bufSizeBeforeHystory);
        int buf_length = UTF8Length(buftext);
        std::vector<std::string>::iterator it;
        // Search through all string in base completion list
        for (it = completionList.begin();
             it != completionList.end(); it++)
        {
            // hide item that start from "." like files in *nix systems
            if ((*it)[0] == '.' && buftext[0] != '.')
                continue;
            if (buf_length == 0 ||
                (UTF8StringCompare(*it, buftext, buf_length) == 0))
                typedTextCompletion.push_back(*it);
        }
    }
    else // Filter
        filterCompletion(completionList, getBufferText());
}

void ListInteractive::filterCompletion(std::vector<std::string> completion, std::string filter)
{
    int flt_length = UTF8Length(filter);
    
    // split buffer to str for match
    vector<string> parts = splitString(filter, " ");
    std::vector<std::string>::iterator it, pit;
    bool skip;
    for (it = completion.begin();
             it != completion.end(); it++)
    {
        skip = false;
        if (flt_length != 0)
            for (pit = parts.begin();
                 pit != parts.end(); pit++)
            {
                if (UTF8StrStr(*it, *pit) == -1)
                {
                    skip = true;
                    break;
                }
            }
        if (!skip)
            typedTextCompletion.push_back(*it);
    }
}


bool ListInteractive::tryComplete()
{
    int i = 0; // match size
    bool match = true;
    int buf_length = UTF8Length(getRightText());
    vector<std::string>::const_iterator it1, it2;
    updateTextCompletion();
    if (!typedTextCompletion.empty())
        if (typedTextCompletion.size() == 1) // only 1 item to expand
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            setRightText(*it);
        }
        else
        {
            while(match) // find size of matched chars to expand
            {
                uint compareIdx = buf_length + i;
                it1 = typedTextCompletion.begin();
                for (it2 = typedTextCompletion.begin()+1;
                     it2 != typedTextCompletion.end() && match; it2++)
                {
                    std::string s1 = *it1;
                    std::string s2 = *it2;
                    if (s1.length() < compareIdx ||
                        s2.length() <= compareIdx ||
                        s1[compareIdx] != s2[compareIdx])
                        match = false;
                }
                if (match)
                    i++;
            }
        }
    if (i > 0)
    {
        vector<std::string>::const_iterator it = typedTextCompletion.begin();
        setRightText(string(*it, 0, buf_length + i));
        return true;
    }
    return false;
}

std::string ListInteractive::getRightText() const
{
    if (separatorChars.empty())
        return GCInteractive::getBufferText();
    else
    {
        string str = GCInteractive::getBufferText();
        string::size_type pos = str.find_last_of(separatorChars, str.length());
        if (pos != string::npos)
            return string(str, pos + 1);
        else
            return str;
    }
}

void ListInteractive::setRightText(std::string text)
{
    if (separatorChars.empty())
        GCInteractive::setBufferText(text);
    else
    {
        string str = GCInteractive::getBufferText();
        string::size_type pos = str.find_last_of(separatorChars, str.length());
        if (pos != string::npos)
        {
                GCInteractive::setBufferText(string(str, 0,
                                                    pos + 1 < bufSizeBeforeHystory ? pos + 1 : bufSizeBeforeHystory) 
                                             + text);
        }
        else
            GCInteractive::setBufferText(text);
    }
}

std::string ListInteractive::getLeftText() const
{
    if (separatorChars.empty())
        return GCInteractive::getBufferText();
    else
    {
        string str = GCInteractive::getBufferText();
        string::size_type pos = str.find_last_of(separatorChars, str.length());
        if (pos != string::npos)
            return string(str, 0, pos + 1);
        else
            return str;
    }
}

void ListInteractive::charEntered(const char *c_p, int modifiers)
{
    wchar_t wc = 0;
    UTF8Decode(c_p, 0, strlen(c_p), wc);

    char C = toupper((char)wc);
    if (C == 'I' && (modifiers & GeekBind::META))
    {
        if (completionStyle == Standart)
            completionStyle = Fast;
        else if (completionStyle == Fast)
            completionStyle = Filter;
        else
            completionStyle = Standart;
        gc->appendDescriptionStr(_("Completion type: "));
        switch (completionStyle)
        {
        case Standart: gc->descriptionStr += "\"Standart\""; break;
        case Fast: gc->descriptionStr += "\"Fast complete\""; break;
        case Filter: gc->descriptionStr += "\"Filter\""; break;
        }
        return;
    }
    if (completionStyle == Filter)
        return charEnteredFilter(c_p, modifiers);

    if  (C == '\t' && modifiers == 0) // TAB - expand and scroll if many items to expand found
    {
        if (!tryComplete())
        {
            // scroll
            pageScrollIdx += scrollSize;
            if (pageScrollIdx >= typedTextCompletion.size())
                pageScrollIdx = 0;
            char buff[256];
            int pages = (((int)typedTextCompletion.size() - 1 <= 0) ?
                         0 : (typedTextCompletion.size() - 1) / scrollSize) +1;
            sprintf(buff,_(scrollPages), (pageScrollIdx / scrollSize) + 1,
                    pages);
            gc->descriptionStr = buff;
            gc->appendDescriptionStr(_(ctrlZDescr));
            completedIdx = -1;
            return;
        } else
            playMatch();
    } else if (C == '\t' && modifiers == GeekBind::CTRL) { // ctrl + TAB
            pageScrollIdx -= scrollSize;
            if (pageScrollIdx < 0)
                pageScrollIdx = typedTextCompletion.size() - scrollSize;
            if (pageScrollIdx < 0)
                pageScrollIdx = 0;
            char buff[256];
            int pages = (((int)typedTextCompletion.size() - 1 <= 0) ?
                         0 : (typedTextCompletion.size() - 1) / scrollSize) +1;
            sprintf(buff,_(scrollPages), (pageScrollIdx / scrollSize) + 1,
                    pages);
            gc->descriptionStr = buff;
            gc->appendDescriptionStr(_(ctrlZDescr));
            completedIdx = -1;
            return;
    }
    // expand completion on M-/
    else if (((modifiers & GeekBind::META) != 0) && C == '/')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            completedIdx++;
            if (completedIdx < pageScrollIdx)
                completedIdx = pageScrollIdx;
            if (completedIdx >= typedTextCompletion.size())
                completedIdx = pageScrollIdx;
            if (completedIdx > pageScrollIdx + scrollSize - 1)
                completedIdx = pageScrollIdx;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setRightText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            gc->appendDescriptionStr(_(ctrlZDescr));
            playMatch();
            return;
        }
    }
    // revers expand completion on M-? ( i.e. S-M-/)
    else if (((modifiers & GeekBind::META ) != 0) && C == '?')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (completedIdx <= 0)
                completedIdx = pageScrollIdx + scrollSize;
            completedIdx--;
            if (completedIdx < pageScrollIdx)
                completedIdx = pageScrollIdx + scrollSize - 1;
            if (completedIdx > typedTextCompletion.size() - 1)
                completedIdx = typedTextCompletion.size() - 1;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setRightText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            gc->appendDescriptionStr(_(ctrlZDescr));
            playMatch();
            return;
        }
    }
    else if ((C == '\n' || C == '\r'))
    {
        if(mustMatch)
        {
            tryComplete(); // complite if only matching enable
            std::string buftext = getRightText();
            std::vector<std::string>::iterator it;

            // is righttext in completion list
            for (it = completionList.begin();
                 it != completionList.end(); it++)
            {
                if (*it == buftext)
                {
                    GCInteractive::charEntered(c_p, modifiers);
                    return;
                }
            }
            // not matching
            gc->beep();
            return;
        }
    }
    std::string oldBufText = getBufferText();

    GCInteractive::charEntered(c_p, modifiers);

    std::string buftext = getBufferText();
    // reset page scroll only if text changed
    if (oldBufText != buftext)
        pageScrollIdx = 0;

    // if must_always_match with completion items - try complete
    if(mustMatch && buftext.length() > oldBufText.length() && completionStyle == Fast)
        tryComplete();

    // Refresh completion (text to complete is only text before bufSizeBeforeHystory)
    updateTextCompletion();

    // if match on - complete again
    // and if no any item to expand than revert to old text but when text increased
    if(mustMatch &&
       buftext.length() > oldBufText.length())
    {
        if (completionStyle == Fast)
            tryComplete();
        if (typedTextCompletion.empty())
        {
            setBufferText(oldBufText); // revert old text
            gc->beep();
        }
        else
            playMatch();
    }

    // show unique match
    if (typedTextCompletion.size() == 1 && mustMatch)
    {
        gc->descriptionStr = _("Unique match, RET - complete and finish");
    }
}

void ListInteractive::charEnteredFilter(const char *c_p, int modifiers)
{
    wchar_t wc = 0;
    UTF8Decode(c_p, 0, strlen(c_p), wc);

    char C = toupper((char)wc);
    if  (C == '\t' && modifiers == 0) // TAB - expand and scroll if many items to expand found
    {
        // scroll
        pageScrollIdx += scrollSize;
        if (pageScrollIdx >= typedTextCompletion.size())
            pageScrollIdx = 0;
        char buff[256];
        int pages = (((int)typedTextCompletion.size() - 1 <= 0) ?
                     0 : (typedTextCompletion.size() - 1) / scrollSize) +1;
        sprintf(buff,_(scrollPages), (pageScrollIdx / scrollSize) + 1,
                pages);
        gc->descriptionStr = buff;
        completedIdx = -1;
        return;
    } else if (C == '\t' && modifiers == GeekBind::CTRL) { // ctrl + TAB
        pageScrollIdx -= scrollSize;
        if (pageScrollIdx < 0)
            pageScrollIdx = typedTextCompletion.size() - scrollSize;
        if (pageScrollIdx < 0)
            pageScrollIdx = 0;
        char buff[256];
        int pages = (((int)typedTextCompletion.size() - 1 <= 0) ?
                     0 : (typedTextCompletion.size() - 1) / scrollSize) +1;
        sprintf(buff,_(scrollPages), (pageScrollIdx / scrollSize) + 1,
                pages);
        gc->descriptionStr = buff;
        completedIdx = -1;
        return;
    }
    // expand completion on M-/
    else if (((modifiers & GeekBind::META) != 0) && C == '/')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            completedIdx++;
            if (completedIdx < pageScrollIdx)
                completedIdx = pageScrollIdx;
            if (completedIdx >= typedTextCompletion.size())
                completedIdx = pageScrollIdx;
            if (completedIdx > pageScrollIdx + scrollSize - 1)
                completedIdx = pageScrollIdx;
            playMatch();
            gc->appendDescriptionStr(_(applySelected));
            return;
        }
    }
    // revers expand completion on M-? ( i.e. S-M-/)
    else if (((modifiers & GeekBind::META ) != 0) && C == '?')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (completedIdx <= 0)
                completedIdx = pageScrollIdx + scrollSize;
            completedIdx--;
            if (completedIdx < pageScrollIdx)
                completedIdx = pageScrollIdx + scrollSize - 1;
            if (completedIdx > typedTextCompletion.size() - 1)
                completedIdx = typedTextCompletion.size() - 1;
            gc->appendDescriptionStr(_(applySelected));
            playMatch();
            return;
        }
    }
    else if ((C == '\n' || C == '\r'))
    {
        if (canFinish && modifiers != GeekBind::SHIFT) // finish it
            return GCInteractive::charEntered(c_p, modifiers);

        if (mustMatch || modifiers == GeekBind::SHIFT)
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (typedTextCompletion.size() != 1)
                if (completedIdx >= 0)
                    it += completedIdx;
                else
                    return gc->beep();
            if (it < typedTextCompletion.end())
            {
                string::size_type pos = getBufferText().find_last_of(
                    separatorChars, getBufferText().length());
                setRightText(*it);
                if (pos != string::npos)
                    bufSizeBeforeHystory = pos + 1;
                else
                    bufSizeBeforeHystory = 0;
            }
            else
                return gc->beep();
            playMatch();
            completedIdx = -1;
        }
        canFinish = true;
        gc->appendDescriptionStr(_(ret4Finish));
        return;
    }

    if (!mustMatch)
            gc->appendDescriptionStr(_(applySelected));

    gc->appendDescriptionStr(_("M-i - Cycle completion type"));

    std::string oldBufText = getBufferText();

    GCInteractive::charEntered(c_p, modifiers);

    std::string buftext = getBufferText();

    canFinish = false;

    // reset page scroll only if text changed
    if (oldBufText != buftext)
        pageScrollIdx = 0;

    // If match on - complete  again if completion type is "Fast", and
    // if no any  item to expand than revert to old  text but when was
    // text increased (changed), and beep.
    if(mustMatch &&
       buftext.length() > oldBufText.length())
    {
        if (completionStyle == Fast)
            tryComplete();
        if (typedTextCompletion.empty())
        {
            setBufferText(oldBufText); // revert old text
            gc->beep();
        }
        else
            playMatch();
    }
}

void ListInteractive::renderInteractive()
{
    GCInteractive::renderInteractive();

    if (completionStyle == Filter)
    {
        vector<std::string>::const_iterator it = typedTextCompletion.begin();
        if (completedIdx >=0)
        {
            it += completedIdx;
            if (it < typedTextCompletion.end())
            {
                glColor4ubv(clInteractiveExpand->rgba);
                *gc->getOverlay() << '{' << *it <<  '}';
            }
        }
    }
}


void ListInteractive::renderCompletion(float height, float width)
{
    if (completionStyle == Filter)
        renderCompletionFilter(height, width);
    else
        renderCompletionStandart(height, width);
}

void ListInteractive::renderCompletionStandart(float height, float width)
{
    TextureFont *font = gc->getCompletionFont();
    float fh = font->getHeight();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel
    uint nb_cols = cols;
    scrollSize = nb_cols * nb_lines;

    // Find size of right part (expanded part is excluded)
    std::string buftext(GCInteractive::getBufferText(), 0, bufSizeBeforeHystory);
    string::size_type pos = buftext.find_last_of(separatorChars, buftext.length());
    if (pos != string::npos)
        buftext = string(buftext, pos + 1);
    uint buf_length = buftext.size();

    vector<std::string>::const_iterator it = typedTextCompletion.begin();
    int shiftx = width/nb_cols;
    it += pageScrollIdx;
    wchar_t c;

    for (uint i=0; it < typedTextCompletion.end() && i < nb_cols; i++)
    {
        glPushMatrix();
        gc->getOverlay()->beginText();
        for (uint j = 0; it < typedTextCompletion.end() && j < nb_lines; it++, j++)
        {
            std::string s = *it;
            glColor4ubv(clCompletionFnt->rgba);
            if (i * nb_lines + j == completedIdx - pageScrollIdx)
            {
                // border of current completion
                glColor4ubv(clCompletionExpandBg->rgba);
                gc->getOverlay()->rect(0.0f, 0.0f - 2,
                                       (float)shiftx, fh + 1);
            }
            // completion item (only text before match char)
            glColor4ubv(clCompletionFnt->rgba);
            *gc->getOverlay() << std::string(s, 0, buf_length);
            // match char background
            if (s.size() >= buf_length)
            {
                if (!UTF8Decode(s, buf_length, c))
                    continue; //something wrong

                std::string text(s, buf_length, UTF8EncodedSize(c));
                glColor4ubv(clCompletionMatchCharBg->rgba);
                gc->getOverlay()->rect(font->getWidth(string(s, 0, buf_length)), 0.0f - 2,
                                       font->getWidth(text), fh);
                glColor4ubv(clCompletionMatchCharFnt->rgba);
                *gc->getOverlay() << text;
                // rest text of item from match char
                glColor4ubv(clCompletionAfterMatch->rgba);
                if (s.size() > buf_length)
                    *gc->getOverlay() << string(s, buf_length + 1, 128);
                *gc->getOverlay() << "\n";
            }
        }
        gc->getOverlay()->endText();
        glPopMatrix();
        glTranslatef((float) shiftx, 0.0f, 0.0f);
    }
}

void ListInteractive::renderCompletionFilter(float height, float width)
{
    TextureFont *font = gc->getCompletionFont();
    float fh = font->getHeight();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel
    uint nb_cols = cols;
    scrollSize = nb_cols * nb_lines;

    // Find size of right part (expanded part is excluded)
    std::string buftext(GCInteractive::getBufferText(), 0, bufSizeBeforeHystory);
    string::size_type pos = buftext.find_last_of(separatorChars, buftext.length());
    if (pos != string::npos)
        buftext = string(buftext, pos + 1);
    uint buf_length = buftext.size();

    std::vector<string> parts = splitString(buftext, " ");
    vector<std::string>::const_iterator pit;
    size_t f;
    size_t sz;

    bool fg[128];

    vector<std::string>::const_iterator it = typedTextCompletion.begin();
    int shiftx = width/nb_cols;
    it += pageScrollIdx;
    for (uint i=0; it < typedTextCompletion.end() && i < nb_cols; i++)
    {
        glPushMatrix();
        gc->getOverlay()->beginText();
        for (uint j = 0; it < typedTextCompletion.end() && j < nb_lines; it++, j++)
        {
            std::string s = *it;
            glColor4ubv(clCompletionFnt->rgba);
            if (i * nb_lines + j == completedIdx - pageScrollIdx)
            {
                // border of current completion
                glColor4ubv(clCompletionExpandBg->rgba);
                gc->getOverlay()->rect(0.0f, 0.0f - 2,
                                       (float)shiftx, fh + 1);
            }

            memset(fg, 0, 128 * sizeof(bool));

            for (pit = parts.begin();
                 pit != parts.end(); pit++)
            {
                f = UTF8StrStr(*it, *pit);
                if (f != -1)
                {
                    sz = f + ((std::string (*pit)).size());
                    if (sz > 128)
                        sz = 128;
                    for(int indx = f; indx < sz; indx++)
                        fg[indx] = true;
                }
            }

            int max = s.size() > 128?
                128 : s.size();
            bool matchbg = false;
            int start = 0;
            int indx;
            for (indx = 0; indx < max; indx++)
            {
                if (fg[indx] == matchbg)
                    continue;

                string str(s, start, indx - start);
                if (matchbg)
                {
                    glColor4ubv(clCompletionMatchCharBg->rgba);
                     gc->getOverlay()->rect(font->getWidth(string(s, 0, start)), 0.0f - 2,
                                            font->getWidth(str), fh);
                    glColor4ubv(clCompletionMatchCharFnt->rgba);
                }
                else
                    glColor4ubv(clCompletionFnt->rgba);
                *gc->getOverlay() << str;

                matchbg = fg[indx];
                start = indx;
            }
            string str(s, start, indx - start);
            if (matchbg)
            {
                glColor4ubv(clCompletionMatchCharBg->rgba);
                gc->getOverlay()->rect(font->getWidth(string(s, 0, start)), 0.0f - 2,
                                       font->getWidth(str), fh);
                glColor4ubv(clCompletionMatchCharFnt->rgba);
            }
            else
                glColor4ubv(clCompletionFnt->rgba);
            *gc->getOverlay() << str;

            *gc->getOverlay() << "\n";
        }
        gc->getOverlay()->endText();
        glPopMatrix();
        glTranslatef((float) shiftx, 0.0f, 0.0f);
    }
}

void ListInteractive::update(const std::string &buftext)
{
    if (completionStyle == Filter)
    {
        // try update for current buffer if ready for finish
        if (canFinish)
            GCInteractive::update(getBufferText());
        else // otherwise update for current selected item
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (completedIdx >=0)
            {
                it += completedIdx;
                if (it < typedTextCompletion.end())
                    GCInteractive::update(*it);
            }
        }
    }
    else
        GCInteractive::update(buftext);
    updateTextCompletion();
}

void ListInteractive::playMatch()
{
    // play sound if matched
    GeekConsole::Beep *b = gc->getBeeper();
    if(b)
    {
        std::string buftext = getRightText();
        std::vector<std::string>::iterator it;

        // is righttext in completion list
        for (it = typedTextCompletion.begin();
             it != typedTextCompletion.end(); it++)
        {
            if (*it == buftext)
                return b->match();
        }
    }
}

void ListInteractive::setCompletion(std::vector<std::string> completion)
{
    completionList = completion;
    pageScrollIdx = 0;
    updateTextCompletion();
}

void ListInteractive::setMatchCompletion(bool _mustMatch)
{
    mustMatch = _mustMatch;
}

void ListInteractive::setCompletionFromSemicolonStr(std::string str)
{
    completionList.clear();
    uint begin = 0;
    uint f;
    while(1)
    {
        f = str.find(';', begin);
        if (f != string::npos)
        {
            std::string item(str, begin, f - begin);
            completionList.push_back(item);
            begin = f + 1;
        }
        else
        {
            if (str.size() > begin)
                completionList.push_back(std::string(str, begin, str.size() - begin));
            break;
        }
    }
    pageScrollIdx = 0;
    typedTextCompletion = completionList;
}

CelBodyInteractive::CelBodyInteractive(std::string name, CelestiaCore *core):
    ListInteractive(name), celApp(core)
{
}

void CelBodyInteractive::updateTextCompletion()
{
    string str = GCInteractive::getBufferText();
    //skip update for some history and expand action
    if (str.length() != bufSizeBeforeHystory)
        return;
    typedTextCompletion.clear();
    if (completionStyle != Filter)
    {
        if (completionList.size()) {
            std::string buftext = string(getBufferText(), 0, bufSizeBeforeHystory);
            std::vector<std::string>::iterator it;
            int buf_length = UTF8Length(buftext);
            // Search through all string in base completion list
            for (it = completionList.begin();
                 it != completionList.end(); it++)
            {
                if (buf_length == 0 ||
                    (UTF8StringCompare(*it, buftext, buf_length) == 0))
                    typedTextCompletion.push_back(*it);
            }
        } else // if no completion list take it from simulation
            typedTextCompletion = celApp->getSimulation()->
                getObjectCompletion(str,
                                    (celApp->getRenderer()->getLabelMode() & Renderer::LocationLabels) != 0);
    }
    else // Filter
    {
        if (completionList.size())
        {
            filterCompletion(completionList, str);
        }
        else
        {
            string::size_type pos = str.find_last_of(separatorChars, str.length());
            string filter = str;
            string complStr = str;
            if (pos != string::npos)
            {
                filter = string(str, pos + 1);
                complStr = string(str, 0, pos + 1);
            }
            filterCompletion(celApp->getSimulation()->
                             getObjectCompletion(complStr,
                                                 (celApp->getRenderer()->getLabelMode()
                                                  & Renderer::LocationLabels) != 0),
                             filter);
        }
    }
}

void CelBodyInteractive::Interact(GeekConsole *_gc, string historyName)
{
    GCInteractive::Interact(_gc, historyName);
    pageScrollIdx = 0;
    completedIdx = -1;
    lastCompletionSel.clear();

    firstSelection = gc->getCelCore()->
        getSimulation()->getSelection().getName();
    update(getBufferText());
    completionList.clear();
    typedTextCompletion.clear();
    ListInteractive::setColumns(4);
    ListInteractive::separatorChars = "/";
}

void CelBodyInteractive::charEntered(const char *c_p, int modifiers)
{
    wchar_t wc = 0;
    UTF8Decode(c_p, 0, strlen(c_p), wc);

    char C = toupper((char)wc);

    if (((modifiers & GeekBind::META ) != 0) && C == 'C')
    {
        Selection sel = gc->getCelCore()->
            getSimulation()->findObjectFromPath(lastCompletionSel, true);
        celApp->getSimulation()->setSelection(sel);
        celApp->getSimulation()->centerSelection();
        return;
    }
    else if ((C == '\n' || C == '\r'))
    {
        if (mustMatch || modifiers == GeekBind::SHIFT)
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (completedIdx >=0)
                it += completedIdx;
            else if (typedTextCompletion.size() > 1)
                return gc->beep();
            if (it < typedTextCompletion.end())
            {
                string::size_type pos = getBufferText().
                    find_last_of(separatorChars, getBufferText().length());
                setRightText(*it);
                if (pos != string::npos)
                    bufSizeBeforeHystory = pos + 1;
                else
                    bufSizeBeforeHystory = 0;
            }
            else
                gc->beep();
            playMatch();
            completedIdx = -1;
            gc->appendDescriptionStr(_(ret4Finish));
            return;
        }

        // allow finish only if selection found
        Selection sel = celApp->getSimulation()->findObjectFromPath(GCInteractive::getBufferText(), true);
        if (sel.empty()) // dont continue if not match any obj
        {
            gc->beep();
            return;
        }
        // if custom completion present
        if (completionList.size()) {
            vector<std::string>::const_iterator it;
            std::string buftext = getBufferText();

            for (it = typedTextCompletion.begin();
                 it != typedTextCompletion.end(); it++)
            {
                if (UTF8StringCompare(*it, buftext) == 0)
                {
                    GCInteractive::charEntered(c_p, modifiers);
                    return;
                }
            }
        } else // pass event
            GCInteractive::charEntered(c_p, modifiers);
        return;
    }
    else
    {
        // Ignore "must competion", use may set it like for ListInteractive
        // we use other finish validator (see upper)
        setMatchCompletion(false);
        return ListInteractive::charEntered(c_p, modifiers);
    }
}

/* Mark obj if found
 */
void CelBodyInteractive::update(const std::string &buftext)
{
    Selection sel = celApp->getSimulation()->findObjectFromPath(buftext, true);
    std::string desc = describeSelection(sel, celApp);

    if (!desc.empty())
    {
        gc->setInfoText(desc);
        gc->appendDescriptionStr(_("M-c - Select"));
        GeekConsole::Beep *b = gc->getBeeper();
        if (b)
            b->match();
    }
    // mark selected
    MarkerRepresentation markerRep(MarkerRepresentation::Crosshair);
    markerRep.setSize(10.0f);
    markerRep.setColor(Color(0.0f, 1.0f, 0.0f, 0.9f));

    // unmark last selection
    Selection lastsel = gc->getCelCore()->
        getSimulation()->findObjectFromPath(lastCompletionSel, true);
    celApp->getSimulation()->
        getUniverse()->unmarkObject(lastsel, 3);

    celApp->getSimulation()->
        getUniverse()->markObject(sel, markerRep, 3);
    lastCompletionSel = buftext;

    ListInteractive::update(buftext);
};

void CelBodyInteractive::setCompletion(std::vector<std::string> completion)
{
    ListInteractive::setCompletion(completion);
    separatorChars = "";
}

void CelBodyInteractive::setCompletionFromSemicolonStr(std::string completion)
{
    ListInteractive::setCompletionFromSemicolonStr(completion);
    separatorChars = "";
}

void CelBodyInteractive::cancelInteractive()
{
    Selection sel = gc->getCelCore()->
        getSimulation()->findObjectFromPath(lastCompletionSel, true);

    celApp->getSimulation()->
        getUniverse()->unmarkObject(sel, 3);
    lastCompletionSel.clear();

    // restore first selection
    sel = gc->getCelCore()->
        getSimulation()->findObjectFromPath(firstSelection, true);
    celApp->getSimulation()->setSelection(sel);
    firstSelection.clear();
}

/* Flag interactive
*/
void FlagInteractive::Interact(GeekConsole *_gc, string historyName)
{
    ListInteractive::Interact(_gc, historyName);
    setColumns(4);
    separatorChars = defDelim;
    update(getBufferText());
}

void FlagInteractive::setSeparatorChars(std::string s)
{
    separatorChars = s;
}

void FlagInteractive::updateTextCompletion()
{
    std::string buftext = string(getBufferText(), 0, bufSizeBeforeHystory);
    string::size_type pos = buftext.find_last_of(separatorChars, buftext.length());
    if (pos != string::npos)
        buftext = string(buftext, pos + 1);
    typedTextCompletion.clear();
    if (completionStyle != Filter)
    {
        // add items that matched with first chars of buftext
        std::vector<std::string>::iterator it;
        int buf_length = UTF8Length(buftext);

        for (it = completionList.begin();
             it != completionList.end(); it++)
        {
            if (buf_length == 0 ||
                (UTF8StringCompare(*it, buftext, buf_length) == 0))
                typedTextCompletion.push_back(*it);
        }
    }
    else
        filterCompletion(completionList, buftext);
}

/* File chooser interactive */

void FileInteractive::Interact(GeekConsole *_gc, string historyName)
{
    ListInteractive::Interact(_gc, historyName);
    setColumns(4);
    separatorChars = "/";
    fileExt.clear();
    dirCache.clear();
    lastPath = "_"; // foo dir, for rebuild dir cache
    update(getBufferText());
}

void FileInteractive::setFileExtenstion(std::string ext)
{
    fileExt = splitString(ext, ";");
}

void FileInteractive::setRightText(std::string text)
{
    // remove last '/' before pass
    if (!text.empty() && text[text.length() - 1] == '/')
        text.resize(text.length() - 1);

    ListInteractive::setRightText(text);
}

void FileInteractive::update(const std::string &buftext)
{
    // TODO: more info in file interactive?
    if (IsDirectory(dirEntire + getBufferText()))
        gc->descriptionStr = _("[Directory]");
    ListInteractive::update(buftext);
    updateTextCompletion();
}

void FileInteractive::charEntered(const char *c_p, int modifiers)
{
    wchar_t wc = 0;
    UTF8Decode(c_p, 0, strlen(c_p), wc);

    std::string buf = getBufferText();
    int end = buf.length() - 1;

    // don't go outside of current chdir ie. prevent '../'
    if (wc == '/')
    {
        if (!modifiers && buf.length() > 1 && buf[end] =='.' && buf[end - 1] == '.' )
            return;
        // dont allow double //
        if (!modifiers && buf.length() > 1 && buf[end] =='/')
            return;

    }
    // can't dispatch to ListInteractive, because different completion types used
    else if ((wc == '\n' || wc == '\r'))
    {
        // special case for filter completion style:
        // Shift+Return for expand completion
        if (completionStyle == Filter &&
            modifiers == GeekBind::SHIFT)
            return ListInteractive::charEnteredFilter(c_p, modifiers);

        // don't allow select directory!
        if (IsDirectory(dirEntire + getBufferText()))
            return gc->beep();

        // if no filter style dispatch
        if(mustMatch && completionStyle != Filter)
        {
            tryComplete(); // complite if only matching enable
            std::string buftext = getRightText();
            std::vector<std::string>::iterator it;

            // is righttext in completion list
            for (it = typedTextCompletion.begin();
                 it != typedTextCompletion.end(); it++)
            {
                if (*it == buftext)
                {
                    GCInteractive::charEntered(c_p, modifiers);
                    return;
                }
            }
            // not matching
            gc->beep();
            return;
        }
    }
    ListInteractive::charEntered(c_p, modifiers);
}

void FileInteractive::updateTextCompletion()
{
    typedTextCompletion.clear();

    std::string path = dirEntire + string(getBufferText(), 0, bufSizeBeforeHystory);

    string::size_type pos = path.find_last_of(separatorChars, path.length());
    std::string fileToExpand;
    if (pos != string::npos)
    {
        fileToExpand = string(path, pos + 1);
        path = string(path, 0, pos);
    }

    Directory* dir = OpenDirectory(path);
    if (dir != NULL && lastPath != path)
    {
        std::string filename;
        dirCache.clear();
        while (dir->nextFile(filename))
        {
            if (filename[0] == '.')
                continue;
            if (IsDirectory(path + "/" + filename))
            {
                dirCache.push_back(filename + "/");
                continue;
            }

            if (fileExt.empty())
                dirCache.push_back(std::string(filename));
            else
            {
                int extPos = filename.rfind('.');
                if (extPos == (int)string::npos)
                    continue;

                std::string ext = string(filename, extPos, filename.length() - extPos + 1);
                // search file extentions match
                std::vector<std::string>::iterator it;
                for (it = fileExt.begin(); it != fileExt.end(); it++)
                {
                    if (compareIgnoringCase(*it, ext) == 0)
                    {
                        //dirCache.push_back(std::string(filename, 0, extPos));
                        dirCache.push_back(filename);
                        break;
                    }
                }
            }
        }
        delete dir;
        lastPath = path;
    }

    // filter for expand
    std::vector<std::string>::iterator it;

    int buf_length = UTF8Length(fileToExpand);
    if (completionStyle != Filter)
    {
        for (it = dirCache.begin();
             it != dirCache.end(); it++)
        {
            if (buf_length == 0 ||
                (UTF8StringCompare(*it, fileToExpand, buf_length) == 0))
                typedTextCompletion.push_back(*it);
        }
    }
    else
        filterCompletion(dirCache, getRightText());
}

void FileInteractive::setDir(std::string dir, std::string entire)
{
    dirEntire = entire;
    if (!dirEntire.empty() && dirEntire[dirEntire.length() - 1] != '/')
        dirEntire += '/';

    setDefaultValue(dir);
}

/* Pager interactive */

PagerInteractive::PagerInteractive(std::string name)
 :GCInteractive(name, false)
{
    std::istringstream is(
        "\n                   SUMMARY OF PAGER COMMANDS\n\n"
        "     Notes in parentheses indicate the behavior if N is given.\n"
        "\n"
        "h  H              Display this help.\n"
        "q  Q              Exit.\n\n"
        "                          MOVING\n"
        "e  ^E  j  ^N  CR    Forward  one line   (or N lines).\n"
        "y  ^Y  k  ^K  ^P    Backward one line   (or N lines).\n"
        "f  ^F  ^V  SPACE    Forward  one window (or N lines).\n"
        "b  ^B  ESC-v        Backward one window (or N lines).\n"
        "z                   Forward  one window (and set window to N).\n"
        "w                   Backward one window (and set window to N).\n"
        "d  ^D               Forward  one half-window (and set half-window to N).\n"
        "u  ^U               Backward one half-window (and set half-window to N).\n"
        "l                   Left  one half screen width (or N positions).\n"
        "r                   Right one half screen width (or N positions).\n"
        "--\n"
        "Default \"window\" is the screen height.\n"
        "Default \"half-window\" is half of the screen height.\n"
        "\n                          SEARCHING\n"
        "/pattern            Search forward for (N-th) matching line.\n"
        "?pattern            Search backward for (N-th) matching line.\n"
        "n                   Repeat previous search (for N-th occurrence).\n"
        "N                   Repeat previous search in reverse direction.\n"
        "\n                          JUMPING\n"
        "g  <  ESC-<         Go to first line in file (or line N).\n"
        "G  >  ESC->         Go to last line in file (or line N).\n"
        "p  %                Go to beginning of file (or N percent into file).\n");
    std::string s;
    while (std::getline(is, s, '\n'))
        helpText.push_back(s);
}

void PagerInteractive::setText(std::string t)
{
    // TODO: need optimisation of split
    lines = &text;
    text.clear();
    std::istringstream is(t);
    std::string s;
    while (std::getline(is, s, '\n'))
        text.push_back(s);
}

void PagerInteractive::appendText(std::string t)
{
    lines = &text;
    std::istringstream is(t);
    std::string s;
    while (std::getline(is, s, '\n'))
        text.push_back(s);
}

void PagerInteractive::setText(std::vector<std::string> t)
{
    lines = &text;
    text = t;
}

void PagerInteractive::Interact(GeekConsole *_gc, string historyName)
{
    GCInteractive::Interact(_gc, historyName);
    pageScrollIdx = 0;
    leftScrollIdx = 0;
    scrollSize = 0;
    richedEnd = false;
    state = PG_NOP;
    lastSearchStr = "";

    chopLine = -1;
    windowSize = -1;
    halfWindowSize = -1;
    text.clear();
    lines = &text;
    N = -1;
}

// forward scrollIdx
// Dont increment if end of view is visible
void PagerInteractive::forward(int fwdby)
{
    if (fwdby > 0)
    {
        if (pageScrollIdx < (int) lines->size() - (int) scrollSize)
        {
            pageScrollIdx += fwdby;
            if (pageScrollIdx > (int) lines->size() - (int) scrollSize)
            { // end exceed
                pageScrollIdx = lines->size() - scrollSize;
            }
        } else
            gc->beep();
    } else
        if (pageScrollIdx == 0)
            gc->beep();
        else
            pageScrollIdx += fwdby;
}

int PagerInteractive::getChopLine()
{
    if (chopLine == -1)
        return width;
    else
        return chopLine;
}

int PagerInteractive::getWindowSize()
{
    if (windowSize == -1)
        return scrollSize;
    else
        return windowSize;
}

int PagerInteractive::getHalfWindowSize()
{
    if (halfWindowSize == -1)
        return scrollSize / 2;
    else
        return halfWindowSize;
}

void PagerInteractive::charEntered(const char *c_p, int modifiers)
{
    char c = *c_p;

    switch (state)
    {
    case PG_NOP:
        if ((c >= '0' && c <= '9')) {
            state = PG_ENTERDIGIT;
            GCInteractive::charEntered(c_p, modifiers);
            break;
        } else if (c == '/') {
            state = PG_SEARCH_FWD;
            return;
        } else if (c == '?') {
            state = PG_SEARCH_BWD;
            return;
        }
        else {
            processChar(c_p, modifiers);
        }
        if (lines == &helpText)
            gc->appendDescriptionStr(_("q - quit from help"));
        else
            gc->appendDescriptionStr(_("q - finish, h - help"));
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
            vector<std::string>::const_reverse_iterator it = lines->rbegin();
            if (getBufferText() != "")
            {
                lastSearchStr = getBufferText();
            }
            int lastSearch = pageScrollIdx;
            setBufferText("");
            if (lastSearch > 0) {
                it += lines->size() - lastSearch;
                for (uint j = 0; it < lines->rend(); it++, j++)
                {
                    if ((std::string (*it)).find(lastSearchStr) != string::npos)
                    {
                        if (--N < 1) { // skip N matches
                            pageScrollIdx = lastSearch - j - 1;
                            gc->descriptionStr = _("Search backward");
                            gc->descriptionStr += ": " + lastSearchStr;
                            return;
                        }
                    }
                }
            }
            N = -1;
            gc->descriptionStr = _("Pattern not found: ") + lastSearchStr;
            return;
        } else {
            GCInteractive::charEntered(c_p, modifiers);
            gc->descriptionStr = _("Search backward");
        }
        break;
    }
    case PG_SEARCH_FWD:
    {
        if (c == '\n' || c == '\r') {
            state = PG_NOP;
            vector<std::string>::const_iterator it = lines->begin();
            if (getBufferText() != "")
            {
                lastSearchStr = getBufferText();
            }
            int lastSearch = pageScrollIdx + 1;
            setBufferText("");
            if (lastSearch < (int) lines->size()) {
                it += lastSearch;
                for (uint j = 0; it < lines->end(); it++, j++)
                {
                    if ((std::string (*it)).find(lastSearchStr) != string::npos)
                    {
                        if (--N < 1) { // skip N matches
                            pageScrollIdx = lastSearch + j;
                            gc->descriptionStr = _("Search forward");
                            gc->descriptionStr += ": " + lastSearchStr;
                            return;
                        }
                    }
                }
            }
            N = -1;
            gc->descriptionStr = _("Pattern not found: ") + lastSearchStr;
            return;
        } else {
            GCInteractive::charEntered(c_p, modifiers);
            gc->descriptionStr = _("Search forward");
        }
        break;
    }
    } // switch

    if (leftScrollIdx < 0)
        leftScrollIdx = 0;
    if (pageScrollIdx >= (int) lines->size())
        pageScrollIdx = lines->size() - 1;
    if (pageScrollIdx < 0)
        pageScrollIdx = 0;
}

// Commands are based on both more and vi. Commands may be preceded by
// a decimal number, called N in.
// N = -1 is default
void PagerInteractive::processChar(const char *c_p, int modifiers)
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
                pageScrollIdx = lines->size() - scrollSize;
            else
                pageScrollIdx = lines->size() - scrollSize - N;
            leftScrollIdx = 0;
        } else if (c == 'p' || c == '%') {
            // Go to a position N percent
            if (N < 0) // default
                pageScrollIdx = 0;
            else
                pageScrollIdx = lines->size() * N / 100;
            leftScrollIdx = 0;
        } else if (c == ' ' || c == 'f' || c == CTRL_V) {
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
            // Scroll forward  N lines, default 1. The  entire N lines
            // are displayed, even if N is more than the screen size.
            if (N > 0)
                forward(N);
            else
                forward(1);
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
        } else if (c == 'r') {
            if (N > 0)
                chopLine = N;
            leftScrollIdx -= getChopLine() * gc->getCompletionFont()->getWidth("X");
        } else if (c == 'l') {
            if (N > 0)
                chopLine = N;
            leftScrollIdx += getChopLine() * gc->getCompletionFont()->getWidth("X");
        } else if (c == 'q' || c == 'Q') {
            if (lines != &helpText) // quit from interactive (finish)
                GCInteractive::charEntered("\r", 0);
            else { // quit from help
                lines = &text;
                pageScrollIdx = 0; leftScrollIdx = 0;
            }
        } else if (c == 'h' || c == 'H') { // help
            lines = &helpText;
            pageScrollIdx = 0; leftScrollIdx = 0;
        } else if (c == 'n') { // Repeat previous search (for N-th occurrence)
            state = PG_SEARCH_FWD;
            charEntered("\n", 0);
            return; // dont contine (N must be not reset)
        } else if (c == 'N') { // search prev
            state = PG_SEARCH_BWD;
            charEntered("\n", 0);
            return;
        } else {
            gc->beep();
        }
        break;
    }
    N = -1;
}

void PagerInteractive::renderCompletion(float height, float width)
{
    TextureFont *font = gc->getCompletionFont();
    float fwidth = font->getWidth("X");
    float fh = font->getHeight();
    vector<std::string>::const_iterator it = lines->begin();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel
    scrollSize = nb_lines;
    this->width = width;

    if (pageScrollIdx < 0)
        pageScrollIdx = 0;

    it += pageScrollIdx;

    glColor4ubv(clCompletionFnt->rgba);
    glPushMatrix();
    glTranslatef((float) -leftScrollIdx, 0.0f, 0.0f);
    gc->getOverlay()->beginText();
    uint j;
    for (j= 0; it < lines->end() && j < nb_lines; it++, j++)
    {
        *gc->getOverlay() << *it;
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
    if ((lines->size() - nb_lines) > 0)
        scrolled_perc = 100 * pageScrollIdx / (lines->size() - nb_lines);
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
        if (lines->size() > nb_lines)
        {
            float scrollH = height + fh - 2;
            glTranslatef(width - scrollW - 2.0f, -height, 0.0f);
            glColor4ubv(clCompletionExpandBg->rgba);
            gc->getOverlay()->rect(0.0f, 0.0f, scrollW, scrollH);
            // bar
            float scrollHInner = scrollH - 2;
            float scrollSize = scrollHInner / ((float)lines->size() / nb_lines);
            if (scrollSize < 2.0f)
                scrollSize = 2.0f;
            float offsetY = scrollHInner -
                pageScrollIdx * scrollHInner / lines->size() - scrollSize + 1;
            glColor4ubv(clCompletionFnt->rgba);
            gc->getOverlay()->rect(1.0f, offsetY, scrollW - 2.0f, scrollSize);
        }
    }
    gc->getOverlay()->endText();
}

void PagerInteractive::renderInteractive()
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

ListInteractive *listInteractive;
PasswordInteractive *passwordInteractive;
CelBodyInteractive *celBodyInteractive;
ColorChooserInteractive *colorChooserInteractive;
FlagInteractive *flagInteractive;
FileInteractive *fileInteractive;
PagerInteractive *pagerInteractive;

/******************************************************************
 *  Functions
 ******************************************************************/

int GCFunc::call(GeekConsole *gc, int state, std::string value)
{
    int res;
    switch (type)
    {
    case C:
        return cFun(gc, state, value);
    case Cvoid:
        vFun(); gc->finish(); return 1;
    case Lua:
        lua_rawgeti(lua, LUA_REGISTRYINDEX, luaCallBack);
        lua_pushinteger ( lua, state );                     // push second argument on stack
        lua_pushstring  ( lua, value.c_str());              // push first argument on stack
        res = lua_pcall ( lua, 2, 1, 0 );              // call function taking 2 argsuments and getting one return value
        if (res == 0)
            return lua_tointeger ( lua, -1 );
        else
        {
            gc->getCelCore()->flash("Error during executing lua function");
            break;
        }
    case LuaNamed:
        lua_pcall       ( lua, 0, LUA_MULTRET, 0 );
        lua_getfield    ( lua, LUA_GLOBALSINDEX, luaFunName.c_str());   // push global function on stack
        if (!lua_isfunction(lua, -1)) {
            gc->finish();
            gc->getCelCore()->flash("Lua function \""+luaFunName+"\" is undefined.");
            return 1;
        }
        lua_pushinteger ( lua, state );                     // push second argument on stack
        lua_pushstring  ( lua, value.c_str());              // push first argument on stack
        res = lua_pcall ( lua, 2, 1, 0 );              // call function taking 2 argsuments and getting one return value
        if (res == 0)
            return lua_tointeger ( lua, -1 );
        else
        {
            gc->getCelCore()->flash("Error during executing lua function");
            break;
        }
    case Alias:
        return gc->execFunction(aliasfun, params);
    default:
        break;
    }
    gc->finish();
    return 0;
}

static int execFunction(GeekConsole *gc, int state, std::string value)
{
    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "exec-function", "M-x", "Exec function");
        std::vector<std::string> v = gc->getFunctionsNames();
        std::sort(v.begin(), v.end());
        listInteractive->setCompletion(v);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case -1:
    {
        // describe key binds for this function
        GCFunc *f = gc->getFunctionByName(value);
        if (!f)
            break;
        gc->setInfoText(f->getInfo());
        const std::vector<GeekBind *>& gbs = gc->getGeekBinds();
        std::string bindstr;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
        {
            GeekBind *gb = *it;
            std::string str = gb->getBinds(value);
            if (!str.empty())
                bindstr += ' ' + gb->getName() + ": " + str;
        }
        if (!bindstr.empty())
            gc->descriptionStr += " [Matched;" + bindstr + ']';
        break;
    }
    case 1:
        return gc->execFunction(value);
    }
    return state;
}

static void startMacro()
{
    geekConsole->setMacroRecord(true);
}

static void endMacro()
{
    geekConsole->setMacroRecord(false);
}

static void endAndCallMacro()
{
    geekConsole->callMacro();
}

static int saveMacro(GeekConsole *gc, int state, std::string value)
{
    static string svmFunName; // save fun name here
    static string bindspace; // save name of bind space here

    geekConsole->setMacroRecord(false, true);

    if (state == 0)
    {
        gc->setInteractive(listInteractive, "exec-function", _("Alias name:"), _("Set name for alias"));
        gc->setInfoText("Current macro:\n" + gc->getCurrentMacro());
        listInteractive->setCompletion(gc->getFunctionsNames());
    } else if (state == 1)
    {
        svmFunName = value;
        if (gc->getFunctionByName(value))
        {
            gc->setInteractive(listInteractive, "yes-no", _("Are you sure to overwrite?"), "\"" + value + "\" " + _("already defined."));
            listInteractive->setCompletionFromSemicolonStr("yes;no");
            listInteractive->setMatchCompletion(true);
            gc->setInfoText(string(_("Info about")) + "\"" + value + "\":\n" + gc->getFunctionByName(value)->getInfo());
        }
        else // register function
            state = 3; // skip next state
    }

    if (state == 2)
    {
        if (value != "yes")
        {
            gc->setInteractive(listInteractive, "exec-function", _("Set other alias name:"),
                               string(_("Set name for alias, last was")) + "\"" + svmFunName + "\"");
            gc->setInfoText(string(_("Current macro")) + ":\n" + gc->getCurrentMacro());
            listInteractive->setCompletion(gc->getFunctionsNames());
            return 0; // recall with 0 state
        }
        state = 3;
    }

    if (state == 3) // register function and ask for bind space
    {
        gc->reRegisterFunction(GCFunc("exec function", gc->getCurrentMacro().c_str()), svmFunName);
        // TODO: maybe good to save macro in cfg file for load it at startup
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Bind key: Select bindspace"));
        gc->setInfoText(svmFunName + "\n" +
                        _("You can continue and assign key or break by ESC or C-g"));
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
    }

    if (state == 4 ) // ask for key
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Key bind"), _("Choose key bind, and parameters. Example: C-c g @param 1@"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(false);
        }
        return 4;
    } else if (state == -1 -4) // describe current bind
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->descriptionStr = gb->getBindDescr(value);
    } else if (state == 5) // finish
    {
        gc->bind(bindspace, value, svmFunName);
    }

    return state;
}

static int bindKey(GeekConsole *gc, int state, std::string value)
{
    static string bindspace;
    static string keybind;
    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Bind key: Select bindspace"));
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case 1:
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Key bind"), _("Choose key bind, and parameters. Example: C-c g @param 1@"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(false);
        }
        break;
    }
    case -1-1:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->descriptionStr = gb->getBindDescr(value);
        break;
    }
    case 2:
        keybind = value;
        gc->setInteractive(listInteractive, "exec-function", _("Function"), _("Select function to bind ") + keybind );
        listInteractive->setCompletion(gc->getFunctionsNames());
        listInteractive->setMatchCompletion(true);
        break;
    case 3:
        gc->bind(bindspace, keybind, value);
        break;
    default:
        break;
    }
    return state;
}

static int unBindKey(GeekConsole *gc, int state, std::string value)
{
    static string bindspace;

    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Unbind key: Select bindspace"));
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case 1:
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Un bind key"), _("Choose key bind"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(true);
        }
        break;
    }
    case -1-1:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->descriptionStr = gb->getBindDescr(value);
        break;
    }
    case 2:
        gc->unbind(bindspace, value);
        break;
    default:
        break;
    }
    return state;
}

static int describebindKey(GeekConsole *gc, int state, std::string value)
{
    static string bindspace;

    switch (state)
    {
    case 0:
    {
        gc->setInteractive(listInteractive, "bindkey-space", _("BindSpace"), _("Describe bind key: Select bindspace"));
        const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
        std::vector<string> completion;
        for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
             it != gbs.end(); it++)
        {
            GeekBind *gb = *it;
            std::string name = gb->getName();
            completion.push_back(name);
        }
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case 1:
    {
        bindspace = value;
        gc->setInteractive(listInteractive, "bindkey-key", _("Key bind for describing"), _("Choose key bind"));
        GeekBind *gb = gc->getGeekBind(value);
        if (gb)
        {
            listInteractive->setCompletion(gb->getAllBinds());
            listInteractive->setMatchCompletion(true);
        }
        break;
    }
    case -1-1:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
            gc->descriptionStr = gb->getBindDescr(value);
        break;
    }
    case 2:
    {
        GeekBind *gb = gc->getGeekBind(bindspace);
        if (gb)
        {
            std::stringstream ss;
            gc->setInteractive(pagerInteractive, "", _("Describe bind key"));
            ss << value << _(" runs the command \"");
            ss << gb->getFunName(value) << "\"";

            if (gb->getParams(value) != "")
                ss << _("\nwith params \"") << gb->getParams(value);
            ss << "\"\n\n";

            ss << _("It is bound to");
            // all binds of function
            const std::vector<GeekBind *> &gbs = gc->getGeekBinds();
            std::vector<string> completion;

            std::string function = gb->getFunName(value);
            for (std::vector<GeekBind *>::const_iterator it = gbs.begin();
                 it != gbs.end(); it++)
            {
                GeekBind *b = *it;
                std::string binds = b->getBinds(function);
                if (!binds.empty())
                    ss << " \"" << b->getName() << "\": " << binds;
            }
            ss << ".\n\n";

            GCFunc *f = gc->getFunctionByName(gb->getFunName(value));
            if (f)
            {
                std::string s = f->getInfo();
                if (s != "")
                    ss << f->getInfo() << "\n";
                if (f->getType() == GCFunc::Alias) {
                    ss << _("It is alias to ") << "\"" << f->getAliasFun() << "\"";
                    if (f->getAliasParams() != "")
                        ss << _("\nwith params \"") << f->getAliasParams() << "\"";

                }
            } else {
                ss << _("Function not defined\n");
            }

            pagerInteractive->setText(ss.str());
        }
        break;
    }
    default:
        break;
    }
    return state;
}

static bool isInteractsInit = false;

void initGCInteractives(GeekConsole *gc)
{
    if (!isInteractsInit)
    {
        listInteractive = new ListInteractive("list");
        passwordInteractive = new PasswordInteractive("passwd");
        celBodyInteractive = new CelBodyInteractive("cbody", gc->getCelCore());
        flagInteractive = new FlagInteractive("flag");
        fileInteractive = new FileInteractive("file");
        colorChooserInteractive = new ColorChooserInteractive("colorch");
        pagerInteractive = new PagerInteractive("pager");
        isInteractsInit = true;
    }
    gc->registerAndBind("", "M-x",
                        GCFunc(execFunction), "exec function");
    gc->registerFunction(GCFunc(bindKey), "bind");
    gc->registerFunction(GCFunc(unBindKey), "unbind");
    gc->registerFunction(GCFunc(describebindKey), "describe key");
    gc->registerAndBind("", "C-x C-k s",
                        GCFunc(startMacro, _("Define macro.\n"
                                             "Record subsequent keyboard input (only for geekconsole binds),\n"
                                             "record interactive action of geekconsole.\n"
                                             "To end record use \"macro end\",\n"
                                             "to call macro use \"macro end and call\"")),
                        "macro start");
    gc->registerAndBind("", "C-x C-k e",
                        GCFunc(endMacro, _("End macro.\n"
                                             "To call macro use \"macro end and call\"\n"
                                             "To save macro as alias and define key use \"macro save\"")),
                        "macro end");
    gc->registerAndBind("", "C-x C-k n",
                        GCFunc(saveMacro, _("Assign name to last macro.\n"
                                                  "Also bind you can bind key")),
                        "macro save");
    gc->registerAndBind("", "C-x e",
                        GCFunc(endAndCallMacro, _("Call last macro, "
                                                  "ending it first if currently being defined.\n"
                                                  "Also bind to key")),
                        "macro end and call");
}

void destroyGCInteractives()
{
    delete listInteractive;
    delete passwordInteractive;
    delete celBodyInteractive;
    delete colorChooserInteractive;
    delete flagInteractive;
    delete fileInteractive;
    delete pagerInteractive;
}

void initGeekConsole(CelestiaCore *celApp)
{
    geekConsole = new GeekConsole(celApp);

    initGCInteractives(geekConsole);
    initGCStdInteractivsFunctions(geekConsole);
}

void shutdownGeekconsole()
{
    destroyGCInteractives();
    delete geekConsole;
}

GeekConsole *getGeekConsole()
{
    return geekConsole;
}
