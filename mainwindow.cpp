#include <QMovie>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_settingsdialog.h"

#ifdef SQUEEZEMAINWINDOW_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif

char keypadKey[10][5]={{'0',' ',' ',' ',' '},
                       {'1',' ',' ',' ',' '},
                       {'A','B','C','2',' '},
                       {'D','E','F','3',' '},
                       {'G','H','I','4',' '},
                       {'J','K','L','5',' '},
                       {'M','N','O','6',' '},
                       {'P','Q','R','S','7'},
                       {'T','U','V','8',' '},
                       {'W','X','Y','Z','9'}};

// globally declared so that multiple classes access the same image cache
SlimImageCache *imageCache;

QDataStream & operator<< (QDataStream& stream, const Album& al)
{
    stream << al.songtitle;
    stream << al.albumtitle;
    stream << al.album_id;
    stream << al.year;
    stream << al.artist;
    stream << al.artist_id;
    stream << al.coverid;
    stream << al.artist_album;
    stream << al.albumTextKey;
    return stream;
}

QDataStream & operator>> (QDataStream& stream, Album& al)
{
    stream >> al.songtitle;
    stream >> al.albumtitle;
    stream >> al.album_id;
    stream >> al.year;
    stream >> al.artist;
    stream >> al.artist_id;
    stream >> al.coverid;
    stream >> al.artist_album;
    stream >> al.albumTextKey;
    return stream;
}

QDataStream & operator<< (QDataStream& stream, const Artist& art)
{
    stream << art.id;
    stream << art.name;
    stream << art.textKey;
    return stream;
}

QDataStream & operator>> (QDataStream& stream, Artist& art)
{
    stream >> art.id;
    stream >> art.name;
    stream >> art.textKey;
    return stream;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    DEBUGF("");
    ui->setupUi(this);
    setWindowTitle("QtSqueeze3");
    squeezePlayer = new QProcess( this );
    slimCLI = new SlimCLI( this );
    serverInfo = new SlimServerInfo(this);

    lastKey=0;
    keyOffset=0;
    connect(&keypadTimer, SIGNAL(timeout()),
            this,SLOT(ResetKeypadTimer()));

    mySettings = new QSettings("qtsqueeze3", "qtsqueeze3");
    activeDevice = NULL;
    isStartUp = true;

    loadConfig();  // load configuration files

    QPixmap splash;
    splash.load( ":/img/lib/images/squeezeplayer_large.png" );
    waitWindow = new QSplashScreen( this, splash );
    waitWindow->setGeometry( ( geometry().width() - 400 )/ 2, ( geometry().height() - 200 )/2,
                             400, 200 );

    cfType = DISPLAY_PLAYLIST;
    imageCache = new SlimImageCache();
    m_disp = new SqueezeDisplay(ui->lblSlimDisplay, this);
    playlistCoverFlow = new SqueezePictureFlow(ui->cfPlaylistWidget);
    artistselectCoverFlow = new SqueezePictureFlow(ui->cfArtistSelectWidget,ALBUMSELECT);
    albumselectCoverFlow = new SqueezePictureFlow(ui->cfAlbumSelectWidget,ALBUMSELECT);

    applyDisplayConfig();
    getImages = true;
    m_disp->Init();
    imageCache->Init(SlimServerAddr,(qint16)SlimServerHttpPort.toInt());
    imageCache->start();

    DEBUGF("FINISHED LOADING CONFIGS");

    waitWindow->hide();
    connect( slimCLI, SIGNAL(cliInfo(QString)), waitWindow, SLOT(showMessage(QString)) );

}

MainWindow::~MainWindow()
{
    imageCache->Stop();
    //    mySettings->setValue("UI/CurrentTab",ui->tabWidget->currentIndex());
    mySettings->sync();
    squeezePlayer->close();
    imageCache->deleteLater();
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);

    bool hidePlaylist = playlistCoverFlow->isHidden();
    bool hideArtistList = artistselectCoverFlow->isHidden();
    bool hideAlbumList = albumselectCoverFlow->isHidden();

    ui->controlFrame->setGeometry(0,0,width(),height()/2);
    ui->lblSlimDisplay->setGeometry(ui->lblSlimDisplay->x(), ui->lblSlimDisplay->y(), ui->controlFrame->width() - 2*ui->lblSlimDisplay->x(), ui->controlFrame->height() - ui->controlFrame_2->height() - 3);
    ui->controlFrame_2->move(ui->controlFrame_2->x(), ui->controlFrame->height() - 3 - ui->controlFrame_2->height());
    ui->arrowKeyFrame->move(ui->arrowKeyFrame->x(), ui->controlFrame->height() - 3 - ui->arrowKeyFrame->height());
    ui->keypadFrame->move(ui->keypadFrame->x(),ui->controlFrame->height() - 3 - ui->keypadFrame->height());
    m_disp->resetDimensions();

    ui->cfPlaylistWidget->setGeometry(0,(ui->controlFrame->geometry().bottom()+1), width(), height()-(ui->controlFrame->geometry().bottom()+2));
    ui->cfArtistSelectWidget->setGeometry(0,(ui->controlFrame->geometry().bottom()+1), width(), height()-(ui->controlFrame->geometry().bottom()+2));
    ui->cfAlbumSelectWidget->setGeometry(0,(ui->controlFrame->geometry().bottom()+1), width(), height()-(ui->controlFrame->geometry().bottom()+2));
    playlistCoverFlow->resetDimensions(ui->cfPlaylistWidget);
    artistselectCoverFlow->resetDimensions(ui->cfArtistSelectWidget);
    albumselectCoverFlow->resetDimensions(ui->cfAlbumSelectWidget);
    playlistCoverFlow->setHidden(hidePlaylist);
    ui->cfPlaylistWidget->setHidden(hidePlaylist);
    artistselectCoverFlow->setHidden(hideArtistList);
    ui->cfArtistSelectWidget->setHidden(hideArtistList);
    albumselectCoverFlow->setHidden(hideAlbumList);
    ui->cfAlbumSelectWidget->setHidden(hideAlbumList);
    if(activeDevice)
        if(activeDevice->getDisplayBuffer())
            m_disp->PaintSqueezeDisplay(activeDevice->getDisplayBuffer());
}


