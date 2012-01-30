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
    if( p.isNull() ) // oops, no image returned, substitute default image
        p.load(QString::fromUtf8(":/img/lib/images/noAlbumImage.png"));

    imageCache.insert(QString(coverID),p);
    httpReplyList.remove(reply);
    delete reply;

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
    if(imageCache.contains(QString(cover_id))) {
        return imageCache.value(QString(cover_id));
    }
    RequestArtwork(cover_id);
    return NULL;
}

void SlimImageCache::run()
{
    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
            this,SLOT(ArtworkReqply(QNetworkReply*)));
    exec();
}

