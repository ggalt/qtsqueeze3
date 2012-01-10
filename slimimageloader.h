#ifndef SLIMIMAGELOADER_H
#define SLIMIMAGELOADER_H

#include <QHttp>
#include <QFile>
#include <QDataStream>
#include <QBuffer>

#include "squeezedefines.h"
#include "ImageLoader.h"

class SlimImageLoader : public ImageLoader
{
public:
    SlimImageLoader(SlimCLI *slimcli);
    ~SlimImageLoader();

    bool AlbumArtAvailable( QString albumArtID );

    QPixmap GetAlbumArt( QString albumArtID, bool waitforrequest = true );
    bool ReadImageFile( void );   // read in existing images (if any) into serverImageList (save bandwidth)
    void WriteImageFile( void );  // write out contents of serverImageList
    void GetCoverImage( QString albumArtID, QString thisImageURL, QIODevice *buffer );
    void InitImageCollection( void );
    void slotImageCollection( void );

private:
    bool checkRefreshDate(void);
    bool refreshImageFromServer(void);

    SlimCLI *cli;

    QHttp *imageURL;          // an http object to get the current cover image
    QByteArray ba;            // bytearray for HTTP request buffer
    QBuffer mybuffer;           // buffer for HTTP request
    QString imageSizeStr;        // text representation of the image size we want (e.g., "/cover_200x200")
    int imageSize;            // int version of image size (we'll always have a square)
    int httpID;               // ID of HTTP transaction
    SlimImageItem serverImageList;  // hash of images retrieved from server by album_image_id
    QHash< int, QString > HttpRequestImageId;  // hash of HTTP request IDs to albumArtistID (for use in matching returned image requests to artwork IDs)
    QHashIterator< QString, QPixmap > *coverIterator;
    bool bNeedRefresh;          // the SqueezeServer database was updated since the last time we connected, so refresh
    int lastRefresh;     // when was the database last refreshed
};


#endif // SLIMIMAGELOADER_H
