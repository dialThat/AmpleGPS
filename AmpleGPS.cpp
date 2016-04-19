//
//  AmpleGPS.cpp
//  
//
//  Created by n/a on 4/16/16.
//
//

#include <AmpleGPS.h>



void AmpleGPS::begin(){
    callBack = NULL;
     latitudeDegrees = longitudeDegrees =
     geoidheight = altitude =
    speedOnGround = bearing = magvariation = HDOP =0.0;
    
    hour = minute = seconds = year = month = day =
    fixquality = satellitesCount = 0; // uint8_t

    fix = false; // boolean
    milliseconds = 0; // uint16_t

    
    readBuffer = (char *)malloc(MAXLINELENGTH + 1);
    bufferIndex = 0;
    
    //  nmeaSentence[MAXLINELENGTH + 1];
    nmeaSentence = (char *)malloc(MAXLINELENGTH + 1);

    satellitesBuffer = (GSV_Satellite *)calloc(MAXSATELLITES, sizeof(GSV_Satellite));
    
    Serial.println(sizeof(GSV_Satellite));
    Serial.println(sizeof(GSV_Satellite *));

    Serial.println((MAXSATELLITES * sizeof(GSV_Satellite)));
    Serial.println((unsigned long) satellitesBuffer);

    unparsedSentence = NULL;
    callBack = NULL;
    mode = GPS_COMPLETE_DATA;

}

boolean AmpleGPS::parseSentence(char *nmeaSentence) {
    // do checksum check
    
    char *p = nmeaSentence;
//    Serial.println(nmeaSentence);
    
    if (strstr(nmeaSentence, "$GPGSV")) {
//        Serial.println("$GPGSV: ");

        unsigned int totalMessageNumber = 0;
        unsigned int actualMessageNumber = 0;
        GSV_Satellite *aSatellite = satellitesBuffer;
        //        1    = Total number of messages of this type in this cycle
        //        2    = Message number
        //        3    = Total number of SVs in view
        //
        
        
        p += 6;
        //        Serial.print(*p);
        
        totalMessageNumber = strtol(p+1,&p,10);
        
        actualMessageNumber = strtol(p+1,&p,10);
        
        satellitesCount = strtol(p+1,&p,10);
        
        if(actualMessageNumber && totalMessageNumber && satellitesCount)// basic check
        {
            aSatellite = satellitesBuffer + (actualMessageNumber - 1)*4;
            
            int maxCount = 4;       // max 4 sats in sentence
            
            if(1==actualMessageNumber)// if this is the first sentence in this serie, invalidate all satellites
            {// the assumption is, that number 1 is the first in line here
                gsv_isDirty = true;
            }
            else if (totalMessageNumber == actualMessageNumber)
            {
                gsv_isDirty = false;
                maxCount = satellitesCount - (actualMessageNumber - 1)*4;
                
                GSV_Satellite *aSat = satellitesBuffer + satellitesCount;
                
                for(int i = 0; i<(MAXSATELLITES - satellitesCount);i++)// fill up the remaining slots with zeroes
                {
                    aSat -> number = aSat->elevation = aSat -> azimuth = aSat -> strength = 0;
                    aSat ++;

                }
                performCallBack(GPS_UPDATE_SATS);
            }
            
            // now parse all satellites
            for (int i=0; i < maxCount; i++)
            {
                p = parseSingleSatellite(p, aSatellite);
                aSatellite ++;
            }
            
        }
        return true;
    }
    
    // look for a few common sentences
    else if (strstr(nmeaSentence, "$GPGGA")) {
        
        //        Serial.println("$GPGGA");
        // get time
        p += 6;
        p=parseTime(p);
        
        p = parsePairOfCoords(p);
        
        if(NULL == p)
            return false;
        
        fixquality = strtol(p+1,&p,10);
        
        satellitesCount = strtol(p+1,&p,10);
        
        HDOP = strtod(p+1, &p);
        
        altitude = strtod (p+1, &p);
        
        p = strchr(p, ',')+1;
        p = strchr(p, ',')+1;
        if (',' != *p)
        {// when will we ever use it
            geoidheight = strtod (p, &p);
//            Serial.print("$geoidheight: "); Serial.println(geoidheight);
            
        }
        
        return true;
    }
    
    else if (strstr(nmeaSentence, "$GPRMC")) {
//                Serial.println("$GPRMC");
        p += 6;
        p=parseTime(p);
        
        p++;

        if (*p == 'A')
            fix = true;
        else if (*p == 'V')
            fix = false;
        else
            return false;
        
        p++;// p hasn't changed yet.

        // parse out coords
        p = parsePairOfCoords(p);

        if(NULL == p)
            return false;
        
        // speedOnGround
        speedOnGround = strtod(p+1, &p);
//        Serial.print("speedOnGround: "); Serial.println(speedOnGround);

        bearing = strtod(p+1, &p);
//        Serial.print("bearing: "); Serial.println(bearing);

        uint32_t fulldate = strtol(p+1, &p, 10);
        day = fulldate / 10000;
        month = (fulldate % 10000) / 100;
        year = (fulldate % 100);
        
//        Serial.println(p+1);

        performCallBack(GPS_COMPLETE_DATA);
        
        return true;
    }
    
    else if (strstr(nmeaSentence, "$GPGLL")) {
        
        //        Serial.println("$GPGLL");
        // get coords
        p += 6;
        p = parsePairOfCoords(p);
        
        if(NULL == p)
            return false;
        
        p=parseTime(p);
        
        p++;
        // Serial.println(p);
        if (*p == 'A')
            fix = true;
        else if (*p == 'V')
            fix = false;
        else
            return false;

        return true;
    }
    return false;
}



