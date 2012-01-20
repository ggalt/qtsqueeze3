#ifndef SLIMDATABASEFETCH_H
#define SLIMDATABASEFETCH_H

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTcpSocket>
#include <QTimer>
#include <QTime>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QImageReader>

#include "squeezedefines.h"

class SlimDatabaseFetch : public QThread
{
    Q_OBJECT
public:
    SlimDatabaseFetch(QObject *parent = 0);
    ~SlimDatabaseFetch(void);

    void Init(QString serveraddr, qint16 cliport, qint16 httpport, QString cliuname = NULL, QString clipass = NULL);

    SlimAlbumItem AlbumArtist2AlbumInfo(void) {return m_AlbumArtist2AlbumInfo;}
    SlimItemList Artist2AlbumIds(void) {return m_Artist2AlbumIds;}
    SlimImageItem Id2Art(void) {return m_Id2Art;}
    SlimAlbumItem AlbumID2AlbumInfo(void) {return m_AlbumID2AlbumInfo;}    // AlbumID to Album Info


signals:
    void FinishedUpdatingDatabase(void);

public slots:
    void cliConnected(void);
    void cliDisconnected(void);
    void cliError(QAbstractSocket::SocketError socketError);
    void cliMessageReady(void);
    void ArtworkReqply(QNetworkReply*);


protected:
    void run();

private:
    bool ProcessResponse(void);
    void RemoveNewLineFromResponse(void);
    void RequestArtwork(QByteArray coverID);

    QMutex mutex;
    QWaitCondition condition;

    DatabaseInfo s;

    QTcpSocket *slimCliSocket;// socket for CLI interface
    QByteArray response;      // buffer to hold CLI response
    QList<QByteArray> responseList; // command response processed into "tag - response" pairs
    QString SlimServerAddr;   // server IP address
    quint16 cliPort;          // port to use for cli, usually 9090, but we allow the user to change this
    QString cliUsername;      // username for cli if needed
    QString cliPassword;      // password for cli if needed **NOTE: DANGER, DANGER this is done in clear text, so don't use a password you care about!!
    bool useAuthentication;   // test for using authentication
    bool isAuthenticated;     // have we been authenticated?
    QByteArray MaxRequestSize;      // max size of any cli request (used for limiting each request for albums, artists, songs, etc., so we don't time out or overload things)
    int iTimeOut;             // number of milliseconds before CLI blocking requests time out

    SlimAlbumItem m_AlbumArtist2AlbumInfo;         // Album+Artist name to Album info (including coverid)
    SlimItemList m_Artist2AlbumIds;    // Artist name to list of albums
    SlimImageItem m_Id2Art;       // coverid to artwork
    SlimAlbumItem m_AlbumID2AlbumInfo;    // AlbumID to Album Info

    QNetworkAccessManager *slimHttp;
    quint16 httpPort;         // port for http connection to retrieve images, usually 9000.
    QString imageSizeStr;        // text representation of the image size we want (e.g., "/cover_200x200")
    QHash< QNetworkReply*,QByteArray > httpReplyList; // associate image request with coverid

    bool finishedData;
    bool finishedImages;
};

#endif // SLIMDATABASEFETCH_H
