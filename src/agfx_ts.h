#ifndef _AGFX_TS_H
#define _AGFX_TS_H

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct {
    int16_t  a1;
    int16_t  b1;
    int16_t  a2;
    int16_t  b2;
} agfx_ts_cal_t;

extern void AGFX_TS_Calibrate(const uint16_t width, const uint16_t height);
extern uint16_t AGFX_TS_FixX(const uint16_t x);
extern uint16_t AGFX_TS_FixY(const uint16_t y);
extern void AGFX_TS_GetCalData(agfx_ts_cal_t * const calData);
extern void AGFX_TS_SetCalData(const agfx_ts_cal_t * const calData);
extern uint8_t AGFX_TS_GetXY(uint16_t * const tx, uint16_t * const ty);

#ifdef __cplusplus
}
#endif

#endif _AGFX_TS_H

