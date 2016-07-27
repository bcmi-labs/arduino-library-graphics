#include <agfx.h>

AGFX agfx = AGFX();

const int
  pixelWidth  = 800, // LCD dimensions
  pixelHeight = 480,
  iterations  = 255;  // Fractal iteration limit or 'dwell'
const float
  centerReal  = -0.6, // Image center point in complex plane
  centerImag  =  0.0,
  rangeReal   =  3.0, // Image coverage in complex plane
  rangeImag   =  3.0,
  startReal   = centerReal - rangeReal * 0.5,
  startImag   = centerImag + rangeImag * 0.5,
  incReal     = rangeReal / (float)pixelWidth,
  incImag     = rangeImag / (float)pixelHeight;
 
void setup()
{
  pinMode(13,OUTPUT);   // Arduino status LED
  digitalWrite(13,LOW); // LED off
  
  /* Initialize LCD */
  agfx.begin();
  agfx.fill(AGFX_DARKGRAY);
}
 
void loop()
{
  int           x,y,n;
  float         a,b,a2,b2,posReal,posImag;
  long          startTime,elapsedTime;
 
  delay(100);              // Brief pause, else 1st few pixels are lost
 
  digitalWrite(13,HIGH);   // LED on while rendering
  startTime = millis();
 
  agfx.background(AGFX_DARKGRAY);
  posImag = startImag;
  for(y = 0; y < pixelHeight; y++) {
    posReal = startReal;
    for(x = 0; x < pixelWidth; x++) {
      a = posReal;
      b = posImag;
      for(n = iterations; n > 0 ; n--) {
        a2 = a * a;
        b2 = b * b;
        if((a2 + b2) >= 4.0f) break;
        b  = posImag + a * b * 2.0f;
        a  = posReal + a2 - b2;
      }
      
      AGFX_LL_DrawPoint(x, y, 0xFF000000 | ((n*67) << 3));
      
      posReal += incReal;
    }
    posImag -= incImag;
  }
 
  elapsedTime = millis() - startTime;
  digitalWrite(13,LOW);    // LED off when done
  
  delay(2000); // Stall a few seconds, then repeat
  
  agfx.fill(AGFX_DARKGRAY);
}

