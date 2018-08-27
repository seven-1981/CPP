#ifndef _ZEIT_H
#define _ZEIT_H 

#include <ctime>

void UnixTimeToDateTime(unsigned long int unixtime,
                           int *pJahr, int *pMonat, int *pTag,
                           int *pStunde, int *pMinute, int *pSekunde)
{
    const unsigned long int SEKUNDEN_PRO_TAG   =  86400ul; /*  24* 60 * 60 */
    const unsigned long int TAGE_IM_GEMEINJAHR =    365ul; /* kein Schaltjahr */
    const unsigned long int TAGE_IN_4_JAHREN   =   1461ul; /*   4*365 +   1 */
    const unsigned long int TAGE_IN_100_JAHREN =  36524ul; /* 100*365 +  25 - 1 */
    const unsigned long int TAGE_IN_400_JAHREN = 146097ul; /* 400*365 + 100 - 4 + 1 */
    const unsigned long int TAGN_AD_1970_01_01 = 719468ul; /* Tagnummer bezogen auf den 1. Maerz des Jahres "Null" */
    unsigned long int TagN = TAGN_AD_1970_01_01 + unixtime/SEKUNDEN_PRO_TAG;
    unsigned long int Sekunden_seit_Mitternacht = unixtime%SEKUNDEN_PRO_TAG;
    unsigned long int temp;

    /* Schaltjahrregel des Gregorianischen Kalenders:
       Jedes durch 100 teilbare Jahr ist kein Schaltjahr, es sei denn, es ist durch 400 teilbar. */
    temp = 4 * (TagN + TAGE_IN_100_JAHREN + 1) / TAGE_IN_400_JAHREN - 1;
    *pJahr = 100 * temp;
    TagN -= TAGE_IN_100_JAHREN * temp + temp / 4;

    /* Schaltjahrregel des Julianischen Kalenders:
       Jedes durch 4 teilbare Jahr ist ein Schaltjahr. */
    temp = 4 * (TagN + TAGE_IM_GEMEINJAHR + 1) / TAGE_IN_4_JAHREN - 1;
    *pJahr += temp;
    TagN -= TAGE_IM_GEMEINJAHR * temp + temp / 4;

    /* TagN enthaelt jetzt nur noch die Tage des errechneten Jahres bezogen auf den 1. Maerz. */
    *pMonat = (5 * TagN + 2) / 153;
    *pTag = TagN - (*pMonat * 153 + 2) / 5 + 1;

    /*  153 = 31+30+31+30+31 Tage fuer die 5 Monate von Maerz bis Juli
        153 = 31+30+31+30+31 Tage fuer die 5 Monate von August bis Dezember
              31+28          Tage fuer Januar und Februar (siehe unten)
        +2: Justierung der Rundung
        +1: Der erste Tag im Monat ist 1 (und nicht 0).
    */
    *pMonat += 3; /* vom Jahr, das am 1. Maerz beginnt auf unser normales Jahr umrechnen: */
    if (*pMonat > 12)
    {   /* Monate 13 und 14 entsprechen 1 (Januar) und 2 (Februar) des naechsten Jahres */
        *pMonat -= 12;
        ++*pJahr;
    }

    *pStunde  = Sekunden_seit_Mitternacht / 3600;
    *pMinute  = Sekunden_seit_Mitternacht % 3600 / 60;
    *pSekunde = Sekunden_seit_Mitternacht        % 60;
}

#endif