bool MainWindow::Create(void)
{
    /*
    There is a fairly large amount of work to do on startup, but we don't want to keep the user guessing,
    so we first create the window to give a visual cue that things have started and then we send a signal
    to the program to do the rest of the processing, while displaying a progress bar to keep the user interested
*/
    slotDisablePlayer();

    // SET UP PLAYER INFO
    getplayerMACAddress();

    DEBUGF( "CREATING SQUEEZESLAVE INSTANCE" );

    connect( squeezePlayer, SIGNAL(readyReadStandardError()), this, SLOT(SqueezePlayerError()) );
    connect( squeezePlayer, SIGNAL(readyReadStandardOutput()), this, SLOT(SqueezePlayerOutput()) );

#ifdef Q_OS_LINUX
    QString program = "squeezeslave";
    QString playeropt = "-l";
#else
    QString program = "squeezeslave";
    QString playeropt = "-D";
#endif

    QStringList args;

    args.append(QString("-m" + QString( MacAddress )));
    args.append(playeropt);
    args.append(QString("-P" + SlimServerAudioPort));
    if( PortAudioDevice.length() > 0 ) {  // NOTE: list contains the number, name and a description.  We need just the number
        PortAudioDevice = PortAudioDevice.left(PortAudioDevice.indexOf(":")).trimmed();
        args.append(QString("-o"+PortAudioDevice));
    }
    args.append(SlimServerAddr);
    DEBUGF( "player command " << program << " " << args );
    qDebug() << "startiong player with command " << program << " " << args;

    progstart.start();

    squeezePlayer->start( program, args );
    // the short answer is that QProcess::state() doesn't always return "running" under Linux, even when the app is running
    // however, we need to have it running in order for it to be picked up as a "device" later by the CLI interface
    // so we give a bit of a delay (2 seconds) and allow for an earlier exit if we find that state() has returned something useful
#ifdef Q_OS_WIN32   // this doesn't appear to work under Linux
    if( squeezePlayer->state() == QProcess::NotRunning ) {
        DEBUGF( "Squeezeslave did not start.  Current state is: " << squeezePlayer->state() << " and elapsed time is: " << progstart.elapsed() );
        DEBUGF( "Error message (if any): " << squeezePlayer->errorString() );
        return false;
    }
#else
    while( squeezePlayer->state() != QProcess::NotRunning ) {
        if( progstart.elapsed() >= 2000 )
            break;
    }
#endif

    // initialize the CLI interface.  Make sure that you've set the appropriate server address and port
    slimCLI->SetMACAddress( MacAddress );
    slimCLI->SetSlimServerAddress( SlimServerAddr );
    slimCLI->SetCliPort(SlimServerCLIPort);
    //    slimCLI->SetHttpPort(SlimServerHttpPort);

    connect( serverInfo, SIGNAL(FinishedInitializingDevices()),
             this, SLOT(slotSetActivePlayer()) );              // we want to wait to set up the display until the devices are established
    slimCLI->SetServerInfo(serverInfo);                         // give pointer so CLI can get server info and put into SlimServerInfo
    slimCLI->Init();

    DEBUGF("###Setup Display");
    //    SetUpDisplay();

    // set up connection between interface buttons and the slimserver
    connect( ui->btn0, SIGNAL(clicked()), this, SLOT(slot0PAD()) );
    connect( ui->btn1, SIGNAL(clicked()), this, SLOT(slot1PAD()) );
    connect( ui->btn2, SIGNAL(clicked()), this, SLOT(slot2PAD()) );
    connect( ui->btn3, SIGNAL(clicked()), this, SLOT(slot3PAD()) );
    connect( ui->btn4, SIGNAL(clicked()), this, SLOT(slot4PAD()) );
    connect( ui->btn5, SIGNAL(clicked()), this, SLOT(slot5PAD()) );
    connect( ui->btn6, SIGNAL(clicked()), this, SLOT(slot6PAD()) );
    connect( ui->btn7, SIGNAL(clicked()), this, SLOT(slot7PAD()) );
    connect( ui->btn8, SIGNAL(clicked()), this, SLOT(slot8PAD()) );
    connect( ui->btn9, SIGNAL(clicked()), this, SLOT(slot9PAD()) );

    connect( ui->btnUpArrow ,SIGNAL(clicked()), this, SLOT(slotUpArrow()) );
    connect( ui->btnDownArrow ,SIGNAL(clicked()), this, SLOT(slotDownArrow()) );
    connect( ui->btnLeftArrow ,SIGNAL(clicked()), this, SLOT(slotLeftArrow()) );
    connect( ui->btnRightArrow ,SIGNAL(clicked()), this, SLOT(slotRightArrow()) );

    connect( ui->btnAdd, SIGNAL(clicked()), this, SLOT(slotAdd()) );
    connect( ui->btnFForward,SIGNAL(clicked()), this, SLOT(slotFForward()) );
    connect( ui->btnNowPlaying ,SIGNAL(clicked()), this, SLOT(slotNowPlaying()) );
    connect( ui->btnPause ,SIGNAL(clicked()), this, SLOT(slotPause()) );
    connect( ui->btnPlay ,SIGNAL(clicked()), this, SLOT(slotPlay()) );
    connect( ui->btnRepeat ,SIGNAL(clicked()), this, SLOT(slotRepeat()) );
    connect( ui->btnRewind ,SIGNAL(clicked()), this, SLOT(slotRewind()) );
    connect( ui->btnStop ,SIGNAL(clicked()), this, SLOT(slotStop()) );

    //    // allow mouse clicks on CoverFlow to cause playlist change
    connect( playlistCoverFlow, SIGNAL(NextSlide()), this, SLOT(slotFForward()) );
    connect( playlistCoverFlow, SIGNAL(PrevSlide()), this, SLOT(slotPrev()) );

    connect( ui->btnBright ,SIGNAL(clicked()), this, SLOT(slotBright()) );
    connect( ui->btnPower ,SIGNAL(clicked()), this, SLOT(slotPower()) );
    connect( ui->btnSearch ,SIGNAL(clicked()), this, SLOT(slotSearch()) );
    connect( ui->btnShuffle ,SIGNAL(clicked()), this, SLOT(slotShuffle()) );
    connect( ui->btnSize ,SIGNAL(clicked()), this, SLOT(slotSize()) );
    connect( ui->btnSleep ,SIGNAL(clicked()), this, SLOT(slotSleep()) );

    connect( ui->btnMute ,SIGNAL(clicked()), this, SLOT(slotMute()) );
    connect( ui->btnUpVolume ,SIGNAL(clicked()), this, SLOT(slotVolUp()) );
    connect( ui->btnDownVolume ,SIGNAL(clicked()), this, SLOT(slotVolDown()) );

    DEBUGF("###Create Return");
    //    slotEnablePlayer();
    return true;
}

