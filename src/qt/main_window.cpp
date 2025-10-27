#include "main_window.h"
#include "qt_platform.h"
#include "qt_ui.h"

#include <QApplication>
#include <QCloseEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

extern "C" {
#include "../app.h"
#include "../platform.h"
#include "../analytics.h"
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_app(nullptr)
    , m_statusLabel(nullptr)
    , m_frameTimer(nullptr)
    , m_currentScreen(UI_SCREEN_STUDY)
    , m_studyAction(nullptr)
    , m_analyticsAction(nullptr)
    , m_libraryAction(nullptr)
{
    setWindowTitle(tr("HyperRecall"));
    resize(1280, 720);

    createMenus();
    createStatusBar();

    // Set up frame timer for processing
    m_frameTimer = new QTimer(this);
    m_frameTimer->setInterval(16); // ~60 FPS
    connect(m_frameTimer, &QTimer::timeout, this, &MainWindow::processFrame);
}

MainWindow::~MainWindow()
{
    if (m_frameTimer) {
        m_frameTimer->stop();
    }
}

void MainWindow::setAppContext(AppContext *app)
{
    m_app = app;
    
    // Set the UI widget as central widget
    if (app && app->ui) {
        auto *qtUi = reinterpret_cast<QtUiContext *>(app->ui);
        setCentralWidget(qtUi->widget());
    }
    
    updateWindowTitle();
}

void MainWindow::startEventLoop()
{
    if (m_frameTimer) {
        m_frameTimer->start();
    }
}

void MainWindow::createMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, &QAction::triggered, this, &MainWindow::onFileExit);

    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    
    m_studyAction = viewMenu->addAction(tr("&Study"));
    m_studyAction->setShortcut(QKeySequence(Qt::Key_F1));
    m_studyAction->setStatusTip(tr("Switch to study screen"));
    m_studyAction->setCheckable(true);
    m_studyAction->setChecked(true);
    connect(m_studyAction, &QAction::triggered, this, &MainWindow::onViewStudy);
    
    m_analyticsAction = viewMenu->addAction(tr("&Analytics"));
    m_analyticsAction->setShortcut(QKeySequence(Qt::Key_F2));
    m_analyticsAction->setStatusTip(tr("Switch to analytics screen"));
    m_analyticsAction->setCheckable(true);
    connect(m_analyticsAction, &QAction::triggered, this, &MainWindow::onViewAnalytics);
    
    m_libraryAction = viewMenu->addAction(tr("&Library"));
    m_libraryAction->setShortcut(QKeySequence(Qt::Key_F3));
    m_libraryAction->setStatusTip(tr("Switch to library screen"));
    m_libraryAction->setCheckable(true);
    connect(m_libraryAction, &QAction::triggered, this, &MainWindow::onViewLibrary);

    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    aboutAction->setStatusTip(tr("Show information about HyperRecall"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onHelpAbout);
}

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel(tr("Ready"), this);
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_frameTimer) {
        m_frameTimer->stop();
    }
    
    if (m_app && m_app->platform) {
        platform_request_close(m_app->platform);
    }
    
    event->accept();
}

void MainWindow::onFileExit()
{
    close();
}

void MainWindow::onHelpAbout()
{
    QMessageBox::about(this, tr("About HyperRecall"),
        tr("<h2>HyperRecall</h2>"
           "<p>Version 0.1.0</p>"
           "<p>A spaced repetition learning application.</p>"
           "<p><b>Qt6 UI Backend - Functional Prototype</b></p>"
           "<p>All three main screens are implemented with placeholder content.</p>"
           "<p>Copyright © 2024</p>"));
}

void MainWindow::onViewStudy()
{
    if (m_app && m_app->ui) {
        ui_request_screen(m_app->ui, UI_SCREEN_STUDY);
        m_currentScreen = UI_SCREEN_STUDY;
        m_studyAction->setChecked(true);
        m_analyticsAction->setChecked(false);
        m_libraryAction->setChecked(false);
        updateWindowTitle();
    }
}

void MainWindow::onViewAnalytics()
{
    if (m_app && m_app->ui) {
        ui_request_screen(m_app->ui, UI_SCREEN_ANALYTICS);
        m_currentScreen = UI_SCREEN_ANALYTICS;
        m_studyAction->setChecked(false);
        m_analyticsAction->setChecked(true);
        m_libraryAction->setChecked(false);
        updateWindowTitle();
    }
}

void MainWindow::onViewLibrary()
{
    if (m_app && m_app->ui) {
        ui_request_screen(m_app->ui, UI_SCREEN_LIBRARY);
        m_currentScreen = UI_SCREEN_LIBRARY;
        m_studyAction->setChecked(false);
        m_analyticsAction->setChecked(false);
        m_libraryAction->setChecked(true);
        updateWindowTitle();
    }
}

void MainWindow::updateWindowTitle()
{
    QString screenName;
    switch (m_currentScreen) {
        case UI_SCREEN_STUDY:
            screenName = "Study";
            break;
        case UI_SCREEN_ANALYTICS:
            screenName = "Analytics";
            break;
        case UI_SCREEN_LIBRARY:
            screenName = "Library";
            break;
        default:
            screenName = "";
            break;
    }
    
    if (!screenName.isEmpty()) {
        setWindowTitle(QString("HyperRecall - %1").arg(screenName));
    } else {
        setWindowTitle("HyperRecall");
    }
}

void MainWindow::processFrame()
{
    if (!m_app || !m_app->platform) {
        return;
    }

    // Process application frame
    HrPlatformFrame frame_info = {};
    
    if (platform_begin_frame(m_app->platform, &frame_info)) {
        // Update status bar with frame info
        if (m_statusLabel && frame_info.index % 60 == 0) {
            m_statusLabel->setText(
                tr("Frame: %1 | FPS: %2")
                    .arg(frame_info.index)
                    .arg(frame_info.delta_time > 0 ? 1.0 / frame_info.delta_time : 0, 0, 'f', 1)
            );
        }
        
        // Let UI process the frame
        if (m_app->ui) {
            ui_process_frame(m_app->ui, &frame_info);
        }
        
        // Record analytics
        if (m_app->analytics) {
            analytics_record_frame(m_app->analytics, &frame_info);
        }
        
        platform_end_frame(m_app->platform);
    } else {
        // Platform signaled close
        m_frameTimer->stop();
        close();
    }
}
