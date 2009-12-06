#include "cgame.h"
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
#define CEL_DESCR_SEP " | "
#endif

// margin from bottom
#define BOTTOM_MARGIN 5.0

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

GeekConsole *geekConsole = NULL;
const std::string ctrlZDescr("C-z - Unexpand");
const std::string strUniqMatchedRET("Unique match, RET - complete and finish");

std::string historyDir("history/");

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
            if (color == colorTable[i].colorName)
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
    return Color(c.rgba[0] / 255, (float)c.rgba[1] / 255,
                 (float)c.rgba[2] / 255, (float)c.rgba[3] / 255);
}

static FormattedNumber SigDigitNum(double v, int digits)
{
    return FormattedNumber(v, digits,
                           FormattedNumber::GroupThousands |
                           FormattedNumber::SignificantDigits);
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

std::string describeSelection(Selection sel, CelestiaCore *celAppCore)
{
    std::stringstream ss;
    if (!sel.empty())
    {
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
            ss << "[Star] Dist: ";
            distance2Sstream(ss, distance);
            ss << CEL_DESCR_SEP << _("Radius: ")
               << SigDigitNum(star->getRadius() / 696000.0f, 2)
               << " " << _("Rsun")
               << "  (" << SigDigitNum(star->getRadius(), 3) << " km" << ")"
               << CEL_DESCR_SEP "class ";
            if (star->getSpectralType()[0] == 'Q')
                ss <<  _("Neutron star");
            else if (star->getSpectralType()[0] == 'X')
                ss <<  _("Black hole");
            else
                ss << star->getSpectralType();
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
                ss << CEL_DESCR_SEP << planetsnum << " Planets, " << dwarfsnum
                   << " Dwarf, " << astersnum << " Asteroid";
            }
            break;
        case Selection::Type_Body:
            distance = v.length() * 1e-6,
                v * astro::microLightYearsToKilometers(1.0);
            body = sel.body();
            kmDistance = astro::lightYearsToKilometers(distance);
            ss << "[Body] Dist: ";
            distance = astro::kilometersToLightYears(kmDistance - body->getRadius());
            distance2Sstream(ss, distance);
            ss << CEL_DESCR_SEP << _("Radius: ");
            distance = astro::kilometersToLightYears(body->getRadius());
            distance2Sstream(ss, distance);
            break;
        case Selection::Type_DeepSky:
        {
            dso = sel.deepsky();
            distance = v.length() * 1e-6 - dso->getRadius();
            char descBuf[128];
            dso->getDescription(descBuf, sizeof(descBuf));
            ss << "[DSO, "
               << descBuf
               << "] ";
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
            ss << CEL_DESCR_SEP << _("Radius: ");
            distance2Sstream(ss, dso->getRadius());
            break;
        }
        case Selection::Type_Location:
        {
            location = sel.location();
            body = location->getParentBody();
            ss << "[Location] "
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
    curFun(NULL)
{
    overlay = new Overlay();
    *overlay << setprecision(6);
    *overlay << ios::fixed;
    // create global key bind space
//    geekConsole->createGeekBind("Global");
    GeekBind *gb = new GeekBind("Global");
    geekBinds.push_back(gb);
    curKey.len = 0;
}

GeekConsole::~GeekConsole()
{
    for_each(geekBinds.begin(), geekBinds.end(), deleteFunc<GeekBind*>());;
    delete overlay;
}

void GeekConsole::execFunction(GCFunc *fun)
{
    curFun = fun;
    funState = 0;
    isVisible = true;
    fun->call(this, funState, "");
}

bool GeekConsole::execFunction(std::string funName)
{
    GCFunc *f = getFunctionByName(funName);
    curFun = f;
    funState = 0;
    if (f)
    {
        isVisible = true;
        f->call(this, funState, "");
    }
    else
        isVisible = false;
    return f;
}

bool GeekConsole::execFunction(std::string funName, std::string param)
{
    GCFunc *f = getFunctionByName(funName);
    curFun = f;
    funState = 0;
    if (f)
    {
        isVisible = true;
        std::string val;
        call("");
        if (!param.empty())
        {
            if (param[param.size() - 1] == '@')
                param.resize(param.size() - 1);
            if (param[0] == '@')
                param.erase(0, 1);
        }
        else
            return true;
        vector<string> res;
        string::size_type next_pos, curr_pos=0;

        for (int i=0; ;i++) {
            next_pos = param.find_first_of("@", curr_pos);
            val = (param.substr(curr_pos, next_pos-curr_pos));
            funState++;
            if (!isVisible)
            {
                if (val == "EXEC")
                {
                    GCFunc *f = getFunctionByName(funName);
                    if (!f)
                    {
                        DPRINTF(1, "GFunction not found: '%s'", val.c_str());
                        return false;
                    }
                    isVisible = true;
                    curFun = f;
                    funState = 0;
                    call("");
                }
            }
            else
                call(val);

            if (string::npos == next_pos)
                break;
            curr_pos = next_pos+1;
        }
        // int i = 0;
        // size_t pos = 0, end;
        // std::string val;
        // curFun->call(this, funState, "");
        // pos = param.find('@');
        // funState++;
        // while(pos != std::string::npos && isVisible)
        // {
        //     end = param.find('@', pos + 1);
        //     val = param.substr(pos + 1, end - 1);
        //     cout << val << "' --- val state: " << funState << " " << param << "\n";
        //     isVisible = true;
        //     curFun->call(this, funState, val);
        //     pos = param.find('@', end);
        // }
    }
    else
        isVisible = false;
    return f;
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
bool GeekConsole::charEntered(const char sym, const wchar_t wc, int modifiers)
{
    if (!isVisible)
    {
        curKey.c[curKey.len] = sym;
        curKey.mod[curKey.len] = modifiers;
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
            const std::vector<GeekBind::KeyBind>& binds = (*gb)->getBinds();
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
                        if (execFunction(it->gcFunName, it->params))
                        {
                            if (curKey.len > 1)
                                getCelCore()->flash(curKey.keyToStr() + " (" + it->gcFunName + 
                                                    ") " + it->params, 2.5);
                        }
                        else
                        {
                            getCelCore()->flash(curKey.keyToStr() + " (" + it->gcFunName +
                                                    ") not defined", 1.5);
                        }
                        curKey.len = 0;
                        return true;
                    }
                    else
                        isKeyPrefix = true;
            }
        }
        if (!isKeyPrefix || curKey.len == MAX_KEYBIND_LEN)
        {
            // for key length 1 dont flash messg.
            if (curKey.len > 1)
                getCelCore()->flash(curKey.keyToStr() + " is undefined");
            curKey.len = 0;
            return false;
        }
        else
            getCelCore()->flash(curKey.keyToStr() + "-", 15.0);
        return true;
    } // !isVisible

    char C = toupper((char)wc);
    switch (C)
    {
    case '\023':  // Ctrl+S
        if (consoleType >= Big)
            consoleType = Tiny;
        else
            consoleType++;
        return true;
    case '\007':  // Ctrl+G
    case '\033':  // ESC
        finish();
    return true;
    }

    if (curInteractive)
        curInteractive->charEntered(wc, modifiers);
    return true;
}

void GeekConsole::render()
{
    if (!isVisible || !curInteractive)
        return;
    if (font == NULL)
        font = getFont(celCore);
    if (titleFont == NULL)
        titleFont = getTitleFont(celCore);
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
        nb_lines = (height * 0.4 - 2 * titleFontH) / fontH;
        break;
    case GeekConsole::Big:
    default:
        nb_lines = (height * 0.8 - 2 * titleFontH) / fontH;
        break;
    }
    complH = (nb_lines+1) * fontH + nb_lines;
    rectH = complH + 2 * titleFontH;
    overlay->begin();
    glTranslatef(0.0f, BOTTOM_MARGIN, 0.0f); //little margin from bottom

    // background
    glColor4ubv(clBackground->rgba);
    overlay->rect(0.0f, 0.0f, width, rectH);

    // Interactive & description rects
    glColor4ubv(clBgInteractive->rgba);
    overlay->rect(0.0f, 0.0f , width, titleFontH);
    overlay->rect(0.0f, rectH - titleFontH , width, titleFontH);
    glColor4ubv(clBgInteractiveBrd->rgba);
    overlay->rect(0.0f, 0.0f , width-1, titleFontH, false);
    overlay->rect(0.0f, rectH - titleFontH , width-1, titleFontH, false);

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
}

