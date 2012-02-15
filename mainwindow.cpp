#include <QMovie>

#include "mainwindow.h"
#include "ui_mainwindow.h"

// uncomment the following to turn on debugging
#define SLIMCLI_DEBUG

#ifdef SLIMCLI_DEBUG
#define DEBUGF(...) qDebug() << this->objectName() << Q_FUNC_INFO << __VA_ARGS__;
#else
#define DEBUGF(...)
#endif

SlimImageItem imageCache;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    DEBUGF(QTime::currentTime());
    ui->setupUi(this);
    squeezePlayer = new QProcess( this );
    slimCLI = new SlimCLI( this );
    serverInfo = new SlimServerInfo(this);

    mySettings = new QSettings("qtsqueeze3", "qtsqueeze3");
//    vertTransTimer = new QTimeLine( 350, this );
//    horzTransTimer = new QTimeLine( 700, this );
//    bumpTransTimer = new QTimeLine( 100, this );
//    vertTransTimer->setFrameRange( 0, 0 );
//    horzTransTimer->setFrameRange( 0, 0 );
//    bumpTransTimer->setFrameRange( 0, 0 );
//    isTransition = false;
//    transitionDirection = transNONE;
//    xOffsetOld = xOffsetNew = 0;
//    yOffsetOld = yOffsetNew = 0;
    activeDevice = NULL;
    //    PortAudioDevice = "";
//    ScrollOffset = 0;
//    scrollState = NOSCROLL;
//    Brightness = 255;
//    line1Alpha = 0;
    isStartUp = true;
//    connect( &scrollTimer, SIGNAL( timeout() ), this, SLOT(slotUpdateScrollOffset()) );
//    connect( vertTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
//    connect( horzTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
//    connect( bumpTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
//    connect( vertTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
//    connect( horzTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
//    connect( bumpTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));

    DEBUGF( "CONNECTING OPTIONS PAGE");
    connect(ui->btnApplyConnection, SIGNAL(clicked()),this,SLOT(updateConnectionConfig()));
    connect(ui->btnDefaultsConnection, SIGNAL(clicked()), this, SLOT(setConfigConnection2Defaults()));
    connect(ui->btnApplyDisplay,SIGNAL(clicked()),this,SLOT(updateDisplayConfig()));
    connect(ui->btnDefaultsDisplay,SIGNAL(clicked()),this,SLOT(setConfigDisplay2Defaults()));

    connect(ui->btnCoverFlowColor,SIGNAL(clicked()),this,SLOT(setCoverFlowColor()));
    connect(ui->btnDisplayBackgroundColor,SIGNAL(clicked()),this,SLOT(setDisplayBackgroundColor()));
    connect(ui->btnDisplayTextColor,SIGNAL(clicked()),this,SLOT(setDisplayTextColor()));

    QPixmap splash;
    splash.load( ":/img/lib/images/squeezeplayer_large.png" );
    waitWindow = new QSplashScreen( this, splash );
    waitWindow->setGeometry( ( geometry().width() - 400 )/ 2, ( geometry().height() - 200 )/2,
                             400, 200 );

    m_disp = new SqueezeDisplay(ui->lblSlimDisplay);

    loadDisplayConfig();
    loadConnectionConfig();
    m_disp->Init();
    DEBUGF("FINISHED LOADING CONFIGS");

    waitWindow->hide();
    connect( slimCLI, SIGNAL(cliInfo(QString)), waitWindow, SLOT(showMessage(QString)) );

}

MainWindow::~MainWindow()
{
    mySettings->setValue("UI/CurrentTab",ui->tabWidget->currentIndex());
    mySettings->sync();
    qDebug() << "current index: " << ui->tabWidget->currentIndex();
    qDebug() << mySettings->fileName() << " is writable " << mySettings->isWritable();
    squeezePlayer->close();
    delete ui;
}


