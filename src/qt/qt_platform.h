#ifndef HYPERRECALL_QT_PLATFORM_H
#define HYPERRECALL_QT_PLATFORM_H

#include <QObject>
#include <QTimer>

extern "C" {
#include "../platform.h"
}

class QWidget;

/**
 * @brief Qt-based implementation of the platform abstraction layer.
 */
class QtPlatformHandle : public QObject {
    Q_OBJECT

public:
    explicit QtPlatformHandle(QWidget *window, const HrPlatformConfig *config);
    ~QtPlatformHandle() override;

    bool beginFrame(HrPlatformFrame *out_frame);
    void endFrame();
    void requestClose();
    bool isActive() const;

signals:
    void frameReady();

private slots:
    void onFrameTimeout();

private:
    QWidget *m_window;
    QTimer *m_frameTimer;
    bool m_closeRequested;
    bool m_frameInProgress;
    uint64_t m_frameIndex;
    double m_previousTime;
    int m_targetFps;
    bool m_isActive;
};

#endif /* HYPERRECALL_QT_PLATFORM_H */