void GeekConsole::call(const std::string &value)
{
    if(!curFun) return;
    funState = curFun->call(this, funState, value);
}

void GeekConsole::InteractFinished(std::string value)
{
    if(!curFun) return;
    funState++;
    call(value);
}

void GeekConsole::finish()
{
    isVisible = false;
    curFun = NULL;
    if (curInteractive)
        curInteractive->cancelInteractive();
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
    if (gb)
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

/******************************************************************
 *  Interactives
 ******************************************************************/

GCInteractive::GCInteractive(std::string _InteractiveName, bool _useHistory)
{
    InteractiveName = _InteractiveName;
    useHistory = _useHistory;
    if (!useHistory)
        return;
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
            if (compareIgnoringCase("." + InteractiveName, ext) == 0)
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
            std::ofstream outfile(string(historyDir + name + "." + InteractiveName).c_str(), ofstream::out);

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

void GCInteractive::charEntered(const wchar_t wc, int modifiers)
{
    char C = toupper((char)wc);
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
        if (typedHistoryCompletionIdx > typedHistoryCompletion.size()-1)
            typedHistoryCompletionIdx = 0;
        rit = typedHistoryCompletion.rbegin() + typedHistoryCompletionIdx;
        typedHistoryCompletionIdx++;
        buf = *rit;
        gc->descriptionStr = ctrlZDescr;
        gc->describeCurText(getBufferText());
        return;
    case '\016':  // Ctrl+N forw history
        if (typedHistoryCompletion.empty())
            return;
        if (typedHistoryCompletionIdx > typedHistoryCompletion.size()-1)
            typedHistoryCompletionIdx = 0;
        it = typedHistoryCompletion.begin() + typedHistoryCompletionIdx;
        typedHistoryCompletionIdx--;
        buf = *it;
        gc->descriptionStr = ctrlZDescr;
        gc->describeCurText(getBufferText());
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
            while (buf.size() && !strchr(" /:,;", buf[buf.size() - 1])) {
                setBufferText(string(buf, 0, buf.size() - 1));
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
            buf += wc;
            prepareHistoryCompletion();
            bufSizeBeforeHystory = buf.size();
        }
    gc->descriptionStr.clear();
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
    if (bufSizeBeforeHystory <= buf.size())
    {
        glColor4ubv(clInteractiveExpand->rgba);
    	std::string s = string(buf, bufSizeBeforeHystory, buf.size() - bufSizeBeforeHystory);
    	*gc->getOverlay() << s;
        glColor4ubv(clInteractiveFnt->rgba);
        *gc->getOverlay() << "|";
    }
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
    ListInteractive::setColumns(2);
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
            if (i * nb_lines + j == completedIdx - pageScrollIdx)
            {
                // border of current completion
                glColor4ubv(clCompletionExpandBg->rgba);
                gc->getOverlay()->rect(-2.0f, 0.0f - 2,
                                       marg + 2, fh + 4);
            }
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
}

void ListInteractive::updateTextCompletion()
{
    std::string buftext = string(getBufferText(), 0, bufSizeBeforeHystory);
    std::vector<std::string>::iterator it;
    typedTextCompletion.clear();
    int buf_length = UTF8Length(buftext);
    // Search through all string in base completion list
    for (it = completionList.begin();
         it != completionList.end(); it++)
    {
        if (buf_length == 0 ||
            (UTF8StringCompare(*it, buftext, buf_length) == 0))
            typedTextCompletion.push_back(*it);
    }
}

bool ListInteractive::tryComplete()
{
    int i = 0; // match size
    bool match = true;
    int buf_length = UTF8Length(getBufferText());
    vector<std::string>::const_iterator it1, it2;
    if (!typedTextCompletion.empty())
        if (typedTextCompletion.size() == 1) // only 1 item to expand
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            setBufferText(*it);
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
        setBufferText(string(*it, 0, buf_length + i));
        return true;
    }
    return false;
}

void ListInteractive::charEntered(const wchar_t wc, int modifiers)
{
    char C = toupper((char)wc);
    if  (C == '\t') // TAB - expand and scroll if many items to expand found
    {
        if (!tryComplete())
        {
            // scroll
            pageScrollIdx += scrollSize;
            if (pageScrollIdx > typedTextCompletion.size())
                pageScrollIdx = 0;
            char buff[128];
            sprintf(buff,"%i of %i pages, M-/ next expand, M-? prev expand, C-z unexpand", (pageScrollIdx / scrollSize) + 1,
                    (typedTextCompletion.size() / scrollSize) + 1);
            gc->descriptionStr = buff;
            gc->describeCurText(getBufferText());
            completedIdx = pageScrollIdx;
            return;
        }

    }
    // expand completion on M-/
    else if (((modifiers & GeekBind::META) != 0) && C == '/')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            completedIdx++;
            if (completedIdx >= typedTextCompletion.size())
                completedIdx = pageScrollIdx;
            if (completedIdx > pageScrollIdx + scrollSize - 1)
                completedIdx = pageScrollIdx;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setBufferText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            gc->descriptionStr = ctrlZDescr;
            gc->describeCurText(getBufferText());
            return;
        }
    }
    // revers expand completion on M-? ( i.e. S-M-/)
    else if (((modifiers & GeekBind::META ) != 0) && C == '?')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (completedIdx == 0)
                completedIdx = pageScrollIdx + scrollSize;
            completedIdx--;
            if (completedIdx < pageScrollIdx)
                completedIdx = pageScrollIdx + scrollSize;
            if (completedIdx > typedTextCompletion.size())
                completedIdx = typedTextCompletion.size() - 1;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setBufferText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            gc->descriptionStr = ctrlZDescr;
            gc->describeCurText(getBufferText());
            return;
        }
    }
    else if ((C == '\n' || C == '\r'))
    {
        if(mustMatch)
        {
            tryComplete(); // complite if only matching enable
            std::string buftext = getBufferText();
            std::vector<std::string>::iterator it;
            for (it = typedTextCompletion.begin();
                 it != typedTextCompletion.end(); it++)
            {
                if (*it == buftext)
                {
                    GCInteractive::charEntered(wc, modifiers);
                    return;
                }
            }
            return;
        }
    }
    std::string oldBufText = getBufferText();

    GCInteractive::charEntered(wc, modifiers);

    std::string buftext = getBufferText();
    // reset page scroll only if text changed
    if (oldBufText != buftext)
        pageScrollIdx = 0;

    // if must_always_match with completion items - try complete

    if(mustMatch && buftext.length() > oldBufText.length())
        tryComplete();

    // Refresh completion (text to complete is only first bufSizeBeforeHystory chars
    // of buf text)
    updateTextCompletion();

    // if match on - complete again
    // and if no any item to expand than revert to old text when text increased
    if(mustMatch && buftext.length() > oldBufText.length())
    {
        tryComplete();
        if (typedTextCompletion.empty())
        {
            setBufferText(oldBufText);
            updateTextCompletion();
        }
    }

    if (typedTextCompletion.size() == 1)
    {
        gc->descriptionStr = strUniqMatchedRET;
	    gc->describeCurText(getBufferText());
    }
}

