#include "slimcli.h"

#include <QApplication>
#include <QtAlgorithms>
#include "slimcli.h"
#include "slimdevice.h"

// uncomment the following to turn on debugging
// #define SLIMCLI_DEBUG

#ifdef SLIMCLI_DEBUG
#define DEBUGF(...) qDebug() << __VA_ARGS__
#else
#define DEBUGF(...)
#define VERBOSE(...)
#endif

#define PATH "./qtsqueezeimage.dat"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ---------------------------- OBJECT INITIALIZATION ---------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

SlimCLI::SlimCLI( QObject *parent, const char *name ) : QObject( parent ) {
    macAddress = '\0';
    ipAddress = '\0';
    SlimServerAddr = "127.0.0.1";
    cliPort = 9090;        // default, but user can reset
    httpPort = 9000;        // default, but user can reset
    MaxRequestSize = "100";    // max size of any cli request (used for limiting each request for albums, artists, songs, etc., so we don't time out or overload things)
    iTimeOut = 10000;             // number of milliseconds before CLI blocking requests time out
    useAuthentication = false;  // assume we don't need it unless we do
    isAuthenticated = true;   // we will assume that authentication is not used (and therefore we have been authenticated!!)
    maintainConnection = true;
    imageSize = 200;
    imageSizeStr = "/cover_200x200";
    httpID = 0;               // ID of HTTP transaction
    // NOTE: Init() must be called explicitly **after** we've set a few additional items such as the username and password if any
}

SlimCLI::~SlimCLI(){
    maintainConnection = false;
    disconnect( slimCliSocket, SIGNAL(disconnected()),      this, SLOT( LostConnection() ) );
    SendCommand ( "exit" );  // shut down CLI interface
    slimCliSocket->flush();
    delete slimCliSocket;
    delete imageURL;
    WriteImageFile(); // save images
}


void SlimCLI::Init( void )
{
    imageURL = new QHttp( this );
    imageURL->setHost( SlimServerAddr, httpPort );

    slimCliSocket = new QTcpSocket( this );
    bool ok;
    qint64 buffsize = MaxRequestSize.toInt( &ok ) * 5000;  // initialize the read buffer to read up to MaxRequestSize * 5000 bytes
    if( !ok )
        buffsize = 500000;
    slimCliSocket->setReadBufferSize( buffsize );


    if( !cliUsername.isEmpty() && !cliPassword.isEmpty() ) { // we need to authenticate
        useAuthentication = true;
        isAuthenticated = false;  // will be reset later if we succeed
    }

    emit cliInfo( QString( "Loading Images" ) );

    ReadImageFile();  // read in images

    connect( slimCliSocket, SIGNAL(connected()),
             this, SLOT( CLIConnectionOpen() ) );
    connect( slimCliSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), \
             this, SLOT( ConnectionError( QAbstractSocket::SocketError ) ) );

    emit cliInfo( QString( "Connecting to Squeezebox Server" ) );
    Connect();    // connect to host
    //NOTE: WE HAVE NOT YET CONNECTED THE READYREAD SIGNAL TO THE MSGWAITING SLOT SO THAT WE CAN DO SOME BLOCKING CALLS TO THE CLI
}

bool SlimCLI::SetupDevices( void )
{
    emit cliInfo( "Analyzing Attached Players" );
    QByteArray cmd = QByteArray("player count ?\n" );

    int loopCounter = 0;

    while( !deviceList.contains( this->macAddress.toLower() ) && loopCounter++ < 5 ) {
        if( !SendBlockingCommand( cmd ) ) {
            QString myMsg = QString( "Error sending blocking command :" ) + cmd;
            DEBUGF( myMsg );
            return false;
        }

        QList<QByteArray> a = response.split( ' ' );  // response should be "player count #", split into 3 fields

        int playerCount = QString( a.at(2) ).trimmed().toInt();

        if( playerCount <= 0 ) { // we have a problem
            DEBUGF( "NO DEVICES" );
            return false;
        }

        for( int i = 0; i < playerCount; i++ ) {
            cmd = QString( "player id %1 ?\n" ).arg( i ).toAscii();
            if( !SendBlockingCommand( cmd ) )
                return false;

            a.clear();
            a = response.split( ' ' );
            QString thisId;
            if( a.at( 3 ).contains( ':' ) )
                thisId = a.at( 3 ).toLower().toPercentEncoding(); // escape encode the MAC address if necessary
            else
                thisId = a.at( 3 ).toLower();

            cmd = QString( "player name %1 ?\n" ).arg( i ).toAscii();

            if( !SendBlockingCommand( cmd ) )
                return false;
            a.clear();
            a = response.split( ' ' );
            QString thisName = a.at( 3 );
            GetDeviceNameList().insert( thisName, thisId );  // insert hash of key=Devicename value=MAC address

            cmd = QString( "player ip %1 ?\n" ).arg( i ).toAscii();

            if( !SendBlockingCommand( cmd ) )
                return false;
            a.clear();
            a = response.split( ' ' );
            QString deviceIP = a.at( 3 );
            deviceIP = deviceIP.section( QString( "%3A" ), 0, 0 );
            DEBUGF( thisId.toAscii() << " | " << thisName.toAscii()  << " | " << deviceIP.toAscii()  << " | " << i );
            deviceList.insert( thisId, new SlimDevice( thisId.toAscii(), thisName.toAscii(), deviceIP.toAscii(), QByteArray::number( i ) ) );
        }
    }
    if( loopCounter >= 5 ) // we failed above
        return false;
    else
        return true;
}

