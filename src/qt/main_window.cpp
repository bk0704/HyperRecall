#include "main_window.h"
#include "qt_platform.h"

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
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_app(nullptr)
    , m_statusLabel(nullptr)
    , m_frameTimer(nullptr)
{
    setWindowTitle(tr("HyperRecall"));
    resize(1280, 720);

    // Create central widget (placeholder for now)
    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);
    
    auto *placeholderLabel = new QLabel(tr("HyperRecall - Qt6 Backend"), centralWidget);
    placeholderLabel->setAlignment(Qt::AlignCenter);
    QFont font = placeholderLabel->font();
    font.setPointSize(24);
    placeholderLabel->setFont(font);
    layout->addWidget(placeholderLabel);
    
    auto *infoLabel = new QLabel(
        tr("Application initialized successfully.\n"
           "Core subsystems: Config, Database, Analytics, Sessions.\n\n"
           "This is a minimal Qt6 UI backend. Full UI features to be implemented."),
        centralWidget);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    setCentralWidget(centralWidget);

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
           "<p>Qt6 UI Backend</p>"
           "<p>Copyright © 2024</p>"));
}

void MainWindow::processFrame()
{
    if (!m_app || !m_app->platform) {
        return;
    }

    // Process application frame
    HrPlatformFrame frame_info = {0};
    
    if (platform_begin_frame(m_app->platform, &frame_info)) {
        // Update status bar with frame info
        if (m_statusLabel && frame_info.index % 60 == 0) {
            m_statusLabel->setText(
                tr("Frame: %1 | FPS: %2")
                    .arg(frame_info.index)
                    .arg(frame_info.delta_time > 0 ? 1.0 / frame_info.delta_time : 0, 0, 'f', 1)
            );
        }
        
        // Let UI process the frame (minimal stub for now)
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