bool MainWindow::Create(void)
{
    /*
    There is a fairly large amount of work to do on startransUP, but we don't want to keep the user guessing,
    so we first create the window to give a visual cue that things have started and then we send a signal
    to the program to do the rest of the processing, while displaying a progress bar to keep the user interested
*/
    slotDisablePlayer();

    DEBUGF("SETTING UP INFO ON CONFIG PAGE");
    setupConfig();  // load defaults into config

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
        qDebug() << PortAudioDevice;
        PortAudioDevice = PortAudioDevice.left(PortAudioDevice.indexOf(":")).trimmed();
        qDebug() << PortAudioDevice;
        args.append(QString("-o"+PortAudioDevice));
    }
    args.append(SlimServerAddr);

    qDebug() << program << " " << args;

    DEBUGF( "player command " << program << " " << args );

    QTime progstart;
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

    // establish the proper URL for the web interface
    QUrl slimWeb( QString( "http://") + SlimServerAddr + QString( ":"+SlimServerHttpPort+"/" ) );
    ui->webView->setUrl( slimWeb );

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
//    connect( CoverFlow, SIGNAL(NextSlide()), this, SLOT(slotFForward()) );
//    connect( CoverFlow, SIGNAL(PrevSlide()), this, SLOT(slotPrev()) );

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
    slotEnablePlayer();
    return true;
}

void MainWindow::slotUpdateCoverFlow( int trackIndex )
{
    DEBUGF(QTime::currentTime());
//    if(!CoverFlow->IsReady())
//        return;

//    int currSlide = CoverFlow->centerIndex();
//    DEBUGF( "UPDATE COVERFLOW TO INDEX: " << trackIndex );
//    if( abs( trackIndex - currSlide ) > 4 ) {
//        if( trackIndex > currSlide ) {
//            CoverFlow->showSlide( currSlide + 2 );
//            CoverFlow->setCenterIndex( trackIndex - 2 );
//        }
//        else {
//            CoverFlow->showSlide( currSlide - 2 );
//            CoverFlow->setCenterIndex( trackIndex + 2 );
//        }
//    }
//    CoverFlow->showSlide( trackIndex );
}

void MainWindow::slotCreateCoverFlow( void )
{
    DEBUGF(QTime::currentTime());
    //  delete CoverFlow; // there seems to be some memory leakage with CoverFlow when you clear it and add new items
    //  CoverFlow = new SqueezePictureFlow( ui->cfWidget );
    //  CoverFlow->setMinimumSize( flowRect.width(), flowRect.height() );
    //  CoverFlow->setContentsMargins( 50, 0, CoverFlow->width() - 50, CoverFlow->height() );
    CoverFlow->clear();
    ui->cfWidget->setEnabled( false );

//    if( CoverFlow->LoadAlbumList(activeDevice->getDevicePlayList())) {
//        ui->cfWidget->setEnabled( true );
//        DEBUGF( "CURRENT PLAYLIST INDEX IS: " << activeDevice->getDevicePlaylistIndex() );
//        int playListIndex = activeDevice->getDevicePlaylistIndex();
//        if( playListIndex > 4 )
//            CoverFlow->setCenterIndex( playListIndex - 4 );
//        CoverFlow->showSlide( playListIndex );
//    }
}

void MainWindow::slotLeftArrow( void )
{
    DEBUGF(QTime::currentTime());
//    StopScroll();
//    isTransition = true;
//    transitionDirection = transRIGHT;
//    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
//    if( activeDevice->getDisplayBuffer().line0 == "Squeezebox Home" ) {  // are we at the "left-most" menu option?
//        bumpTransTimer->start();
//    }
//    else {
//        horzTransTimer->start();
//    }
    activeDevice->SendDeviceCommand( QString( "button arrow_left\n" ) );
}

void MainWindow::slotRightArrow( void )
{
    DEBUGF(QTime::currentTime());
//    StopScroll();
//    isTransition = true;
//    transitionDirection = transLEFT;
//    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
//    horzTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_right\n" ) );
}

void MainWindow::slotUpArrow( void )
{
    DEBUGF(QTime::currentTime());
//    StopScroll();
//    isTransition = true;
//    transitionDirection = transDOWN;
//    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
//    vertTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_up\n" ) );
}

void MainWindow::slotDownArrow( void )
{
    DEBUGF(QTime::currentTime());
//    StopScroll();
//    isTransition = true;
//    transitionDirection = transUP;
//    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
//    vertTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_down\n" ) );
}

