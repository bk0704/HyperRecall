#include "qt_platform.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QWidget>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "../platform.h"
}

QtPlatformHandle::QtPlatformHandle(QWidget *window, const HrPlatformConfig *config)
    : QObject(window)
    , m_window(window)
    , m_frameTimer(new QTimer(this))
    , m_closeRequested(false)
    , m_frameInProgress(false)
    , m_frameIndex(0)
    , m_previousTime(0.0)
    , m_targetFps(60)
    , m_isActive(true)
{
    if (config != nullptr && config->target_fps > 0) {
        m_targetFps = config->target_fps;
    }

    // Set up frame timer for rendering updates
    int interval_ms = (m_targetFps > 0) ? (1000 / m_targetFps) : 16; // ~60fps default
    m_frameTimer->setInterval(interval_ms);
    connect(m_frameTimer, &QTimer::timeout, this, &QtPlatformHandle::onFrameTimeout);
}

QtPlatformHandle::~QtPlatformHandle()
{
    m_frameTimer->stop();
}

bool QtPlatformHandle::beginFrame(HrPlatformFrame *out_frame)
{
    if (m_closeRequested) {
        return false;
    }

    m_frameInProgress = true;
    m_frameIndex++;

    // Calculate delta time
    static QElapsedTimer timer;
    if (m_frameIndex == 1) {
        timer.start();
        m_previousTime = 0.0;
    }

    double currentTime = timer.elapsed() / 1000.0; // Convert to seconds
    double deltaTime = (m_frameIndex == 1) ? 0.016 : (currentTime - m_previousTime);
    m_previousTime = currentTime;

    if (out_frame != nullptr) {
        out_frame->index = m_frameIndex;
        out_frame->delta_time = deltaTime;
        out_frame->render_width = m_window ? m_window->width() : 1280;
        out_frame->render_height = m_window ? m_window->height() : 720;
        out_frame->resized = false; // TODO: track actual resize events
    }

    return true;
}

void QtPlatformHandle::endFrame()
{
    m_frameInProgress = false;
    // Qt handles the actual frame presentation through its event loop
}

void QtPlatformHandle::requestClose()
{
    m_closeRequested = true;
    m_frameTimer->stop();
    if (m_window) {
        m_window->close();
    }
}

bool QtPlatformHandle::isActive() const
{
    return m_isActive && !m_closeRequested;
}

void QtPlatformHandle::onFrameTimeout()
{
    emit frameReady();
}

// C API implementation
extern "C" {

// Define the opaque platform handle structure
struct PlatformHandle {
    QtPlatformHandle *qtHandle;
};

struct PlatformHandle *platform_create(const HrPlatformConfig *config)
{
    (void)config; // Unused for now
    // Note: The actual window creation is handled by MainWindow
    // This function just creates the platform handle wrapper
    // The window will be attached later by main_qt.cpp
    
    // For now, we return a placeholder that will be properly initialized
    // when attached to a real window
    auto *handle = static_cast<struct PlatformHandle *>(
        malloc(sizeof(struct PlatformHandle))
    );
    
    if (handle != nullptr) {
        std::memset(handle, 0, sizeof(struct PlatformHandle));
    }
    
    return handle;
}

void platform_destroy(struct PlatformHandle *handle)
{
    if (handle == nullptr) {
        return;
    }
    
    // The QtPlatformHandle is owned by the QObject parent
    // Just free the wrapper struct
    free(handle);
}

bool platform_begin_frame(struct PlatformHandle *handle, HrPlatformFrame *out_frame)
{
    if (handle == nullptr) {
        return false;
    }
    
    auto *qtHandle = reinterpret_cast<QtPlatformHandle *>(handle);
    return qtHandle->beginFrame(out_frame);
}

void platform_end_frame(struct PlatformHandle *handle)
{
    if (handle == nullptr) {
        return;
    }
    
    auto *qtHandle = reinterpret_cast<QtPlatformHandle *>(handle);
    qtHandle->endFrame();
}

void platform_request_close(struct PlatformHandle *handle)
{
    if (handle == nullptr) {
        return;
    }
    
    auto *qtHandle = reinterpret_cast<QtPlatformHandle *>(handle);
    qtHandle->requestClose();
}

bool platform_is_active(const struct PlatformHandle *handle)
{
    if (handle == nullptr) {
        return false;
    }
    
    auto *qtHandle = reinterpret_cast<const QtPlatformHandle *>(handle);
    return qtHandle->isActive();
}

} // extern "C"
