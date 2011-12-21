#ifndef SLIMCLI_H
#define SLIMCLI_H

// QT4 Headers
#include <QObject>
#include <QTcpSocket>
#include <QHttp>
#include <QBuffer>
#include <QTimer>
#include <QVector>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QByteArray>
#include <QList>
#include <QPixmap>
#include <QFile>
#include <QDataStream>

typedef enum { NOLOGIN=101,
               CONNECTION_ERROR,
               CONNECTION_TIMEOUT,
               SETUP_NOLOGIN,
               SETUP_DATABASEERROR,
               SETUP_NODEVICES,
               SETUP_NOARTISTS,
               SETUP_NOALBUMS,
               SETUP_NOSONGS,
               SETUP_NOYEARS,
               SETUP_NOGENRES,
               SETUP_NOPLAYLISTS
             } cli_error_msg;

class SlimDevice; // forward declaraion

typedef QHash< QString, SlimDevice* > SlimDevices;
typedef QHash< QString, QString > SlimItem;
typedef QHash< QString, QStringList > SlimMultiItem;
typedef QHash< QString, QPixmap> SlimImageItem;

/*
  * NOTE: This class establishes an object to communicate with a SqueezeCenter Server
  * via the SqueezeCenter command line interface (usually located at port 9090).  You
  * MUST set the IP address of the SqueezeCenter server and the port BEFORE you call init().
  * Otherwise it will default to the localhost and port 9090.
*/

class SlimCLI : public QObject {
  Q_OBJECT

public:
  SlimCLI( QObject *parent=0, const char *name = NULL );
  ~SlimCLI();

  void Connect( void );
  void Init( void );

  bool SendCommand( QByteArray cmd = NULL );
  bool SendBlockingCommand( QByteArray cmd = NULL );

  QByteArray GetResponse( void ) { return response; }
  QByteArray MacAddressOfResponse( void );
  QByteArray ResponseLessMacAddress( void );
  void RemoveNewLineFromResponse( void );

  QPixmap GetAlbumArt( QString albumArtID, bool waitforrequest = true );
  bool AlbumArtAvailable( QString albumArtID );

  // ------------------------------------------------------------------------------------------
  bool    SetMACAddress( unsigned char *macaddr );      // used with either a 6 char array, or a full xx:xx:xx:xx structure
  bool    SetMACAddress( QString addr );
  QByteArray GetMACAddress( void ) { return macAddress; }
  // ------------------------------------------------------------------------------------------
  bool    SetIPAddress( QString ipAddrs );
  QString GetIPAddress( void ) { return ipAddress; }
  // ------------------------------------------------------------------------------------------
  void    SetSlimServerAddress( QString addr );
  QString GetSlimServerAddress( void ) { return SlimServerAddr; }
  // ------------------------------------------------------------------------------------------
  void    SetCliPort( int port );
  void    SetCliPort( QString port );
  int     GetCliPort( void ) { return cliPort; }
  // ------------------------------------------------------------------------------------------
  void    SetHttpPort( int port ) { httpPort = port; }
  void    SetHttpPort( QString port ) { httpPort = port.toInt(); }
  int     GetHttpPort( void ) { return httpPort; }
  // ------------------------------------------------------------------------------------------
  void    SetCliUsername( QString username );
  QString GetCliUsername( void ) { return cliUsername; }
  // ------------------------------------------------------------------------------------------
  void    SetCliPassword( QString password );
  QString GetCliPassword( void ) { return cliPassword; }
  // ------------------------------------------------------------------------------------------
  void    SetCliMaxRequestSize( QByteArray max ) { MaxRequestSize = max; }
  void    SetCliTimeOutPeriod( QString t ) { iTimeOut = t.toInt(); }    // NOTE: NO ERROR CHECKING!!!
  // ------------------------------------------------------------------------------------------
  bool    NeedAuthentication( void ) { return useAuthentication; }