void MainWindow::slotplaylistCoverFlowReady(void)
{
    playlistCoverFlow->FlipToSlide(activeDevice->getDevicePlaylistIndex());

    // check to see if we have all images, and mark it so we
    // don't redo it
    if(getImages)
        imageCache->CheckImages(serverInfo->GetAllAlbumList());

    // due to threading, it's possible to end up here **before**
    // we've established a list of albums, so if we haven't, don't
    // mark it as though we have
    if(serverInfo->GetAllAlbumList().count()>1)
        getImages = false;
}

void MainWindow::slotUpdateplaylistCoverFlow( int trackIndex )
{
    DEBUGF("");

    //    int currSlide = playlistCoverFlow->centerIndex();
    //    DEBUGF( "UPDATE playlistCoverFlow TO INDEX: " << trackIndex );
    //    if( abs( trackIndex - currSlide ) > 4 ) {
    //        if( trackIndex > currSlide ) {
    //            playlistCoverFlow->showSlide( currSlide + 2 );
    //            playlistCoverFlow->setCenterIndex( trackIndex - 2 );
    //        }
    //        else {
    //            playlistCoverFlow->showSlide( currSlide - 2 );
    //            playlistCoverFlow->setCenterIndex( trackIndex + 2 );
    //        }
    //    }
    playlistCoverFlow->FlipToSlide(trackIndex );
}

