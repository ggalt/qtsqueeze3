#include <QDir>
#include <QFile>

#include "slimserverinfo.h"
#include "slimcli.h"
#include "slimdevice.h"
#include "slimdatabasefetch.h"

#ifdef SLIMSERVERINFO_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif

SlimServerInfo::SlimServerInfo(QObject *parent) :
    QObject(parent)
{
    setObjectName("SlimServerInfo");
    DEBUGF("");
//    httpPort = 9000;
    freshnessDate = 0;
}

SlimServerInfo::~SlimServerInfo()
{
    DEBUGF("");
    WriteDataFile();
}

bool SlimServerInfo::Init(SlimCLI *cliRef)
{
    DEBUGF("");
    // get cli and references sorted out
    cli = cliRef;
    SlimServerAddr = cli->GetSlimServerAddress();
    cliPort = cli->GetCliPort();

    cli->EmitCliInfo(QString("Getting Database info"));
    QByteArray cmd = QByteArray("serverstatus\n" );
    if(!cli->SendBlockingCommand(cmd))
        return false;
    if( !ProcessServerInfo(cli->GetResponse()) )
        return false;

    if( !SetupDevices())
        return false;

    if( !ReadDataFile() )
        refreshImageFromServer();
    else
        checkRefreshDate();     // see if we need to update the database
    return true;
}

bool SlimServerInfo::SetupDevices( void )
{
    DEBUGF("");
    QByteArray response;
    QList<QByteArray> respList;
    QByteArray cmd;

    cli->EmitCliInfo( "Analyzing Attached Players" );

    //    int loopCounter = 0;
    //    while(loopCounter++ < 5 ) {

    if( playerCount <= 0 ) { // we have a problem
        DEBUGF( "NO DEVICES" );
        return false;
    }
    DEBUGF("player count is:" << playerCount);

    for( int i = 0; i < playerCount; i++ ) {
        cmd = QString( "player id %1 ?\n" ).arg( i ).toAscii();
        if( !cli->SendBlockingCommand( cmd ) )
            return false;
        response = cli->GetResponse();
        DEBUGF(response);

        respList.clear();
        respList = response.split( ' ' );

        QString thisId;
        if( respList.at( 3 ).contains( ':' ) )
            thisId = respList.at( 3 ).toLower().toPercentEncoding(); // escape encode the MAC address if necessary
        else
            thisId = respList.at( 3 ).toLower();

        cmd = QString( "player name %1 ?\n" ).arg( i ).toAscii();

        if( !cli->SendBlockingCommand( cmd ) )
            return false;
        response = cli->GetResponse();
        respList.clear();
        respList = response.split( ' ' );
        DEBUGF(response);
        QString thisName = respList.at( 3 );
        //            GetDeviceNameList().insert( thisName, thisId );  // insert hash of key=Devicename value=MAC address

        cmd = QString( "player ip %1 ?\n" ).arg( i ).toAscii();

        if( !cli->SendBlockingCommand( cmd ) )
            return false;
        response = cli->GetResponse();
        DEBUGF(response);
        respList.clear();
        respList = response.split( ' ' );
        QString deviceIP = respList.at( 3 );
        deviceIP = deviceIP.section( QString( "%3A" ), 0, 0 );
        DEBUGF( thisId.toAscii() << " | " << thisName.toAscii()  << " | " << deviceIP.toAscii()  << " | " << i );
        deviceList.insert( thisId, new SlimDevice( thisId.toAscii(), thisName.toAscii(), deviceIP.toAscii(), QByteArray::number( i ) ) );
    }
    if(!deviceList.contains( cli->GetMACAddress().toLower()))
        return false;
    else
        return true;
}

void SlimServerInfo::InitDevices( void )
{
    DEBUGF("");
    cli->EmitCliInfo( QString( "Initializing attached devices" ) );

    QHashIterator< QString, SlimDevice* > i( deviceList );
    while (i.hasNext()) {
        i.next();
        i.value()->Init( cli );
        DEBUGF( "Initializing device: " << i.value()->getDeviceMAC() );
    }
    QByteArray command = "subscribe playlist,mixer,pause,sync,client \n";  // subscribe to playlist messages so that we can get info on it.

    cli->SendCommand( command ); // this will cause us to get messages on playlist change
    // displaystatus subscribe:all is command to get messages
    // we need to issue the command for each attached device, and since we're already at the end of the list, let's just go backwards

    while (i.hasPrevious()) {
        i.previous();
        command = i.value()->getDeviceMAC() + " displaystatus subscribe:all";
        DEBUGF( "SENDING DISPLAY COMMAND FOR DEVICE " << command );
        cli->SendCommand( command );
    }
    emit FinishedInitializingDevices();
}

SlimDevice *SlimServerInfo::GetDeviceFromMac( QByteArray mac )   // use percent encoded MAC address
{
    DEBUGF("");
    return deviceList.value( QString( mac ), NULL );
}

bool SlimServerInfo::ProcessServerInfo(QByteArray response)
{
    DEBUGF("");
    QList<QByteArray> aList = response.split( ' ' );  // split response into various pairs

    QListIterator<QByteArray> fields(aList);
    while(fields.hasNext()){
        QString line = QString(fields.next());
        if(line.section("%3A",0,0)=="lastscan")
            lastServerRefresh = line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="version")
            serverVersion=line.section("%3A",1,1).toAscii();
        else if(line.section("%3A",0,0)=="info%20total%20albums")
            totalAlbums=line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="info%20total%20artists")
            totalArtists=line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="info%20total%20genres")
            totalGenres=line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="info%20total%20songs")
            totalSongs=line.section("%3A",1,1).toAscii().toInt();
        else if(line.section("%3A",0,0)=="player%20count")
            playerCount=line.section("%3A",1,1).toAscii().toInt();
    }
    return true;

}

