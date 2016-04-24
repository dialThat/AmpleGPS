//
//  AmpleGPS.cpp
//  
//
//  Created by dial on 4/16/16.
//
//

#include <AmpleGPS.h>



void AmpleGPS::begin(){

    atitudeDegrees = longitudeDegrees = altitude = //geoidheight
    speedOnGround = bearing = HDOP =0.0;
    
    hour = minute = seconds = year = month = day =
    fixquality = satellitesCount = 0; // uint8_t

    fix = false; // boolean
    milliseconds = 0; // uint16_t

    bufferIndex = 0;
    
    unparsedSentence = callBack = NULL;

    mode = GPS_COMPLETE_DATA;       //default mode

}

boolean AmpleGPS::parseSentence(char *nmeaSentence) {
    // do checksum check
    
     char *p = nmeaSentence;
//    Serial.print("\n   ");
//    Serial.println(nmeaSentence);
    
    if (strstr_P(nmeaSentence, PSTR(GPGSV_NMEA))) {
//        Serial.println("$GPGSV: ");

        unsigned int totalMessageNumber = 0;
        unsigned int actualMessageNumber = 0;
        GSV_Satellite *aSatellite = satellitesBuffer;
        //        1    = Total number of messages of this type in this cycle
        //        2    = Message number
        //        3    = Total number of SVs in view
        //
        
        
        p += 3;
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
    else if (strstr_P(nmeaSentence, PSTR(GPGGA_NMEA))) {// gga is broken on certain uBlox devices,
        
        //        Serial.println("$GPGGA");
        // get time
        p += 3;
        p=parseTime(p);
        
        p = parsePairOfCoords(p);
        
        if(NULL == p)
            return false;
        
        fixquality = strtol(p+1,&p,10);
        
        satellitesCount = strtol(p+1,&p,10);
        
        HDOP = strtod(p+1, &p);// not needed, only to parse
        
        altitude = strtod (p+1, &p);
        
//        skip the rest
        
        return true;
    }
    
    else if (strstr_P(nmeaSentence, PSTR(GPRMC_NMEA))) {
//                Serial.println("$GPRMC");
        p += 3;
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
    
    else if (strstr_P(nmeaSentence, PSTR(GPGLL_NMEA))) {
        
        //        Serial.println("$GPGLL");
        // get coords
        p += 3;
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
        if (strstr_P(readBuffer, PSTR("$GP")))
        {
//            Serial.println("valid nmeaPrefix");

            uint32_t diff = (uint32_t)((char *)(checksumString - readBuffer));
            char *nmeaCheckString = readBuffer + diff; // real pointer to '*' in the copy
            
            uint16_t checkSum = strtol((nmeaCheckString+1),NULL,16); // read the checksum from the string
            if(checkSum == nmeaCheckSum)                        // do check
            {
                uint8_t length = strlen(readBuffer) - strlen(nmeaCheckString) - 3;// 3 is the prefix

                strncpy(nmeaSentence, (const char *)(readBuffer+3), length);     // copy the string; double buffer
                *(nmeaSentence + length) = '\0';
                
                unparsedSentence = nmeaSentence;                    // valid, but not parsed yet.
                
//                            Serial.println("valid checksum");
            }
            checksumString = NULL;

        }

        if(GPS_DO_NOT_PARSE & mode)
            performCallBack(GPS_DO_NOT_PARSE);
        else
        {   //             Serial.println("will parse");
            
            if(true == parseSentence(nmeaSentence))
            {
                unparsedSentence = NULL;
                
//                Serial.println("did parse");
                
            }
            else
            {
                if(*nmeaSentence == '$')
                {
                    performCallBack(GPS_VIEW_UNPARSED);
                    unparsedSentence = NULL;
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


float AmpleGPS::getDistanceInKm( float targetLat, float targetLon)
{
    //----------------------------------------
    
    //convert decimal degree into radian
    float latRad = latitudeDegrees * half_piRad;
    float lonRad = longitudeDegrees * half_piRad;
    float tlatRad = targetLat * half_piRad;
    float tlonRad = targetLon * half_piRad;
    
    //Calculate the distance in KM
    float latSin = sin((latRad - tlatRad)*0.5);
    float lonSin = sin((lonRad - tlonRad)*0.5);
    float distance = doubleEarth * asin(sqrt((latSin*latSin) + cos(latRad) * cos(tlatRad) * (lonSin * lonSin)));
    
    return distance;
    
    //convert to miles
//    distance.miles = distance.km * 0.621371192;
//    distance.feet = distance.miles *5280;
//
// --------------------------------

}

