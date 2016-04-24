
/*  Example program <2k for Arduino with GPS and OLED
 *  
 *  This version is stripped down to fit on an Uno or a Nano,
 *  where Flash is 32k bytes. It uses about 20800 bytes and
 *  about 1850 bytes of dynamic memory.
 *  
 *  The single serial line is used for the GPS.
 *  Don't connect the USB-cable and the GPS at the same time,
 *  as it may damage your board or the receiver
 *  First flash it, then discconnect the USB, then connect the GPS.
 *  
 *  Again:
 *  This is a version for OLED and GPS.

 *  Serial is occupied. It cannot be used for debugging here.
 *  If you want to see the data on the screen of your computer,
 *  please use the version without OLED.
 *  
 *  Finally:
 *  The GPS can not be attached until the Arduino is flashed,
 *  If you have another board, or no OLED attached, please modify
 *  this code accordingly.
 *  SoftSerial or Serial2, in case of a Mega or similar, are tested.
 */



#include <AmpleGPS.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

char displayString[14];     // for printing
char compassDir []= "NSEW";


AmpleGPS GPS;


// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
// #define GPSECHO  true

void setup()
{
    // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
    // also spit it out
    Serial.begin(9600);
    delay(1000);
    
    
    GPS.callBack = updateGPSData;
    uint8_t newMode =
    //                        GPS_VIEW_UNPARSED |
    //                        GPS_PAUSED |
    //                        GPS_NEW_COORDS |
    //                        GPS_NEW_TIME |
    //                        GPS_UPDATE_SATS |
    GPS_COMPLETE_DATA;

    GPS.begin();            // GPS can start without parameter
    GPS.mode = newMode;
    
    
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
    
    // Show image buffer on the display hardware.
    // Since the buffer is intialized with an Adafruit splashscreen
    // internally, this will display the splashscreen.
    display.display();
    delay(2000);
    
    // Clear the buffer.
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    
}

void loop()
{
    while (Serial.available())
    {
        char c = Serial.read();
        c = GPS.readSentence(c);
// replacement as one line of code
//        GPS.readSentence(Serial.read());
        
    }
}



void drawSphere(void) {
    int radius = display.height()/2 -1;
    int mid_y = radius;
    int mid_x = display.width() - radius -1;
    
    
    display.drawCircle(mid_x, mid_y, radius, WHITE);
    display.drawCircle(mid_x, mid_y, radius/2, WHITE);
    display.drawLine(mid_x, 0, mid_x, radius * 2, WHITE);
    display.drawLine(mid_x - radius, mid_y, mid_x + radius, mid_y, WHITE);
}

void drawSatellites(void) {
    if(GPS.satellitesCount > 0)
    {
        int skyRadius = display.height()/2 -1;
        int mid_y = skyRadius;
        int mid_x = display.width() - skyRadius -1;
        float markerSize = (float)skyRadius/4;
        
        GSV_Satellite *aSat = GPS.satellitesBuffer;
        for(int i = 0; i < GPS.satellitesCount; i++)
        {
            
            if(0 < aSat -> strength)
            {
                float rad = (float)skyRadius * cos((float)aSat -> elevation);
                float xPos = sin(aSat -> azimuth) * rad + mid_x;
                float yPos = cos(aSat -> azimuth) * rad + mid_y;
                rad = (float)(aSat -> strength)/66;
                //            rad = (float)(1 - (aSat -> strength)) + (rad * 2 * (aSat -> strength))* markerSize;
                
                display.fillCircle(xPos, yPos, rad, WHITE);
                //                display.setCursor(xPos, yPos);
                //                display.println(aSat -> number);
            }
            
            aSat++;
        }
    }
}

void writeData (void)
{
    display.setCursor(0,0);
    
    sprintf_P(displayString, PSTR("%02d.%02d.20%02d"),GPS.day, GPS.month, GPS.year);
    display.println(displayString);
    sprintf_P(displayString, PSTR("%02d:%02d:%02d\n"),GPS.hour, GPS.minute, GPS.seconds);
    display.println(displayString);
    
    //   sprintf(displayString, F("%4.1f m\n"),GPS.altitude);
    //   display.println(displayString);
    display.print(GPS.altitude);display.println(F(" m"));
    //   sprintf(displayString, "%3.2f knots\n",GPS.speedOnGround);
    display.print(GPS.speedOnGround);;display.println(F(" knots\n"));
    
    for(int i=0; i< 2; i++)
    {
        float coord = (0==i) ? GPS.latitudeDegrees : GPS.longitudeDegrees;
        int j = (coord < 0.0)? 1 : 0;
        j += i*2;
        char direction = *(compassDir + j);
        
        float degrees = floor(coord);
        float f_minutes = (coord - degrees) * 60;
        
        unsigned int deg = (unsigned int) degrees;
        unsigned int i_minutes = (unsigned int) floor(f_minutes);
        unsigned int secs = (unsigned int)round((f_minutes - (float)i_minutes)*1000);
        
        sprintf_P(displayString, PSTR("%c %02d %02d.%03d"),direction, deg, i_minutes, secs);
        
        display.println(displayString);
        
    }
}


int updateGPSData (uint8_t mode)
{
    boolean unparsed = false;
    if(GPS_VIEW_UNPARSED & mode || GPS_DO_NOT_PARSE & mode)
    {
        char *unparsedSentence = GPS.unparsedSentence;
        if(unparsedSentence)
        {
            unparsed = true;
        }
    }
    display.clearDisplay();
    
    writeData();        // draws the text string on the left side of the screen
    drawSphere();       // drwas the sheres
    drawSatellites();   // drwas the staelites as dots
    display.display();
    return 0;
}
