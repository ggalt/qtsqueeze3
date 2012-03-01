#include "slimimagecache2.h"
#include <QtGlobal>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>


#ifdef SLIMIMAGECACHE_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif


SlimImageCache::SlimImageCache(QObject *parent) :
    QThread(parent)
{
    DEBUGF("");
    SlimServerAddr = "172.0.0.1";
    httpPort=9090;
    imageSizeStr = "cover_200x200";
    cachePath = QDir::homePath()+DATAPATH;
    requestingImages = false;
    QDir ck(cachePath);
    if(!ck.exists()) {
        ck.mkpath(cachePath);
    }
    albumList.clear();  // make sure it's empty
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
}

void SlimImageCache::Init(QString serveraddr, qint16 httpport, QString imageDim, QString cliuname, QString clipass)
{
    DEBUGF("");
    SlimServerAddr = serveraddr;
    httpPort = httpport;
    imageSizeStr = imageDim;
    cliUsername = cliuname;
    cliPassword = clipass;
}

void SlimImageCache::RequestArtwork(QByteArray coverID, QString artist_album)
{
    DEBUGF("");

    // establish a single-shot timer and event loop so we can capture the
    // http reply from the LMS with the cover image
    QEventLoop q;
    QTimer tT;

    tT.setSingleShot(true);
    connect(&tT, SIGNAL(timeout()), &q, SLOT(quit()),Qt::DirectConnection);
    connect(imageServer, SIGNAL(finished(QNetworkReply*)),
            &q, SLOT(quit()),Qt::DirectConnection);


    // build the URL and send the request
    QString urlString = QString("http://%1:%2/music/%3/%4")
            .arg(SlimServerAddr)
            .arg(httpPort)
            .arg(QString(coverID))
            .arg(imageSizeStr);
    QNetworkRequest req;
    req.setUrl(QUrl(urlString));
    QNetworkReply *reply = imageServer->get(req);

    // request sent so start the timer and event loop
    tT.start(5000); // 5s timeout
    q.exec();

    if(tT.isActive()){
        // download complete
        tT.stop();
    } else {
        DEBUGF("Cover Request Timed Out for " << artist_album);
        reply->deleteLater();
        return;
    }

    QByteArray buff;
    buff = reply->readAll();

    //    p.fromImageReader(&reader);
    if(buff.isEmpty()) { // oops, no image returned, substitute default image
        DEBUGF("returned null image for " << artist_album);
        reply->deleteLater();
        return;
    }

    QFile f;
    QDir d;
    d.setCurrent(cachePath);
    QString fileName = QString("%1.JPG").arg(artist_album.trimmed().toUpper());
    f.setFileName(fileName);

    if(f.open(QIODevice::WriteOnly)) {
        f.write(buff);
        f.close();
    }
    else
        DEBUGF("Failed to open: "  << fileName);

    // NOTE: IMPORTANT to delete the reply **LATER** not now
    reply->deleteLater();
}

QByteArray SlimImageCache::RetrieveCover(const Album &a)
{
    // intended to be run outside of the "run" loop to retrieve images
    // because QPixmaps are "dangerous" outside of the GUI thread, we return
    // a QByteArray of the QPixmap data
    QMutexLocker m(&mutex);
    QFile f;
    QByteArray buf;
    buf.clear();
    QString fileName = QString("%1.JPG").arg(a.artist_album.trimmed().toUpper());

    QDir d;
    d.setCurrent(cachePath);
    f.setFileName(fileName);

    if(f.exists()) {
        if(f.open(QIODevice::ReadOnly)) {
            buf = f.readAll();
            DEBUGF("buf is " << buf.size() << " Bytes");
            f.close();
        }
    }
    return buf;
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
    imageServer = new QNetworkAccessManager();
    mutex.unlock();
    while(isrunning) {
        if(albumList.isEmpty()) {
            condition.wait(&mutex);
        }
        else {
            mutex.lock();
            DEBUGF("Checking for images");

            QDir d;
            d.setCurrent(cachePath);
            QFile f;
            QString fileName;
            QListIterator<Album> it(albumList);
            while(it.hasNext()) {
                Album a = it.next();
                fileName = QString("%1.JPG").arg(a.artist_album.trimmed().toUpper());
                f.setFileName(fileName);
                if(!f.exists()) {
                    if(!a.coverid.isEmpty())
                        RequestArtwork(a.coverid,a.artist_album.trimmed().toUpper());
                }
            }
            emit ImagesReady(); // signal that we're done
            albumList.clear();  // empty album list so we return to a wait state

            mutex.unlock();
        }   // end else

    }
}