char AmpleGPS::readSentence(char c) {
    
    if (mode & GPS_PAUSED) return c;
    
    if (c == '\n') {        // sentence is at end
        strcpy(nmeaSentence, (const char *)readBuffer);     // copy the string; double buffer
        
        uint32_t diff = (uint32_t)((char *)(checksumString - readBuffer));
        char *nmeaCheckString = nmeaSentence + diff; // real pointer to '*' in the copy
        
        uint16_t checkSum = strtol((nmeaCheckString+1),NULL,16); // read the checksum from the string
        if(checkSum == nmeaCheckSum)                        // do check
        {
            *nmeaCheckString = '\0';                             // end the nmea where checksumString did start
            checksumString = NULL;

            unparsedSentence = nmeaSentence;                    // valid, but not parsed yet.
            
            //            Serial.println("valid checksum");
        }
        else {
            Serial.print("--: ");
            Serial.print(checkSum);
            Serial.print(", ");
            Serial.println(nmeaCheckSum);
            Serial.println((const char *)readBuffer);
            
        }
        
        if(GPS_DO_NOT_PARSE & mode)
            performCallBack(GPS_DO_NOT_PARSE);
        else
        {    //            Serial.println("will parse");
            
            if(true == parseSentence(nmeaSentence))
            {
                unparsedSentence = NULL;
//                                Serial.println("did parse");
                
            }
            else
            {
                if(*nmeaSentence == '$')
                {
                    performCallBack(GPS_VIEW_UNPARSED);
                }
            }
        }
        
    }
    else if (c == '$')
    {
        bufferIndex = 0;
        checksumString = NULL;
        nmeaCheckSum = 0;
        
    }
    else if (c == '*')          // set the pointer to the checksumString
    {
        checksumString = readBuffer + bufferIndex;
        
    }
    
    if(NULL == checksumString)
    {
        if(bufferIndex)     // omit the first sign ('$')
        {
            nmeaCheckSum ^= c;
        }
    }
    
    *(readBuffer + bufferIndex)=c;
    bufferIndex++;
    return c;
}