void SlimCLI::InitDevices( void )
{
    emit cliInfo( QString( "Initializing attached devices" ) );
    QHashIterator< QString, SlimDevice* > i( deviceList );
    while (i.hasNext()) {
        i.next();
        i.value()->Init( this );
        DEBUGF( "Initializing device: " << i.value()->getDeviceMAC() );
    }
    command = "subscribe playlist,mixer,pause,sync,client \n";  // subscribe to playlist messages so that we can get info on it.

    SendCommand( command ); // this will cause us to get messages on playlist change
    // displaystatus subscribe:all is command to get messages
    // we need to issue the command for each attached device, and since we're already at the end of the list, let's just go backwards

    while (i.hasPrevious()) {
        i.previous();
        command = i.value()->getDeviceMAC() + " displaystatus subscribe:all";
        DEBUGF( "SENDING DISPLAY COMMAND FOR DEVICE " << command );
        SendCommand( command );
    }
    emit FinishedInitializingDevices();
}

bool SlimCLI::SetupLogin( void ){
    command.clear();
    QString cmd = QString ("login %1 %2\n" )
                  .arg( QString( cliUsername.toLatin1() ) )
                  .arg( QString( cliPassword.toLatin1() ) );
    command.append( cmd );

    if( !waitForResponse() ) // NOTE: WE HAVE NO NEED TO PROCESS THE RESPONSE SINCE SUCCESS GIVES NO INFO AND FAILURE TRIGGERS A DISCONNECT
        return false;
    else
        return true;
}

void SlimCLI::InitImageCollection( void )
{
    coverIterator = new QHashIterator< QString, QPixmap >( serverImageList );
    connect( this, SIGNAL( ImageReceived(int) ), this, SLOT( slotImageCollection() ) );
    slotImageCollection();
}

void SlimCLI::slotImageCollection( void )
{
    if( coverIterator->hasNext() ) {
        coverIterator->next();
        AlbumArtAvailable( coverIterator->key() );
    }
    else  // if we are done, disconnect
        disconnect( this, SIGNAL( ImageReceived(int) ), this, SLOT( slotImageCollection() ) );
}

void SlimCLI::ReadImageFile( void )
{
    QFile file;
    if( file.exists( PATH ) ) // there is a file, so read from it
        file.setFileName( PATH );
    else
        return;

    //update the images
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);   // read the data serialized from the file
    in >> serverImageList;
    DEBUGF( "Reading file of size: " << file.size() );
    file.close();
    return;
}

void SlimCLI::WriteImageFile( void )
{
    QFile file;
    file.setFileName( PATH );

    //update the images
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);   // read the data serialized from the file
    out << serverImageList;
    DEBUGF( "Writing file of size: " << file.size() );
    file.close();
    return;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ---------------------------- COMMUNICATION WITH SERVER AND SLOTS -------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

void SlimCLI::Connect( void ){
    QString myMsg = QString( "Connecting to %1 on port %2" ).arg( SlimServerAddr ).arg( cliPort );
    DEBUGF( myMsg );
    slimCliSocket->connectToHost( SlimServerAddr, cliPort );
}