void MainWindow::slotCreateplaylistCoverFlow( void )
{
    DEBUGF("");
    //  delete playlistCoverFlow; // there seems to be some memory leakage with playlistCoverFlow when you clear it and add new items
    //  playlistCoverFlow = new SqueezePictureFlow( ui->cfWidget );
    //  playlistCoverFlow->setMinimumSize( flowRect.width(), flowRect.height() );
    //  playlistCoverFlow->setContentsMargins( 50, 0, playlistCoverFlow->width() - 50, playlistCoverFlow->height() );
    playlistCoverFlow->resetDimensions(ui->cfPlaylistWidget);
    playlistCoverFlow->clear();

    playlistCoverFlow->LoadAlbumList(activeDevice->getDevicePlayList());
    ui->cfArtistSelectWidget->hide();
    ui->cfAlbumSelectWidget->hide();
    ui->cfPlaylistWidget->show();
    playlistCoverFlow->FlipToSlide(activeDevice->getDevicePlaylistIndex());
}

void MainWindow::SetupSelectionCoverFlows(void)
{
    if(isStartUp) {
        isStartUp = false;
        artistselectCoverFlow->resetDimensions(ui->cfArtistSelectWidget);
        albumselectCoverFlow->resetDimensions(ui->cfAlbumSelectWidget);

        artistselectCoverFlow->clear();
        albumselectCoverFlow->clear();

        // set up Artist Selection Coverflow
        QList<Album> artists;
        artistselectCoverFlow->GetAlbumKeyTextJumpList()->clear();
        artistselectCoverFlow->GetTitleJumpList()->clear();

        QListIterator<Artist> artIt(serverInfo->GetAllArtistList());
        int slideCount = 0;
        while(artIt.hasNext()) {
            Artist a = artIt.next();
            artistselectCoverFlow->GetTitleJumpList()->insert(a.name,slideCount);   // map artist name to slide count
            if(!artistselectCoverFlow->GetAlbumKeyTextJumpList()->contains(a.textKey))
                artistselectCoverFlow->GetAlbumKeyTextJumpList()->insert(a.textKey,slideCount);
            QListIterator<QString> albumIt(serverInfo->Artist2AlbumIds().value(a.name));
            while(albumIt.hasNext()) {
                artists.append(serverInfo->AlbumID2AlbumInfo().value(albumIt.next()));
                slideCount++;
            }
        }
        artistselectCoverFlow->LoadAlbumList(artists);
        connect(artistselectCoverFlow, SIGNAL(SelectSlide(int)),
                this,SLOT(ArtistAlbumCoverFlowSelect()));

        // set up Album Selection Coverflow
        albumselectCoverFlow->GetAlbumKeyTextJumpList()->clear();
        albumselectCoverFlow->GetTitleJumpList()->clear();
        slideCount = 0; // reset
        QListIterator<Album> albumIt(serverInfo->GetAllAlbumList());
        while(albumIt.hasNext()) {
            Album a = albumIt.next();
            albumselectCoverFlow->GetTitleJumpList()->insert(a.albumtitle,slideCount);
            if(!albumselectCoverFlow->GetAlbumKeyTextJumpList()->contains(a.albumTextKey))
                albumselectCoverFlow->GetAlbumKeyTextJumpList()->insert(a.albumTextKey,slideCount);
            slideCount++;
        }
        albumselectCoverFlow->LoadAlbumList(serverInfo->GetAllAlbumList());
        connect(albumselectCoverFlow, SIGNAL(SelectSlide(int)),
                this,SLOT(AlbumCoverFlowSelect()));
    }
}

void MainWindow::ArtistAlbumCoverFlowSelect(void)
{
    DEBUGF(QString("playlistcontrol cmd:load album_id:%1").arg(artistselectCoverFlow->GetCenterAlbum().album_id.data()));
    activeDevice->SendDeviceCommand(QString("playlistcontrol cmd:load album_id:%1")
                                    .arg(artistselectCoverFlow->GetCenterAlbum().album_id.data()));
}

void MainWindow::AlbumCoverFlowSelect(void)
{
    DEBUGF("");
    activeDevice->SendDeviceCommand(QString("playlistcontrol cmd:load album_id:%1")
                                    .arg(albumselectCoverFlow->GetCenterAlbum().album_id.data()));
}

