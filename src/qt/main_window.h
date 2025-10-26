#ifndef HYPERRECALL_MAIN_WINDOW_H
#define HYPERRECALL_MAIN_WINDOW_H

#include <QMainWindow>

class QLabel;

extern "C" {
#include "../app.h"
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
    void processFrame();

private:
    void createMenus();
    void createStatusBar();
    
    AppContext *m_app;
    QLabel *m_statusLabel;
    QTimer *m_frameTimer;
};

#endif /* HYPERRECALL_MAIN_WINDOW_H */
