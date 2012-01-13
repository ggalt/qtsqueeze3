#include <QHostAddress>

#include "slimdatabasefetch.h"
#include "slimserverinfo.h"

SlimDatabaseFetch::SlimDatabaseFetch(QObject *parent) :
    QThread(parent)
{
    SlimServerAddr = "127.0.0.1";
    cliPort = 9090;        // default, but user can reset
    MaxRequestSize = "100";    // max size of any cli request (used for limiting each request for albums, artists, songs, etc., so we don't time out or overload things)
}

SlimDatabaseFetch::~SlimDatabaseFetch(void)
{
}

void SlimDatabaseFetch::Init(QString serveraddr,
                             qint16 cliport, qint16 httpport,
                             QString cliuname = NULL, QString clipass = NULL)
{
    SlimServerAddr = serveraddr;
    cliPort = cliport;
    httpPort = httpport;
    cliUsername = cliuname;
    cliPassword = clipass;

    slimCliSocket = new QTcpSocket(this);
}

void SlimDatabaseFetch::run()
{
    connect(slimCliSocket,SIGNAL(connected()),
            this,SLOT(cliConnected()));
    connect(slimCliSocket,SIGNAL(disconnected()),
            this,SLOT(cliDisconnected()));
    connect(slimCliSocket,SIGNAL(error(QAbstractSocket::SocketError)),
            this,SLOT(cliError(QAbstractSocket::SocketError)));
    connect(slimCliSocket,SIGNAL(readyRead()),
            this,SLOT(cliMessageReady()));

    slimCliSocket->connectToHost(QHostAddress(SlimServerAddr),cliPort);

    exec();     // start event loop
}


// --------------------------------- SLOTS --------------------------------------

void SlimDatabaseFetch::cliConnected(void)
{
    QByteArray cmd;
    if(!cliUsername.isNull()){ // username present, get authentication
        cmd = QString ("login %1 %2\n" )
                .arg( cliUsername )
                .arg( cliPassword ).toAscii();
        slimCliSocket->write(cmd);      // NOTE: no relevant response given except for a disconnect on error
    }
}

void SlimDatabaseFetch::cliDisconnected(void)
{
    qDebug() << "CLI disconnected";
}

void SlimDatabaseFetch::cliError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "CLI Error: " << socketError;
}

void SlimDatabaseFetch::cliMessageReady(void)
{
    iTimeOut = 10000;
    QTime t;
    t.start();
    while( slimCliSocket->bytesAvailable() ) {
        while( !slimCliSocket->canReadLine () ) { // wait for a full line of content  NOTE: protect against unlikely infinite loop with timer
            if( t.elapsed() > iTimeOut ) {
                return;
            }
        }
        response = slimCliSocket->readLine();
        RemoveNewLineFromResponse();
        if( !ProcessResponse() )
            return;
        if( t.elapsed() > 2*iTimeOut)   // there is some issue, and we don't want to loop forever
            return;
    }
}

//------------------------------- HELPER FUNCTIONS ---------------------------------------------
void SlimDatabaseFetch::RemoveNewLineFromResponse( void )
{
    while( response.contains( '\n' ) )
        response.replace( response.indexOf( '\n' ), 1, " " );

}

bool SlimDatabaseFetch::ProcessResponse(void)
{
    return true;
}

// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
/*
  this is all a bunch of garbage that needs to be cleaned up!!!
  */




