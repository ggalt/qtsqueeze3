#include "slimimagecache.h"
#include <QtGlobal>


SlimImageCache::SlimImageCache(QObject *parent) :
    QObject(parent)
{
    SlimServerAddr = "172.0.0.1";
    httpPort=9090;
    imageSizeStr = "cover_200x200";
}

SlimImageCache::~SlimImageCache()
{
    if(imageServer)
        delete imageServer;
}
void SlimImageCache::Init(QString serveraddr, qint16 httpport)
{
    SlimServerAddr = serveraddr;
    httpPort = httpport;
    imageServer = new QNetworkAccessManager();
    imageCache.clear();
}

void SlimImageCache::Init(QString serveraddr, qint16 httpport, QString imageDim, QString cliuname, QString clipass)
{
    SlimServerAddr = serveraddr;
    httpPort = httpport;
    imageSizeStr = imageDim;
    cliUsername = cliuname;
    cliPassword = clipass;
    imageServer = new QNetworkAccessManager();
    imageCache.clear();
}

void SlimImageCache::ArtworkReqply(QNetworkReply *reply)
{
    QPixmap p;
    QImageReader reader(reply);
    QByteArray coverID = httpReplyList.value(reply);

    qDebug() << "retrieved image for id: " << coverID;

    p.fromImageReader(&reader);
    qDebug() << "read image";
    if( p.isNull() ) // oops, no image returned, substitute default image
        p.load(":/img/lib/images/noAlbumImage.png");

    QString artist_album = Cover2ArtistAlbum.value(coverID);
    imageCache.insert(artist_album.toUpper(),p);
    httpReplyList.remove(reply);
    delete reply;

    emit ImageReady(coverID);
}

void SlimImageCache::RequestArtwork(QByteArray coverID)
{
    QString urlString = QString("http://%1:%2/music/%3/%4")
            .arg(SlimServerAddr)
            .arg(httpPort)
            .arg(QString(coverID))
            .arg(imageSizeStr);
    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    QNetworkReply *reply = imageServer->get(req);
    httpReplyList.insert(reply,coverID);
    qDebug() << "requesting artwork with id: " << coverID;
}

QPixmap SlimImageCache::RetrieveCover(QString artist_album, QByteArray cover_id)
{
    QPixmap pic;
    if(imageCache.contains(artist_album.toUpper())) {
        qDebug() << "returning real image for cover id:" << cover_id;
        return imageCache.value(artist_album.toUpper());
    }
    if(cover_id.isEmpty())
        return NULL;
    Cover2ArtistAlbum.insert(cover_id,artist_album);
    RequestArtwork(cover_id);
    qDebug() << "RetrieveCover is returning NULL";
    return NULL;
}

QPixmap SlimImageCache::RetrieveCover(QByteArray coverID)
{
    QString artist_album = Cover2ArtistAlbum.value(coverID);
    return RetrieveCover(artist_album);
}

QPixmap SlimImageCache::RetrieveCover(QString artist_album)
{
    return imageCache.value(artist_album.toUpper());
}

//void SlimImageCache::run()
//{
//    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
//            this,SLOT(ArtworkReqply(QNetworkReply*)));
//    exec();
//}