//void MainWindow::slotUpdateScrollOffset( void )
//{
//    DEBUGF("SLOTUPDATESCROLLOFFSET");
//    if( scrollState == PAUSE_SCROLL ) {
//        scrollTimer.stop();
//        scrollState = SCROLL;
//        scrollTimer.setInterval( 33 );
//        scrollTimer.start();
//    }
//    else if( scrollState == SCROLL ) {
//        ScrollOffset += Line1FontWidth;
//        //        if( ScrollOffset >= ( lineWidth - Line1Rect.width()) ) {
//        if( ScrollOffset >= lineWidth ) {
//            scrollState = FADE_OUT;
//            line1Alpha = 0;
//        }
//    }
//    else if( scrollState == FADE_IN ) {
//        line1Alpha -= 15;
//        if( line1Alpha <= 0 ) {
//            line1Alpha = 0;
//            ScrollOffset = 0;
//            scrollTimer.stop();
//            scrollState = PAUSE_SCROLL;
//            scrollTimer.setInterval( 5000 );
//            scrollTimer.start();
//        }
//    }
//    else if( scrollState == FADE_OUT ) {
//        line1Alpha += 7;
//        ScrollOffset += Line1FontWidth; // keep scrolling while fading
//        if( line1Alpha >= 255 ) {
//            line1Alpha = 255;
//            ScrollOffset = 0;
//            scrollState = FADE_IN;
//        }
//    }
//    PaintTextDisplay();
//}

//void MainWindow::slotUpdateTransition( int frame )
//{
//    switch( transitionDirection ) {
//    case transLEFT:
//        xOffsetOld = 0 - frame;
//        xOffsetNew = displayRect.width() - frame;
//        yOffsetOld = yOffsetNew = 0;
//        break;
//    case transRIGHT:
//        xOffsetOld = 0 + frame;
//        xOffsetNew = frame - displayRect.width();
//        yOffsetOld = yOffsetNew = 0;
//        break;
//    case transUP:
//        xOffsetOld = xOffsetNew = 0;
//        yOffsetOld = 0 - frame;
//        yOffsetNew = Line1Rect.height() - frame;
//        break;
//    case transDOWN:
//        xOffsetOld = xOffsetNew = 0;
//        yOffsetOld = 0 + frame;
//        yOffsetNew = frame - Line1Rect.height();
//        break;
//    case transNONE:
//    default:
//        xOffsetOld = xOffsetNew = 0;
//        yOffsetOld = yOffsetNew = 0;
//        break;
//    }
//    PaintTextDisplay();
//}

//void MainWindow::slotTransitionFinished ( void )
//{
//    vertTransTimer->stop();
//    horzTransTimer->stop();
//    bumpTransTimer->stop();
//    isTransition = false;
//    xOffsetOld = xOffsetNew = 0;
//    yOffsetOld = yOffsetNew = 0;
//    transitionDirection = transNONE;
//    slotResetSlimDisplay();
//}

//void MainWindow::StopScroll( void )
//{
//    scrollTimer.stop();
//    ScrollOffset = 0;
//    line1Alpha = 0;
//    scrollState = PAUSE_SCROLL;
//}


void MainWindow::slotSetActivePlayer( void )
{
    DEBUGF(QTime::currentTime());
    if( this->isStartUp ) { // if this is the first time through, let's trigger a process to make sure we have all the images
    }
    slotSetActivePlayer( serverInfo->GetDeviceFromMac( MacAddress.toPercentEncoding().toLower() ) );
}

void MainWindow::slotSetActivePlayer( SlimDevice *d )
{
    DEBUGF(QTime::currentTime());
    serverInfo->SetCurrentDevice( d );
    m_disp->SetActiveDevice(d);
    m_disp->slotUpdateSlimDisplay();
    activeDevice = d;
    slotCreateCoverFlow();
    connect( activeDevice, SIGNAL(NewSong()),
             m_disp, SLOT(slotResetSlimDisplay()) );
    connect( activeDevice, SIGNAL(SlimDisplayUpdate()),
             m_disp, SLOT(slotUpdateSlimDisplay()) );
    connect( activeDevice, SIGNAL(CoverFlowUpdate( int )),
             this, SLOT(slotUpdateCoverFlow(int)) );
    connect( activeDevice, SIGNAL(CoverFlowCreate()),
             this, SLOT(slotCreateCoverFlow()) );

    /*
  connect( d, SIGNAL(NewSong()),
           this, SLOT(slotUpdateSlimDisplay()) );
  connect( d, SIGNAL(NewPlaylist()),
           this, SLOT(slotUpdateCoverFlow()) );
    void NewSong( void );
    void NewPlaylist( void );
    void VolumeChange( int newvol );
    void ModeChange( QString newmode );
    void Mute( bool mute );
    // void HeartBeat( void );
//    void GotSongCover( QPixmap image );
    void UpdateCoverFlow( QPixmap image );
    void ClientChange( QByteArray mydeviceMAC, QByteArray myresponse );
    void DisplayUpdate( void );
    void CoversReady( void );

    void Duration( float t );
    void TimeText( QString tt );
*/
}

