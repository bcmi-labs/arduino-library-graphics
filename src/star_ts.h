#ifndef _STAR_TS_H
#define _STAR_TS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ft6x06.h"

// Comment this to disable touchscreen update via
// irq and use polling mode
#define ENABLE_STAR_TS_IRQ
//#define TS_MULTI_TOUCH_SUPPORTED    1

#define TS_MAX_NB_TOUCH                 ((uint32_t)FT6206_MAX_DETECTABLE_TOUCH)
#define TS_NO_IRQ_PENDING               ((uint8_t)0)
#define TS_IRQ_PENDING                  ((uint8_t)1)
#define TS_SWAP_NONE                    ((uint8_t)0x01)
#define TS_SWAP_X                       ((uint8_t)0x02)
#define TS_SWAP_Y                       ((uint8_t)0x04)
#define TS_SWAP_XY                      ((uint8_t)0x08)

typedef struct {
    uint8_t  touchDetected;                /*!< Total number of active touches detected at last scan */
    uint16_t touchX[TS_MAX_NB_TOUCH];      /*!< Touch X[0], X[1] coordinates on 12 bits */
    uint16_t touchY[TS_MAX_NB_TOUCH];      /*!< Touch Y[0], Y[1] coordinates on 12 bits */
#if (TS_MULTI_TOUCH_SUPPORTED == 1)
    uint8_t  touchWeight[TS_MAX_NB_TOUCH]; /*!< Touch_Weight[0], Touch_Weight[1] : weight property of touches */
    uint8_t  touchEventId[TS_MAX_NB_TOUCH];     /*!< Touch_EventId[0], Touch_EventId[1] : take value of type @ref TS_TouchEventTypeDef */
    uint8_t  touchArea[TS_MAX_NB_TOUCH];   /*!< Touch_Area[0], Touch_Area[1] : touch area of each touch */
    uint32_t gestureId; /*!< type of gesture detected : take value of type @ref TS_GestureIdTypeDef */
#endif  /* TS_MULTI_TOUCH_SUPPORTED == 1 */
} TS_StateTypeDef;

typedef enum {
    TS_OK                = 0x00, /*!< Touch Ok */
    TS_ERROR             = 0x01, /*!< Touch Error */
    TS_TIMEOUT           = 0x02, /*!< Touch Timeout */
    TS_DEVICE_NOT_FOUND  = 0x03  /*!< Touchscreen device not found */
} TS_StatusTypeDef;

typedef enum {
    GEST_ID_NO_GESTURE = 0x00, /*!< Gesture not defined / recognized */
    GEST_ID_MOVE_UP    = 0x01, /*!< Gesture Move Up */
    GEST_ID_MOVE_RIGHT = 0x02, /*!< Gesture Move Right */
    GEST_ID_MOVE_DOWN  = 0x03, /*!< Gesture Move Down */
    GEST_ID_MOVE_LEFT  = 0x04, /*!< Gesture Move Left */
    GEST_ID_ZOOM_IN    = 0x05, /*!< Gesture Zoom In */
    GEST_ID_ZOOM_OUT   = 0x06, /*!< Gesture Zoom Out */
    GEST_ID_NB_MAX     = 0x07 /*!< max number of gesture id */
} TS_GestureIdTypeDef;

typedef enum {
    TOUCH_EVENT_NO_EVT        = 0x00, /*!< Touch Event : undetermined */
    TOUCH_EVENT_PRESS_DOWN    = 0x01, /*!< Touch Event Press Down */
    TOUCH_EVENT_LIFT_UP       = 0x02, /*!< Touch Event Lift Up */
    TOUCH_EVENT_CONTACT       = 0x03, /*!< Touch Event Contact */
    TOUCH_EVENT_NB_MAX        = 0x04  /*!< max number of touch events kind */
} TS_TouchEventTypeDef;

//extern char * ts_event_string_tab[TOUCH_EVENT_NB_MAX];
//extern char * ts_gesture_id_string_tab[GEST_ID_NB_MAX];

uint8_t STAR_TS_Init(uint16_t ts_SizeX, uint16_t ts_SizeY);
uint8_t STAR_TS_GetState(TS_StateTypeDef *TS_State);
#if (TS_MULTI_TOUCH_SUPPORTED == 1)
uint8_t STAR_TS_Get_GestureId(TS_StateTypeDef *TS_State);
uint8_t STAR_TS_ResetTouchData(TS_StateTypeDef *TS_State);
#endif /* TS_MULTI_TOUCH_SUPPORTED == 1 */


#ifdef __cplusplus
}
#endif

#endif /* _STAR_TS_H */