void ListInteractive::renderCompletion(float height, float width)
{
    TextureFont *font = gc->getCompletionFont();
    float fh = font->getHeight();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel
    uint nb_cols = cols;
    scrollSize = nb_cols * nb_lines;
    std::string buftext(getBufferText(), 0, bufSizeBeforeHystory);
    uint buf_length = buftext.size();
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
                                       font->getWidth(s), fh);
            }
            // completion item (only text before match char)
            glColor4ubv(clCompletionFnt->rgba);
            *gc->getOverlay() << std::string(s, 0, buf_length);
            // match char background
            // match char background
            if (s.size() >= buf_length)
            {
                if (clCompletionMatchCharBg->rgba[3] > 0)
                {
                    std::string text(s, buf_length, 1);
                    glColor4ubv(clCompletionMatchCharBg->rgba);
                    gc->getOverlay()->rect(font->getWidth(string(s, 0, buf_length)), 0.0f - 2,
                                           font->getWidth(text), fh);
                    glColor4ubv(clCompletionMatchCharFnt->rgba);
                    *gc->getOverlay() << text;
                }
                // rest text of item from match char
                glColor4ubv(clCompletionAfterMatch->rgba);
                if (s.size() > buf_length)
                    *gc->getOverlay() << string(s, buf_length + 1, 255);
                *gc->getOverlay() << "\n";
            }
        }
        gc->getOverlay()->endText();
        glPopMatrix();
        glTranslatef((float) shiftx, 0.0f, 0.0f);
    }
}