  // ------------------------------------------------------------------------------------------
  SlimDevices GetDeviceList( void ) { return deviceList; }
  SlimDevice *GetDeviceFromMac( QByteArray mac );   // use percent encoded MAC address
  SlimDevice *GetCurrentDevice( void ) { return currentDevice; }
  SlimItem GetDeviceMACList( void ) { return deviceMACList; }   // list of Devices connected to SqueezeCenter (holds name and MAC address)
  SlimItem GetDeviceNameList( void ) { return deviceNameList; }
  void SetCurrentDevice( SlimDevice *d ) { currentDevice = d; }
  void InitImageCollection( void );


signals:
  void isConnected( void );               // we're connected to the SqueezeCenter server
  void cliError( int errmsg , const QString &message = "" );
  void cliInfo( QString msg );
  void FinishedInitializingDevices( void );
  void ImageReceived( int httpRequest );

public slots:
  //void cliTimeout( void );
  void slotImageReceived( int httpId, bool err );

private:
  QTcpSocket *slimCliSocket;// socket for CLI interface
  QHttp *imageURL;          // an http object to get the current cover image
  QByteArray ba;            // bytearray for HTTP request buffer
  QBuffer mybuffer;           // buffer for HTTP request
  QString imageSizeStr;        // text representation of the image size we want (e.g., "/cover_200x200")
  int imageSize;            // int version of image size (we'll always have a square)
  int httpID;               // ID of HTTP transaction

  QByteArray command;       // string to build a command (different from "currCommand" below that is used to check what the CLI sends back
  QByteArray response;      // buffer to hold CLI response
  QList<QByteArray> responseList; // command response processed into "tag - response" pairs

  SlimDevices deviceList;
  SlimDevice *currentDevice;  // convenience pointer that we can return with a locked mutex
  SlimItem deviceNameList;    // name to MAC address conversion
  SlimItem deviceMACList;
  SlimImageItem serverImageList;  // hash of images retrieved from server by album_image_id
  QHash< int, QString > HttpRequestImageId;  // hash of HTTP request IDs to albumArtistID (for use in matching returned image requests to artwork IDs)

  QByteArray macAddress;       // NOTE: this is stored in URL escaped form, since that is how we mostly use it.  If you need it in plain text COPY IT to another string and use QUrl::decode() on that string.
  QString ipAddress;        // IP address of this device (i.e., the player)
  QString SlimServerAddr;   // server IP address
  quint16 cliPort;          // port to use for cli, usually 9090, but we allow the user to change this
  quint16 httpPort;         // port for http connection to retrieve images, usually 9000.

  QString cliUsername;      // username for cli if needed
  QString cliPassword;      // password for cli if needed **NOTE: DANGER, DANGER this is done in clear text, so don't use a password you care about!!
  bool useAuthentication;   // test for using authentication
  bool isAuthenticated;     // have we been authenticated?
  QByteArray MaxRequestSize;      // max size of any cli request (used for limiting each request for albums, artists, songs, etc., so we don't time out or overload things)
  int iTimeOut;             // number of milliseconds before CLI blocking requests time out

  bool maintainConnection;  // do we reconnect if the connection is ended (set to false when shutting down so we don't attempt a reconnect
  bool bConnection;         // is connected?

  QHashIterator< QString, QPixmap > *coverIterator;
  void ReadImageFile( void );   // read in existing images (if any) into serverImageList (save bandwidth)
  void WriteImageFile( void );  // write out contents of serverImageList

  void DeviceMsgProcessing( void ); // messages forwarded to devices
  void SystemMsgProcessing( void ); // messages forwarded to the system for processing

  bool SetupLogin( void );
  bool SetupDevices( void );
  void InitDevices( void );

  void ProcessLoginMsg( void );
  void ProcessControlMsg( void );
  void processStatusMsg( void );
  bool waitForResponse( void );
  void GetCoverImage( QString albumArtID, QString thisImageURL, QIODevice *buffer );

private slots:
  bool msgWaiting( void );          // we have a message waiting from the server
  bool CLIConnectionOpen( void );   // CLI interface successfully established
  void LostConnection( void );      // lost connection (check if we want reconnect)
  void ConnectionError( int err );  // error message sent with connection
  void ConnectionError( QAbstractSocket::SocketError );
  void SentBytes( int b );          // bytes sent to SqueezeCenter
  void slotImageCollection( void );
};

#endif // SLIMCLI_H
