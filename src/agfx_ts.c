#include <stdint.h>
#include "agfx_ll.h"
#include "agfx_ts.h"
#include "agfx_colors.h"
#include "star_ts.h"

static agfx_ts_cal_t cal = { 1000, 0, 1000, 0 };

static TS_StateTypeDef  tsState = { 0 };
static int16_t aPhysX[2], aPhysY[2], aLogX[2], aLogY[2];

static void WaitPress(const uint8_t p)
{
    uint16_t start = 0;
    uint8_t status = TS_OK;
    uint32_t exit1 = 0;
    uint32_t exit2 = 0;

    do {
        exit2 = 0;
        status = STAR_TS_GetState(&tsState);
        if (status == TS_OK) {
            if ( ((p == 0) && (tsState.touchDetected == 0)) ||
                    ((p == 1) && ((tsState.touchDetected == 1) || (tsState.touchDetected == 2))) )
            {
                start = HAL_GetTick();
                do {
                    status = STAR_TS_GetState(&tsState);
                    if (status == TS_OK) {
                        if (((p == 0) && ((tsState.touchDetected == 1) || (tsState.touchDetected == 2))) ||
                                ((p == 1) && ((tsState.touchDetected == 0))))
                        {
                            exit2 = 1; /* exit request from second level while loop */
                        }
                        else if ((HAL_GetTick() - 100) > start) {
                            exit2 = 1;
                            exit1  = 1;
                        }
                        HAL_Delay(10);
                    }
                }
                while (!exit2);
            }
        }
        if (!exit1)
            HAL_Delay(10);
    }
    while (!exit2);
}

static void Aquire(int16_t LogX, int16_t LogY,
        int16_t * pPhysX, int16_t * pPhysY)
{
    AGFX_LL_FillCircle(LogX, LogY, 15, AGFX_QUAD_ALL, AGFX_RED);
    AGFX_LL_FillCircle(LogX, LogY, 12, AGFX_QUAD_ALL, AGFX_WHITE);

    WaitPress(1);

    *pPhysX = tsState.touchX[0];
    *pPhysY = tsState.touchY[0];

    WaitPress(0);

    AGFX_LL_FillCircle(LogX, LogY, 15, AGFX_QUAD_ALL, AGFX_GREEN);
}

void AGFX_TS_Calibrate(const uint16_t width, const uint16_t height)
{
    uint8_t i;

    AGFX_LL_Clear(AGFX_WHITE);

    /* Get touch points for SW calibration processing */
    aLogX[0] = 40;
    aLogY[0] = 40;
    aLogX[1] = width - 40;
    aLogY[1] = height - 40;

    for (i = 0; i < 2; i++)
        Aquire(aLogX[i], aLogY[i], &aPhysX[i], &aPhysY[i]);

    /* Compute calibration coefficients */
    cal.a1 = (1000 * (aLogX[1] - aLogX[0])) / (aPhysX[1] - aPhysX[0]);
    cal.b1 = (1000 * aLogX[0]) - cal.a1 * aPhysX[0];

    cal.a2 = (1000 * (aLogY[1] - aLogY[0])) / (aPhysY[1] - aPhysY[0]);
    cal.b2 = (1000 * aLogY[0]) - cal.a2 * aPhysY[0];
}

uint16_t AGFX_TS_FixX(const uint16_t x)
{
    return ((cal.a1 * x) + cal.b1) / 1000;
}

uint16_t AGFX_TS_FixY(const uint16_t y)
{
    return ((cal.a2 * y) + cal.b2) / 1000;
}

void AGFX_TS_GetCalData(agfx_ts_cal_t * const calData)
{
    memcpy(calData, &cal, sizeof(agfx_ts_cal_t));
}

void AGFX_TS_SetCalData(const agfx_ts_cal_t * const calData)
{
    memcpy(&cal, calData, sizeof(agfx_ts_cal_t));
}

uint8_t AGFX_TS_GetXY(uint16_t * const tx, uint16_t * const ty)
{
    static TS_StateTypeDef state;
    static uint16_t xOld=0, yOld=0;
    uint16_t x, y;

    if (STAR_TS_GetState(&state) == TS_OK) {
        if (state.touchDetected) {
            x = AGFX_TS_FixX(state.touchX[0]);
            y = AGFX_TS_FixY(state.touchY[0]);
            if (x!=xOld || y!=yOld) {
                xOld = x;
                yOld = y;
                *tx = x;
                *ty = y;
                return 1;
            }
        }
    }

    return 0;
}