void MainWindow::ChangeCoverflowDisplay(void)
{
    DEBUGF("");
    if(lastMenuHeading0 == activeDevice->getDisplayBuffer()->line0
            && lastMenuHeading1 == activeDevice->getDisplayBuffer()->line1) {
        return;
    }

    lastMenuHeading0 = activeDevice->getDisplayBuffer()->line0;
    lastMenuHeading1 = activeDevice->getDisplayBuffer()->line1;

    /*

    QString curArtist;
    switch(cfType) {
    case DISPLAY_ARTISTSELECTION:
        curArtist = artistselectCoverFlow->GetCenterAlbum().artist;
        while(artistselectCoverFlow->GetCenterAlbum().artist == curArtist) {
            artistselectCoverFlow->showNext();
            curArtist = artistselectCoverFlow->GetCenterAlbum().artist;
        }
        break;
    case DISPLAY_ALBUMSELECTION:
        albumselectCoverFlow->showNext();
        break;
    case DISPLAY_PLAYLIST:
    default:
        break;
    }
    QString curArtist;
    switch(cfType) {
    case DISPLAY_ARTISTSELECTION:
         curArtist = artistselectCoverFlow->GetCenterAlbum().artist;
        while(artistselectCoverFlow->GetCenterAlbum().artist == curArtist) {
            artistselectCoverFlow->showPrevious();
            curArtist = artistselectCoverFlow->GetCenterAlbum().artist;
        }
        break;
    case DISPLAY_ALBUMSELECTION:
        albumselectCoverFlow->showPrevious();
        break;
    case DISPLAY_PLAYLIST:
    default:
        break;
    }


  */

    if(activeDevice->getDisplayBuffer()->line0=="Artists") {
        playlistCoverFlow->hide();
        ui->cfPlaylistWidget->hide();
        albumselectCoverFlow->hide();
        ui->cfAlbumSelectWidget->hide();

        if(artistselectCoverFlow->GetTitleJumpList()->contains(activeDevice->getDisplayBuffer()->line1)) {
            artistselectCoverFlow->FlipToSlide(
                        artistselectCoverFlow->GetTitleJumpList()->value(activeDevice->getDisplayBuffer()->line1));
        } else {
            artistselectCoverFlow->setCenterIndex(0);
        }
        artistselectCoverFlow->show();
        ui->cfArtistSelectWidget->show();
        cfType = DISPLAY_ARTISTSELECTION;
    }
    else if(activeDevice->getDisplayBuffer()->line0=="Albums") {
        playlistCoverFlow->hide();
        ui->cfPlaylistWidget->hide();
        artistselectCoverFlow->hide();
        ui->cfArtistSelectWidget->hide();
        if(albumselectCoverFlow->GetTitleJumpList()->contains(activeDevice->getDisplayBuffer()->line1)) {
            albumselectCoverFlow->FlipToSlide(
                        albumselectCoverFlow->GetTitleJumpList()->value(activeDevice->getDisplayBuffer()->line1));
        } else {
            albumselectCoverFlow->setCenterIndex(0);
        }
        albumselectCoverFlow->show();
        ui->cfAlbumSelectWidget->show();
        cfType = DISPLAY_ALBUMSELECTION;
    }
    else {
        albumselectCoverFlow->hide();
        ui->cfAlbumSelectWidget->hide();
        artistselectCoverFlow->hide();
        ui->cfArtistSelectWidget->hide();
        playlistCoverFlow->show();
        ui->cfPlaylistWidget->show();
        cfType = DISPLAY_PLAYLIST;
    }
}

void MainWindow::UpdateCoverflowFromKeypad(int key)
{
    if(key == lastKey && keypadTimer.isActive()) {
        keypadTimer.stop();
        keyOffset>=4 ? keyOffset=0 : keyOffset++;
        if(keypadKey[key][keyOffset]==' ')
            keyOffset=0;
    }
    lastKey=key;
    if(activeDevice->getDisplayBuffer()->line0=="Artists") {
        artistselectCoverFlow->JumpTo(QString(keypadKey[key][keyOffset]));
    } else if(activeDevice->getDisplayBuffer()->line0=="Albums") {
        albumselectCoverFlow->JumpTo(QString(keypadKey[key][keyOffset]));
    }
    activeDevice->SendDeviceCommand(QString("button %1\n").arg(key));
    keypadTimer.start(1000);
}

void MainWindow::ResetKeypadTimer(void)
{
    keypadTimer.stop();
    keyOffset = 0;
}

//void MainWindow::ChangeToAlbumSelection(int tab)
//{
//    if(tab==1) {    // artist tab
//        activeDevice->SendDeviceCommand(QString("ir %1 %2").arg(IR_menu_browse_artist,0,16).arg(progstart.elapsed()));
//        qDebug() << QString("ir %1 %2").arg(IR_menu_browse_artist,0,16).arg(progstart.elapsed());
//    }
//}

void MainWindow::slotPlay( void )
{
//    if(cfType==DISPLAY_ARTISTSELECTION)
//        ArtistAlbumCoverFlowSelect();
//    else if(cfType==DISPLAY_ALBUMSELECTION)
//        AlbumCoverFlowSelect();
//    else
        activeDevice->SendDeviceCommand( QString( "button play.single\n" ) );
    slotNowPlaying();
}

void MainWindow::slotAdd( void )
{
//    if(cfType==DISPLAY_ARTISTSELECTION)
//        activeDevice->SendDeviceCommand(QString("playlistcontrol cmd:add album_id:%1")
//                                        .arg(artistselectCoverFlow->GetCenterAlbum().album_id.data()));
//    else if(cfType==DISPLAY_ALBUMSELECTION)
//        activeDevice->SendDeviceCommand(QString("playlistcontrol cmd:add album_id:%1")
//                                        .arg(albumselectCoverFlow->GetCenterAlbum().album_id.data()));
//    else
        activeDevice->SendDeviceCommand( QString( "button add.single\n" ) );
    slotNowPlaying();
}