bool SlimCLI::CLIConnectionOpen( void ){
    // NOTE: these commands are sent from here rather than using the SendCommand() function because we DO NOT want to place the MAC address in front of them
    // we'll use a blocking call for authentication and grabbing the available players because it's easier and it's quick

    emit cliInfo( QString( "Establishing Connection to Squeezebox Server" ) );
    DEBUGF( "Authenticating" );
    if( useAuthentication )
        if( !SetupLogin() ) {
        emit cliError( 0, QString( "No login" ) );
        return false;
    }

    isAuthenticated = true;   // we've made it to here, so we are authenticated (either through u/p combo, or because there is no u/p

    connect( imageURL, SIGNAL(requestFinished(int,bool)),
             this, SLOT(slotImageReceived(int,bool)) );

    DEBUGF( "Device Setup" );
    if( !SetupDevices() ) {
        emit cliError( 0, QString( "Error collecting information on devices attached to server" ) );
        return false;
    }

    connect( slimCliSocket, SIGNAL(readyRead()),         this, SLOT( msgWaiting() ) );
    connect( slimCliSocket, SIGNAL(disconnected()),      this, SLOT( LostConnection() ) );

    DEBUGF( "Device Init" );
    InitDevices();    // this will initalize the devices and set up receiving playing and display messages from the cli

    return true;
}

void SlimCLI::LostConnection( void ){
    // just in case we want to restart things, let's first disconnect the signals and slots that will be established at successful connection
    disconnect( slimCliSocket, SIGNAL(readyRead()), 0, 0 );
    disconnect( slimCliSocket, SIGNAL(disconnected()), 0, 0 );
    disconnect( imageURL, SIGNAL( requestFinished ( int, bool ) ), 0, 0 );

    if( !isAuthenticated && useAuthentication ) // we probably lost the connection due to a bad password
        emit cliError( NOLOGIN );
    if( maintainConnection ) {
        // VERBOSE( VB_IMPORTANT, "fixing lost connection" );
        Connect();
    }
}

void SlimCLI::ConnectionError( int err ){
    emit cliError( CONNECTION_ERROR );
    QString myMsg = QString( "Connection error: %1" ).arg( err );
    // VERBOSE( VB_IMPORTANT, myMsg );
}

void SlimCLI::ConnectionError( QAbstractSocket::SocketError err ){
    emit cliError( CONNECTION_ERROR );
    QString myMsg = QString( "Connection error: %1" ).arg( slimCliSocket->errorString() );
    // VERBOSE( VB_IMPORTANT, myMsg );
}

void SlimCLI::SentBytes( int b ){
    DEBUGF( "Bytes written to socket: " << b );
}

bool SlimCLI::SendCommand( QByteArray c ){
    // NOTE:: SendCommand assumes that the command string has been filled in and already been put in URL escape form
    // and that a MAC address is at the beginning of the command (if needed only!!)
    if( !c.isNull() )
    {
        command.clear();
        command = c;
    }

    if( !command.trimmed().endsWith( "\n" ) ) // need to terminate with a \n
        command = command.trimmed() + "\n";

    if( slimCliSocket->write( command ) > 0 ) {
        return true;
    }
    else
        return false;
}

bool SlimCLI::SendBlockingCommand( QByteArray c ) {
    // NOTE:: SendCommand assumes that the command string has been filled in and already been put in URL escape form
    // and that a MAC address is at the beginning of the command (if needed only!!)
    // REMEMBER, THIS BLOCKS UNTIL A RETURN IS RECEIVED!! USE WITH CARE

    if( !c.isNull() )
    {
        command.clear();
        command = c;
    }

    if( !command.trimmed().endsWith( "\n" ) ) // need to terminate with a \n
        command = command.trimmed() + "\n";

    return waitForResponse();
}

bool SlimCLI::waitForResponse( void ){
    slimCliSocket->write( command );
    if( !slimCliSocket->waitForReadyRead() )
        return false;
    QTime t;
    t.start();

    QByteArray lineBuf = "";

    while( t.elapsed() < iTimeOut ) {
        if( slimCliSocket->canReadLine() ) {    // most times, we're going to stop here, but if the socket receives a large amount of data, it seems to need to dump it in chucks, hence the line below.
            lineBuf += slimCliSocket->readLine();
            break;
        }
        else {  // if we haven't received the \n yet, take what we've got and loop.  Sometimes the socket seems to get clogged with data otherwise.  We'll capture the rest of the line above, if it comes before we time out.
            lineBuf += slimCliSocket->read( slimCliSocket->bytesAvailable() );
            DEBUGF( "LESS THAN A FULL LINE RECEIVED: " << lineBuf );
        }
    }
    if( t.elapsed() >= iTimeOut ) {
        QString myMsg = QString( "Read line of %1 length, with the following info: %2").arg( lineBuf.size() ).arg ( QString( lineBuf ) );
        // VERBOSE( VB_IMPORTANT, myMsg );
        QString errmsg = QString( "Connection timed out with current timeout limit of %1 milliseconds.  Consider increasing." ).arg( iTimeOut );
        emit cliError( CONNECTION_TIMEOUT, errmsg );
        return false;
    }
    response = lineBuf;
    RemoveNewLineFromResponse();

    return true;
}

