#include "slimimageloader.h"
#include "slimcli.h"
#include "slimserverinfo.h"

// uncomment the following to turn on debugging
//#define SLIMCLI_DEBUG

#ifdef SLIMCLI_DEBUG
#define DEBUGF(...) qDebug() << __VA_ARGS__
#else
#define DEBUGF(...)
#endif



#define PATH "./.qtsqueezeimage.dat"

SlimImageLoader::SlimImageLoader(SlimCLI *slimcli)
{
    cli = slimcli;
}

SlimImageLoader::~SlimImageLoader()
{
    WriteImageFile();
}

QPixmap SlimImageLoader::GetAlbumArt( QString albumArtID, bool waitforrequest )
{

}

void SlimImageLoader::InitImageCollection( void )
{
    coverIterator = new QHashIterator< QString, QPixmap >( serverImageList );
    connect( this, SIGNAL( ImageReceived(int) ), this, SLOT( slotImageCollection() ) );
    slotImageCollection();
}

void SlimImageLoader::slotImageCollection( void )
{
    if( coverIterator->hasNext() ) {
        coverIterator->next();
//        AlbumArtAvailable( coverIterator->key() );
    }
    else  // if we are done, disconnect
        disconnect( this, SIGNAL( ImageReceived(int) ), this, SLOT( slotImageCollection() ) );
}

bool SlimImageLoader::ReadImageFile( void )
{
    QFile file;
    ImageFile imgFile;
    if( file.exists( PATH ) ) // there is a file, so read from it
        file.setFileName( PATH );
    else
        return false;

    //update the images
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);   // read the data serialized from the file
    in >> lastRefresh;
    in >> serverImageList;
    DEBUGF( "Reading file of size: " << file.size() );
    file.close();
//    lastRefresh = imgFile.refreshDate;
//    serverImageList = imgFile.imgList;
    return true;
}

void SlimImageLoader::WriteImageFile( void )
{
    QFile file;
    ImageFile imgFile;
    file.setFileName( PATH );

    //update the images
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);   // read the data serialized from the file
//    imgFile.refreshDate = lastRefresh;
//    imgFile.imgList = serverImageList;
    out << lastRefresh;
    out << serverImageList;
//    out << imgFile;
    DEBUGF( "Writing file of size: " << file.size() );
    file.close();
    return;
}

void SlimImageLoader::GetCoverImage( QString albumArtID, QString thisImageURL, QIODevice *buffer )
{

}

bool SlimImageLoader::checkRefreshDate(void)
{
    SlimServerInfo *s = cli->GetServerInfo();
    if(s->GetLastRefresh()!=lastRefresh)
        bNeedRefresh = true;
    else
        bNeedRefresh = false;
    lastRefresh = s->GetLastRefresh();  // this way we'll feed the latest refresh date into the file for next time
    return bNeedRefresh;
}

bool SlimImageLoader::refreshImageFromServer(void)
{


}