void MainWindow::slotLeftArrow( void )
{
    DEBUGF("");
//    if(cfType==DISPLAY_ARTISTSELECTION) {
//        artistselectCoverFlow->showPrevious();
//    } else if(cfType== DISPLAY_ALBUMSELECTION) {
//        albumselectCoverFlow->showPrevious();
//    } else {
        m_disp->LeftArrowEffect();
        activeDevice->SendDeviceCommand( QString( "button arrow_left\n" ) );
//    }
}

void MainWindow::slotRightArrow( void )
{
    DEBUGF("");
//    if(cfType==DISPLAY_ARTISTSELECTION) {
//        artistselectCoverFlow->showNext();
//    } else if(cfType== DISPLAY_ALBUMSELECTION) {
//        albumselectCoverFlow->showNext();
//    } else {
        m_disp->RightArrowEffect();
        activeDevice->SendDeviceCommand( QString( "button arrow_right\n" ) );
//    }
}

void MainWindow::slotUpArrow( void )
{
    DEBUGF("");
    m_disp->UpArrowEffect();
    activeDevice->SendDeviceCommand( QString( "button arrow_up\n" ) );
}

void MainWindow::slotDownArrow( void )
{
    DEBUGF("");
    m_disp->DownArrowEffect();
    activeDevice->SendDeviceCommand( QString( "button arrow_down\n" ) );
}

void MainWindow::slotSetActivePlayer( void )
{
    DEBUGF("");
    if( isStartUp ) { // if this is the first time through, let's trigger a process to make sure we have all the images
    }
    slotSetActivePlayer( serverInfo->GetDeviceFromMac( MacAddress.toPercentEncoding().toLower() ) );
    slotEnablePlayer();
}

void MainWindow::slotSetActivePlayer( SlimDevice *d )
{
    DEBUGF("");
    serverInfo->SetCurrentDevice( d );
    m_disp->SetActiveDevice(d);
    m_disp->slotUpdateSlimDisplay();
    activeDevice = d;
    connect( activeDevice, SIGNAL(NewSong()),
             m_disp, SLOT(slotResetSlimDisplay()) );
    connect( activeDevice, SIGNAL(SlimDisplayUpdate()),
             m_disp, SLOT(slotUpdateSlimDisplay()) );
    connect( activeDevice, SIGNAL(SlimDisplayUpdate()),
             this, SLOT(ChangeCoverflowDisplay()) );
    connect( activeDevice, SIGNAL(playlistCoverFlowUpdate( int )),
             this, SLOT(slotUpdateplaylistCoverFlow(int)) );
    connect( activeDevice, SIGNAL(playlistCoverFlowCreate()),
             this, SLOT(slotCreateplaylistCoverFlow()) );
    connect( playlistCoverFlow, SIGNAL(CoverFlowReady()),
             this, SLOT(slotplaylistCoverFlowReady()));
}

void MainWindow::applyDisplayConfig(void)
{
    DEBUGF("DISPLAY CONFIG");
    if( m_disp==NULL)
        DEBUGF("display isn't allocated");
    m_disp->setTextColor(displayTextColor);
    m_disp->setDisplayBackgroundColor(displayBackground);
    playlistCoverFlow->setBackgroundColor(coverflowBackground);
    albumselectCoverFlow->setBackgroundColor(coverflowBackground);
    artistselectCoverFlow->setBackgroundColor(coverflowBackground);
    tempdisplayTextColor = m_disp->getTextColor();
    tempdisplayBackground = m_disp->getDisplayBackgroundColor();
    tempcoverflowBackground = coverflowBackground;
    m_disp->setScrollSpeed(scrollSpeed);
    m_disp->setScrollInterval(scrollInterval);
}

//void MainWindow::applyConnectionConfig(void)
//{
//    DEBUGF("CONNECTION CONFIG");
//    SlimServerAddr = mySettings->value("Server/Address","127.0.0.1").toString();
//    SlimServerAudioPort = mySettings->value("Server/AudioPort","3483").toString();
//    SlimServerCLIPort = mySettings->value("Server/CLIPort", "9090").toString();
//    SlimServerHttpPort = mySettings->value("Server/HttpPort", "9000").toString();
//    PortAudioDevice = mySettings->value("Audio/Device","").toString();
//}

void MainWindow::saveDisplayConfig(void)
{
    DEBUGF("");
    mySettings->setValue("UI/CoverFlowColor",(uint)coverflowBackground.rgb());
    mySettings->setValue("UI/DisplayBackground",(uint)displayBackground.rgb());
    mySettings->setValue("UI/DisplayTextColor", (uint)displayTextColor.rgb());
    mySettings->setValue("UI/ScrollInterval",scrollInterval);
    mySettings->setValue("UI/ScrollSpeed",scrollSpeed);
    mySettings->sync();
}