void ListInteractive::setCompletion(std::vector<std::string> completion)
{
    completionList = completion;
    pageScrollIdx = 0;
    typedTextCompletion =  completion;
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
    GCInteractive(name), celApp(core), cols(4)
{

}

void CelBodyInteractive::updateTextCompletion()
{
    string str = GCInteractive::getBufferText();

    //skip update for some history and expand action
    if (str.length() != bufSizeBeforeHystory)
        return;
    typedTextCompletion = celApp->getSimulation()->
        getObjectCompletion(str,
                            (celApp->getRenderer()->getLabelMode() & Renderer::LocationLabels) != 0);
}

std::string CelBodyInteractive::getRightText() const
{
    string str = GCInteractive::getBufferText();
    string::size_type pos = str.rfind('/', str.length());
    if (pos != string::npos)
        return string(str, pos + 1);
    else
        return str;
}

void CelBodyInteractive::setRightText(std::string text)
{
    string str = GCInteractive::getBufferText();
    string::size_type pos = str.rfind('/', str.length());
    if (pos != string::npos)
    {
        GCInteractive::setBufferText(string(str, 0, pos + 1) + text);
    }
    else
        GCInteractive::setBufferText(text);
}

void CelBodyInteractive::Interact(GeekConsole *_gc, string historyName)
{
    GCInteractive::Interact(_gc, historyName);
    updateTextCompletion();
    pageScrollIdx = 0;
    completedIdx = -1;
    lastCompletionSel = Selection();
}

bool CelBodyInteractive::tryComplete()
{
    int i = 0; // match size
    bool match = true;
    int buf_length = UTF8Length(getRightText());
    vector<std::string>::const_iterator it1, it2;
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

void CelBodyInteractive::charEntered(const wchar_t wc, int modifiers)
{
    char C = toupper((char)wc);
    if  (C == '\t') // TAB - expand and scroll if many items to expand found
    {
        if (!tryComplete())
        {
            // scroll
            pageScrollIdx += scrollSize;
            if (pageScrollIdx > typedTextCompletion.size())
                pageScrollIdx = 0;
            char buff[128];
            sprintf(buff,"%i of %i pages, M-/ next expand, M-? prev expand, C-z unexpand", (pageScrollIdx / scrollSize) + 1,
                    (typedTextCompletion.size() / scrollSize) + 1);
            gc->descriptionStr = buff;
            completedIdx = pageScrollIdx;
            updateDescrStr();
            return;
        }

    }
    // expand completion on M-/
    else if (((modifiers & GeekBind::META) != 0) && C == '/')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            completedIdx++;
            if (completedIdx >= typedTextCompletion.size())
                completedIdx = pageScrollIdx;
            if (completedIdx > pageScrollIdx + scrollSize - 1)
                completedIdx = pageScrollIdx;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setRightText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            updateDescrStr();
            gc->describeCurText(getRightText());
            return;
        }
    }
    // revers expand completion on M-? ( i.e. S-M-/)
    else if (((modifiers & GeekBind::META ) != 0) && C == '?')
    {
        if (!typedTextCompletion.empty())
        {
            vector<std::string>::const_iterator it = typedTextCompletion.begin();
            if (completedIdx == 0)
                completedIdx = pageScrollIdx + scrollSize;
            completedIdx--;
            if (completedIdx < pageScrollIdx)
                completedIdx = pageScrollIdx + scrollSize;
            if (completedIdx > typedTextCompletion.size())
                completedIdx = typedTextCompletion.size() - 1;
            it += completedIdx;
            uint oldBufSizeBeforeHystory = bufSizeBeforeHystory;
            setRightText(*it);
            bufSizeBeforeHystory = oldBufSizeBeforeHystory;
            updateDescrStr();
            gc->describeCurText(getRightText());
            return;
        }
    }
    // M-c
    else if (((modifiers & GeekBind::META ) != 0) && C == 'C')
    {
        celApp->getSimulation()->setSelection(lastCompletionSel);
        celApp->getSimulation()->centerSelection();
        return;
    }
    else if ((C == '\n' || C == '\r'))
    {
        Selection sel = celApp->getSimulation()->findObjectFromPath(GCInteractive::getBufferText(), true);
        if (sel.empty()) // dont continue if not match any obj
            return;
        GCInteractive::charEntered(wc, modifiers);
        return;
    }

    std::string oldBufText = GCInteractive::getBufferText();

    GCInteractive::charEntered(wc, modifiers);

    std::string buftext = GCInteractive::getBufferText();

    // reset page scroll only if text changed
    if (oldBufText != buftext)
        pageScrollIdx = 0;

    updateTextCompletion();

    updateDescrStr();
}

