
#include <cstdio>

#include "arclib.h"
#include "linux_pass_ioctl.h"

using namespace std;

void log (const char *format, ...) {
    va_list args;
    va_start( args, format );
    vprintf( format, args );
    puts( "" );
    va_end( args );
}

void shorten_string (BYTE *target, int max) {
    BYTE *cur = target + max - 1;
    for (; cur >= target; cur -= sizeof(BYTE)) {
        if (' ' != *cur) break;
        *cur = 0;
    }
}

int main (int argc, char *argv[]) {
    LinuxPassIoctlInterface *iface;
    CArclib card;
    sSYSTEM_INFO sysinfo;
    sDRV_MAP drives;
    sGUI_PHY_DRV *drive;
    sHwMonInfo fans, therm;
    int idx;

    setvbuf( stdout, NULL, _IOLBF, 0 );

    log( "opening device /dev/sg2" );
    iface = new LinuxPassIoctlInterface();
    if (!iface->init( 2 )) {
        log( "error initializing with /dev/sg2" );
        return 2;
    }

    if (ARC_SUCCESS != card.ArcInitSession( iface )) {
        log( "error starting control session" );
        return 2;
    }

    if (ARC_SUCCESS != card.ArcGetSysInfo( &sysinfo )) {
        log( "error getting system info" );
        return 3;
    }

    shorten_string( sysinfo.gsiVendorName,   40 );
    shorten_string( sysinfo.gsiModelName,     8 );
    shorten_string( sysinfo.gsiSerialNumber, 16 );
    shorten_string( sysinfo.gsiFirmVersion,  16 );

    // length specs in format because strings are NOT terminated
    log( "device is %.40s %.8s, serial %.16s, firmware %.16s",
            sysinfo.gsiVendorName,
            sysinfo.gsiModelName,
            sysinfo.gsiSerialNumber,
            sysinfo.gsiFirmVersion
        );

    if (ARC_SUCCESS != card.ArcHelpBuildDeviceMap( &drives )) {
        log( "error building drive map" );
        return 3;
    }

    for (idx = 0; idx < drives.total; idx++) {
        drive = &( drives.drv[ idx ].drvInfo );

        if (0 == drive->gpdDeviceState) {
            log( "drive %d is absent", idx + 1 );
        } else {
            shorten_string( drive->gpdModelName,    40 );
            shorten_string( drive->gpdSerialNumber, 20 );
            shorten_string( drive->gpdFirmRev,       8 );

            // length specs in format because strings are NOT terminated
            log( "drive %d is %.40s, serial %.20s, firmware %.8s",
                    idx + 1,
                    drive->gpdModelName,
                    drive->gpdSerialNumber,
                    drive->gpdFirmRev
                );
        }
    }

    if (ARC_SUCCESS != card.ArcGetHWMon( HWMON_FAN_INFO, &fans )) {
        log( "error getting fan status" );
        return 3;
    }

    if (ARC_SUCCESS != card.ArcGetHWMon( HWMON_TMP_INFO, &therm )) {
        log( "error getting thermal status" );
        return 3;
    }

    for (idx = 0; idx < fans.HwItemCount; idx++) {
        log( "fan %d speed: %d", idx, fans.ItemInfo[ idx ] );
    }

    for (idx = 0; idx < therm.HwItemCount; idx++) {
        log( "temp %d: %d", idx + 1, therm.ItemInfo[ idx ] );
    }
}
