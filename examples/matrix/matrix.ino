#include <agfx.h>
#include <agfx_ts.h>

AGFX agfx = AGFX();

#define WINDOWX         690
#define WINDOWY         430
#define N_DOTS          1024 // Number of dots
#define SCALE           8192
#define INCREMENT       512
#define SPEED           10
#define PI2             6.283185307179586476925286766559f
#define RED_COLORS      0x100
#define GREEN_COLORS    0x100
#define BLUE_COLORS     0x100
#define REDSHIFT        16
#define GREENSHIFT      8
#define BLUESHIFT       0

static char buff[512];
static int16_t sine[SCALE];
static int16_t cosi[SCALE];
static int16_t speedX = 0;
static int16_t speedY = 0;
static int16_t speedZ = 0;

void initialize()
{
    uint16_t i;
    for (i = 0; i < SCALE; i++) {
        sine[i] = (int)(sin(PI2 * i / SCALE) * SCALE);
        cosi[i] = (int)(cos(PI2 * i / SCALE) * SCALE);
    }
}

uint16_t fastsqrt(uint32_t n)
{
    uint16_t c = 0x8000;
    uint16_t g = 0x8000;

    for(;;) {
        if(g * g > n)
            g ^= c;
        c >>= 1;
        if(c == 0)
            return g;
        g |= c;
    }
}

void matrix(int16_t (&xyz)[3][N_DOTS], uint8_t (&rgb)[3][N_DOTS])
{
    static uint32_t t = 0;
    uint16_t i;
    int16_t x = -SCALE;
    int16_t y = -SCALE;
    uint16_t d;
    uint16_t s;

    for (i = 0; i < N_DOTS; i++) {
        xyz[0][i] = x;
        xyz[1][i] = y;

        d = fastsqrt(x * x + y * y);
        s = sine[(t * 30) % SCALE] + SCALE;

        xyz[2][i] = sine[(d + s) % SCALE] *
            sine[(t * 10) % SCALE] / SCALE / 2;

        rgb[0][i] = (cosi[xyz[2][i] + SCALE / 2] + SCALE) *
            (RED_COLORS - 1) / SCALE / 2;

        rgb[1][i] = (cosi[(xyz[2][i] + SCALE / 2 + 2 * SCALE / 3) % SCALE] + SCALE) *
            (GREEN_COLORS - 1) / SCALE / 2;

        rgb[2][i] = (cosi[(xyz[2][i] + SCALE / 2 + SCALE / 3) % SCALE] + SCALE) *
            (BLUE_COLORS - 1) / SCALE / 2;

        x += INCREMENT;
        if (x >= SCALE) {
            x = -SCALE;
            y += INCREMENT;
        }
    }
    t++;
}

void rotate(int16_t (&xyz)[3][N_DOTS], uint8_t (&rgb)[3][N_DOTS], uint16_t angleX,
    uint16_t angleY, uint16_t angleZ)
{
    uint16_t i;
    int16_t tmpX;
    int16_t tmpY;
    int16_t sinx = sine[angleX];
    int16_t cosx = cosi[angleX];
    int16_t siny = sine[angleY];
    int16_t cosy = cosi[angleY];
    int16_t sinz = sine[angleZ];
    int16_t cosz = cosi[angleZ];

    for (i=0; i<N_DOTS; i++) {
        tmpX      = (xyz[0][i] * cosx - xyz[2][i] * sinx) / SCALE;
        xyz[2][i] = (xyz[0][i] * sinx + xyz[2][i] * cosx) / SCALE;
        xyz[0][i] = tmpX;

        tmpY      = (xyz[1][i] * cosy - xyz[2][i] * siny) / SCALE;
        xyz[2][i] = (xyz[1][i] * siny + xyz[2][i] * cosy) / SCALE;
        xyz[1][i] = tmpY;

        tmpX      = (xyz[0][i] * cosz - xyz[1][i] * sinz) / SCALE;
        xyz[1][i] = (xyz[0][i] * sinz + xyz[1][i] * cosz) / SCALE;
        xyz[0][i] = tmpX;
    }
}

void draw(int16_t (&xyz)[3][N_DOTS], uint8_t (&rgb)[3][N_DOTS])
{
    static uint16_t oldProjX[N_DOTS] = {0};
    static uint16_t oldProjY[N_DOTS] = {0};
    static uint8_t oldDotSize[N_DOTS] = {0};
    uint16_t i;
    uint16_t projX;
    uint16_t projY;
    uint16_t projZ;
    uint16_t dotSize;
    uint32_t color;

    for (i=0; i<N_DOTS; i++) {
        projZ   = SCALE - (xyz[2][i] + SCALE) / 4;
        projX   = WINDOWX / 2 + (xyz[0][i] * projZ / SCALE) / 25;
        projY   = WINDOWY / 2 + (xyz[1][i] * projZ / SCALE) / 25;
        dotSize = 3 - (xyz[2][i] + SCALE) * 2 / SCALE;

        AGFX_LL_DrawCircle(oldProjX[i], oldProjY[i], oldDotSize[i], AGFX_QUAD_ALL, AGFX_BLACK);

        if (projX > dotSize &&
                projY > dotSize &&
                projX < WINDOWX - dotSize &&
                projY < WINDOWY - dotSize) {

            color = 0xFF000000 + (rgb[0][i]<<REDSHIFT) + (rgb[1][i]<<GREENSHIFT) +
                    (rgb[2][i] << BLUESHIFT);
            AGFX_LL_DrawCircle(projX, projY, dotSize, AGFX_QUAD_ALL, color);

            oldProjX[i] = projX;
            oldProjY[i] = projY;
            oldDotSize[i] = dotSize;
        }
    }
}