/*  slimdatabase.h from mythsqueezebox2
class SlimDatabase : public QObject
{
    Q_OBJECT

public:
    SlimDatabase( QObject *parent=0, const char *name = NULL );
    ~SlimDatabase( void );

    void Init( SlimCLI *interface );

    QPixmap GetCoverImage( void ) { return coverImage; }
    QPixmap GetCoverImage( QString albumName );

    SlimItem GetArtistList( void ) { return artistList; }    // list of artists available in SqueezeCenter
    SlimItem GetAlbumList( void ) { return albumList; }     // list of albums available in SqueezeCenter
    SlimItem GetAlbumArtworkIdList( void ) { return albumArtworkIdList; } // list of album artwork ids (album name is key, artwork track id is value)
    SlimImageItem GetAlbumArtworkList( void ) { return albumArtworkList; } // album covers by album title
    QStringList GetAlphaSortAlbums( void ) { return alphaSortAlbums; }
    QStringList GetAlphaSortArtists( void ) { return alphaSortArtists; }
    QStringList GetArtistAlbums( QString artistname ) { return GetArtistAlbumList().value( artistname ); }
    SlimMultiItem GetArtistAlbumList( void ) { return artistAlbums; }
    SlimItem GetArtistForAlbum( void ) { return albumsByArtist; }
    SlimItem GetSongList( void ) { return songList; }     // list of songs available in SqueezeCenter
    SlimItem GetGenreList( void ) { return genreList; }    // list of genres available in SqueezeCenter
    SlimItem GetYearList( void ) { return yearList; }     // list of years available in SqueezeCenter
    SlimItem GetPlaylistList( void ) { return playlistList; } // list of playlists available in SqueezeCenter
    SlimItem GetDeviceMACList( void ) { return deviceMACList; }   // list of Devices connected to SqueezeCenter (holds name and MAC address)

    bool FinishedFillingDatabase( void );
    int ProcessDatabaseInitialization( QByteArray Response );
    bool SetupDevices( void );

    bool SetupArtists( void );
    bool SetupAlbums( void );
    bool SetupGenres( void );
    bool SetupYears( void );
    bool SetupSongs( void );
    bool SetupPlaylists( void );
    bool SetupAlbumCovers( void );
    bool RetrieveNextAlbumCover( void );

    void SetMaxRequestSize( QByteArray max ) { MaxRequestSize = max; }    // note no error checking to ensure this is a number or is positive

public slots:
    void ResponseReceiver( QByteArray Response ) { response = Response; }
    void ImageReceived( int id, bool error );

signals:
    void SendCliCommand( QByteArray cmd, QObject *o );
    void deviceUpdateFinished( void );
    void databaseSetupFinished( void );
    void gotAllAlbumArt( void );
    void CoverRetrieved( int covercnt, int albcnt );

private:
    SlimCLI *cliInterface;
    QByteArray command;
    QByteArray response;      // holds the response from the cli for processing
    QByteArray MaxRequestSize;      // max size of any cli request (used for limiting each request for albums, artists, songs, etc., so we don't time out or overload things)


    QPixmap coverImage;       // cover image pixmap
    QByteArray ba;            // a byte array to write the cover image data to
    QBuffer buffer;           // a buffer to handle the writing
    QString initCoverImageURLString;    // holds the full URL for the image, which gets modified for each image request

    QHashIterator<QString, QString> *initImageIter; // an iterator so we can move through the hash effectively

    int playlistIndexCount;   // number of titles in current playlist
    int currIndex;            // where we are in the playlist (0-N)
    int albumCount;           // total number of albums, used when showing progress on displaying retrieval of album covers
    int coverCount;           // number of covers retrieved so far
    int artistCount;
    int genreCount;
    int yearCount;
    int songCount;
    int playlistCount;

    SlimItem artistList;    // list of artists available in SqueezeCenter
    SlimItem albumList;     // list of albums available in SqueezeCenter
    SlimItem albumArtworkIdList; // list of "album artwork track ids" for retreiving album artwork
    SlimImageItem albumArtworkList;  // list of cover art by album title
    QStringList alphaSortAlbums;    // we want an alphabetical sorted list of albums in some cases
    QStringList alphaSortArtists;   // we want an alphabetical sorted list of artists in some cases
    SlimItem songList;      // list of songs available in SqueezeCenter
    SlimItem genreList;     // list of genres available in SqueezeCenter
    SlimItem yearList;      // list of years available in SqueezeCenter
    SlimItem playlistList;  // list of playlists available in SqueezeCenter
    SlimItem deviceMACList; // list of the MAC addresses of devices connected to SqueezeCenter
    SlimMultiItem artistAlbums;   // list of albums for each artist, key is artist name, value is album list
    SlimItem albumsByArtist;  // list of albums indexed to artist ( key is album name, value is artist name )

    bool setupGotArtists;
    bool setupGotAlbums;
    bool setupGotAlbumCovers;
    bool setupGotGenres;
    bool setupGotSongs;
    bool setupGotYears;
    bool setupGotPlaylists;
    bool setupGotDevices;

    void getCoverImage( void );
    void SendCommand( QString c ) { emit SendCliCommand( c.toAscii(), this ); }   // convenience so that we don't have to change a bunch of code
};

#endif // SLIMDATABASE_H

  */


