#ifndef _AGFX_LL_H
#define _AGFX_LL_H

#ifdef __cplusplus
 extern "C" {
#endif

#define POLY_MAX_PTS            128
#define AGFX_QUAD_TOPRIGHT      0x01
#define AGFX_QUAD_BOTRIGHT      0x02
#define AGFX_QUAD_BOTLEFT       0x04
#define AGFX_QUAD_TOPLEFT       0x08
#define AGFX_QUAD_ALL           (AGFX_QUAD_TOPRIGHT | AGFX_QUAD_BOTRIGHT | AGFX_QUAD_BOTLEFT | AGFX_QUAD_TOPLEFT)

typedef struct {
    uint16_t x;
    uint16_t y;
} agfx_point_t;

typedef struct {
    uint16_t bmpOff;
    uint8_t w, h;
    uint8_t hShift;
    int8_t xOff, yOff;
} GFXglyph;

typedef struct {
    uint8_t *bmp;
    GFXglyph *glyph;
    uint8_t first, last;
    uint8_t vShift;
} GFXfont;

extern void AGFX_LL_Init(const uint16_t width, const uint16_t height);
extern void AGFX_LL_Clear(const uint32_t color);
extern void AGFX_LL_DrawPoint(uint16_t x0, uint16_t y0, uint32_t color);
extern void AGFX_LL_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color);
extern void AGFX_LL_DrawRect(const uint16_t x0, const uint16_t y0, const uint16_t width, const uint16_t height, const uint32_t color);
extern void AGFX_LL_FillRect(const uint16_t x0, const uint16_t y0, const uint16_t width, const uint16_t height, const uint32_t color);
extern void AGFX_LL_DrawPolygon(const agfx_point_t *pts, uint16_t nPts, const uint32_t color);
extern void AGFX_LL_FillPolygon(const agfx_point_t *pts, const uint16_t nPts, const uint32_t color);
extern void AGFX_LL_DrawCircle(const uint16_t x0, const uint16_t y0, const uint16_t r, const uint8_t quadMask, const uint32_t color);
extern void AGFX_LL_FillCircle(const uint16_t x0, const uint16_t y0, const uint16_t r, const uint8_t quadMask, const uint32_t color);
extern void AGFX_LL_DrawEllipseInRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint32_t color);
extern void AGFX_LL_FillEllipseInRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint32_t color);
extern uint16_t AGFX_LL_DrawChar(const uint16_t x0, const uint16_t y0, const uint32_t color, const uint8_t c);
extern uint16_t AGFX_LL_DrawText(const uint16_t x0, const uint16_t y0, const uint32_t color, const char *str);
extern void AGFX_LL_SetFontFace(const GFXfont * const font);
extern void AGFX_LL_SetFontMag(const uint8_t mag);
extern uint16_t AGFX_LL_GetFontHeight(void);

#ifdef __cplusplus
}
#endif

#endif // _AGFX_LL_H