void update_info(void)
{
    static char str[64];

    AGFX_LL_DrawText(10, 468, AGFX_LIGHTBLUE, str);
    sprintf(str, "sx=%+3i sy=%+3i sz=%+3i", speedX, speedY, speedZ);
    AGFX_LL_DrawText(10, 468, AGFX_WHITE, str);
}

uint8_t get_input(void)
{
    uint16_t tx, ty;

    if (AGFX_TS_GetXY(&tx, &ty) && tx>700) {
        if (ty>0 && ty<70)
            return 1;
        else if (ty>70 && ty<140)
            return 2;
        else if (ty>140 && ty<210)
            return 3;
        else if (ty>210 && ty<280)
            return 4;
        else if (ty>280 && ty<350)
            return 5;
        else if (ty>350 && ty<420)
            return 6;
        else if (ty>420)
            return 7;
    }

    return 0;
}

void setup_win(void)
{
    AGFX_LL_Clear(AGFX_BLACK);
    AGFX_LL_FillRect(700, 0, 100, 70, AGFX_LIGHTBLUE);
    AGFX_LL_DrawRect(700, 0, 100, 70, AGFX_WHITE);
    AGFX_LL_DrawText(710, 50, AGFX_WHITE, "X +");
    AGFX_LL_FillRect(700, 70, 100, 70, AGFX_LIGHTBLUE);
    AGFX_LL_DrawRect(700, 70, 100, 70, AGFX_WHITE);
    AGFX_LL_DrawText(710, 120, AGFX_WHITE, "X -");
    AGFX_LL_FillRect(700, 140, 100, 70, AGFX_LIGHTBLUE);
    AGFX_LL_DrawRect(700, 140, 100, 70, AGFX_WHITE);
    AGFX_LL_DrawText(710, 190, AGFX_WHITE, "Y +");
    AGFX_LL_FillRect(700, 210, 100, 70, AGFX_LIGHTBLUE);
    AGFX_LL_DrawRect(700, 210, 100, 70, AGFX_WHITE);
    AGFX_LL_DrawText(710, 260, AGFX_WHITE, "Y -");
    AGFX_LL_FillRect(700, 280, 100, 70, AGFX_LIGHTBLUE);
    AGFX_LL_DrawRect(700, 280, 100, 70, AGFX_WHITE);
    AGFX_LL_DrawText(710, 330, AGFX_WHITE, "Z +");
    AGFX_LL_FillRect(700, 350, 100, 70, AGFX_LIGHTBLUE);
    AGFX_LL_DrawRect(700, 350, 100, 70, AGFX_WHITE);
    AGFX_LL_DrawText(710, 400, AGFX_WHITE, "Z -");
    AGFX_LL_FillRect(700, 420, 100, 60, AGFX_LIGHTBLUE);
    AGFX_LL_DrawRect(700, 420, 100, 60, AGFX_WHITE);
    AGFX_LL_DrawText(710, 470, AGFX_WHITE, "RES");
    AGFX_LL_FillRect(0, 434, 700, 45, AGFX_LIGHTBLUE);
    AGFX_LL_DrawRect(0, 434, 700, 45, AGFX_LIGHTBLUE);
}

#include "FreeSerifItalic24pt7b.h"
void setup()
{
    agfx.begin();

    AGFX_LL_SetFontFace(&FreeSerifItalic24pt7b);
    AGFX_LL_SetFontMag(1);

    setup_win();
    initialize();
    update_info();
}

void loop()
{
    static int16_t angleX = 0;
    static int16_t angleY = 0;
    static int16_t angleZ = 0;
    static int16_t xyz[3][N_DOTS];
    static uint8_t rgb[3][N_DOTS];
    uint8_t input;

    matrix(xyz, rgb);
    rotate(xyz, rgb, angleX, angleY, angleZ);
    draw(xyz, rgb);

    input = get_input();
    switch (input) {
        case 1: speedX += SPEED; break;
        case 2: speedX -= SPEED; break;
        case 3: speedY += SPEED; break;
        case 4: speedY -= SPEED; break;
        case 5: speedZ += SPEED; break;
        case 6: speedZ -= SPEED; break;
        case 7:
            speedX = 0;
            speedY = 0;
            speedZ = 0;
            angleX = 0;
            angleY = 0;
            angleZ = 0;
            break;
        case 0:
        default:
            break;
    }
    if (input)
        update_info();

    angleX += speedX;
    angleY += speedY;
    angleZ += speedZ;

    if (angleX >= SCALE)
        angleX -= SCALE;
    else if (angleX < 0)
        angleX += SCALE;

    if (angleY >= SCALE)
        angleY -= SCALE;
    else if (angleY < 0)
        angleY += SCALE;

    if (angleZ >= SCALE)
        angleZ -= SCALE;
    else if (angleZ < 0)
        angleZ += SCALE;
}

