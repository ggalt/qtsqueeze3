#include "slimimagecache.h"
#include <QtGlobal>


SlimImageCache::SlimImageCache(QObject *parent) :
    QThread(parent)
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

    qDebug() << "Inserting in cache";
    imageCache.insert(QString(coverID),p);
    qDebug() << "Done Inserting in cache";
    httpReplyList.remove(reply);
    qDebug() << "Removed reply ";
    delete reply;
    qDebug() << "Deleted reply";
    emit ImageReady(coverID);

    if(httpReplyList.isEmpty()) {
        qDebug() << "FINISHED GETTING IMAGES";
    }
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

QPixmap SlimImageCache::RetrieveCover(QByteArray cover_id)
{
    QPixmap pic;
    if(imageCache.contains(QString(cover_id))) {
        qDebug() << "returning real image for cover id:" << cover_id;
        return imageCache.value(QString(cover_id));
//        pic = imageCache.value(QString(cover_id));
//        qDebug() << "returning picture of size:" << pic.size();
//        return pic;
    }
    if(cover_id.isEmpty())
        return NULL;
    RequestArtwork(cover_id);
    qDebug() << "RetrieveCover is returning NULL";
    return NULL;
}

void SlimImageCache::run()
{
    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
            this,SLOT(ArtworkReqply(QNetworkReply*)));
    exec();
}