void MainWindow::loadDisplayConfig(void)
{
    DEBUGF("DISPLAY CONFIG");
    if( m_disp==NULL)
        DEBUGF("display isn't allocated");
    m_disp->setTextColor(QColor::fromRgb(mySettings->value("UI/DisplayTextColor",QColor(Qt::cyan).rgb()).toUInt()));
    m_disp->setDisplayBackgroundColor(QColor::fromRgb(mySettings->value("UI/DisplayBackground",QColor(Qt::black).rgb()).toUInt()));
    coverflowBackground = QColor::fromRgb(mySettings->value("UI/CoverFlowColor",QColor(Qt::white).rgb()).toUInt());
    temptextcolorGeneral = m_disp->getTextColor();
    tempdisplayBackgroundColor = m_disp->getDisplayBackgroundColor();
    tempcoverflowBackground = coverflowBackground;
    //    textcolorGeneral= QColor(Qt::cyan);
    //    textcolorLine1 = QColor(Qt::cyan);
    //    displayBackgroundColor = QColor(Qt::black);
    //    coverflowBackground = QColor(Qt::white);
    m_disp->setScrollSpeed(mySettings->value("UI/ScrollInterval",5000).toInt());
    m_disp->setScrollInterval(mySettings->value("UI/ScrollSpeed",30).toInt());
    DEBUGF("DISPLAY CONFIG");
}

void MainWindow::loadConnectionConfig(void)
{
    DEBUGF("CONNECTION CONFIG");
    SlimServerAddr = mySettings->value("Server/Address","127.0.0.1").toString();
    SlimServerAudioPort = mySettings->value("Server/AudioPort","3483").toString();
    SlimServerCLIPort = mySettings->value("Server/CLIPort", "9090").toString();
    SlimServerHttpPort = mySettings->value("Server/HttpPort", "9000").toString();
    PortAudioDevice = mySettings->value("Audio/Device","").toString();
    DEBUGF("CONNECTION CONFIG");
}

void MainWindow::updateDisplayConfig(void)
{
    DEBUGF(QTime::currentTime());
    mySettings->setValue("UI/CoverFlowColor",(uint)tempcoverflowBackground.rgb());
    mySettings->setValue("UI/DisplayBackground",(uint)tempdisplayBackgroundColor.rgb());
    mySettings->setValue("UI/DisplayTextColor", (uint)temptextcolorGeneral.rgb());
    mySettings->setValue("UI/ScrollInterval",ui->spnInterval->value());
    mySettings->setValue("UI/ScrollSpeed",ui->spnSpeed->value());
    mySettings->sync();

    loadDisplayConfig();
//    CoverFlow->setBackgroundColor(coverflowBackground);
    QPixmap p = QPixmap(64,37);
    p.fill(coverflowBackground.rgb());
    ui->lblCoverFlowColor->setPixmap(p);
    p.fill(m_disp->getDisplayBackgroundColor().rgb());
    ui->lblDisplayBackgroundColor->setPixmap(p);
    p.fill(m_disp->getTextColor().rgb());
    ui->lblDisplayTextColor->setPixmap(p);
}

void MainWindow::updateConnectionConfig(void)
{
    DEBUGF(QTime::currentTime());
    mySettings->setValue("Server/Address",ui->txtServerAddress->text());
    mySettings->setValue("Server/AudioPort",ui->txtAudioPort->text());
    mySettings->setValue("Server/CLIPort", ui->txtCLIPort->text());
    mySettings->setValue("Server/HttpPort", ui->txtHttpPort->text());
    mySettings->setValue("Audio/Device",ui->cbAudioOutput->currentText());
    mySettings->sync();

    ui->lblUpdateOnRestart->setText("Changes Take Effect on Restart");
}

void MainWindow::setConfigDisplay2Defaults(void)
{
    DEBUGF(QTime::currentTime());
    mySettings->setValue("UI/CoverFlowColor",(uint)QColor(Qt::white).rgb());
    mySettings->setValue("UI/DisplayBackground",(uint)QColor(Qt::black).rgb());
    mySettings->setValue("UI/DisplayTextColor",(uint)QColor(Qt::cyan).rgb());
    mySettings->setValue("UI/ScrollInterval",5000);
    mySettings->setValue("UI/ScrollSpeed",30);
    mySettings->sync();

    loadDisplayConfig();
//    CoverFlow->setBackgroundColor(coverflowBackground);
}

