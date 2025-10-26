#ifndef HYPERRECALL_MAIN_WINDOW_H
#define HYPERRECALL_MAIN_WINDOW_H

#include <QMainWindow>

class QLabel;
class QTimer;

extern "C" {
#include "../app.h"
#include "../ui.h"
}

/**
 * @brief Main application window for the Qt6 backend.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void setAppContext(AppContext *app);
    void startEventLoop();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onFileExit();
    void onHelpAbout();
    void onViewStudy();
    void onViewAnalytics();
    void onViewLibrary();
    void processFrame();

private:
    void createMenus();
    void createStatusBar();
    void updateWindowTitle();
    
    AppContext *m_app;
    QLabel *m_statusLabel;
    QTimer *m_frameTimer;
    UiScreenId m_currentScreen;
    
    // Menu actions
    QAction *m_studyAction;
    QAction *m_analyticsAction;
    QAction *m_libraryAction;
};

#endif /* HYPERRECALL_MAIN_WINDOW_H */
