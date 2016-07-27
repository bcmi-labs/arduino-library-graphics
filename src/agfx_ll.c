#include "star_dsi.h"
#include "agfx_ll.h"

// TODO: use a common macro include
#define ABS(_a)              ( (_a) < 0 ? -(_a) : (_a) )
#define MIN(_a, _b)          ( (_a) > (_b) ? (_b) : (_a) )
#define MAX(_a, _b)          ( (_a) > (_b) ? (_a) : (_b) )

static uint16_t panelWidth;
static uint16_t panelHeight;
static uint16_t xPoly[POLY_MAX_PTS];
static const GFXfont *gfxFont = NULL;
static uint8_t fontMag = 1;

void AGFX_LL_Init(const uint16_t width, const uint16_t height)
{
    panelWidth = STAR_DSI_PanelWidth();
    panelHeight = STAR_DSI_PanelHeight();
}

void AGFX_LL_Clear(const uint32_t color)
{
    STAR_DSI_FillRectDma(0, 0, panelWidth, panelHeight, color);
}

void AGFX_LL_DrawPoint(uint16_t x0, uint16_t y0, uint32_t color)
{
    STAR_DSI_DrawPoint(x0, y0, color);
}

void AGFX_LL_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        uint32_t color)
{
    int8_t stepx, stepy;
    int16_t dy = y1 - y0;
    int16_t dx = x1 - x0;
    int16_t fract;

    if (!dy && !dx)
        return;
    if (!dy) {
        STAR_DSI_FillRectDma(MIN(x0, x1), y0, ABS(dx)+1, 1, color);
        return;
    }
    if (!dx) {
        STAR_DSI_FillRectDma(x0, MIN(y0, y1), 1, ABS(dy)+1, color);
        return;
    }

    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;
    dx <<= 1;

    STAR_DSI_DrawPoint(x0, y0, color);
    if (dx > dy) {
        fract = dy - (dx >> 1);
        while (x0 != x1) {
            if (fract >= 0) {
                y0 += stepy;
                fract -= dx;
            }
            x0 += stepx;
            fract += dy;
            STAR_DSI_DrawPoint(x0, y0, color);
        }
    }
    else {
        fract = dx - (dy >> 1);
        while (y0 != y1) {
            if (fract >= 0) {
                x0 += stepx;
                fract -= dy;
            }
            y0 += stepy;
            fract += dx;
            STAR_DSI_DrawPoint(x0, y0, color);
        }
    }
}

void AGFX_LL_DrawRect(const uint16_t x0, const uint16_t y0, const uint16_t width,
        const uint16_t height, const uint32_t color)
{
    STAR_DSI_FillRectDma(x0, y0, width, 1, color);
    if (height)
        STAR_DSI_FillRectDma(x0, y0+height-1, width, 1, color);
    STAR_DSI_FillRectDma(x0, y0, 1, height, color);
    if (width)
        STAR_DSI_FillRectDma(x0+width-1, y0, 1, height, color);
}

void AGFX_LL_FillRect(const uint16_t x0, const uint16_t y0, const uint16_t width,
        const uint16_t height, const uint32_t color)
{
    STAR_DSI_FillRectDma(x0, y0, width, height, color);
}

void AGFX_LL_DrawPolygon(const agfx_point_t *pts, uint16_t nPts, const uint32_t color)
{
    agfx_point_t *first = pts;

    if (!nPts)
        return;
    if (nPts==1) {
        STAR_DSI_DrawPoint(pts->x, pts->y, color);
        return;
    }

    while (--nPts) {
        AGFX_LL_DrawLine(pts->x, pts->y, (pts+1)->x, (pts+1)->y, color);
        pts++;
    }
    AGFX_LL_DrawLine(pts->x, pts->y, first->x, first->y, color);
}