// ----------------------------------------------------------------------------------
//  Image processing

//QPixmap SlimServerInfo::GetAlbumArt( QString album, QString artist )
//{
//    QString coverID = AlbumArtist2AlbumInfo().value(album.trimmed()+artist.trimmed()).coverid;\
//    return Id2Art().value(coverID);
//}

//QList<Album> SlimServerInfo::GetArtistAlbumList(QString artist)
//{

//}

bool SlimServerInfo::ReadDataFile( void )
{
    DEBUGF("");
    QString dataPath = QDir::homePath()+DATAPATH;
    QDir d(dataPath);
    if(!d.exists())
        d.mkpath(dataPath);
    QDir::setCurrent(dataPath);

    QFile file;
    if( file.exists(DATAFILE) ) // there is a file, so read from it
        file.setFileName(DATAFILE);
    else {
        DEBUGF("no file to read at " << dataPath+DATAFILE);
        return false;
    }

    quint16 albumCount;

    //update the images
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);   // read the data serialized from the file
    in >> freshnessDate;
    in >> m_Artist2AlbumIds;
    in >> m_AlbumArtist2AlbumID;
    in >> albumCount;

    for(int t=0; t<albumCount; t++) {
        Album a;
        in >> a.albumtitle;
        in >> a.album_id;
        in >> a.artist;
        in >> a.artist_album;
        in >> a.artist_id;
        in >> a.coverid;
        in >> a.year;
        m_albumList.append(a);
        DEBUGF("READING:"<< a.albumtitle << a.album_id << a.artist << a.artist_album << a.artist_id << a.coverid << a.year);
        DEBUGF("ALBUM NUMBER:" <<t);
    }

    DEBUGF("reading in info on " << albumCount << " files");

    m_AlbumID2AlbumInfo.clear();
    for( int c = 0; c < albumCount; c++ ) {
        QString key;
        Album a;
        in >> key;
        in >> a.albumtitle;
        in >> a.album_id;
        in >> a.artist;
        in >> a.artist_album;
        in >> a.artist_id;
        in >> a.coverid;
        in >> a.year;
        m_AlbumID2AlbumInfo.insert(key,a);
    }
#ifdef SLIMCLI_DEBUG
    QHashIterator< QString, Album > aa(m_AlbumID2AlbumInfo);
    aa.toBack();
    while(aa.hasPrevious()) {
        aa.previous();
        Album a = aa.value();
    }
#endif

    DEBUGF( "Reading file of size: " << file.size() );
    file.close();
    return true;
}

void SlimServerInfo::WriteDataFile( void )
{
    DEBUGF("");
    QString dataPath = QDir::homePath()+DATAPATH;
    QDir d(dataPath);
    if(!d.exists())
        d.mkpath(dataPath);
    QDir::setCurrent(dataPath);

    QFile file;
    file.setFileName(DATAFILE);

    //update the images
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);   // read the data serialized from the file
    out << lastServerRefresh;
    out << m_Artist2AlbumIds;
    out << m_AlbumArtist2AlbumID;
    out << (qint16)m_AlbumID2AlbumInfo.count();

    QListIterator<Album> al(m_albumList);
    DEBUGF("m_albumList count is" << m_albumList.count());
    al.toBack();
    while(al.hasPrevious()) {
        Album a = al.previous();
        DEBUGF("WRITING:"<< a.albumtitle << a.album_id << a.artist << a.artist_album << a.artist_id << a.coverid << a.year);
        out << a.albumtitle << a.album_id << a.artist << a.artist_album << a.artist_id << a.coverid << a.year;
    }

    QHashIterator< QString, Album > aa(m_AlbumID2AlbumInfo);
    aa.toBack();
    while(aa.hasPrevious()) {
        aa.previous();
        Album a = aa.value();
        out << aa.key() << a.albumtitle << a.album_id << a.artist << a.artist_album << a.artist_id << a.coverid << a.year;
    }

    DEBUGF( "Writing file of size: " << file.size() );
    file.close();
    return;
}

void SlimServerInfo::checkRefreshDate(void)
{
    DEBUGF("");
    if(lastServerRefresh!=freshnessDate)
        refreshImageFromServer();
}

void SlimServerInfo::refreshImageFromServer(void)
{
    DEBUGF("");
    db = new SlimDatabaseFetch();
    connect(db,SIGNAL(FinishedUpdatingDatabase()),
            this,SLOT(DatabaseUpdated()));

    db->Init(SlimServerAddr,cliPort,cli->GetCliUsername(),cli->GetCliPassword());
    db->start();    // init database fetching thread
}

void SlimServerInfo::DatabaseUpdated(void)
{
    DEBUGF("");
    m_AlbumArtist2AlbumID = db->AlbumArtist2AlbumID();
    m_AlbumID2AlbumInfo = db->AlbumID2AlbumInfo();
    m_Artist2AlbumIds = db->Artist2AlbumIds();
    m_albumList = db->GetAllAlbumList();
    db->exit();
    db->deleteLater();
}