void CelBodyInteractive::updateDescrStr()
{
    Selection sel = celApp->getSimulation()->findObjectFromPath(GCInteractive::getBufferText(), true);
    std::string desc = describeSelection(sel, celApp);
    if (!desc.empty())
    {
        gc->descriptionStr = desc + _(", M-c - Select");
    }
    // mark selected
    MarkerRepresentation markerRep(MarkerRepresentation::Crosshair);
    markerRep.setSize(10.0f);
    markerRep.setColor(Color(0.0f, 1.0f, 0.0f, 0.9f));
    celApp->getSimulation()->
        getUniverse()->unmarkObject(lastCompletionSel, 3);
    celApp->getSimulation()->
        getUniverse()->markObject(sel, markerRep, 3);
    lastCompletionSel = sel;
}

void CelBodyInteractive::renderCompletion(float height, float width)
{
    if (typedTextCompletion.empty())
        return;
    TextureFont *font = gc->getCompletionFont();
    float fh = font->getHeight();
    uint nb_lines = height / (fh + 1); // +1 because overlay margin be-twin '\n' is 1 pixel
    uint nb_cols = cols;
    scrollSize = nb_cols * nb_lines;
    std::string buftext(GCInteractive::getBufferText(), 0, bufSizeBeforeHystory);
    string::size_type pos = buftext.rfind('/', buftext.length());
    if (pos != string::npos)
        buftext = string(buftext, pos + 1);

    uint buf_length = buftext.size();
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
                                       font->getWidth(s), fh);
            }
            // completion item (only text before match char)
            glColor4ubv(clCompletionFnt->rgba);
            *gc->getOverlay() << std::string(s, 0, buf_length);
            // match char background
            if (s.size() >= buf_length)
            {
                if (clCompletionMatchCharBg->rgba[3] > 0)
                {
                    std::string text(s, buf_length, 1);
                    glColor4ubv(clCompletionMatchCharBg->rgba);
                    gc->getOverlay()->rect(font->getWidth(string(s, 0, buf_length)), 0.0f - 2,
                                           font->getWidth(text), fh);
                    glColor4ubv(clCompletionMatchCharFnt->rgba);
                    *gc->getOverlay() << text;
                }
                // rest text of item from match char
                glColor4ubv(clCompletionAfterMatch->rgba);
                if (s.size() > buf_length)
                    *gc->getOverlay() << string(s, buf_length + 1, 255);
                *gc->getOverlay() << "\n";
            }
        }
        gc->getOverlay()->endText();
        glPopMatrix();
        glTranslatef((float) shiftx, 0.0f, 0.0f);
    }
}

