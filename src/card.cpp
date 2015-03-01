#include <cstdio>

#include <arclib/arclib.h>
#include <arclib/linux_pass_ioctl.h>

extern "C" {
    #include <time.h>

    #include "common.h"
    #include "card.h"
}

using namespace std;


void shorten_string (BYTE *target, int max) {
    BYTE *cur = target + max - 1;
    for (; cur >= target; cur -= sizeof(BYTE)) {
        if (' ' != *cur) break;
        *cur = 0;
    }
}


class Card {
private:
    LinuxPassIoctlInterface *iface;
    CArclib card;
    sSYSTEM_INFO sysinfo;
    sDRV_MAP drives;

public:
    int open (int index) {
        int idx;

        iface = new LinuxPassIoctlInterface();
        if (!iface->init( index )) {
            errlog( "error initializing with /dev/sg2" );
            goto error;
        }

        if (ARC_SUCCESS != card.ArcInitSession( iface )) {
            errlog( "error starting control session" );
            goto error;
        }


        if (ARC_SUCCESS != card.ArcGetSysInfo( &sysinfo )) {
            errlog( "error getting system info" );
            goto error;
        }

        shorten_string( sysinfo.gsiVendorName,   40 );
        shorten_string( sysinfo.gsiModelName,     8 );
        shorten_string( sysinfo.gsiSerialNumber, 16 );
        shorten_string( sysinfo.gsiFirmVersion,  16 );

        if (ARC_SUCCESS != card.ArcHelpBuildDeviceMap( &drives )) {
            errlog( "error building drive map" );
            return 3;
        }

        return 1;

    error:
        return 0;
    }

    int meta (char message[], size_t size) {
        sGUI_PHY_DRV *drive;
        size_t written;
        size_t total = 0;
        int idx;

        written = snprintf( message, size, "meta card 0\n");
        if (written < 0 || written >= size) goto error;
        message += written;
        size  -= written;
        total += written;

        // length specs in format because strings are NOT terminated
        written = snprintf( message, size,
                "device vendor \"%.40s\" model \"%.8s\" serial \"%.16s\" firmware \"%.16s\"\n",
                sysinfo.gsiVendorName,
                sysinfo.gsiModelName,
                sysinfo.gsiSerialNumber,
                sysinfo.gsiFirmVersion
            );
        if (written < 0 || written >= size) goto error;
        message += written;
        size  -= written;
        total += written;

        for (idx = 0; idx < drives.total; idx++) {
            drive = &( drives.drv[ idx ].drvInfo );

            if (0 == drive->gpdDeviceState) {
                written = snprintf( message, size,
                        "drive %u.%u absent\n",
                        drives.drv[idx].encindex,
                        drives.drv[idx].drvindex );
                if (written < 0 || written >= size) goto error;
                message += written;
                size  -= written;
                total += written;
            } else {
                shorten_string( drive->gpdModelName,    40 );
                shorten_string( drive->gpdSerialNumber, 20 );
                shorten_string( drive->gpdFirmRev,       8 );

                // length specs in format because strings are NOT terminated
                written = snprintf( message, size,
                        "drive %u.%u model \"%.40s\" serial \"%.20s\" firmware \"%.8s\"\n",
                        drives.drv[idx].encindex,
                        drives.drv[idx].drvindex,
                        drive->gpdModelName,
                        drive->gpdSerialNumber,
                        drive->gpdFirmRev
                    );
                if (written < 0 || written >= size) goto error;
                message += written;
                size  -= written;
                total += written;
            }
        }

        written = snprintf( message, size, "\n" );
        if (written < 0 || written >= size) goto error;
        message += written;
        size  -= written;
        total += written;

        return total;

    error:
        return -1;
    }

    int status (char message[], size_t size) {
        sHwMonInfo fans, therm;
        time_t start_time;
        int idx, therm_base;
        size_t written;
        size_t total = 0;

        if (ARC_SUCCESS != card.ArcGetHWMon( HWMON_FAN_INFO, &fans )) {
            errlog( "error getting fan status" );
            goto error;
        }

        if (ARC_SUCCESS != card.ArcGetHWMon( HWMON_TMP_INFO, &therm )) {
            errlog( "error getting thermal status" );
            goto error;
        }

        written = snprintf( message, size, "status card 0\n");
        if (written < 0 || written >= size) goto error;
        message += written;
        size  -= written;
        total += written;

        start_time = time(0);
        written = strftime( message, size,
                "time %Y-%m-%dT%H:%M:%S%z\n",
                localtime( &start_time ) );
        if (written < 0 || written >= size) goto error;
        message += written;
        size  -= written;
        total += written;

        for (idx = 0; idx < fans.HwItemCount; idx++) {
            written = snprintf( message, size,
                    "fan %d speed %d\n", idx, fans.ItemInfo[ idx ] );
            if (written < 0 || written >= size) goto error;
            message += written;
            size  -= written;
            total += written;
        }


        if (therm.HwItemCount > drives.total) {
            written = snprintf( message, size,
                    "cpu temp %d\ncontroller temp %d\n",
                    therm.ItemInfo[ 0 ], therm.ItemInfo[ 1 ] );
            if (written < 0 || written >= size) goto error;
            message += written;
            size  -= written;
            total += written;

            therm_base = 2;
        } else {
            therm_base = 0;
        }

        for (idx = 0; idx < drives.total; idx++) {
            if (0 == drives.drv[idx].drvInfo.gpdDeviceState) continue;

            written = snprintf( message, size,
                    "drive %u.%u temp %d\n",
                    drives.drv[idx].encindex,
                    drives.drv[idx].drvindex,
                    therm.ItemInfo[ idx + therm_base ] );
            if (written < 0 || written >= size) goto error;
            message += written;
            size  -= written;
            total += written;
        }

        written = snprintf( message, size, "\n" );
        if (written < 0 || written >= size) goto error;
        message += written;
        size  -= written;
        total += written;

        return total;

    error:
        return -1;
    }
};


card_t card_open (int index) {
    Card *self = new Card();

    if (!self->open( index )) {
        errlog( "error opening device /dev/sg%d", index );
        delete self;
        return (card_t) NULL;
    }

    return static_cast<card_t>(self);
}

int card_meta (card_t card, char message[], size_t size) {
    Card *self = static_cast<Card*>(card);
    return self->meta( message, size );
}

int card_status (card_t card, char message[], size_t size) {
    Card *self = static_cast<Card*>(card);
    return self->status( message, size );
}

void card_free (card_t card) {
    Card *self = static_cast<Card*>(card);
    delete self;
}
