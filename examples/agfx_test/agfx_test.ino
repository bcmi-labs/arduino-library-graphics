#include <agfx.h>

AGFX agfx = AGFX();

void setup()
{
    agfx.begin();
}

void loop()
{
    agfx.fill(AGFX_WHITE);

    agfx.background(AGFX_WHITE);
    agfx.demoLine(2);
    delay(1000);

    agfx.background(AGFX_WHITE);
    agfx.demoPolygon();
    delay(2000);

    agfx.background(AGFX_WHITE);
    agfx.demoCircle();
    delay(2000);

    agfx.background(AGFX_WHITE);
    agfx.demoEllipse();
    delay(2000);

    agfx.background(AGFX_WHITE);
    agfx.demoText();
    delay(3000);

    // Blocking demo
    agfx.background(AGFX_WHITE);
    agfx.demoTouch();
}

