#include "star_dsi.h"
#include "stm32f469xx.h"

// TODO
//#define ENABLE_IRQ


// alfran: DMA2D_Color_Mode DMA2D Color Mode

#define DMA2D_ARGB8888                       ((uint32_t)0x00000000)             /*!< ARGB8888 DMA2D color mode */
#define DMA2D_RGB888                         ((uint32_t)0x00000001)             /*!< RGB888 DMA2D color mode   */
#define DMA2D_RGB565                         ((uint32_t)0x00000002)             /*!< RGB565 DMA2D color mode   */
#define DMA2D_ARGB1555                       ((uint32_t)0x00000003)             /*!< ARGB1555 DMA2D color mode */
#define DMA2D_ARGB4444                       ((uint32_t)0x00000004)             /*!< ARGB4444 DMA2D color mode */


#define BG_LAYER_ADDR                   ((uint32_t)0xC0000000)
//#define NUM_LAYERS                      ((uint32_t)2)
#define BG_LAYER_IDX                    ((uint32_t)0)
#define FG_LAYER_IDX                    ((uint32_t)1)
#define LCD_OTM8009A_ID                 ((uint32_t)0)
#define FB_ADDR(_x, _y) \
    (hltdc.LayerCfg[currLayer].FBStartAdress + 4*(panelWidth*(_y)+(_x)))

// #ifdef BOARD_DISCO469
//     #define RES_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOH_CLK_ENABLE()
//     #define GPIO_RES_PORT               HAL_GPIOH
//     #define GPIO_RES_PIN                GPIO_PIN_7
// #else
    #define RES_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOK_CLK_ENABLE()
    #define GPIO_RES_PORT               HAL_GPIOK
    #define GPIO_RES_PIN                GPIO_PIN_7
//#endif

static uint32_t  currLayer = BG_LAYER_IDX;
static DSI_VidCfgTypeDef hdsivid;
static DMA2D_HandleTypeDef hdma2d;
static LTDC_HandleTypeDef hltdc;
static DSI_HandleTypeDef hdsi;
static uint32_t panelWidth = OTM8009A_800X480_WIDTH;
static uint32_t panelHeight = OTM8009A_800X480_HEIGHT;

