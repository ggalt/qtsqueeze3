#include "slimimageloader.h"

#define PATH "./.qtsqueezeimage.dat"

slimImageLoader::slimImageLoader(SlimCLI *slimcli)
{
    cli = slimcli;
}

slimImageLoader::~slimImageLoader()
{
    WriteImageFile();
}

QPixmap slimImageLoader::GetAlbumArt( QString albumArtID, bool waitforrequest = true )
{

}

void SlimCLI::InitImageCollection( void )
{
    coverIterator = new QHashIterator< QString, QPixmap >( serverImageList );
    connect( this, SIGNAL( ImageReceived(int) ), this, SLOT( slotImageCollection() ) );
    slotImageCollection();
}

void SlimCLI::slotImageCollection( void )
{
    if( coverIterator->hasNext() ) {
        coverIterator->next();
        AlbumArtAvailable( coverIterator->key() );
    }
    else  // if we are done, disconnect
        disconnect( this, SIGNAL( ImageReceived(int) ), this, SLOT( slotImageCollection() ) );
}

bool slimImageLoader::ReadImageFile( void )
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
    in >> imgFile;
    DEBUGF( "Reading file of size: " << file.size() );
    file.close();
    lastRefresh = imgFile.refreshDate;
    serverImageList = imgFile.imgList;
    return true;
}

void slimImageLoader::WriteImageFile( void )
{
    QFile file;
    ImageFile imgFile;
    file.setFileName( PATH );

    //update the images
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);   // read the data serialized from the file
    imgFile.refreshDate = lastRefresh;
    imgFile.imgList = serverImageList;
    out << imgFile;
    DEBUGF( "Writing file of size: " << file.size() );
    file.close();
    return;
}

void slimImageLoader::GetCoverImage( QString albumArtID, QString thisImageURL, QIODevice *buffer )
{

}

bool slimImageLoader::checkRefreshDate(void)
{
    ServerInfo s = cli->GetServerInfo();
    if(s.lastRefresh!=this->lastRefresh)
        bNeedRefresh = true;
    else
        bNeedRefresh = false;
    this->lastRefresh = s.lastRefresh;  // this way we'll feed the latest refresh date into the file for next time
    return bNeedRefresh;
}

bool slimImageLoader::refreshImageFromServer(void)
{

}
