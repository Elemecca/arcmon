#include <cstdio>

#include <arclib/arclib.h>
#include <arclib/linux_pass_ioctl.h>

extern "C" {
    #include <time.h>
    #include <errno.h>
    #include <string.h>

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
    string scsi_id;
    int sg_id;

public:
    Card (int sg_id, char *scsi_id)
        : sg_id(sg_id), scsi_id(scsi_id) {}

    int open() {
        int idx;

        iface = new LinuxPassIoctlInterface();
        if (!iface->init( sg_id )) {
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

        written = snprintf( message, size, "meta card %s\n", scsi_id.c_str() );
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

        written = snprintf( message, size, "status card %s\n", scsi_id.c_str() );
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


int card_open (card_t card) {
    Card *self = static_cast<Card*>(card);
    return self->open();
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


int cards_find (card_t cards[], size_t max) {
    FILE *devices, *device_strs;
    int sg_idx, card_idx, host, chan, id, lun, type;
    char strs[32], dev_file[32], scsi_id[32];
    int count;
    char *status;
    Card *card;

    devices = fopen( "/proc/scsi/sg/devices", "r" );
    if (!devices) {
        errlog( "unable to open /proc/scsi/sg/devices: %s", strerror( errno ) );
        goto error;
    }

    device_strs = fopen( "/proc/scsi/sg/device_strs", "r" );
    if (!device_strs) {
        errlog( "unable to open /proc/scsi/sg/device_strs: %s", strerror( errno ) );
        goto error;
    }

    card_idx = 0;
    for (sg_idx = 0;; sg_idx++) {
        count = fscanf( devices,
                "%d %d %d %d %d %*d %*d %*d %*d\n",
                &host, &chan, &id, &lun, &type
            );
        if (EOF == count) break;
        else if (5 != count) {
            if (0 != errno) {
                errlog( "error reading device list: %s", strerror( errno ) );
            } else {
                errlog( "unexpected input or EOF in /proc/scsi/sg/devices" );
            }
            goto error;
        }

        status = fgets( strs, 32, device_strs );
        if (!status) {
            if (feof( device_strs )) {
                errlog( "unexpected EOF reading /proc/scsi/sg/device_strs" );
            } else {
                errlog( "error reading /proc/scsi/sg/device_strs: %s", strerror( errno ) );
            }
            goto error;
        }

        // don't consider empty device slots
        if (-1 == host)
            continue;

        // only match type 3 (CPU) devices
        if (3 != type)
            continue;

        // match the vendor and model strings
        // there don't appear to be numeric counterparts
        if (0 != strncmp( "Areca   \tRAID controller \t", strs, 26 ))
            continue;

        snprintf( scsi_id,  32, "%d:%d:%d:%d", host, chan, id, lun );

        card = new Card( sg_idx, scsi_id );
        cards[ card_idx++ ] = static_cast<card_t>( card );
    }

    fclose( devices );
    fclose( device_strs );
    return 0;

error:
    if (devices)
        fclose( devices );
    if (device_strs)
        fclose( device_strs );
    return -1;
}