void MainWindow::saveConnectionConfig(void)
{
    DEBUGF("");
    mySettings->setValue("Server/Address",SlimServerAddr);
    mySettings->setValue("Server/AudioPort",SlimServerAudioPort);
    mySettings->setValue("Server/CLIPort", SlimServerCLIPort);
    mySettings->setValue("Server/HttpPort", SlimServerHttpPort);
    mySettings->setValue("Audio/Device", PortAudioDevice);
    mySettings->sync();
}

void MainWindow::setConfigDisplay2Defaults(void)
{
    DEBUGF("");
    mySettings->setValue("UI/CoverFlowColor",(uint)QColor(Qt::white).rgb());
    mySettings->setValue("UI/DisplayBackground",(uint)QColor(Qt::black).rgb());
    mySettings->setValue("UI/DisplayTextColor",(uint)QColor(Qt::cyan).rgb());
    mySettings->setValue("UI/ScrollInterval",5000);
    mySettings->setValue("UI/ScrollSpeed",30);
    mySettings->sync();

    coverflowBackground = QColor::fromRgb(mySettings->value("UI/CoverFlowColor",QColor(Qt::white).rgb()).toUInt());
    displayBackground = QColor::fromRgb(mySettings->value("UI/DisplayBackground",QColor(Qt::black).rgb()).toUInt());
    displayTextColor = QColor::fromRgb(mySettings->value("UI/DisplayTextColor",QColor(Qt::cyan).rgb()).toUInt());
    scrollInterval = mySettings->value("UI/ScrollInterval",5000).toInt();
    scrollSpeed = mySettings->value("UI/ScrollSpeed",30).toInt();

}

void MainWindow::setConfigConnection2Defaults(void)
{
    DEBUGF("");
    mySettings->setValue("Server/Address","127.0.0.1");
    mySettings->setValue("Server/AudioPort","3483");
    mySettings->setValue("Server/CLIPort", "9090");
    mySettings->setValue("Server/HttpPort", "9000");
    mySettings->setValue("Audio/Device","");
    mySettings->sync();

}

void MainWindow::loadConfig(void)
{
    DEBUGF("");
    QProcess proc;
    QStringList args;
    args.append(QString("-L"));
    proc.start("squeezeslave", args );

    proc.waitForReadyRead(2000);

    QByteArray m_out = proc.readAllStandardOutput();
    outDevs = m_out.split('\n');
    proc.close();

    tempcoverflowBackground = coverflowBackground = QColor::fromRgb(mySettings->value("UI/CoverFlowColor",QColor(Qt::white).rgb()).toUInt());
    tempdisplayBackground = displayBackground = QColor::fromRgb(mySettings->value("UI/DisplayBackground",QColor(Qt::black).rgb()).toUInt());
    tempdisplayTextColor = displayTextColor = QColor::fromRgb(mySettings->value("UI/DisplayTextColor",QColor(Qt::cyan).rgb()).toUInt());
    scrollInterval = mySettings->value("UI/ScrollInterval",5000).toInt();
    scrollSpeed = mySettings->value("UI/ScrollSpeed",30).toInt();

    lmsUsername = mySettings->value("Server/Username","").toString();
    lmsPassword = mySettings->value("Server/Password","").toString();
    SlimServerAddr = mySettings->value("Server/Address","127.0.0.1").toString();
    SlimServerAudioPort = mySettings->value("Server/AudioPort","3483").toString();
    SlimServerCLIPort = mySettings->value("Server/CLIPort", "9090").toString();
    SlimServerHttpPort = mySettings->value("Server/HttpPort", "9000").toString();
    PortAudioDevice = mySettings->value("Audio/Device","").toString();
}

void MainWindow::setCoverFlowColor(void)
{
    DEBUGF("");
    QColorDialog *dlg = new QColorDialog(coverflowBackground);
    dlg->exec();
    tempcoverflowBackground = dlg->selectedColor();
}

void MainWindow::setDisplayBackgroundColor(void)
{
    DEBUGF("");
    QColorDialog *dlg = new QColorDialog(m_disp->getDisplayBackgroundColor());
    dlg->exec();
    tempdisplayBackground = dlg->selectedColor();
}

void MainWindow::setDisplayTextColor(void)
{
    DEBUGF("");
    QColorDialog *dlg = new QColorDialog(m_disp->getTextColor());
    dlg->exec();
    tempdisplayTextColor = dlg->selectedColor();
}


void MainWindow::getplayerMACAddress( void )
{
    DEBUGF("");
    MacAddress = QByteArray( "00:00:00:00:00:04" );

    QList<QNetworkInterface> netList = QNetworkInterface::allInterfaces();
    QListIterator<QNetworkInterface> i( netList );

    QNetworkInterface t;

    while( i.hasNext() ) {  // note: this grabs the first functional, non-loopback address there is.  It may not the be interface on which you really connect to the slimserver
        t = i.next();
        if( !t.flags().testFlag( QNetworkInterface::IsLoopBack ) &&
                t.flags().testFlag( QNetworkInterface::IsUp ) &&
                t.flags().testFlag( QNetworkInterface::IsRunning ) ) {
            MacAddress = t.hardwareAddress().toAscii().toLower();
            return;
        }
    }
}

