#ifndef SLIMSERVERINFO_H
#define SLIMSERVERINFO_H

#include <QObject>

#include "squeezedefines.h"


class SlimServerInfo : public QObject
{
    Q_OBJECT
public:
    explicit SlimServerInfo(QObject *parent = 0);
    bool Init(SlimCLI *cliRef);

    bool SetupDevices( void );
    void InitDevices( void );
    bool GetDatabaseInfo(void);

    SlimDevices GetDeviceList( void ) { return deviceList; }
    SlimDevice *GetDeviceFromMac( QByteArray mac );   // use percent encoded MAC address
    SlimDevice *GetCurrentDevice( void ) { return currentDevice; }
    void SetCurrentDevice( SlimDevice *d ) { currentDevice = d; }
    int GetLastRefresh(void) { return lastRefresh; }


signals:
    
public slots:

private:
    bool ProcessServerInfo(QByteArray resp);

    int totalAlbums;
    int totalArtists;
    int totalGenres;
    int totalSongs;
    int playerCount;
    QByteArray serverVersion;
    int lastRefresh;

    SlimDevice *currentDevice;  // convenience pointer
    SlimItem deviceMACList;
    SlimDevices deviceList;

    SlimCLI *cli;               // convenience pointer
    QString SlimServerAddr;   // server IP address
    quint16 cliPort;          // port to use for cli, usually 9090, but we allow the user to change this
    quint16 httpPort;         // port for http connection to retrieve images, usually 9000.
};

#endif // SLIMSERVERINFO_H
