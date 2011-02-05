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

    void shiftX(char c) {
        xoffset += font->getAdvance(c);
    }
};

#endif // _GCOVERLAY_H_
