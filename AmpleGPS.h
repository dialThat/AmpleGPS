//
//  AmpleGPS.h
//  
//
//  Created by dial on 4/16/16.
//
//

#ifndef AMPLEGPS_H
#define AMPLEGPS_H

#endif /* AmpleGPS_h */


#define GPRMC_NMEA "RMC"
#define GPVTG_NMEA "VTG"
#define GPGGA_NMEA "GGA"
#define GPGSA_NMEA "GSA"
#define GPGSV_NMEA "GSV"
#define GPGLL_NMEA "GLL"


#include "Arduino.h"
    
#define MAXLINELENGTH 87 // max length for NMEA sentences to parse: 82 + '$'+ checksum (*00) +'\0' [=87]
#define MAXSATELLITES 16 // supposed to be 16, but some are carried on while not in sight
    
const float half_piRad PROGMEM = 0.017453293;//  = π/180
const uint16_t doubleEarth PROGMEM = 12742;//  = 2 * 6371// diameter of the Earth


    enum {//GPS_Mode
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
        uint16_t milliseconds;
        
        char readBuffer[MAXLINELENGTH];//82+3(checksum, '*XX'+'$'+'/0'
        uint8_t bufferIndex=0;//volatile
//        const char *nmeaPrefix = "$GP";// keep in mind that 'GP" may be changed
        
//        char nmeaSentence[MAXLINELENGTH + 1];
        char nmeaSentence[81];//82 -'GP' +'/0';
        char *checksumString;
        uint16_t nmeaCheckSum;
        
        uint8_t hour, minute, seconds, year, month, day;
        

        uint8_t mode;
        callBackFunction callBack;
        
        float latitudeDegrees, longitudeDegrees, altitude;
        float speedOnGround, bearing, HDOP;
        boolean fix;
        uint8_t fixquality, satellitesCount;
        
        //        gsv_satellite satellites[MAXSATELLITES];//inserted for GPGSV @dial
        GSV_Satellite satellitesBuffer[MAXSATELLITES];
        boolean gsv_isDirty;
        
        char *unparsedSentence;
        
        char readSentence(char c);
        boolean parseSentence(char *nmeaSentence);
        void begin();
        float getDistanceInKm( float targetLat, float targetLon);

    private:
        char *parseSingleSatellite(char *p, gsv_satellite *aSatellite);
        char *parseSingleCoord (char *p, float *degreeValue);
        char *parsePairOfCoords (char *p);
        char *parseTime(char *p);
        int performCallBack(uint8_t suggestedMode);
        
    };