#include "slimimagecache.h"
#include <QtGlobal>
#include <QDebug>
#include <QImageReader>

#ifdef SLIMIMAGECACHE_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif


SlimImageCache::SlimImageCache(QObject *parent) :
    QObject(parent)
{
    DEBUGF("");
    SlimServerAddr = "172.0.0.1";
    httpPort=9090;
    imageSizeStr = "cover_200x200";
    cachePath = QDir::homePath()+"/.qtsqueeze3_imagecache/";
}

SlimImageCache::~SlimImageCache()
{
    DEBUGF("");
    if(imageServer)
        delete imageServer;
}
void SlimImageCache::Init(QString serveraddr, qint16 httpport)
{
    DEBUGF("");
    SlimServerAddr = serveraddr;
    httpPort = httpport;
    imageServer = new QNetworkAccessManager();
    imageCache.clear();
    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
            this,SLOT(ArtworkReqply(QNetworkReply*)));
}

void SlimImageCache::Init(QString serveraddr, qint16 httpport, QString imageDim, QString cliuname, QString clipass)
{
    DEBUGF("");
    SlimServerAddr = serveraddr;
    httpPort = httpport;
    imageSizeStr = imageDim;
    cliUsername = cliuname;
    cliPassword = clipass;
    imageServer = new QNetworkAccessManager();
    imageCache.clear();
    connect(imageServer,SIGNAL(finished(QNetworkReply*)),
            this,SLOT(ArtworkReqply(QNetworkReply*)));
}

void SlimImageCache::RequestArtwork(QByteArray coverID, QString artist_album)
{
    DEBUGF("");
    QString urlString = QString("http://%1:%2/music/%3/%4")
            .arg(SlimServerAddr)
            .arg(httpPort)
            .arg(QString(coverID))
            .arg(imageSizeStr);
    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    QNetworkReply *reply = imageServer->get(req);
    char str[100];
    sprintf(str, "reply pointer is %p",reply);
    DEBUGF("Sending request:" << str);
}

void SlimImageCache::ArtworkReqply(QNetworkReply *reply)
{
    DEBUGF("");
    QPixmap p;
    //    QImageReader reader(reply);
    QString artist_album = httpReplyList.value(reply);

    QByteArray buff;
    buff = reply->readAll();
    p.loadFromData(buff);

    //    p.fromImageReader(&reader);
    if(p.isNull()) { // oops, no image returned, substitute default image
        p.load(":/img/lib/images/noAlbumImage.png");
        DEBUGF("returned null image for " << artist_album);
    }

    QFile f;
    f.setFileName(cachePath+artist_album.trimmed().toUpper()+".JPG");
    DEBUGF("writing file: " << f.fileName());
    if(f.open(QFile::WriteOnly)) {
        DEBUGF("Success opening file" << f.fileName());
        f.write(buff);
        f.close();
    }
    DEBUGF("Removing reply");
    httpReplyList.remove(reply);
    DEBUGF("Reply removed");

    char str[100];
    sprintf(str, "reply pointer is %p",reply);
    delete reply;
    DEBUGF("reply deleted" << str);
    if(httpReplyList.isEmpty()) {
        DEBUGF("emitting imagesready");
        emit ImagesReady();
    }
    DEBUGF("Exiting Artwork Reply");
}

QPixmap SlimImageCache::RetrieveCover(const Album &a)
{
    QFile f;
    QPixmap pic;

    f.setFileName(cachePath+a.artist_album.trimmed().toUpper()+".JPG");
    DEBUGF("Looking for file: " << f.fileName());
    if(f.exists()) {
        if(f.open(QFile::ReadOnly)) {
            QImageReader r(&f);
            pic.fromImage(r.read());
            f.close();
            if(pic.isNull())
                return pic; // success
        }
    }

    // if we are here, we've failed to read the file
    // so deliver a dummy file and retrieve the real image
    if(!a.coverid.isEmpty())
        RequestArtwork(a.coverid,a.artist_album.trimmed().toUpper());

    pic.load(":/img/lib/images/noAlbumImage.png");
    return pic;
}