void AGFX_LL_FillPolygon(const agfx_point_t *pts, const uint16_t nPts, uint32_t color)
{
    uint16_t n, x, y, i, j, swap;
    uint16_t yMin = 0, yMax = panelHeight;
    uint16_t xMin = 0, xMax = panelWidth;

    if (!nPts)
        return;
    if (nPts==1) {
        STAR_DSI_DrawPoint(pts->x, pts->y, color);
        return;
    }

    // Find min/max y
    for (i=0; i<nPts; i++) {
        yMin = MIN(yMin, pts[i].y);
        yMax = MAX(yMax, pts[i].y);
        xMin = MIN(xMin, pts[i].x);
        xMax = MAX(xMax, pts[i].x);
    }

    for (y=yMin; y<yMax; y++) {
        // Node list
        n = 0;
        j = nPts - 1;
        for (i=0; i<nPts; i++) {
            if (pts[i].y<y && pts[j].y>=y || pts[j].y<y && pts[i].y>=y) {
                xPoly[n++] = pts[i].x + (y-pts[i].y)/(float)(pts[j].y-pts[i].y)
                        *(float)(pts[j].x-pts[i].x);
            }
            j = i;
        }

        // Sort n
        i = 0;
        while (i < n-1) {
            if (xPoly[i] > xPoly[i+1]) {
                swap = xPoly[i];
                xPoly[i] = xPoly[i+1];
                xPoly[i+1] = swap;
                if (i)
                    i--;
            }
            else
                i++;
        }

        // Fill
        for (i=0; i<n; i+=2) {
            if (xPoly[i] >= xMax)
                break;
            if (xPoly[i+1] > xMin) {
                if (xPoly[i] < xMin)
                    xPoly[i]=xMin;
                if (xPoly[i+1] > xMax)
                    xPoly[i+1] = xMax;
                AGFX_LL_DrawLine(xPoly[i], y, xPoly[i+1], y, color);
            }
        }
    }
}

void AGFX_LL_DrawCircle(const uint16_t x0, const uint16_t y0, const uint16_t r,
        const uint8_t quadMask, const uint32_t color)
{
    int16_t x, y, dec2;

    if (!r)
        return;
    if (r==1) {
        STAR_DSI_DrawPoint(x0, y0, color);
        return;
    }

    x = r;
    y = 0;
    dec2 = 1 - x;
    while (y <= x) {
        if (quadMask & AGFX_QUAD_TOPRIGHT) {
            STAR_DSI_DrawPoint(x0+x, y0-y, color);
            STAR_DSI_DrawPoint(x0+y, y0-x, color);
        }
        if (quadMask & AGFX_QUAD_BOTRIGHT) {
            STAR_DSI_DrawPoint(x0+x, y0+y, color);
            STAR_DSI_DrawPoint(x0+y, y0+x, color);
        }
        if (quadMask & AGFX_QUAD_BOTLEFT) {
            STAR_DSI_DrawPoint(x0-y, y0+x, color);
            STAR_DSI_DrawPoint(x0-x, y0+y, color);
        }
        if (quadMask & AGFX_QUAD_TOPLEFT) {
            STAR_DSI_DrawPoint(x0-x, y0-y, color);
            STAR_DSI_DrawPoint(x0-y, y0-x, color);
        }

        y++;
        if (dec2 <= 0)
            dec2 += 2 * y + 1;
        else {
            x--;
            dec2 += 2 * (y - x) + 1;
        }
    }
}

void AGFX_LL_FillCircle(const uint16_t x0, const uint16_t y0, const uint16_t r,
        const uint8_t quadMask, const uint32_t color)
{
    int16_t x, y, dec2;

    if (!r)
        return;
    if (r==1) {
        STAR_DSI_DrawPoint(x0, y0, color);
        return;
    }

    x = r;
    y = 0;
    dec2 = 1 - x;
    while (y <= x) {
        if (quadMask & AGFX_QUAD_TOPRIGHT) {
            AGFX_LL_DrawLine(x0, y0-y, x0+x, y0-y, color);
            AGFX_LL_DrawLine(x0, y0-x, x0+y, y0-x, color);
        }
        if (quadMask & AGFX_QUAD_BOTRIGHT) {
            AGFX_LL_DrawLine(x0, y0+y, x0+x, y0+y, color);
            AGFX_LL_DrawLine(x0, y0+x, x0+y, y0+x, color);
        }
        if (quadMask & AGFX_QUAD_BOTLEFT) {
            AGFX_LL_DrawLine(x0-y, y0+x, x0, y0+x, color);
            AGFX_LL_DrawLine(x0-x, y0+y, x0, y0+y, color);
        }
        if (quadMask & AGFX_QUAD_TOPLEFT) {
            AGFX_LL_DrawLine(x0-x, y0-y, x0, y0-y, color);
            AGFX_LL_DrawLine(x0-y, y0-x, x0, y0-x, color);
        }

        y++;
        if (dec2 <= 0)
            dec2 += 2 * y + 1;
        else {
            x--;
            dec2 += 2 * (y - x) + 1;
        }
    }
}

