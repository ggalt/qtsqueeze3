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

#include "squeezedefines.h"

class SlimDatabaseFetch : public QThread
{
    Q_OBJECT
public:
    SlimDatabaseFetch(QObject *parent = 0);
    ~SlimDatabaseFetch(void);

    void Init(QString serveraddr, qint16 cliport, qint16 httpport, QString cliuname = NULL, QString clipass = NULL);

signals:
    void FinishedUpdatingDatabase(DatabaseInfo&);

public slots:
    void cliConnected(void);
    void cliDisconnected(void);
    void cliError(QAbstractSocket::SocketError socketError);
    void cliMessageReady(void);


protected:
    void run();

private:
    bool ProcessResponse(void);
    void RemoveNewLineFromResponse(void);

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


    quint16 httpPort;         // port for http connection to retrieve images, usually 9000.
    QString imageSizeStr;        // text representation of the image size we want (e.g., "/cover_200x200")
};

#endif // SLIMDATABASEFETCH_H