/*  slimdatabase.cpp from mythsqueezebox2

SlimDatabase::SlimDatabase( QObject *parent, const char *name ) : QObject( parent )
{
    setupGotArtists = false;
    setupGotAlbums = false;
    setupGotAlbumCovers = false;
    setupGotGenres = false;
    setupGotSongs = false;
    setupGotYears = false;
    setupGotPlaylists = false;
    setupGotDevices = false;
    MaxRequestSize = "100";
    albumCount = -1;
    coverCount = -1;
    artistCount = -1;
    genreCount = -1;
    yearCount = -1;
    songCount = -1;
    playlistCount = -1;

}

SlimDatabase::~SlimDatabase( void )
{
}

void SlimDatabase::Init( SlimCLI *interface )
{
    DEBUGF( "Initializing database info" );
    cliInterface = interface;   // set so we can directly call the cli if needed (like here)
}

bool SlimDatabase::FinishedFillingDatabase( void )
{
    bool done = setupGotArtists && setupGotAlbums && setupGotGenres && setupGotYears && setupGotPlaylists;
    // get some basic information about the database of artists, albums, genres and years
    DEBUGF( "Are we done? " << done );
    if( done ) {
        qSort(alphaSortAlbums.begin(), alphaSortAlbums.end() ); // sort ablum list by alpha
        qSort(alphaSortArtists.begin(), alphaSortArtists.end() );  // sort artist list by alpha
        SetupAlbumCovers();
        emit databaseSetupFinished();
    }
    return done;
}

int SlimDatabase::ProcessDatabaseInitialization( QByteArray Response  )
{
    response = Response.trimmed();    // make sure there is no confusing white space at the end or start of the line
    DEBUGF( "## START OF DATABASE INITIALIZATION ##" );
    if( !response.contains( ':' ) )  // oops!, still escape encoded!!  Substitute ':' for '%3A', we'll URL::decode later
        response.replace( "%3A", ":" );

    DEBUGF( "Processing the following response for data: " << response );
    QList<QByteArray> list = response.split( ' ' ); // split into a list of the fields

    // NOTE: first three fields are the original command (e.g., "artist 0 500") and response ends with "count%3A<total artists>"
    // these next items are to retain the relevant info on the original request so that we can determine what we are processing, and
    // to ask for more data if we need to (i.e., there are more than 500 items in the list)

    QString requestType;
    QString startPosition;
    QString endPosition;
    int ReceiveCount = MaxRequestSize.toInt();  // initialize to standard request size, just in case
    int c = 0;

    for( QList<QByteArray>::Iterator it = list.begin(); it != list.end(); ++it, c++ )
    {
        QString field = QUrl::fromPercentEncoding( *it );
        DEBUGF( "new field: " << c << " - " << field );
        switch( c ) // we want to preserve the fields 0-2, which contain the original command (e.g., "artists 0 1000")
        {
        case 0:
            requestType = field.trimmed();
            break;
        case 1:
            startPosition = field.trimmed();
            break;
        case 2:
            endPosition = field.trimmed();
            ReceiveCount = endPosition.toInt() + startPosition.toInt(); // our start position plus our requested # of items gives the total count of received items so far (e.g., 0 + 1000 = 1000, then 1000 + 1000 = 2000)
            break;
        default:	// this is where most of the processing will occur
            // NOTE: for albums, there will be an extraneous fourth field of "tags:l%2Cj%2Ca" which will be ignored

            if( field.section( ':', 0, 0 ).trimmed() == "id" ) {
                // if the first field is an "id", then we know we have an "id:<number>" followed by a "<requestType>:<name>" pair
                QString idTag = field.section( ':', 1, 1 );	// preserve the id tag
                ++it;											// iterate to the next pair (should be "<requestType>:<name>" pair)
                if( it == list.end() )	// oops error, we somehow got to the end!!  This means there is an "id" without a <requesttype> pair following it!!
                {
                    DEBUGF( "Unexpected Termination of list" );
                    return SETUP_FAILURE;	// get out!!
                }
                field = QUrl::fromPercentEncoding( *it ); // get the next field pair
                DEBUGF( "data field: " << c << " - " << field );
                QString nameTag = field.section( ':', 1, 1 );	// grab the relevant info

                // now process depending on request type, we also could use the first "section" from "field" here
                if( requestType == "artists" ) {
                    artistList.insert( nameTag, idTag );
                    alphaSortArtists.append( nameTag ); // insert artist name into QStringList for alpha sort later
                }
                else if( requestType == "albums" )  { // albums are a special case because we are getting the artwork id and artist as well as the album id
                    // step 1, get the album and albumid tag
                    albumList.insert( nameTag, idTag );
                    // step 2, iterate to the next pair (should be "<artwork_track_id>:<idtag>" pair), but sometimes isn't
                    ++it;
                    field = QUrl::fromPercentEncoding( *it ); // get the next field pair
                    if( field.section( ':', 0, 0 ).trimmed() == "artwork_track_id" ) { // we've got artwork, so process
                        QString imageIdTag = field.section( ':', 1, 1 );
                        albumArtworkIdList.insert( nameTag, imageIdTag );       // for looking up image ids from album title
                        alphaSortAlbums.append( nameTag ); // insert album name into QStringList for alpha sort later
                        DEBUGF( "Got Artwork ID: " << imageIdTag );
                        // iterate to the next pair (should be "artist:<artistname>" pair), but only if we need to (i.e., we captured an artwork tag here)
                        ++it;
                        field = QUrl::fromPercentEncoding( *it ); // get the next field pair
                    }
                    else {
                        DEBUGF( "**NO ARTWORK TAG **" );
                    }
                    if( field.section( ':', 0, 0 ).trimmed() == "artist" ) { // we've got artists name, so process
                        QString theartistName = field.section( ':', 1, 1 );
                        DEBUGF( "Got Artist Name: " << theartistName );
                        if( artistAlbums.contains( theartistName ) ) {    // artist already exists, so go to that key and add to QStringList of albums
                            QStringList temp = artistAlbums.value( theartistName ); // get current string of albums associated with artist
                            temp.append( nameTag );
                            artistAlbums.insert( theartistName, temp );   // replace the artist:albumlist pair with an updated list
                        }
                        else {
                            artistAlbums.insert( theartistName, QStringList( nameTag ) );
                        }
                        // step 4, capture the album -> artist relationship
                        albumsByArtist.insert( nameTag, theartistName );
                    }
                }
                else if( requestType == "genres" )
                    genreList.insert( nameTag, idTag );
                else if( requestType == "songs" )
                    songList.insert( nameTag, idTag );
                else if( requestType == "playlists" )
                    playlistList.insert( nameTag, idTag );
            }	// end if field.section == "id"
            else if( field.section( ':', 0, 0 ).trimmed() == "year" )	// special case, "year" has no "id" field, so only "year:<year>" pairs
                yearList.insert( field.section( ':', 1, 1 ), field.section( ':', 1, 1 ) );
            else if( field.section( ':', 0, 0 ).trimmed() == "count" )	// last field is the total count of items of this type, if more than 500, get the rest
            {
                QString thisCount = field.section( ':', 1, 1 );	// OK, what is the real number of items
                int fullCount = thisCount.toInt();  // make it a number for convenience
                DEBUGF( "The count is: " << thisCount );
                // preserve count for later
                if( requestType == "albums" )   // save total ablum count for later
                    albumCount = fullCount;
                else if( requestType == "artists" )
                    artistCount = fullCount;
                else if( requestType == "genres" )
                    genreCount = fullCount;
                else if( requestType == "songs" )
                    songCount = fullCount;
                else if( requestType == "playlists" )
                    playlistCount = fullCount;
                else if( requestType == "years" )
                    yearCount = fullCount;

                if( fullCount > ReceiveCount )	// if we didn't get enough, get the rest
                {
                    DEBUGF( "OK, we didn't get enough data.  We started at " << startPosition << " and ended at " << ReceiveCount << " and need to get " << fullCount );
                    command.clear();
                    if( requestType == "albums" )
                        command = QString( requestType + " " + QString::number( ReceiveCount ) + " " + MaxRequestSize + " tags:l,j,a").toAscii();		// NOTE: this requests more items than there are (we really should ask for thisCount - end), but let's not bother to do the conversions from QString to int and back to QString since SqueezeCenter will return only the total number of items that exist
                    else
                        command = QString( requestType + " " + QString::number( ReceiveCount ) + " " + MaxRequestSize ).toAscii();		// NOTE: this requests more items than there are (we really should ask for thisCount - end), but let's not bother to do the conversions from QString to int and back to QString since SqueezeCenter will return only the total number of items that exist
                    DEBUGF( "Sending new command to get more data: " << command );
                    if( !cliInterface->SendBlockingCommand( command ) )    // NOTE, no need for MAC address send the command to get more
                        return SETUP_FAILURE;	// get out!!
                    else
                        return SETUP_NEEDMOREDATA;		// return here so we skip tagging this type of item as having been received below
                }
            }	// end of else/if field.section == count
        }	// end of switch statement
    }		// end for loop

    // tag the particular request has having been taken care of and return success
    if( requestType == "artists" )
        setupGotArtists = true;
    else if( requestType == "albums" )
        setupGotAlbums = true;
    else if( requestType == "songs" )
        setupGotSongs = true;
    else if( requestType == "genres" )
        setupGotGenres = true;
    else if( requestType == "years" )
        setupGotYears = true;
    else if( requestType == "playlists" )
        setupGotPlaylists = true;

    return SETUP_SUCCESS;
}

bool SlimDatabase::SetupArtists( void )
{
    bool retval = false;
    artistList.clear();
    alphaSortArtists.clear();
    QByteArray blockingCommand = "artists 0 " + MaxRequestSize;         // create request respecting the "max request size" limit set in settings

    int MaxLoop = 1000;

    if( cliInterface->SendBlockingCommand( blockingCommand ) ) {	// ask for list of artists.
        int setupSuccess;
        DEBUGF( "SENT BLOCKING COMMAND TO GET ARTIST DATA" );
        for( int c = 0; c < MaxLoop; c++ ) {
            if( artistCount > -1 )
                MaxLoop = 1 + ( artistCount / MaxRequestSize.toInt() ); // give us a real number for the maximum number of loops we should allow before declaring an error
            setupSuccess = ProcessDatabaseInitialization( cliInterface->GetResponse() );
            if( setupSuccess == SETUP_SUCCESS ) {
                retval = true;
                break;
            }
            if( setupSuccess == SETUP_FAILURE ) {
                retval = false;
                break;
            }
        }
        if( setupSuccess != SETUP_SUCCESS ) {   // we've had a problem
            QString errstr = QString( "Error setting up artists.  We've iterated %1 loops grabbing %2 artists with each loop without success.  Error code %3." )
                             .arg( MaxLoop )
                             .arg( MaxRequestSize.toInt() ) // I know it doesn't make much sense to convert to int and then reconvert to string, but this allows us to see if the conversion process caused an issue
                             .arg( setupSuccess );
            DEBUGF( errstr );
            VERBOSE( VB_IMPORTANT, errstr );
        }
    }
    return retval;
}

bool SlimDatabase::SetupAlbums( void )
{
    DEBUGF( "ALBUM SETUP ROUTINE START" );
    bool retval = false;
    albumList.clear();
    albumArtworkIdList.clear();
    albumArtworkList.clear();
    alphaSortAlbums.clear();
    QByteArray blockingCommand = "albums 0 " + MaxRequestSize + " tags:l,j,a";      // create request respecting the "max request size" limit set in settings

    int MaxLoop = 1000;

    if( cliInterface->SendBlockingCommand( blockingCommand ) ) {	// ask for list of albums.
        int setupSuccess;
        DEBUGF( "SENT BLOCKING COMMAND TO GET ALBUM DATA" );
        for( int c = 0; c < MaxLoop; c++ ) {
            if( albumCount > -1 )
                MaxLoop = 1 + ( albumCount / MaxRequestSize.toInt() ); // give us a real number for the maximum number of loops we should allow before declaring an error
            setupSuccess = ProcessDatabaseInitialization( cliInterface->GetResponse() );
            if( setupSuccess == SETUP_SUCCESS ) {
                retval = true;
                break;
            }
            if( setupSuccess== SETUP_FAILURE ) {
                retval = false;
                break;
            }
        }
        if( setupSuccess != SETUP_SUCCESS ) {   // we've had a problem
            QString errstr = QString( "Error setting up albums.  We've iterated %1 loops grabbing %2 albums with each loop without success.  Error code %3." )
                             .arg( MaxLoop )
                             .arg( MaxRequestSize.toInt() ) // I know it doesn't make much sense to convert to int and then reconvert to string, but this allows us to see if the conversion process caused an issue
                             .arg( setupSuccess );
            DEBUGF( errstr );
            VERBOSE( VB_IMPORTANT, errstr );
        }
    }
    return retval;
}

bool SlimDatabase::SetupGenres( void )
{
    bool retval = false;
    genreList.clear();
    QByteArray blockingCommand = "genres 0 " + MaxRequestSize;         // create request respecting the "max request size" limit set in settings

    int MaxLoop = 1000;

    if( cliInterface->SendBlockingCommand( blockingCommand ) ) {	// ask for list of genres.
        int setupSuccess;
        DEBUGF( "SENT BLOCKING COMMAND TO GET GENRE DATA" );
        for( int c = 0; c < MaxLoop; c++ ) {
            if( genreCount > -1 )
                MaxLoop = 1 + ( genreCount / MaxRequestSize.toInt() ); // give us a real number for the maximum number of loops we should allow before declaring an error
            setupSuccess = ProcessDatabaseInitialization( cliInterface->GetResponse() );
            if( setupSuccess == SETUP_SUCCESS ) {
                retval = true;
                break;
            }
            if( setupSuccess== SETUP_FAILURE ) {
                retval = false;
                break;
            }
        }
        if( setupSuccess != SETUP_SUCCESS ) {   // we've had a problem
            QString errstr = QString( "Error setting up genres.  We've iterated %1 loops grabbing %2 genres with each loop without success.  Error code %3." )
                             .arg( MaxLoop )
                             .arg( MaxRequestSize.toInt() ) // I know it doesn't make much sense to convert to int and then reconvert to string, but this allows us to see if the conversion process caused an issue
                             .arg( setupSuccess );
            DEBUGF( errstr );
            VERBOSE( VB_IMPORTANT, errstr );
        }
    }
    return retval;
}

bool SlimDatabase::SetupYears( void )
{
    bool retval = false;
    yearList.clear();
    QByteArray blockingCommand = "years 0 " + MaxRequestSize;         // create request respecting the "max request size" limit set in settings

    int MaxLoop = 1000;

    if( cliInterface->SendBlockingCommand( blockingCommand ) ) {	// ask for list of years
        int setupSuccess;
        DEBUGF( "SENT BLOCKING COMMAND TO GET YEAR DATA" );
        for( int c = 0; c < MaxLoop; c++ ) {
            if( yearCount > -1 )
                MaxLoop = 1 + ( yearCount / MaxRequestSize.toInt() ); // give us a real number for the maximum number of loops we should allow before declaring an error
            setupSuccess = ProcessDatabaseInitialization( cliInterface->GetResponse() );
            if( setupSuccess == SETUP_SUCCESS ) {
                retval = true;
                break;
            }
            if( setupSuccess== SETUP_FAILURE ) {
                retval = false;
                break;
            }
        }
        if( setupSuccess != SETUP_SUCCESS ) {   // we've had a problem
            QString errstr = QString( "Error setting up years.  We've iterated %1 loops grabbing %2 years with each loop without success.  Error code %3." )
                             .arg( MaxLoop )
                             .arg( MaxRequestSize.toInt() ) // I know it doesn't make much sense to convert to int and then reconvert to string, but this allows us to see if the conversion process caused an issue
                             .arg( setupSuccess );
            DEBUGF( errstr );
            VERBOSE( VB_IMPORTANT, errstr );
        }
    }
    return retval;
}

bool SlimDatabase::SetupSongs( void )
{
    bool retval = false;
    songList.clear();
    QByteArray blockingCommand = "songs 0 " + MaxRequestSize;         // create request respecting the "max request size" limit set in settings

    int MaxLoop = 1000;

    if( cliInterface->SendBlockingCommand( blockingCommand ) ) {	// ask for list of songs
        int setupSuccess;
        DEBUGF( "SENT BLOCKING COMMAND TO GET YEAR DATA" );
        for( int c = 0; c < MaxLoop; c++ ) {
            if( songCount > -1 )
                MaxLoop = 1 + ( songCount / MaxRequestSize.toInt() ); // give us a real number for the maximum number of loops we should allow before declaring an error
            setupSuccess = ProcessDatabaseInitialization( cliInterface->GetResponse() );
            if( setupSuccess == SETUP_SUCCESS ) {
                retval = true;
                break;
            }
            if( setupSuccess== SETUP_FAILURE ) {
                retval = false;
                break;
            }
        }
        if( setupSuccess != SETUP_SUCCESS ) {   // we've had a problem
            QString errstr = QString( "Error setting up songs.  We've iterated %1 loops grabbing %2 songs with each loop without success.  Error code %3." )
                             .arg( MaxLoop )
                             .arg( MaxRequestSize.toInt() ) // I know it doesn't make much sense to convert to int and then reconvert to string, but this allows us to see if the conversion process caused an issue
                             .arg( setupSuccess );
            DEBUGF( errstr );
            VERBOSE( VB_IMPORTANT, errstr );
        }
    }
    return retval;
}

bool SlimDatabase::SetupPlaylists( void )
{
    bool retval = false;
    playlistList.clear();
    QByteArray blockingCommand = "playlists 0 " + MaxRequestSize;         // create request respecting the "max request size" limit set in settings

    int MaxLoop = 1000;

    if( cliInterface->SendBlockingCommand( blockingCommand ) ) {	// ask for list of playlists
        int setupSuccess;
        DEBUGF( "SENT BLOCKING COMMAND TO GET SONG DATA" );
        for( int c = 0; c < MaxLoop; c++ ) {
            if( playlistCount > -1 )
                MaxLoop = 1 + ( playlistCount / MaxRequestSize.toInt() ); // give us a real number for the maximum number of loops we should allow before declaring an error
            setupSuccess = ProcessDatabaseInitialization( cliInterface->GetResponse() );
            if( setupSuccess == SETUP_SUCCESS ) {
                retval = true;
                break;
            }
            if( setupSuccess== SETUP_FAILURE ) {
                retval = false;
                break;
            }
        }
        if( setupSuccess != SETUP_SUCCESS ) {   // we've had a problem
            QString errstr = QString( "Error setting up playlists.  We've iterated %1 loops grabbing %2 playlists with each loop without success.  Error code %3." )
                             .arg( MaxLoop )
                             .arg( MaxRequestSize.toInt() ) // I know it doesn't make much sense to convert to int and then reconvert to string, but this allows us to see if the conversion process caused an issue
                             .arg( setupSuccess );
            DEBUGF( errstr );
            VERBOSE( VB_IMPORTANT, errstr );
        }
    }
    return retval;
}

bool SlimDatabase::SetupAlbumCovers( void )
{
    initImageIter = new QHashIterator<QString, QString> ( albumArtworkIdList ); // establish iterator for getting album art
    RetrieveNextAlbumCover(); // start asynchronous command to get album covers, but be careful as it takes a while to finish
}

bool SlimDatabase::RetrieveNextAlbumCover( void )
{
    if (initImageIter->hasNext()) {  // while we have an ablum left to get a cover
        ba.resize( 0 );
        if( !buffer.isOpen() ) {
            buffer.setBuffer( &ba );
            buffer.open( QIODevice::ReadWrite );
        }
        initImageIter->next();   // move to the next album and construct the URL to get the cover
        initCoverImageURLString = "/music/" + initImageIter->value() + "/cover_200x200";
        DEBUGF( "Getting: " << initCoverImageURLString );
        cliInterface->GetCoverImage( initCoverImageURLString, &buffer );
    }
    else {    // clean up
        buffer.close();
        DEBUGF( "Finished Getting Album Art" );
        emit gotAllAlbumArt();
    }
}

void SlimDatabase::ImageReceived( int id, bool error ){
    if( !error ) {
        DEBUGF( "Success retrieving image :" << id << " of size " << ba.size() );
        coverImage.loadFromData( ba );
        DEBUGF( "Image size is :" << coverImage.size() << " and image width is " << coverImage.width() );
        DEBUGF( "Image " << id << " is associated with ablum " << initImageIter->key() );
        albumArtworkList.insert( initImageIter->key(), coverImage );   // add the cover art for current album title
        coverCount++;
        emit CoverRetrieved( coverCount, albumCount );
    }
    else {
        DEBUGF( "*****IMAGE ERROR*****" );
        DEBUGF( "QHTTP Error code: " << cliInterface->GetImageURL()->error() << " with error string: " << cliInterface->GetImageURL()->errorString() );
    }

    RetrieveNextAlbumCover(); // get the next image
    buffer.close();
}

QPixmap SlimDatabase::GetCoverImage( QString albumName )
{
    QPixmap p = albumArtworkList.value( albumName );
    QHashIterator< QString, QPixmap> i( albumArtworkList );
    return p;
}


/* vim: set expandtab tabstop=4 shiftwidth=4: */

  */