void AGFX_LL_DrawEllipseInRect(int16_t x0, int16_t y0, int16_t x1,
        int16_t y1, const uint32_t color)
{
    int32_t a = ABS(x1-x0);
    int32_t b = ABS(y1-y0);
    int32_t b1 = b & 1;
    int32_t dx = 4*(1-a)*b*b;
    int32_t dy = 4*(b1+1)*a*a;
    int32_t err = dx+dy+b1*a*a;
    int32_t e2;

    if (x0 > x1) {
        x0 = x1;
        x1 += a;
    }
    if (y0 > y1)
        y0 = y1;

    y0 += (b+1)/2;
    y1 = y0-b1;
    a *= 8*a;
    b1 = 8*b*b;

    do {
        STAR_DSI_DrawPoint(x1, y0, color);
        STAR_DSI_DrawPoint(x0, y0, color);
        STAR_DSI_DrawPoint(x0, y1, color);
        STAR_DSI_DrawPoint(x1, y1, color);
        e2 = 2*err;
        if (e2 <= dy) {
            y0++;
            y1--;
            dy += a;
            err += dy;
        }
        if (e2 >= dx || 2*err > dy) {
            x0++;
            x1--;
            dx += b1;
            err += dx;
        }
    } while (x0 <= x1);

    while (y0-y1 < b) {
        STAR_DSI_DrawPoint(x0-1, y0, color);
        STAR_DSI_DrawPoint(x1+1, y0++, color);
        STAR_DSI_DrawPoint(x0-1, y1, color);
        STAR_DSI_DrawPoint(x1+1, y1--, color);
    }
}

void AGFX_LL_FillEllipseInRect(int16_t x0, int16_t y0, int16_t x1,
        int16_t y1, const uint32_t color)
{
    int32_t a = ABS(x1-x0);
    int32_t b = ABS(y1-y0);
    int32_t b1 = b & 1;
    int32_t dx = 4*(1-a)*b*b;
    int32_t dy = 4*(b1+1)*a*a;
    int32_t err = dx+dy+b1*a*a;
    int32_t e2;

    if (x0 > x1) {
        x0 = x1;
        x1 += a;
    }
    if (y0 > y1)
        y0 = y1;

    y0 += (b+1)/2;
    y1 = y0-b1;
    a *= 8*a;
    b1 = 8*b*b;

    do {
        AGFX_LL_DrawLine(x0, y0, x1, y0, color);
        AGFX_LL_DrawLine(x0, y1, x1, y1, color);
        e2 = 2*err;
        if (e2 <= dy) {
            y0++;
            y1--;
            dy += a;
            err += dy;
        }
        if (e2 >= dx || 2*err > dy) {
            x0++;
            x1--;
            dx += b1;
            err += dx;
        }
    } while (x0 <= x1);

    while (y0-y1 < b) {
        STAR_DSI_DrawPoint(x0-1, y0, color);
        STAR_DSI_DrawPoint(x1+1, y0++, color);
        STAR_DSI_DrawPoint(x0-1, y1, color);
        STAR_DSI_DrawPoint(x1+1, y1--, color);
    }
}

uint16_t AGFX_LL_DrawChar(const uint16_t x0, const uint16_t y0,
        const uint32_t color, const uint8_t c)
{
    if (!gfxFont)
        return 0;

    if (c>=gfxFont->first && c<=gfxFont->last) {
        const GFXglyph * const glyph  = &gfxFont->glyph[c-gfxFont->first];
        if (glyph->w>0 && glyph->h>0) {
            uint16_t off = glyph->bmpOff;
            uint8_t w, h, bits, bit = 0;

            for (h=0; h<glyph->h; h++) {
                for (w=0; w<glyph->w; w++) {
                    if (!(bit++ & 7))
                        bits = gfxFont->bmp[off++];
                    if (bits & 0x80) {
                        if (fontMag == 1)
                            STAR_DSI_DrawPoint(x0+glyph->xOff+w,
                                    y0+glyph->yOff+h, color);
                        else
                            STAR_DSI_FillRectDma(x0+(glyph->xOff+w)*fontMag,
                                    y0+(glyph->yOff+h)*fontMag, fontMag,
                                    fontMag, color);
                    }
                    bits <<= 1;
                }
            }
        }
        return glyph->hShift * fontMag;
    }

    return 0;
}

uint16_t AGFX_LL_DrawText(const uint16_t x0, const uint16_t y0,
        const uint32_t color, const char *str)
{
    uint16_t width = 0;

    while (*str)
        width += AGFX_LL_DrawChar(x0+width, y0, color, *str++);

    return width;
}

void AGFX_LL_SetFontFace(const GFXfont * const font)
{
    gfxFont = font;
}

void AGFX_LL_SetFontMag(const uint8_t mag)
{
    fontMag = mag;
}

uint16_t AGFX_LL_GetFontHeight(void)
{
    return gfxFont ? fontMag * gfxFont->vShift : 0;
}

