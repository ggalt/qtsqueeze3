#include "slimimagecache2.h"
#include <QtGlobal>
#include <QDebug>
#include <QImageReader>
#include <QDataStream>


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
    cachePath = DATAPATH;
    requestingImages = false;
    QDir ck(cachePath);
    if(!ck.exists()) {
        ck.mkpath(cachePath);
    }
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
            this,SLOT(ArtworkReply(QNetworkReply*)));
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
            this,SLOT(ArtworkReply(QNetworkReply*)));
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

    httpReplyList.insert(reply,artist_album);
}

void SlimImageCache::ArtworkReply(QNetworkReply *reply)
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
    QDir d;
    d.setCurrent(cachePath);
    QString fileName = QString("%1.JPG").arg(artist_album.trimmed().toUpper());
    f.setFileName(fileName);

    if(f.open(QIODevice::WriteOnly)) {
        //        QDataStream d(&f);
        //        d << buff;
        f.write(buff);
        f.close();
    }
    else
        DEBUGF("Failed to open: "  << fileName);
    httpReplyList.remove(reply);
    reply->deleteLater();

    if(httpReplyList.isEmpty() && !requestingImages) {
        DEBUGF("emitting imagesready");
        emit ImagesReady();
    }
}

QPixmap SlimImageCache::RetrieveCover(const Album &a)
{
    QFile f;
    QPixmap pic;
    QString fileName = QString("%1.JPG").arg(a.artist_album.trimmed().toUpper());

    QDir d;
    d.setCurrent(cachePath);
    f.setFileName(fileName);

    if(f.exists()) {
        if(f.open(QIODevice::ReadOnly)) {
            QByteArray buf = f.readAll();
            DEBUGF("buf is " << buf.size() << " Bytes");
            pic.loadFromData(buf);
            f.close();
            if(!pic.isNull())
                return pic; // success
        }
    }

    // if we are here, weve failed to read the file
    // so deliver a dummy file and retrieve the real image
    if(!a.coverid.isEmpty())
        RequestArtwork(a.coverid,a.artist_album.trimmed().toUpper());

    pic.load(":/img/lib/images/noAlbumImage.png");
    return pic;
}

void SlimImageCache::CheckImages(QList<Album> list)
{
    QMutexLocker m(&mutex);
    albumList = list;
    condition.wakeAll();
}

void SlimImageCache::run(void)
{
    mutex.lock();
    isrunning = true;
    mutex.unlock();
    while(isrunning) {

    }
}
