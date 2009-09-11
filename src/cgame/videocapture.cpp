#include <agar/core.h>
#include <agar/gui.h>

#include "cgame.h"
#include "videocapture.h"
#include "ui.h"

#ifdef THEORA
#include <celestia/oggtheoracapture.h>
#endif

namespace UI
{
    static char filename[255];
    static int quality=6;
    static float fps=5;
    int aspectRation=0;
    int w, h, margin;

    // arg 1 - AG_Window
    void initMovieCapturer(AG_Event *event)
    {
        AG_Window *win = (AG_Window *)AG_PTR(1);
#ifdef THEORA
        MovieCapture* movieCapture = new OggTheoraCapture();

        /* Get the dimensions of the current viewport */
        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        switch (aspectRation)
        {
        case 0:
            movieCapture->setAspectRatio(1, 1);
            break;
        case 1:
            movieCapture->setAspectRatio(4, 3);
            break;
        case 2:
            movieCapture->setAspectRatio(16, 9);
            break;
        default:
            movieCapture->setAspectRatio(viewport[2], viewport[3]);
            break;
        }
        movieCapture->setQuality(quality);
        bool success = movieCapture->start(filename, w, h-margin, fps);
        if (success)
        {
            celAppCore->initMovieCapture(movieCapture);
            AG_WindowHide(win);
        }
        else
            AG_TextError("Movie capture fail");
#endif
    }

    void showVidCaptureDlg(AG_Event *event)
    {
        AG_Window * win = AG_WindowNewNamed(0, "cel movie capture");
        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        w = viewport[2];
        h = viewport[3];
        margin = 2*(AGWIDGET(agAppMenuWin)->h);
        if (win==NULL)
            return;
    
        AG_WindowSetCaption(win, _("Movie capture"));
        AG_Textbox *tbFilename;
        AG_Box *hBox = AG_BoxNewHoriz(win, AG_BOX_HFILL);
        {
//      AG_BoxSetPadding(hBox, 0);
            tbFilename = AG_TextboxNew(hBox, 0, _("File Name: "));
            AG_TextboxBindUTF8(tbFilename, filename, sizeof(filename));
            AG_TextboxSizeHint(tbFilename, "XXXXXXXXXXXXXXXXXXXXXXXXXX");
            AG_TextboxPrintf(tbFilename, "movie01.ogg");

            AG_ButtonNewFn(hBox, 0, "...",
                           showVidCaptureDlg, NULL);
        }
        AG_NumericalNewIntR(win, NULL, NULL, _("Quality: "), &quality, 
                            0, 10);
        AG_NumericalNewFltR(win, NULL, NULL, _("Frame Rate: "), &fps, 
                            0.01, 30);
        hBox = AG_BoxNewHoriz(win, AG_BOX_HFILL| AG_BOX_HOMOGENOUS);
        {
            AG_MSpinbutton *msb = AG_MSpinbuttonNew(hBox, 0, ",", _("Size: "));
            AG_BindInt(msb, "xvalue", &w);
            AG_BindInt(msb, "yvalue", &h);
            AG_NumericalNewIntR(hBox, NULL, NULL, _("Height margin: "), &margin,
                                0, 100);
        }
        hBox = AG_BoxNewHoriz(win, AG_BOX_HFILL| AG_BOX_HOMOGENOUS);
        {
            const char *radioItems[] = {
                "1:1",
                "4:3",
                "15:9",
                "Display",
                NULL
            };                              
            AG_Radio *r = AG_RadioNew(hBox, 0, radioItems);
            AG_BindInt(r, "value", &aspectRation);

            AG_ButtonNewFn(hBox, 0, "Ok",
                           initMovieCapturer, "%p", win);
        }
        AG_WidgetFocus(tbFilename);
        AG_WindowShow(win);
    }
}