void CelBodyInteractive::cancelInteractive()
{
    celApp->getSimulation()->
        getUniverse()->unmarkObject(lastCompletionSel, 3);
    lastCompletionSel = Selection();
}

ListInteractive *listInteractive;
PasswordInteractive *passwordInteractive;
CelBodyInteractive *celBodyInteractive;
ColorChooserInteractive *colorChooserInteractive;

/******************************************************************
 *  Functions
 ******************************************************************/

int GCFunc::call(GeekConsole *gc, int state, std::string value)
{
    if (!isLuaFunc)
        if (vFun)
        { vFun(); gc->finish(); return 1;}
        else
            return cFun(gc, state, value);
    else
    {
        lua_pcall       ( lua, 0, LUA_MULTRET, 0 );
        lua_getfield    ( lua, LUA_GLOBALSINDEX, luaFunName.c_str());   // push global function on stack
        lua_pushinteger ( lua, state );                     // push second argument on stack
        lua_pushstring  ( lua, value.c_str());              // push first argument on stack
        lua_pcall       ( lua, 2, 1, 0 );                   // call function taking 2 argsuments and getting one return value
        return lua_tointeger ( lua, -1 );
    }
}

static int execFunction(GeekConsole *gc, int state, std::string value)
{
    GCFunc *f;
    switch (state)
    {
    case 0:
        gc->setInteractive(listInteractive, "exec-function", "M-x", "Exec function");
        listInteractive->setCompletion(gc->getFunctionsNames());
        listInteractive->setMatchCompletion(true);
        break;
    case -1:
    {
        // describe key binds for this function
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
        f = gc->getFunctionByName(value);
        if (f)
        {
            gc->execFunction(f);
            return 0;
        }
        break;
    }
    return state;
}

static int gotoBody(GeekConsole *gc, int state, std::string value)
{
    gc->getCelCore()->getSimulation()->
        gotoSelection(5.0, Vector3f::UnitY(), ObserverFrame::ObserverLocal);
    gc->finish();
    return state;
}

static int gotoBodyGC(GeekConsole *gc, int state, std::string value)
{
    gc->getCelCore()->getSimulation()->getObserver().gotoSelectionGC(
        gc->getCelCore()->getSimulation()->getSelection(),
        5.0, 0.0, 0.5,
        Vector3f::UnitY(), ObserverFrame::ObserverLocal);
    gc->finish();
    return state;
}

static int unmarkAll(GeekConsole *gc, int state, std::string value)
{
    Simulation* sim = gc->getCelCore()->getSimulation();
    sim->getUniverse()->unmarkAll();
    gc->finish();
    return state;
}

static int selectBody(GeekConsole *gc, int state, std::string value)
{
    switch (state)
    {
    case 1:
    {
        Selection sel = gc->getCelCore()->getSimulation()->findObjectFromPath(value, true);
        if (!sel.empty())
        {
            gc->getCelCore()->getSimulation()->setSelection(sel);
        }
    }
    gc->finish();
    break;
    case 0:
        gc->setInteractive(celBodyInteractive, "select", _("Target name: "), _("Enter target for select"));
        break;
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
        break;
    case 3:
        gc->bind(bindspace, keybind, value);
        gc->finish();
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
        gc->finish();
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
            gc->getCelCore()->flash(gb->getBindDescr(value), 6);
            gc->finish();
        }
        break;
    }
    default:
        break;
    }
    return state;
}