char *AmpleGPS::parseTime(char *p)
{
    float timed = strtod (p+1, &p);
    uint32_t time = (uint32_t)timed;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);
    milliseconds = fmod(timed, 1.0) * 1000;
    
    performCallBack(GPS_NEW_TIME);
    
    return p;
}


char *AmpleGPS::parseSingleSatellite(char *p, GSV_Satellite *aSatellite){
    
    aSatellite -> number = strtol(p+1,&p,10);
    
    aSatellite -> elevation = strtol(p+1,&p,10);
    aSatellite -> azimuth = strtol(p+1,&p,10);
    
    aSatellite -> strength = strtol(p+1,&p,10);
    return p;
}

char *AmpleGPS::parsePairOfCoords (char *p){
    
    p = parseSingleCoord(p,&latitudeDegrees);
    //    Serial.println(*p);
    
    if(p)
        p = parseSingleCoord(p,&longitudeDegrees);
    //    if(p)     // debug
    //    {
    //        Serial.print("lat: ");
    //        Serial.print(latitudeDegrees,6);
    //        Serial.print("; lon: ");
    //        Serial.println(longitudeDegrees,6);
    //    }
    //    else
    //        Serial.println("something wrong");
    
    
    performCallBack(GPS_NEW_COORDS);
    return p;
}



char *AmpleGPS::parseSingleCoord(char *p, float *degreeValue){
    
    double val = strtod (p+1, &p)/100.0;
    int degrees = floor(val);
    val = (val - degrees)/0.6;
    *degreeValue = val + degrees;
    
    p++;
    if (*p == 'S' || *p == 'W')
    {
        *degreeValue *= -1.0;
    }
    else
        if(!(*p == 'N' || *p == 'E'))
        {
            p = NULL;
        }
    if(p)
        p++;// we know that for sure, single char + '\0'
    
    return p;
}


int AmpleGPS::performCallBack(uint8_t suggestedMode)
{

    int retVal = 0;
    if((GPS_PAUSED & suggestedMode) & mode)       // exclusive, do nothing
        return 0;
    
    if((GPS_DO_NOT_PARSE & suggestedMode)& mode)     // exclusive, only nmeaString can be requested
        retVal = callBack(GPS_DO_NOT_PARSE);
    else
        if((GPS_COMPLETE_DATA & suggestedMode) & mode)    // exclusive, all data can be requested, but no other calls are perfromed
            retVal= callBack(GPS_COMPLETE_DATA);
    
    
        else if(suggestedMode & mode) // GPS_NEW_COORDS GPS_NEW_TIME GPS_VIEW_UNPARSED GPS_UPDATE_SATS
            retVal =callBack(suggestedMode);
    return retVal;
}

/*
 GPS_PAUSED = 0,
 GPS_DO_NOT_PARSE = (1 << 0),
 GPS_NEW_COORDS = (1 << 1),
 GPS_NEW_TIME = (1 << 2),
 GPS_UPDATE_SATS = (1 << 3),
 GPS_RESERVED = (1 << 4),
 GPS_COMPLETE_DATA = (1 << 5),
 GPS_VIEW_UNPARSED = (1 << 6),
*/


/*
 $GPVTG,,T,,M,0.052,N,0.097,K,A*2A
 +$GPGGA,061919.00,5040.58619,66359,E,1,07,1.93,68.3,M,46.8,M,,*62
 $GPGSA,A,3,16,30,23,03,07,09,06,,,,,,2.80,1.93,2.03*0C
 -$GPGSV,3,1,11,02,41,290,,03,11,119,34,05,12,297,,06,47,221,32*75
 -$GPGSV,3,2,11,07,46,168,30,09,77,061,30,16,16,062,27,23,44,069,40*77
 $GPGSV,3,3,11,26,11,034,,29,06,341,,30,20,187,30*4D
 $GPGLL,5040.58619,N,00709.66359,E,061919.00,A,A*6C
 +$GPRMC,061920.00,A,5040.58639,N,00709.66356,E,0.876,247.96,130416,,,A*6A
 
 */

