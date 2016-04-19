//
//  AmpleGPS.h
//  
//
//  Created by n/a on 4/16/16.
//
//

#ifndef AMPLEGPS_H
#define AMPLEGPS_H

#endif /* AmpleGPS_h */


#define GPRMC_NMEA "$GPRMC"
#define GPVTG_NMEA "$GPVTG"
#define GPGGA_NMEA "$GPGGA"
#define GPGSA_NMEA "$GPGSA"
#define GPGSV_NMEA "$GPGSV"
#define GPGLL_NMEA "$GPGLL"


#include "Arduino.h"
    
#define MAXLINELENGTH 88 // max length for NMEA sentences to parse? 82 + '$'+ checksum (*00) +'\0' [=87]
#define MAXSATELLITES 24 // supposed to be 16, but some are carried on while not in sight
    
    
    
    typedef enum {//GPS_Mode
        GPS_PAUSED = 0,
        GPS_DO_NOT_PARSE = (1 << 0),
        GPS_NEW_COORDS = (1 << 1),
        GPS_NEW_TIME = (1 << 2),
        GPS_UPDATE_SATS = (1 << 3),
        GPS_RESERVED = (1 << 4),
        GPS_COMPLETE_DATA = (1 << 5),
        GPS_VIEW_UNPARSED = (1 << 6),
    } ;//GPS_Mode_t;
    
    typedef int (*callBackFunction) (uint8_t callBackMode);
    
    typedef struct gsv_satellite {  //inserted for GPGSV @dial
        uint8_t number;         //4    = SV PRN number
        uint8_t elevation;      //5    = Elevation in degrees, 90 maximum
        uint16_t azimuth;        //6    = Azimuth, degrees from true north, 000 to 359
        uint8_t strength;       //7    = SNR, 00-99 dB (null when not tracking)
    }GSV_Satellite;
    
    
    class AmpleGPS {
    public:
        
        char *readBuffer;//[MAXLINELENGTH + 1];//volatile
        uint8_t bufferIndex=0;//volatile
        
//        char nmeaSentence[MAXLINELENGTH + 1];
        char *nmeaSentence;//[MAXLINELENGTH + 1];
        char *checksumString;//volatile
        uint16_t nmeaCheckSum;
        
        uint8_t hour, minute, seconds, year, month, day;
        uint16_t milliseconds;
        
        // Fixed point latitude and longitude value with degrees stored in units of 1/100000 degrees,
        // and minutes stored in units of 1/100000 degrees.  See pull #13 for more details:
        //   https://github.com/adafruit/Adafruit-GPS-Library/pull/13
        
        uint8_t mode;
        callBackFunction callBack;
        
        float latitudeDegrees, longitudeDegrees;
        float geoidheight, altitude;
        float speedOnGround, bearing, magvariation, HDOP;
        boolean fix;
        uint8_t fixquality, satellitesCount;
        
        //        gsv_satellite satellites[MAXSATELLITES];//inserted for GPGSV @dial
        GSV_Satellite *satellitesBuffer;
        boolean gsv_isDirty;
        
        char *unparsedSentence;
        
        char readSentence(char c);
        boolean parseSentence(char *nmeaSentence);
        void begin();
        
    private:
        char *parseSingleSatellite(char *p, gsv_satellite *aSatellite);
        char *parseSingleCoord (char *p, float *degreeValue);
        char *parsePairOfCoords (char *p);
        char *parseTime(char *p);
        int performCallBack(uint8_t suggestedMode);
        
    };