static StarBrowser starBrowser;
static int selectStar(GeekConsole *gc, int state, std::string value)
{
    const std::string nearest("Nearest");
    const std::string brightestApp("Brightest App");
    const std::string brightestAbs("Brightest Abs");
    const std::string wplanets("With Planets");
    std::vector<std::string> completion;
    switch (state)
    {
    case 0: // sort stars by..
    {
        completion.push_back(nearest);      completion.push_back(brightestApp);
        completion.push_back(brightestAbs); completion.push_back(wplanets);
        gc->setInteractive(listInteractive, "sort-star", _("Sort by:"), "");
        listInteractive->setCompletion(completion);
        listInteractive->setMatchCompletion(true);
        break;
    }
    case 1:
    {
        // set sort type
        if (compareIgnoringCase(value, nearest) == 0)
            starBrowser.setPredicate(0);
        else if (compareIgnoringCase(value, brightestApp) == 0)
            starBrowser.setPredicate(1);
        else if (compareIgnoringCase(value, brightestAbs) == 0)
            starBrowser.setPredicate(2);
        else if (compareIgnoringCase(value, wplanets) == 0)
            starBrowser.setPredicate(3);
        // interact for num of stars
        gc->setInteractive(listInteractive, "num-of-star", _("Num of stars:"), "");
        listInteractive->setCompletionFromSemicolonStr("16;50;100;200;300;400;500");
        break;
    }
    case 2: // select star
    {
        int numListStars = atoi(value.c_str());
        if (numListStars < 1)
            numListStars = 1;
        /* Load the catalogs and set data */
        starBrowser.setSimulation(gc->getCelCore()->getSimulation());
        StarDatabase* stardb = celAppCore->getSimulation()->
            getUniverse()->getStarCatalog();
        starBrowser.refresh();
        vector<const Star*> *stars = starBrowser.listStars(numListStars);
        if (!stars)
        {
            gc->finish();
            break;
        }
        int currentLength = (*stars).size();
        if (currentLength == 0)
        {
            gc->finish();
            break;
        }
        celAppCore->getSimulation()->setSelection(Selection((Star *)(*stars)[0]));

        for (int i = 0; i < currentLength; i++)
        {
            const Star *star=(*stars)[i];
            completion.push_back(stardb->getStarName(*star));
        }

        gc->setInteractive(listInteractive, "select-star", _("Select star"), "");
        listInteractive->setCompletion(completion);
        break;
    }
    case -2-1: // describe star
    {
        cout << 2 << "\n";
        Selection sel = gc->getCelCore()->getSimulation()->
            findObjectFromPath(value, true);
        std::string descr = describeSelection(sel, gc->getCelCore());
        if (!descr.empty())
            gc->descriptionStr = describeSelection(sel, gc->getCelCore());
        break;
    }
    case 3: // finish
    {
        Selection sel = gc->getCelCore()->getSimulation()->findObjectFromPath(value, true);
        if (!sel.empty())
        {
            gc->getCelCore()->getSimulation()->setSelection(sel);
        }
        gc->finish();
        break;
    }
    default:
        break;
    }
    return state;
}

static bool isInteractsInit = false;

void initGCInteractivesAndFunctions(GeekConsole *gc)
{
    if (!isInteractsInit)
    {
        listInteractive = new ListInteractive("list");
        passwordInteractive = new PasswordInteractive("passwd");
        celBodyInteractive = new CelBodyInteractive("cbody", gc->getCelCore());
        colorChooserInteractive = new ColorChooserInteractive("colorch");
        isInteractsInit = true;
    }
    gc->registerAndBind("", "M-x",
                        GCFunc(execFunction), "exec function");
    gc->registerAndBind("", "C-RET",
                        GCFunc(selectBody), "select object");
    gc->registerFunction(GCFunc(gotoBody), "goto object");
    gc->registerAndBind("", "C-c g",
                        GCFunc(gotoBodyGC), "goto object gc");
    gc->registerFunction(GCFunc(bindKey), "bind");
    gc->registerFunction(GCFunc(unBindKey), "unbind");
    gc->registerFunction(GCFunc(describebindKey), "describe key");
    gc->registerFunction(GCFunc(selectStar), "select star");
    gc->registerFunction(GCFunc(unmarkAll), "unmark all");
}

void destroyGCInteractivesAndFunctions()
{
    delete listInteractive;
    delete passwordInteractive;
    delete celBodyInteractive;
}

/******************************************************************
 *  Lua API for geek console
 ******************************************************************/

