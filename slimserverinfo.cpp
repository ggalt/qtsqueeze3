#include "slimserverinfo.h"
#include "slimcli.h"
#include "slimdevice.h"

// uncomment the following to turn on debugging
// #define SLIMCLI_DEBUG

#ifdef SLIMCLI_DEBUG
#define DEBUGF(...) qDebug() << __VA_ARGS__
#else
#define DEBUGF(...)
#endif

#define PATH "./.qtsqueezeimage.dat"

SlimServerInfo::SlimServerInfo(QObject *parent) :
    QObject(parent)
{
    httpPort = 9000;
}

SlimServerInfo::~SlimServerInfo()
{
    WriteImageFile();
}

bool SlimServerInfo::Init(SlimCLI *cliRef)
{
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
}

bool SlimServerInfo::SetupDevices( void )
{
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

    for( int i = 0; i < playerCount; i++ ) {
        cmd = QString( "player id %1 ?\n" ).arg( i ).toAscii();
        if( !cli->SendBlockingCommand( cmd ) )
            return false;
        response = cli->GetResponse();

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
        respList.clear();
        respList = response.split( ' ' );
        QString thisName = respList.at( 3 );
        //            GetDeviceNameList().insert( thisName, thisId );  // insert hash of key=Devicename value=MAC address

        cmd = QString( "player ip %1 ?\n" ).arg( i ).toAscii();

        if( !cli->SendBlockingCommand( cmd ) )
            return false;
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
    //    }
    //    if( loopCounter >= 5 ) // we failed above
    //        return false;
    //    else
    //        return true;
}

void SlimServerInfo::InitDevices( void )
{
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
    return deviceList.value( QString( mac ), NULL );
}

bool SlimServerInfo::ProcessServerInfo(QByteArray response)
{
    QList<QByteArray> aList = response.split( ' ' );  // split response into various pairs

    QListIterator<QByteArray> fields(aList);
    while(fields.hasNext()){
        QString line = QString(fields.next());
        if(line.section("%3A",0,0)=="lastscan")
            lastRefresh = line.section("%3A",1,1).toAscii().toInt();
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

QPixmap SlimServerInfo::GetAlbumArt( QString album, QString artist )
{
    QString coverID = AlbumArtist2AlbumInfo().value(album.trimmed()+artist.trimmed()).coverid;
    if(coverID.isNull()) {  // no cover art id available

    }
}

QList<Album> SlimServerInfo::GetArtistAlbumList(QString artist)
{

}

bool SlimServerInfo::ReadImageFile( void )
{
    QFile file;
    ImageFile imgFile;
    if( file.exists( PATH ) ) // there is a file, so read from it
        file.setFileName( PATH );
    else
        return false;

    //update the images
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);   // read the data serialized from the file
    in >> lastRefresh;
    in >> serverImageList;
    DEBUGF( "Reading file of size: " << file.size() );
    file.close();
//    lastRefresh = imgFile.refreshDate;
//    serverImageList = imgFile.imgList;
    return true;
}

void SlimServerInfo::WriteImageFile( void )
{
    QFile file;
    ImageFile imgFile;
    file.setFileName( PATH );

    //update the images
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);   // read the data serialized from the file
//    imgFile.refreshDate = lastRefresh;
//    imgFile.imgList = serverImageList;
    out << lastRefresh;
    out << serverImageList;
//    out << imgFile;
    DEBUGF( "Writing file of size: " << file.size() );
    file.close();
    return;
}

bool SlimServerInfo::checkRefreshDate(void)
{
    if(GetLastRefresh()!=lastRefresh)
        bNeedRefresh = true;
    else
        bNeedRefresh = false;
    lastRefresh = GetLastRefresh();  // this way we'll feed the latest refresh date into the file for next time
    return bNeedRefresh;
}

bool SlimServerInfo::refreshImageFromServer(void)
{


}