static void LcdResetOnce(void)
{
    GPIO_InitTypeDef  gpio_init_structure;

    RES_GPIO_CLK_ENABLE();

    gpio_init_structure.Pin   = GPIO_RES_PIN;
    gpio_init_structure.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio_init_structure.Pull  = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIO_RES_PORT, &gpio_init_structure);

    // Reset display
    HAL_GPIO_WritePin(GPIO_RES_PORT, GPIO_RES_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(GPIO_RES_PORT, GPIO_RES_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
}

static void LowLevelInit(void)
{
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_LTDC_FORCE_RESET();
    __HAL_RCC_LTDC_RELEASE_RESET();

    __HAL_RCC_DMA2D_CLK_ENABLE();
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();

    __HAL_RCC_DSI_CLK_ENABLE();
    __HAL_RCC_DSI_FORCE_RESET();
    __HAL_RCC_DSI_RELEASE_RESET();

#ifdef ENABLE_IRQ
    //* @brief NVIC configuration for LTDC interrupt that is now enabled
    HAL_NVIC_SetPriority(LTDC_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(LTDC_IRQn);

    //* @brief NVIC configuration for DMA2D interrupt that is now enabled
    HAL_NVIC_SetPriority(DMA2D_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(DMA2D_IRQn);

    //* @brief NVIC configuration for DSI interrupt that is now enabled
    HAL_NVIC_SetPriority(DSI_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(DSI_IRQn);
#endif
}

#ifdef ENABLE_IRQ
void STAR_DSI_DMA2D_IRQHandler(void)
{
    HAL_DMA2D_IRQHandler(&hdma2d);
}

void STAR_DSI_DSI_IRQHandler(void)
{
    HAL_DSI_IRQHandler(&(hdsi));
}

void STAR_DSI_LTDC_IRQHandler(void)
{
    HAL_LTDC_IRQHandler(&(hltdc));
}

void STAR_DSI_LTDC_ER_IRQHandler(void)
{
    HAL_LTDC_IRQHandler(&(hltdc));
}
#endif

static void LayerInit(uint16_t layerIdx, uint32_t FB_Address)
{
    LTDC_LayerCfgTypeDef  Layercfg;

    Layercfg.WindowX0 = 0;
    Layercfg.WindowX1 = panelWidth;
    Layercfg.WindowY0 = 0;
    Layercfg.WindowY1 = panelHeight;
    Layercfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
    Layercfg.FBStartAdress = FB_Address;
    Layercfg.Alpha = 255;
    Layercfg.Alpha0 = 0;
    Layercfg.Backcolor.Blue = 255;
    Layercfg.Backcolor.Green = 255;
    Layercfg.Backcolor.Red = 255;
    Layercfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    Layercfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    Layercfg.ImageWidth = panelWidth;
    Layercfg.ImageHeight = panelHeight;

    HAL_LTDC_ConfigLayer(&hltdc, &Layercfg, layerIdx);
}

uint8_t STAR_DSI_Init(LCD_OrientationTypeDef orientation)
{
    DSI_PLLInitTypeDef dsiPllInit;
    static RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
    uint32_t LcdClock  = 27429; // LcdClk = 27429 kHz
    uint32_t Clockratio  = 0;
    uint32_t laneByteClk_kHz = 0;
    uint32_t VSA;   // Vertical start active time in units of lines
    uint32_t VBP;   // Vertical Back Porch time in units of lines
    uint32_t VFP;   // Vertical Front Porch time in units of lines
    uint32_t VACT;  // Vertical Active time in units of lines = imageSize Y in pixels to display
    uint32_t HSA;   // Horizontal start active time in units of lcdClk
    uint32_t HBP;   // Horizontal Back Porch time in units of lcdClk
    uint32_t HFP;   // Horizontal Front Porch time in units of lcdClk
    uint32_t HACT;  // Horizontal Active time in units of lcdClk = imageSize X in pixels to display

    LcdResetOnce();
    LowLevelInit();

    hdsi.Instance = HAL_DSI;
    HAL_DSI_DeInit(&(hdsi));

    dsiPllInit.PLLNDIV  = 125;
    dsiPllInit.PLLIDF   = DSI_PLL_IN_DIV2;
    dsiPllInit.PLLODF   = DSI_PLL_OUT_DIV1;
    laneByteClk_kHz = 62500; // 500 MHz / 8 = 62.5 MHz = 62500 kHz

    hdsi.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
    // TXEscapeCkdiv = f(LaneByteClk)/15.62 = 4
    hdsi.Init.TXEscapeCkdiv = laneByteClk_kHz/15620;

    HAL_DSI_Init(&(hdsi), &(dsiPllInit));
    Clockratio = laneByteClk_kHz/LcdClock;
    if (orientation == LCD_ORIENTATION_PORTRAIT) {
        VSA  = OTM8009A_480X800_VSYNC;        // 12
        VBP  = OTM8009A_480X800_VBP;          // 12
        VFP  = OTM8009A_480X800_VFP;          // 12
        HSA  = OTM8009A_480X800_HSYNC;        // 120
        HBP  = OTM8009A_480X800_HBP;          // 120
        HFP  = OTM8009A_480X800_HFP;          // 120
        panelWidth = OTM8009A_480X800_WIDTH;  // 480
        panelHeight = OTM8009A_480X800_HEIGHT; // 800
    }
    else {
        // lcd_orientation == LCD_ORIENTATION_LANDSCAPE
        VSA  = OTM8009A_800X480_VSYNC;        // 12
        VBP  = OTM8009A_800X480_VBP;          // 12
        VFP  = OTM8009A_800X480_VFP;          // 12
        HSA  = OTM8009A_800X480_HSYNC;        // 120
        HBP  = OTM8009A_800X480_HBP;          // 120
        HFP  = OTM8009A_800X480_HFP;          // 120
        panelWidth = OTM8009A_800X480_WIDTH;  // 800
        panelHeight = OTM8009A_800X480_HEIGHT; // 480
    }

    HACT = panelWidth;
    VACT = panelHeight;

    hdsivid.VirtualChannelID = LCD_OTM8009A_ID;
    hdsivid.ColorCoding = LCD_DSI_PIXEL_DATA_FMT_RBG888;
    hdsivid.VSPolarity = DSI_VSYNC_ACTIVE_HIGH;
    hdsivid.HSPolarity = DSI_HSYNC_ACTIVE_HIGH;
    hdsivid.DEPolarity = DSI_DATA_ENABLE_ACTIVE_HIGH;
    hdsivid.Mode = DSI_VID_MODE_BURST;  // Mode Video burst ie : one LgP per line
    hdsivid.NullPacketSize = 0xFFF;
    hdsivid.NumberOfChunks = 0;
    hdsivid.PacketSize = HACT;
    hdsivid.HorizontalSyncActive = HSA*Clockratio;
    hdsivid.HorizontalBackPorch = HBP*Clockratio;
    hdsivid.HorizontalLine = (HACT + HSA + HBP + HFP)*Clockratio;
    hdsivid.VerticalSyncActive = VSA;
    hdsivid.VerticalBackPorch = VBP;
    hdsivid.VerticalFrontPorch = VFP;
    hdsivid.VerticalActive = VACT;

    // Enable or disable sending LP command while streaming is active in video mode
    hdsivid.LPCommandEnable = DSI_LP_COMMAND_ENABLE;
    // Largest packet size possible to transmit in LP mode in VSA, VBP, VFP regions
    // Only useful when sending LP packets is allowed while streaming is active in video mode
    hdsivid.LPLargestPacketSize = 64;
    // Largest packet size possible to transmit in LP mode in HFP region during VACT period
    // Only useful when sending LP packets is allowed while streaming is active in video mode
    hdsivid.LPVACTLargestPacketSize = 64;

    // Specify for each region of the video frame, if the transmission of command in LP mode is allowed in this region
    // while streaming is active in video mode
    hdsivid.LPHorizontalFrontPorchEnable = DSI_LP_HFP_ENABLE;   // Allow sending LP commands during HFP period
    hdsivid.LPHorizontalBackPorchEnable  = DSI_LP_HBP_ENABLE;   // Allow sending LP commands during HBP period
    hdsivid.LPVerticalActiveEnable = DSI_LP_VACT_ENABLE;        // Allow sending LP commands during VACT period
    hdsivid.LPVerticalFrontPorchEnable = DSI_LP_VFP_ENABLE;     // Allow sending LP commands during VFP period
    hdsivid.LPVerticalBackPorchEnable = DSI_LP_VBP_ENABLE;      // Allow sending LP commands during VBP period
    hdsivid.LPVerticalSyncActiveEnable = DSI_LP_VSYNC_ENABLE;   // Allow sending LP commands during VSync = VSA period

    HAL_DSI_ConfigVideoMode(&(hdsi), &(hdsivid));
    HAL_DSI_Start(&(hdsi));

    // Timing Configuration
    hltdc.Init.HorizontalSync = (HSA - 1);
    hltdc.Init.AccumulatedHBP = (HSA + HBP - 1);
    hltdc.Init.AccumulatedActiveW = (panelWidth + HSA + HBP - 1);
    hltdc.Init.TotalWidth = (panelWidth + HSA + HBP + HFP - 1);

    // Initialize the LCD pixel width and pixel height
    hltdc.LayerCfg->ImageWidth  = panelWidth;
    hltdc.LayerCfg->ImageHeight = panelHeight;

    // LCD clock configuration
    // PLLSAI_VCO Input = HSE_VALUE/PLL_M = 2 Mhz
    // PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 384 Mhz
    // PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 384 MHz / 7 = 54.857 MHz
    // LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_2 = 54.857 MHz / 2 = 27.429 MHz
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 7;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    // Background value
    hltdc.Init.Backcolor.Blue = 255;
    hltdc.Init.Backcolor.Green = 255;
    hltdc.Init.Backcolor.Red = 255;
    hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    hltdc.Instance = HAL_LTDC;

    // Get LTDC Configuration from DSI Configuration
    HAL_LTDC_StructInitFromVideoConfig(&(hltdc), &(hdsivid));
    // Initialize the LTDC
    HAL_LTDC_Init(&hltdc);
    // DSI display init
    OTM8009A_Init(hdsivid.ColorCoding, orientation);

    // Layer init
    LayerInit(BG_LAYER_IDX, BG_LAYER_ADDR);

    return LCD_OK;
}

uint16_t STAR_DSI_PanelWidth(void)
{
    return panelWidth;
}

uint16_t STAR_DSI_PanelHeight(void)
{
    return panelHeight;
}

void STAR_DSI_DrawPoint(uint16_t x, uint16_t y, uint32_t color)
{
    *(__IO uint32_t*)(hltdc.LayerCfg[currLayer].FBStartAdress +
            4*(y*panelWidth + x)) = color;
}

void STAR_DSI_FillBufferDma(uint32_t layerIdx, void *dst, uint32_t width,
        uint32_t height, uint32_t lineOffset, uint32_t color)
{
    hdma2d.Init.Mode = DMA2D_R2M;
    hdma2d.Init.ColorMode = DMA2D_ARGB8888;
    hdma2d.Init.OutputOffset = lineOffset;
    hdma2d.Instance = HAL_DMA2D;

    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK)
        if (HAL_DMA2D_ConfigLayer(&hdma2d, layerIdx) == HAL_OK)
            if (HAL_DMA2D_Start(&hdma2d, color, (uint32_t)dst,
                        width, height) == HAL_OK)
                HAL_DMA2D_PollForTransfer(&hdma2d, 100);
}

void STAR_DSI_FillRectDma(uint16_t x, uint16_t y, uint16_t width,
        uint16_t height, uint32_t color)
{
    STAR_DSI_FillBufferDma(currLayer, FB_ADDR(x, y), width, height,
            panelWidth - width, color);
}

void STAR_DSI_DisplayOn(void)
{
    HAL_DSI_ShortWrite(&(hdsi), hdsivid.VirtualChannelID,
            DSI_DCS_SHORT_PKT_WRITE_P1, OTM8009A_CMD_DISPON, 0x00);
}

void STAR_DSI_DisplayOff(void)
{
    HAL_DSI_ShortWrite(&(hdsi), hdsivid.VirtualChannelID,
            DSI_DCS_SHORT_PKT_WRITE_P1, OTM8009A_CMD_DISPOFF, 0x00);
}

void DSI_IO_WriteCmd(uint32_t NbrParams, uint8_t *pParams)
{
    if (NbrParams <= 1)
        HAL_DSI_ShortWrite(&hdsi, LCD_OTM8009A_ID,
                DSI_DCS_SHORT_PKT_WRITE_P1, pParams[0], pParams[1]);
    else
        HAL_DSI_LongWrite(&hdsi,  LCD_OTM8009A_ID,
                DSI_DCS_LONG_PKT_WRITE, NbrParams, pParams[NbrParams],
                pParams);
}