static int registerFunction(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gc.registerFunction(string, string)");
    const char *gcFunName = celx.safeGetString(1, AllErrors, "argument 1 to gc.reRegisterFunction must be a string");
    const char *luaFunName = celx.safeGetString(2, AllErrors, "argument 2 to gc.reRegisterFunction must be a string");
    geekConsole->registerFunction(GCFunc(luaFunName, l), gcFunName);
    return 0;
}

static int reRegisterFunction(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gc.reRegisterFunction(string, string)");
    const char *gcFunName = celx.safeGetString(1, AllErrors, "argument 1 to gc.reRegisterFunction must be a string");
    const char *luaFunName = celx.safeGetString(2, AllErrors, "argument 2 to gc.reRegisterFunction must be a string");
    geekConsole->reRegisterFunction(GCFunc(luaFunName, l), gcFunName);
    return 0;
}

static int setListInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(5, 5, "Two or three arguments expected for gc.listInteractive(sCompletion, shistory, sPrefix, sDescr, mustMatch)");
    const char *completion = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    const char *history = celx.safeGetString(2, AllErrors, "argument 2 to gc.listInteractive must be a string");
    const char *prefix = celx.safeGetString(3, AllErrors, "argument 3 to gc.listInteractive must be a string");
    const char *descr = celx.safeGetString(4, AllErrors, "argument 4 to gc.listInteractive must be a string");
    int isMustMatch = celx.safeGetNumber(5, WrongType, "argument 5 to gc.listInteractive must be a number", 0);
    geekConsole->setInteractive(listInteractive, history, prefix, descr);
    listInteractive->setCompletionFromSemicolonStr(completion);
    listInteractive->setMatchCompletion(isMustMatch);
    return 0;
}

static int setPasswdInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gc.listInteractive(sPrefix, sDescr)");
    const char *prefix = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    const char *descr = celx.safeGetString(2, AllErrors, "argument 2 to gc.listInteractive must be a string");
    geekConsole->setInteractive(passwordInteractive, "", prefix, descr);
    return 0;
}

static int setCelBodyInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 3, "Three arguments expected for gc.celBodyInteractive(shistory, sPrefix, sDescr)");
    const char *history = celx.safeGetString(1, AllErrors, "argument 1 to gc.celBodyInteractive must be a string");
    const char *prefix = celx.safeGetString(2, AllErrors, "argument 2 to gc.celBodyInteractive must be a string");
    const char *descr = celx.safeGetString(3, AllErrors, "argument 3 to gc.celBodyInteractive must be a string");
    geekConsole->setInteractive(celBodyInteractive, history, prefix, descr);
    return 0;
}

static int setColorChooseInteractive(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 3, "Three arguments expected for gc.celBodyInteractive(shistory, sPrefix, sDescr)");
    const char *history = celx.safeGetString(1, AllErrors, "argument 1 to gc.colorChooseInteractive must be a string");
    const char *prefix = celx.safeGetString(2, AllErrors, "argument 2 to gc.colorChooseInteractive must be a string");
    const char *descr = celx.safeGetString(3, AllErrors, "argument 3 to gc.colorChooseInteractive must be a string");
    geekConsole->setInteractive(colorChooserInteractive, history, prefix, descr);
    return 0;
}

static int color2HexRGB(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.color2Hext(sColorName)");
    const char *colorname = celx.safeGetString(1, AllErrors, "argument 1 to gc.color2Hex must be a string");
    Color32 c = getColor32FromText(colorname);
    char buf[32];
    sprintf (buf, "#%02X%02X%02X", c.rgba[0], c.rgba[1], c.rgba[2]);
    lua_pushstring(l, buf);
    return 1;
}

static int finishGConsole(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 0, "No arguments expected for gc.finish");
    geekConsole->finish();
    return 0;
}


static int callFun(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One arguments expected for gc.call(sFunName)");
    const char *funName = celx.safeGetString(1, AllErrors, "argument 1 to gc.listInteractive must be a string");
    geekConsole->execFunction(funName);
    return 0;
}

void LoadLuaGeekConsoleLibrary(lua_State* l)
{
    CelxLua celx(l);

    lua_pushstring(l, "gc");
    lua_newtable(l);
    celx.registerMethod("registerFunction", registerFunction);
    celx.registerMethod("reRegisterFunction", reRegisterFunction);
    celx.registerMethod("listInteractive", setListInteractive);
    celx.registerMethod("passwdInteractive", setPasswdInteractive);
    celx.registerMethod("celBodyInteractive", setCelBodyInteractive);
    celx.registerMethod("colorChooseInteractive", setColorChooseInteractive);
    celx.registerMethod("color2HexRGB", color2HexRGB);
    celx.registerMethod("finish", finishGConsole);
    celx.registerMethod("call", callFun);
    lua_settable(l, LUA_GLOBALSINDEX);
}