void MainWindow::setConfigConnection2Defaults(void)
{
    DEBUGF(QTime::currentTime());
    mySettings->setValue("Server/Address","127.0.0.1");
    mySettings->setValue("Server/AudioPort","3483");
    mySettings->setValue("Server/CLIPort", "9090");
    mySettings->setValue("Server/HttpPort", "9000");
    mySettings->setValue("Audio/Device","");
    mySettings->sync();


    ui->lblUpdateOnRestart->setText("Changes Take Effect on Restart");
}

void MainWindow::setupConfig(void)
{
    DEBUGF(QTime::currentTime());
    QProcess proc;
    QStringList args;
    args.append(QString("-L"));
    proc.start("squeezeslave", args );

    proc.waitForReadyRead(2000);

    QByteArray m_out = proc.readAllStandardOutput();
    QList<QByteArray> t = m_out.split('\n');
    proc.close();

    QListIterator<QByteArray> i(t);
    if(i.peekNext().left(6) == "Output") {
        i.next();
    }
    while(i.hasNext()){
        ui->cbAudioOutput->addItem(i.next());
    }

    ui->txtServerAddress->setText(mySettings->value("Server/Address","127.0.0.1").toString());
    ui->txtAudioPort->setText(mySettings->value("Server/AudioPort","3483").toString());
    ui->txtCLIPort->setText(mySettings->value("Server/CLIPort", "9090").toString());
    ui->txtHttpPort->setText(mySettings->value("Server/HttpPort", "9000").toString());
    ui->cbAudioOutput->setCurrentIndex(ui->cbAudioOutput->findText(mySettings->value("Audio/Device","").toString()));
    ui->tabWidget->setCurrentIndex(mySettings->value("UI/CurrentTab",-1).toInt());
    qDebug() << "current tab setting is: " << mySettings->value("UI/CurrentTab",-1).toInt();

    QPixmap p = QPixmap(64,37);
    p.fill(coverflowBackground.rgb());
    ui->lblCoverFlowColor->setPixmap(p);
    p.fill(m_disp->getDisplayBackgroundColor().rgb());
    ui->lblDisplayBackgroundColor->setPixmap(p);
    p.fill(m_disp->getTextColor().rgb());
    ui->lblDisplayTextColor->setPixmap(p);

    ui->spnInterval->setValue(mySettings->value("UI/ScrollInterval",5000).toInt());
    ui->spnSpeed->setValue(mySettings->value("UI/ScrollSpeed",30).toInt());
}

void MainWindow::setCoverFlowColor(void)
{
    DEBUGF(QTime::currentTime());
    QColorDialog *dlg = new QColorDialog(coverflowBackground);
    dlg->exec();
    tempcoverflowBackground = dlg->selectedColor();
}

void MainWindow::setDisplayBackgroundColor(void)
{
    DEBUGF(QTime::currentTime());
    QColorDialog *dlg = new QColorDialog(m_disp->getDisplayBackgroundColor());
    dlg->exec();
    m_disp->setDisplayBackgroundColor(dlg->selectedColor());
}

void MainWindow::setDisplayTextColor(void)
{
    DEBUGF(QTime::currentTime());
    QColorDialog *dlg = new QColorDialog(m_disp->getTextColor());
    dlg->exec();
    m_disp->setTextColor(dlg->selectedColor());
}


void MainWindow::getplayerMACAddress( void )
{
    DEBUGF(QTime::currentTime());
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
    DEBUGF(QTime::currentTime());
    waitWindow->show();
    this->setEnabled( false );
}

void MainWindow::slotEnablePlayer( void )
{
    DEBUGF(QTime::currentTime());
    waitWindow->hide();
    this->setEnabled( true );
}

void MainWindow::SqueezePlayerError( void )
{
    DEBUGF(QTime::currentTime());
    QString errMsg = QString( "SQUEEZE ERROR: " ) + squeezePlayer->readAllStandardError();
    qDebug() << errMsg;
}

void MainWindow::SqueezePlayerOutput( void )
{
    DEBUGF(QTime::currentTime());
    QString errMsg = QString( "SQUEEZE OUTPUT: " ) + squeezePlayer->readAllStandardOutput();
    qDebug() << errMsg;
}



/* vim: set expandtab tabstop=4 shiftwidth=4: */

