#ifndef SLIMSERVERINFO_H
#define SLIMSERVERINFO_H

#include <QObject>
#include <QFile>
#include <QDataStream>
#include <QBuffer>


#include "squeezedefines.h"


class SlimServerInfo : public QObject
{
    Q_OBJECT
public:
    explicit SlimServerInfo(QObject *parent = 0);
    ~SlimServerInfo();
    bool Init(SlimCLI *cliRef);

    bool SetupDevices( void );
    void InitDevices( void );
    bool GetDatabaseInfo(void);

    SlimDevices GetDeviceList( void ) { return deviceList; }
    SlimDevice *GetDeviceFromMac( QByteArray mac );   // use percent encoded MAC address, return pointer to associated device
    SlimDevice *GetCurrentDevice( void ) { return currentDevice; }
    void SetCurrentDevice( SlimDevice *d ) { currentDevice = d; }
    int GetLastRefresh(void) { return lastRefresh; }

    QPixmap GetAlbumArt( QString album, QString artist );
    QPixmap GetAlbumArt(QString coverid) { return m_Id2Art.value(coverid,NULL); }
    QList<Album> GetArtistAlbumList(QString artist);

    SlimItem &AlbumArtist2Art(void) {return m_AlbumArtist2Art;}
    SlimItemList &Artist2AlbumIds(void) {return m_Artist2AlbumIds;}
    SlimImageItem &Id2Art(void) {return m_Id2Art;}
    SlimAlbumItem &AlbumID2AlbumInfo(void) {return m_AlbumID2AlbumInfo;}

    bool ReadImageFile( void );   // read in existing images (if any) into serverImageList (save bandwidth)
    void WriteImageFile( void );  // write out contents of serverImageList

signals:
   void FinishedInitializingDevices(void);

public slots:

private:
    bool ProcessServerInfo(QByteArray resp);
    bool checkRefreshDate(void);
    bool refreshImageFromServer(void);

    int totalAlbums;
    int totalArtists;
    int totalGenres;
    int totalSongs;
    int playerCount;
    QByteArray serverVersion;
    int lastRefresh;

    SlimDevice *currentDevice;  // convenience pointer
//    SlimItem deviceMACList;     // MAC address associated with SlimDevice
    SlimDevices deviceList;     // convert MAC address to SlimDevice pointer

    SlimCLI *cli;               // convenience pointer
    QString SlimServerAddr;   // server IP address
    quint16 cliPort;          // port to use for cli, usually 9090, but we allow the user to change this
    quint16 httpPort;         // port for http connection to retrieve images, usually 9000.

    SlimItem m_AlbumArtist2Art;         // Album+Artist name to coverid
    SlimItemList m_Artist2AlbumIds;    // Artist name to list of albums
    SlimImageItem m_Id2Art;       // coverid to artwork
    SlimAlbumItem m_AlbumID2AlbumInfo;    // AlbumID to Album Info

    QByteArray ba;            // bytearray for HTTP request buffer
    QBuffer mybuffer;           // buffer for HTTP request
    QString imageSizeStr;        // text representation of the image size we want (e.g., "/cover_200x200")
    int imageSize;            // int version of image size (we'll always have a square)
    int httpID;               // ID of HTTP transaction
    SlimImageItem serverImageList;  // hash of images retrieved from server by album_image_id
    QHash< int, QString > HttpRequestImageId;  // hash of HTTP request IDs to albumArtistID (for use in matching returned image requests to artwork IDs)
    QHashIterator< QString, QPixmap > *coverIterator;
    bool bNeedRefresh;          // the SqueezeServer database was updated since the last time we connected, so refresh
};


#endif // SLIMSERVERINFO_H
