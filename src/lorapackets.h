//
// Created by brett on 10/29/2020.
//

#ifndef TBEAM0_LORAPACKETS_H
#define TBEAM0_LORAPACKETS_H

typedef struct{
    double lat;
    double lng;
    int sats;
} GPS_DATA;


typedef struct{
    GPS_DATA info;
    double last_time;
} PARTNER_DATA;

typedef struct{
    char id[20];
    int distmax;
    int downmax;
    int buddylock;
} PAIRING_DATA;

#endif //TBEAM0_LORAPACKETS_H