void MainWindow::slotDisablePlayer( void )
{
    DEBUGF("");
    waitWindow->show();
    this->setEnabled( false );
}

void MainWindow::slotEnablePlayer( void )
{
    DEBUGF("");
    waitWindow->hide();
    slotNowPlaying();   // trigger display update
    setEnabled( true );
    SetupSelectionCoverFlows();
}

void MainWindow::SqueezePlayerError( void )
{
    QString errMsg = QString( "SQUEEZE ERROR: " ) + squeezePlayer->readAllStandardError();
    DEBUGF(errMsg);
}

void MainWindow::SqueezePlayerOutput( void )
{
    QString errMsg = QString( "SQUEEZE OUTPUT: " ) + squeezePlayer->readAllStandardOutput();
    DEBUGF(errMsg);
}

void MainWindow::on_btnSetup_clicked()
{
    QDialog dlg;
    Ui_Dialog *dlgUI = new Ui_Dialog();
    dlgUI->setupUi(&dlg);

    QListIterator<QByteArray> i(outDevs);
    if(i.peekNext().left(6) == "Output") {
        i.next();
    }
    while(i.hasNext()){
        dlgUI->cbAudioOutput->addItem(i.next());
    }

    QPixmap p = QPixmap(64,37);
    p.fill(coverflowBackground.rgb());
    dlgUI->lblCoverFlowColor->setPixmap(p);
    p.fill(m_disp->getDisplayBackgroundColor().rgb());
    dlgUI->lblDisplayBackgroundColor->setPixmap(p);
    p.fill(m_disp->getTextColor().rgb());
    dlgUI->lblDisplayTextColor->setPixmap(p);
    dlgUI->spnInterval->setValue(scrollInterval);
    dlgUI->spnSpeed->setValue(scrollSpeed);

    dlgUI->txtServerAddress->setText(SlimServerAddr);
    dlgUI->txtHttpPort->setText(SlimServerHttpPort);
    dlgUI->txtCLIPort->setText(SlimServerCLIPort);
    dlgUI->txtAudioPort->setText(SlimServerAudioPort);
    dlgUI->txtPassword->setText(lmsPassword);
    dlgUI->txtUsername->setText(lmsUsername);

    int audioItem = -2;
    if(PortAudioDevice.length()>0) {
        audioItem = dlgUI->cbAudioOutput->findText(PortAudioDevice);
        dlgUI->cbAudioOutput->setCurrentIndex(audioItem);
    }

    connect(dlgUI->btnCoverFlowColor,SIGNAL(clicked()),
            this,SLOT(setCoverFlowColor()));
    connect(dlgUI->btnDisplayBackgroundColor,SIGNAL(clicked()),
            this,SLOT(setDisplayBackgroundColor()));
    connect(dlgUI->btnDisplayTextColor,SIGNAL(clicked()),
            this,SLOT(setDisplayTextColor()));
    connect(dlgUI->btnConnectionDefaults,SIGNAL(clicked()),
            this,SLOT(setConfigConnection2Defaults()));
    connect(dlgUI->btnDisplayDefaults,SIGNAL(clicked()),
            this,SLOT(setConfigDisplay2Defaults()));

    dlg.exec();
    if(dlg.result()==QDialog::Accepted) {
        scrollInterval = dlgUI->spnInterval->value();
        scrollSpeed = dlgUI->spnSpeed->value();
        coverflowBackground = tempcoverflowBackground;
        displayBackground = tempdisplayBackground;
        displayTextColor = tempdisplayTextColor;

        SlimServerAddr = dlgUI->txtServerAddress->text();
        SlimServerHttpPort = dlgUI->txtHttpPort->text();
        SlimServerCLIPort = dlgUI->txtCLIPort->text();
        SlimServerAudioPort = dlgUI->txtAudioPort->text();
        lmsPassword = dlgUI->txtPassword->text();
        lmsUsername = dlgUI->txtUsername->text();
        if(dlgUI->cbAudioOutput->currentIndex()==audioItem)
            PortAudioDevice = dlgUI->cbAudioOutput->currentText();
        applyDisplayConfig();
    }

    disconnect(dlgUI->btnCoverFlowColor,SIGNAL(clicked()),
               this,SLOT(setCoverFlowColor()));
    disconnect(dlgUI->btnDisplayBackgroundColor,SIGNAL(clicked()),
               this,SLOT(setDisplayBackgroundColor()));
    disconnect(dlgUI->btnDisplayTextColor,SIGNAL(clicked()),
               this,SLOT(setDisplayTextColor()));
    disconnect(dlgUI->btnConnectionDefaults,SIGNAL(clicked()),
               this,SLOT(setConfigConnection2Defaults()));
    disconnect(dlgUI->btnDisplayDefaults,SIGNAL(clicked()),
               this,SLOT(setConfigDisplay2Defaults()));

}


/* vim: set expandtab tabstop=4 shiftwidth=4: */


