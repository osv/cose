#ifndef _GCOVERLAY_H_
#define _GCOVERLAY_H_

#include <celengine/overlay.h>

/* Extended overlay.

   "\n" will not shift y, use endl() instead.
 */
class GCOverlay : public Overlay
{
public:
    GCOverlay() {};
    const float getXoffset() const
        {return xoffset;}
    const float getYoffset() const
        {return yoffset;}
    void setXoffset(float x) {
        xoffset = x;
    };

    void drawImage(Texture *img){
        if (img)
        {
            glEnable(GL_TEXTURE_2D);
            img->bind();
            float w = img->getWidth();
            float h = img->getHeight();
            float y = 0;
            if (font)
                y = font->getHeight();

            glBegin(GL_QUADS);
            glTexCoord2f(0, 1);
            glVertex2f(xoffset, y - h);
            glTexCoord2f(1, 1);
            glVertex2f(xoffset + w, y - h);
            glTexCoord2f(1, 0);
            glVertex2f(xoffset + w, y);
            glTexCoord2f(0, 0);
            glVertex2f(xoffset, y);
            glEnd();

            useTexture = true;
            fontChanged = true;
            xoffset += w;
        }
    };

    void print(wchar_t c) {
        if (c != '\n')
            Overlay::print(c);
        else
            Overlay::print(' ');
    };

    void print(char c) {
        if (c != '\n')
            Overlay::print(c);
        else
            Overlay::print(' ');
    };

    void endl() {
        if (textBlock > 0)
        {
            glPopMatrix();
            glTranslatef(0.0f, (float) -(1 + font->getHeight()), 0.0f);
            xoffset = 0.0f;
            glPushMatrix();
        }
    }

    void endl(float h) {
        if (textBlock > 0)
        {
            glPopMatrix();
            glTranslatef(0.0f, -h, 0.0f);
            xoffset = 0.0f;
            glPushMatrix();
        }
    }
};

#endif // _GCOVERLAY_H_
