# AmpleGPS

This is a small library for Arduino and alike devices to parse NMEA-sentences from a GPS-receiver.
Compared to other libraries all important sentences (GLL,RMC, GSV and GGA) are parsed. Especially extracting the GSV-sentences, wherein the informations about the satellites are stored, is a feature. These informations can range about a total of 4 sentences for 16 satellites, which is a known limit at this time.

The design is developed around the fact, that a GPS-receiver is always firing. Instead of using timers and interrupts the library lists to a serial port and  parses a sentence once it is complete and the checksum is correct.
After finishing parsing, a callback-function is invoked:

    typedef int (*callBackFunction) (uint8_t callBackMode);
 
An additional mode parameter simple tells, what kind of information has been parsed and hence should and can be requested. It is not the information about the parsing results itself, hence the user is responsible how to deal with it.
The values are:

        GPS_PAUSED = 0,
        GPS_DO_NOT_PARSE = (1 << 0),
        GPS_NEW_COORDS = (1 << 1),
        GPS_NEW_TIME = (1 << 2),
        GPS_UPDATE_SATS = (1 << 3),
        GPS_RESERVED = (1 << 4),
        GPS_COMPLETE_DATA = (1 << 5),
        GPS_VIEW_UNPARSED = (1 << 6),
 
Besides GPS_PAUSED, which by nature does nothing, all values may be delivered. GPS_DO_NOT_PARSE if chosen, handles a valid yet unparsed NMEA-sentence without the checksum. GPS_RESERVED will be used for additional informations.

Another pattern is the use of standard C library function like strtod() instead of handcrafted parsing tools, and the preferred use of pointers to values instead of indexed values.