bool SlimCLI::msgWaiting( void ){
    QTime t;
    t.start();
    while( slimCliSocket->bytesAvailable() && t.elapsed() < iTimeOut ) {  // we need to loop because we often get new messages while processing old and "readyread" misses them -- better to move socket to its own thread, but this works for now
        while( !slimCliSocket->canReadLine () ) { // wait for a full line of content  NOTE: protect against unlikely infinite loop with timer
            if( t.elapsed() > iTimeOut ) {
                // VERBOSE( VB_IMPORTANT, "Error: timed out waiting for a full line from server" );
                return false;
            }
        }

        if( slimCliSocket->bytesAvailable() > response.size() - 1 ) {
            response.resize( slimCliSocket->bytesAvailable() + 1 );
        }
        response = slimCliSocket->readLine();
        RemoveNewLineFromResponse();

        QRegExp MACrx( "[0-9a-fA-F][0-9a-fA-F]%3A[0-9a-fA-F][0-9a-fA-F]%3A[0-9a-fA-F][0-9a-fA-F]%3A[0-9a-fA-F][0-9a-fA-F]%3A[0-9a-fA-F][0-9a-fA-F]%3A[0-9a-fA-F][0-9a-fA-F]" );

        if( MACrx.indexIn( QString( response ) ) >= 0 ) { // if it starts with a MAC address, send it to a device for processing
            DeviceMsgProcessing();
        }
        else {
            SystemMsgProcessing();
        }
    }
    return true;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ---------------------------- MESSAGE PROCESSING ------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


void SlimCLI::DeviceMsgProcessing( void )
{
    DEBUGF( "Device Message: " << this->response );

    if( deviceList.contains( MacAddressOfResponse() ) ) { // check to see if the MAC address corresponds to a known player
        QByteArray res = ResponseLessMacAddress();
        if( res.startsWith( "status" ) ) {  // status message
            deviceList.value( MacAddressOfResponse() )->ProcessStatusSetupMessage( res );
        }
        else if( res.startsWith( "displaystatus" ) ) { // display status message
            deviceList.value( MacAddressOfResponse() )->ProcessDisplayStatusMsg( res );
        }
        else {
            deviceList.value( MacAddressOfResponse() )->ProcessPlayingMsg( res );
        }
    }
    else {  // wait!  Whose MAC address is this?
        QString myMsg = QString( "Unknown MAC address: %1" ).arg( QString( MacAddressOfResponse() ) );
        // VERBOSE( VB_IMPORTANT, myMsg );
        // addNewDevice( MacAddressOfResponse() );
    }
}

void SlimCLI::SystemMsgProcessing( void )
{
    DEBUGF( "SYSTEM MESSAGE: " << this->response );
}

void SlimCLI::ProcessLoginMsg( void )
{
    if( response.left( 5 ) == "login" )
        isAuthenticated = true;
}

void SlimCLI::ProcessControlMsg( void )
{
    DEBUGF( "CONTROLLING MODE message received: " << response );
    responseList = response.split( ' ' );   // break this up into fields delimited by spaces
    if( response.left( 7 ) == "artists" ) {  // it's processing artist information
        if( responseList.at( 3 ).left( 9 ) == "artist_id" ) {
            for( int c = 3; c < responseList.size(); c++ ) {
            }
        }
        return;
    }

    if( response.left( 6 ) == "albums"  )  // its processing album information
        return;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ---------------------------- COVER ART FUNCTIONS -----------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


QPixmap SlimCLI::GetAlbumArt( QString albumArtID, bool waitforrequest )
        // note, this blocks for up to 3 seconds (should make this value user configurable) as default
{
    QTime t;
    t.start();
    if( !AlbumArtAvailable( albumArtID ) && waitforrequest ) { // check to see if we already have the image, and if not, start process of getting it
        while( serverImageList.find( albumArtID ) == serverImageList.end() && t.elapsed() < 3000 ) {  // albumArtID isn't in current hash
            qApp->processEvents();    // allow event queue to move
        }
    }
    if( serverImageList.find( albumArtID ) == serverImageList.end() ) { // OK, we timed out and still didn't get an image, so return a blank image
        return QPixmap( imageSize, imageSize );
    }
    else {
        return serverImageList.value( albumArtID );   // return the image from the hash table.  This will return a default if no image available
    }
}

bool SlimCLI::AlbumArtAvailable( QString albumArtID ) // call to see if image is currently available (it will ask for it and return false if not available)
{
    DEBUGF( "Requesting Album Art ID: " << albumArtID );
    if( serverImageList.find( albumArtID ) == serverImageList.end() ) {  // albumArtID isn't in current hash
        DEBUGF( "Requesting song cover" );
        ba.resize( 0 );
        if( !mybuffer.isOpen() ) {
            mybuffer.setBuffer( &ba );
            mybuffer.open( QIODevice::ReadWrite );
        }
        GetCoverImage( albumArtID, QString( "/music/" + albumArtID + imageSizeStr ), &mybuffer );
        return false;
    }
    else {
        return true;   // image is in the hash table
    }
}

void SlimCLI::GetCoverImage( QString albumArtID, QString thisImageURL, QIODevice *buffer )
{
    if(  useAuthentication ) {  // set up authentication if we need it
        imageURL->setUser( cliUsername, cliPassword );
    }

    httpID = imageURL->get( thisImageURL, buffer );
    HttpRequestImageId.insert( httpID, albumArtID );  // this associates the Album Art ID with the HTTP request, so that we can place the correct ID with the image we are receiving blindly back from the server
}

void SlimCLI::slotImageReceived( int httpId, bool error )
{
    QString imageID = HttpRequestImageId.value( httpId );
    if( !error && imageID.length() > 0 ) {
        DEBUGF( "Success retrieving image :" << imageID << " of size " << ba.size() );
        QPixmap pic;
        pic.loadFromData( ba );
        DEBUGF( "HttpRequest for Image ID: " << imageID );
        serverImageList.insert( imageID, pic );    // this gets the Album Art ID associated with this request from the list of HTTP requests and associates this Album Art ID with the image received from this HTTP request
    }
    else {
        DEBUGF( "*****IMAGE ERROR*****" );
        DEBUGF( "QHTTP Error code: " << imageURL->error() << " with error string: " << imageURL->errorString() );
    }
    mybuffer.close();
    emit ImageReceived( httpId );   // used when we are retrieving a bunch of images
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ---------------------------- HELPER FUNCTIONS --------------------------------------------------
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


bool SlimCLI::SetMACAddress( QString addr )
{
    if( !addr.contains( "%3A" ) ) // not escape encoded

        macAddress = addr.toAscii().toPercentEncoding();
    else
        macAddress = addr.toAscii();
}

bool SlimCLI::SetIPAddress( QString ipAddrs ){
    ipAddress = ipAddrs;
    return true;
}

void SlimCLI::SetSlimServerAddress( QString addr ){
    SlimServerAddr = addr;
}

void SlimCLI::SetCliPort( int port ){
    cliPort = port;
}

void SlimCLI::SetCliPort( QString port ){
    cliPort = port.toInt();
}

void SlimCLI::SetCliUsername( QString username ){
    cliUsername = username;
}

void SlimCLI::SetCliPassword( QString password ){
    cliPassword = password;
}

QByteArray SlimCLI::MacAddressOfResponse( void )
{
    if( response.contains( "%3A" ) )
        return response.left( 27 ).trimmed().toLower();
    else
        return QByteArray();
}

QByteArray SlimCLI::ResponseLessMacAddress( void )
{
    if( response.contains( "%3A" ) )
        return response.right( response.length() - 27 ).trimmed();
    else
        return response.trimmed();
}

void SlimCLI::RemoveNewLineFromResponse( void )
{
    while( response.contains( '\n' ) )
        response.replace( response.indexOf( '\n' ), 1, " " );
}

SlimDevice *SlimCLI::GetDeviceFromMac( QByteArray mac )   // use percent encoded MAC address
{
    return deviceList.value( QString( mac ), NULL );
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */