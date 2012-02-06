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
    vertTransTimer = new QTimeLine( 350, this );
    horzTransTimer = new QTimeLine( 700, this );
    bumpTransTimer = new QTimeLine( 100, this );
    vertTransTimer->setFrameRange( 0, 0 );
    horzTransTimer->setFrameRange( 0, 0 );
    bumpTransTimer->setFrameRange( 0, 0 );
    isTransition = false;
    transitionDirection = transNONE;
    xOffsetOld = xOffsetNew = 0;
    yOffsetOld = yOffsetNew = 0;
    activeDevice = NULL;
    //    PortAudioDevice = "";
    ScrollOffset = 0;
    scrollState = NOSCROLL;
    Brightness = 255;
    line1Alpha = 0;
    isStartUp = true;
    connect( &scrollTimer, SIGNAL( timeout() ), this, SLOT(slotUpdateScrollOffset()) );
    connect( vertTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( horzTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( bumpTransTimer, SIGNAL(frameChanged(int)), this, SLOT(slotUpdateTransition(int)));
    connect( vertTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
    connect( horzTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));
    connect( bumpTransTimer, SIGNAL(finished()), this, SLOT(slotTransitionFinished()));

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
    waitWindow->hide();
    connect( slimCLI, SIGNAL(cliInfo(QString)), waitWindow, SLOT(showMessage(QString)) );

    DEBUGF("LOADING CONFIGS");
    loadDisplayConfig();
    loadConnectionConfig();
    DEBUGF("FINISHED LOADING CONFIGS");
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
    SetUpDisplay();

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

    // allow mouse clicks on CoverFlow to cause playlist change
    connect( CoverFlow, SIGNAL(NextSlide()), this, SLOT(slotFForward()) );
    connect( CoverFlow, SIGNAL(PrevSlide()), this, SLOT(slotPrev()) );

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
    return true;
}

void MainWindow::SetUpDisplay()
{
    DEBUGF(QTime::currentTime());
    // set up coverflow widget
    flowRect = QRect( 0, 0,
                      ui->cfWidget->geometry().width(),
                      ui->cfWidget->geometry().height() );
    CoverFlow = new SqueezePictureFlow( ui->cfWidget, SlimServerAddr, SlimServerHttpPort.toInt(), true, NULL, NULL);
    CoverFlow->setMinimumSize( flowRect.width(), flowRect.height() );
    CoverFlow->setContentsMargins( 50, 0, CoverFlow->width() - 50, CoverFlow->height() );
    CoverFlow->setBackgroundColor(coverflowBackground);

    // set up slim display area
    displayRect = QRect( 0, 0,
                         ui->lblSlimDisplay->geometry().width(),
                         ui->lblSlimDisplay->geometry().height() );
    displayImage = new QImage( displayRect.width(), displayRect.height(), QImage::Format_ARGB32 );
    //    displayBackgroundColor = QColor( mySettings->value("UI/DisplayBackground", "black").toString());
    displayImage->fill( (uint)displayBackgroundColor.rgb() );
    //    displayBackgroundColor.setRgb( 255, 255, 255 );
    //    displayImage->fill( displayBackgroundColor.black() );

    //    textcolorGeneral = QColor( mySettings->value("UI/DisplayTextColor","cyan").toString() );
    //    textcolorLine1 = QColor( mySettings->value("UI/DisplayTextColor","cyan").toString() );

    small.setFamily( "Helvetica" );
    small.setPixelSize( 4 );
    medium.setFamily( "Helvetica" );
    medium.setPixelSize( 4 );
    large.setFamily( "Helvetica" );
    large.setPixelSize( 4 );

    int h = displayRect.height();
    for( int i = 5; QFontInfo( small ).pixelSize() < h/4; i++ ) {
        DEBUGF("setting small font pixel" << i);
        small.setPixelSize( i );
    }
    for( int i = 5; QFontInfo( medium ).pixelSize() < h/3; i++ ) {
        DEBUGF("setting medium font pixel" << i);
        medium.setPixelSize( i );
    }
    for( int i = 5; QFontInfo( large ).pixelSize() < h/2; i++ ) {
        DEBUGF("setting large font pixel" << i);
        large.setPixelSize( i );
    }

    // establish font metrics for Line1 (used in scrolling display)
    ln1FM = new QFontMetrics( large );
    // set starting points for drawing on Slim Display standard messages that come through the CLI
    pointLine0 = QPoint( displayRect.width()/100, ( displayRect.height() / 10 ) + small.pixelSize() );
    pointLine1 = QPoint( displayRect.width()/100, ( 9 * displayRect.height() ) / 10 );
    pointLine1_2 = pointLine1; // temp just in case
    pointLine0Right = QPoint( ( 95 * displayRect.width() ) / 100, ( displayRect.height() / 10 ) + small.pixelSize() );
    pointLine1Right = QPoint( ( 95 * displayRect.width() ) / 100, ( 9 * displayRect.height() ) / 10 );
    upperMiddle = QPoint( displayRect.width() / 2, ( displayRect.height() / 2  ) - (  medium.pixelSize() ) / 4 );
    lowerMiddle = QPoint( displayRect.width() / 2, ( displayRect.height() / 2  ) + ( 5 * medium.pixelSize() ) / 4  );

    Line0Rect = QRect( pointLine0.x(), pointLine0.y() - small.pixelSize(),
                       pointLine0Right.x() - pointLine0.x(),
                       pointLine0.y() );
    Line1Rect = QRect( pointLine1.x(), pointLine1.y() - large.pixelSize(),
                       pointLine1Right.x() - pointLine1.x(),
                       displayRect.height() - ( pointLine1.y() - large.pixelSize() ));
    line1Clipping = QRegion( Line1Rect );
    noClipping = QRegion( displayRect );

    vertTransTimer->setFrameRange( 0, Line1Rect.height() );
    horzTransTimer->setFrameRange( 0, displayRect.width() );
    bumpTransTimer->setFrameRange( 0, ln1FM->width( QChar( 'W')) );

    Line1FontWidth = ln1FM->width( QChar( 'W' ) ) / 40;
    if( Line1FontWidth <= 0 ) // avoid too small a figure
        Line1FontWidth = 1;

    timeRect =  QRectF ( ( qreal )0,
                         ( qreal )( displayRect.height() / 10 ) + ( qreal )small.pixelSize()/2,
                         ( qreal )0,
                         ( qreal )( ( 5 * small.pixelSize() ) / 10 ) );
    timeFillRect = QRectF( timeRect.left(), timeRect.top(), timeRect.width(), timeRect.height() );

    volRect = QRectF (  ( qreal )( displayRect.width() / 20 ),
                        ( qreal )pointLine1.y() - ( qreal )large.pixelSize()/2,
                        ( qreal )( displayRect.width() ) * 0.90,
                        ( qreal )( ( 5 * small.pixelSize() ) / 10 ) );
    volFillRect = QRectF( volRect.left(), volRect.top(), volRect.width(), volRect.height() );

    radius = timeRect.height() / 2;
    slotEnablePlayer();
}

void MainWindow::slotResetSlimDisplay( void )
{
    DEBUGF(QTime::currentTime());
    scrollTimer.stop();
    ScrollOffset = 0;
    line1Alpha = 0;
    boundingRect = ln1FM->boundingRect( Line1Rect, Qt::AlignLeft | Qt::AlignHCenter, activeDevice->getDisplayBuffer().line1 );
    if( boundingRect.width() > Line1Rect.width() ) {
        scrollState = PAUSE_SCROLL;
        scrollTextLen = boundingRect.width() - Line1Rect.width();
        scrollTimer.setInterval( 5000 );
        scrollTimer.start();
    }
    else {
        scrollState = NOSCROLL;
    }
    PaintTextDisplay();
}

void MainWindow::slotUpdateSlimDisplay( void )
{
    DEBUGF(QTime::currentTime());
    //    boundingRect = ln1FM->boundingRect( Line1Rect, Qt::AlignLeft | Qt::AlignHCenter, activeDevice->getDisplayBuffer().line1 );
    lineWidth = ln1FM->width( activeDevice->getDisplayBuffer().line1 );
    pointLine1_2 = QPoint( lineWidth + ( ui->lblSlimDisplay->width() - Line1Rect.width()), ( 9 * displayRect.height() ) / 10 );

    if(  lineWidth > Line1Rect.width() ) {
        if( scrollState == NOSCROLL ) {
            StopScroll();
            scrollTextLen = lineWidth - Line1Rect.width();
            scrollTimer.setInterval( 5000 );
            scrollTimer.start();
        }
    }
    else {
        ScrollOffset = 0;
        scrollState = NOSCROLL;
        scrollTimer.stop();
    }
    PaintTextDisplay();
}

void MainWindow::PaintTextDisplay( void )
{
    DEBUGF(QTime::currentTime());
    if( activeDevice == NULL )
        DEBUGF( "active device is null" );
    DisplayBuffer d = activeDevice->getDisplayBuffer();

    int playedCount = 0;
    int totalCount = 1; // this way we never get a divide by zero error
    QString timeText = "";
    QPainter p( displayImage );
    QBrush b( displayBackgroundColor );
    textcolorGeneral.setAlpha( Brightness );
    textcolorLine1.setAlpha( Brightness - line1Alpha );

    QBrush c( textcolorGeneral );
    QBrush e( c ); // non-filling brush
    e.setStyle( Qt::NoBrush );
    p.setBackground( b );
    p.setBrush( c );
    p.setPen( textcolorGeneral );
    p.setFont( large );
    p.eraseRect( displayImage->rect() );

    // draw Line 0  NOTE: Only transitions left or right, but NOT up and down
    if( d.line0.length() > 0 ) {
        p.setFont( small );
        if( isTransition ) {
            p.drawText( pointLine0.x() + xOffsetOld, pointLine0.y(), transBuffer.line0 );
            p.drawText( pointLine0.x() + xOffsetNew, pointLine0.y(), d.line0 );
        }
        else
            p.drawText( pointLine0.x(), pointLine0.y(), d.line0 );
    }

    // draw Line 1
    if( d.line1.length() > 0 ) {
        if( d.line0.left( 8 ) == "Volume (" ) {   // it's the volume, so emulate a progress bar
            qreal volume = d.line0.mid( 8, d.line0.indexOf( ')' ) - 8 ).toInt();
            volFillRect.setWidth( (qreal)volRect.width() * ( volume / (qreal)100 ) );
            p.setBrush( e );  // non-filling brush so we end up with an outline of a rounded rectangle
            p.drawRoundedRect( volRect, radius, radius );
            p.setBrush( c );
            if( volume > 1 ) // if it's too small, we get a funny line at the start of the progress bar
                p.drawRoundedRect( volFillRect, radius, radius );
        }
        else {
            QBrush cLine1( textcolorLine1 );
            p.setBrush( cLine1 );
            p.setPen( textcolorLine1 );
            p.setFont( large );
            p.setClipRegion( line1Clipping );
            if( isTransition ) {
                p.drawText( pointLine1.x() + xOffsetOld, pointLine1.y() + yOffsetOld, transBuffer.line1);
                p.drawText( pointLine1.x() + xOffsetNew, pointLine1.y() + yOffsetNew, d.line1 );
            } else {
                p.drawText( pointLine1.x() - ScrollOffset, pointLine1.y(), d.line1 );
                if( scrollState != NOSCROLL )
                    p.drawText(pointLine1_2.x() - ScrollOffset, pointLine1.y(), d.line1 );
            }
            p.setClipRegion( noClipping );
            p.setBrush( c );
            p.setPen( textcolorGeneral );
        }
    }

    // deal with "overlay0" (the right-hand portion of the display) this can be time (e.g., "-3:08") or number of items (e.g., "(2 of 7)")
    if( d.overlay0.length() > 0 ) {
        if( Slimp3Display( d.overlay0 ) ) {
            QRegExp rx( "\\W(\\w+)\\W");
            //            QRegExp rx( "\037(\\w+)\037" );
            QStringList el;
            int pos = 0;

            while ((pos = rx.indexIn(d.overlay0, pos)) != -1) {
                el << rx.cap(1);
                pos += rx.matchedLength();
            }

            rx.indexIn( d.overlay0 );
            QStringList::iterator it = el.begin();
            while( it != el.end() ) {
                QString s = *it;
                if( s.left( 12 ) == "leftprogress" ) { // first element
                    int inc = s.at( 12 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                else if( s.left( 14 ) == "middleprogress" ) { // standard element
                    int inc = s.at( 14 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                else if( s.left( 10 ) == "solidblock" ) { // full block
                    playedCount += 4;
                    totalCount += 4;
                }
                else if( s.left( 13 ) == "rightprogress" ) { // end element
                    int inc = s.at( 13 ).digitValue();
                    playedCount += inc;
                    totalCount += 4;
                }
                ++it;
            }
            QChar *data = d.overlay0.data();
            for( int i = ( d.overlay0.length() - 8 ); i < d.overlay0.length(); i++ ) {
                if( *( data + i ) == ' ' ) {
                    timeText = d.overlay0.mid( i + 1 );
                }
            }
        }
        else if( d.overlay0.contains( QChar( 8 ) ) ) {
            QChar elapsed = QChar(8);
            QChar remaining = QChar(5);
            QChar *data = d.overlay0.data();
            for( int i = 0; i < d.overlay0.length(); i++, data++ ) {
                if( *data == elapsed ) {
                    playedCount++;
                    totalCount++;
                }
                else if( *data == remaining )
                    totalCount++;
                else if( *data == ' ' ) {
                    timeText = d.overlay0.mid( i + 1 );
                }
            }
        }
        else {
            timeText = d.overlay0;
        }
        p.setFont( small );
        QFontMetrics fm = p.fontMetrics();
        p.setClipping( false );
        if( isTransition ) {
            p.drawText( ( pointLine0Right.x() + xOffsetNew ) - fm.width(timeText), pointLine0Right.y(), timeText );
        }
        else {
            p.drawText( pointLine0Right.x() - fm.width(timeText), pointLine0Right.y(), timeText );
        }
        if( totalCount > 1 ) {  // make sure we received data on a progress bar, otherwise, don't draw
            timeRect.setLeft( ( qreal )( pointLine0.x() + fm.width( d.line0.toUpper() ) ) );
            timeRect.setRight( ( qreal )( pointLine0Right.x() - ( qreal )( 3 * fm.width( timeText ) / 2 ) ) );
            timeFillRect.setLeft( timeRect.left() );
            timeFillRect.setWidth( ( playedCount * timeRect.width() ) / totalCount );
            p.setBrush( e );  // non-filling brush so we end up with an outline of a rounded rectangle
            p.drawRoundedRect( timeRect, radius, radius );
            p.setBrush( c );
            if( playedCount > 1 ) // if it's too small, we get a funny line at the start of the progress bar
                p.drawRoundedRect( timeFillRect, radius, radius );
        }
    }

    // deal with "overlay1" (the right-hand portion of the display)
    /*
    if( d.overlay1.length() > 0 ) {
        DEBUGF( "Don't know what to do with overlay1 yet" );
    }
*/
    // if we've received a "center" display, it means we're powered down, so draw them
    if( d.center0.length() > 0 ) {
        p.setFont( medium );
        QFontMetrics fm = p.fontMetrics();
        QPoint start = QPoint( upperMiddle.x() - ( fm.width( d.center0 )/2 ), upperMiddle.y() );
        p.drawText( start, d.center0 );
    }

    if( d.center1.length() > 0 ) {
        p.setFont( medium );
        QFontMetrics fm = p.fontMetrics();
        QPoint start = QPoint( lowerMiddle.x() - ( fm.width( d.center1 )/2 ), lowerMiddle.y() );
        p.drawText( start, d.center1 );
    }
    ui->lblSlimDisplay->setPixmap( QPixmap::fromImage( *displayImage) );
}

void MainWindow::slotUpdateCoverFlow( int trackIndex )
{
    DEBUGF(QTime::currentTime());
    if(!CoverFlow->IsReady())
        return;

    int currSlide = CoverFlow->centerIndex();
    DEBUGF( "UPDATE COVERFLOW TO INDEX: " << trackIndex );
    if( abs( trackIndex - currSlide ) > 4 ) {
        if( trackIndex > currSlide ) {
            CoverFlow->showSlide( currSlide + 2 );
            CoverFlow->setCenterIndex( trackIndex - 2 );
        }
        else {
            CoverFlow->showSlide( currSlide - 2 );
            CoverFlow->setCenterIndex( trackIndex + 2 );
        }
    }
    CoverFlow->showSlide( trackIndex );
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

    if( CoverFlow->LoadAlbumList(activeDevice->getDevicePlayList())) {
        ui->cfWidget->setEnabled( true );
        DEBUGF( "CURRENT PLAYLIST INDEX IS: " << activeDevice->getDevicePlaylistIndex() );
        int playListIndex = activeDevice->getDevicePlaylistIndex();
        if( playListIndex > 4 )
            CoverFlow->setCenterIndex( playListIndex - 4 );
        CoverFlow->showSlide( playListIndex );
    }
}

void MainWindow::slotLeftArrow( void )
{
    DEBUGF(QTime::currentTime());
    StopScroll();
    isTransition = true;
    transitionDirection = transRIGHT;
    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
    if( activeDevice->getDisplayBuffer().line0 == "Squeezebox Home" ) {  // are we at the "left-most" menu option?
        bumpTransTimer->start();
    }
    else {
        horzTransTimer->start();
    }
    activeDevice->SendDeviceCommand( QString( "button arrow_left\n" ) );
}

void MainWindow::slotRightArrow( void )
{
    DEBUGF(QTime::currentTime());
    StopScroll();
    isTransition = true;
    transitionDirection = transLEFT;
    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
    horzTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_right\n" ) );
}

void MainWindow::slotUpArrow( void )
{
    DEBUGF(QTime::currentTime());
    StopScroll();
    isTransition = true;
    transitionDirection = transDOWN;
    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
    vertTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_up\n" ) );
}

void MainWindow::slotDownArrow( void )
{
    DEBUGF(QTime::currentTime());
    StopScroll();
    isTransition = true;
    transitionDirection = transUP;
    transBuffer = activeDevice->getDisplayBuffer(); // preserve the current display for transition
    vertTransTimer->start();
    activeDevice->SendDeviceCommand( QString( "button arrow_down\n" ) );
}

void MainWindow::slotUpdateScrollOffset( void )
{
    DEBUGF("SLOTUPDATESCROLLOFFSET");
    if( scrollState == PAUSE_SCROLL ) {
        scrollTimer.stop();
        scrollState = SCROLL;
        scrollTimer.setInterval( 33 );
        scrollTimer.start();
    }
    else if( scrollState == SCROLL ) {
        ScrollOffset += Line1FontWidth;
        //        if( ScrollOffset >= ( lineWidth - Line1Rect.width()) ) {
        if( ScrollOffset >= lineWidth ) {
            scrollState = FADE_OUT;
            line1Alpha = 0;
        }
    }
    else if( scrollState == FADE_IN ) {
        line1Alpha -= 15;
        if( line1Alpha <= 0 ) {
            line1Alpha = 0;
            ScrollOffset = 0;
            scrollTimer.stop();
            scrollState = PAUSE_SCROLL;
            scrollTimer.setInterval( 5000 );
            scrollTimer.start();
        }
    }
    else if( scrollState == FADE_OUT ) {
        line1Alpha += 7;
        ScrollOffset += Line1FontWidth; // keep scrolling while fading
        if( line1Alpha >= 255 ) {
            line1Alpha = 255;
            ScrollOffset = 0;
            scrollState = FADE_IN;
        }
    }
    PaintTextDisplay();
}

void MainWindow::slotUpdateTransition( int frame )
{
    switch( transitionDirection ) {
    case transLEFT:
        xOffsetOld = 0 - frame;
        xOffsetNew = displayRect.width() - frame;
        yOffsetOld = yOffsetNew = 0;
        break;
    case transRIGHT:
        xOffsetOld = 0 + frame;
        xOffsetNew = frame - displayRect.width();
        yOffsetOld = yOffsetNew = 0;
        break;
    case transUP:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = 0 - frame;
        yOffsetNew = Line1Rect.height() - frame;
        break;
    case transDOWN:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = 0 + frame;
        yOffsetNew = frame - Line1Rect.height();
        break;
    case transNONE:
    default:
        xOffsetOld = xOffsetNew = 0;
        yOffsetOld = yOffsetNew = 0;
        break;
    }
    PaintTextDisplay();
}

void MainWindow::slotTransitionFinished ( void )
{
    vertTransTimer->stop();
    horzTransTimer->stop();
    bumpTransTimer->stop();
    isTransition = false;
    xOffsetOld = xOffsetNew = 0;
    yOffsetOld = yOffsetNew = 0;
    transitionDirection = transNONE;
    slotResetSlimDisplay();
}

void MainWindow::StopScroll( void )
{
    scrollTimer.stop();
    ScrollOffset = 0;
    line1Alpha = 0;
    scrollState = PAUSE_SCROLL;
}


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
    activeDevice = d;
    slotUpdateSlimDisplay();
    slotCreateCoverFlow();
    connect( activeDevice, SIGNAL(NewSong()),
             this, SLOT(slotResetSlimDisplay()) );
    connect( activeDevice, SIGNAL(SlimDisplayUpdate()),
             this, SLOT(slotUpdateSlimDisplay()) );
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
    DEBUGF(QTime::currentTime());
    DEBUGF("DISPLAY CONFIG");
    textcolorGeneral= QColor::fromRgb(mySettings->value("UI/DisplayTextColor",QColor(Qt::cyan).rgb()).toUInt());
    textcolorLine1 = QColor::fromRgb(mySettings->value("UI/DisplayTextColor",QColor(Qt::cyan).rgb()).toUInt());
    displayBackgroundColor = QColor::fromRgb(mySettings->value("UI/DisplayBackground",QColor(Qt::black).rgb()).toUInt());
    coverflowBackground = QColor::fromRgb(mySettings->value("UI/CoverFlowColor",QColor(Qt::white).rgb()).toUInt());
    temptextcolorGeneral = textcolorGeneral;
    tempdisplayBackgroundColor = displayBackgroundColor;
    tempcoverflowBackground = coverflowBackground;
    //    textcolorGeneral= QColor(Qt::cyan);
    //    textcolorLine1 = QColor(Qt::cyan);
    //    displayBackgroundColor = QColor(Qt::black);
    //    coverflowBackground = QColor(Qt::white);
    scrollSpeed = mySettings->value("UI/ScrollInterval",5000).toInt();
    scrollInterval = mySettings->value("UI/ScrollSpeed",30).toInt();
    DEBUGF("DISPLAY CONFIG");
}

void MainWindow::loadConnectionConfig(void)
{
    DEBUGF(QTime::currentTime());
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
    CoverFlow->setBackgroundColor(coverflowBackground);
    QPixmap p = QPixmap(64,37);
    p.fill(coverflowBackground.rgb());
    ui->lblCoverFlowColor->setPixmap(p);
    p.fill(displayBackgroundColor.rgb());
    ui->lblDisplayBackgroundColor->setPixmap(p);
    p.fill(textcolorGeneral.rgb());
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
    CoverFlow->setBackgroundColor(coverflowBackground);
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
    p.fill(displayBackgroundColor.rgb());
    ui->lblDisplayBackgroundColor->setPixmap(p);
    p.fill(textcolorGeneral.rgb());
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
    QColorDialog *dlg = new QColorDialog(displayBackgroundColor);
    dlg->exec();
    tempdisplayBackgroundColor = dlg->selectedColor();
}

void MainWindow::setDisplayTextColor(void)
{
    DEBUGF(QTime::currentTime());
    QColorDialog *dlg = new QColorDialog(textcolorGeneral);
    dlg->exec();
    temptextcolorGeneral = dlg->selectedColor();
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

bool MainWindow::Slimp3Display( QString txt )
{
    DEBUGF(QTime::currentTime());
    QRegExp rx( "\037" );      // the CLI overlay for the Slimp3 display uses 0x1F (037 octal) to delimit the segments of the counter
    if( rx.indexIn( txt ) != -1 )
        return true;
    else
        return false;

}


/* vim: set expandtab tabstop=4 shiftwidth=4: */

