#include "qt_ui.h"
#include "study_screen.h"
#include "analytics_screen.h"
#include "library_screen.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "../ui.h"
#include "../theme.h"
}

QtUiContext::QtUiContext(const UiConfig *config, QWidget *parentWidget)
    : QObject(parentWidget)
    , m_themes(nullptr)
    , m_sessions(nullptr)
    , m_analytics(nullptr)
    , m_database(nullptr)
    , m_importExport(nullptr)
    , m_enableDevtools(false)
    , m_currentScreen(UI_SCREEN_STUDY)
    , m_mainWidget(nullptr)
    , m_screenStack(nullptr)
    , m_studyScreen(nullptr)
    , m_analyticsScreen(nullptr)
    , m_libraryScreen(nullptr)
    , m_toastLabel(nullptr)
    , m_elapsedTime(0.0)
{
    if (config != nullptr) {
        m_enableDevtools = config->enable_devtools;
    }
    
    memset(&m_chainedCallbacks, 0, sizeof(m_chainedCallbacks));
    memset(&m_lastFrame, 0, sizeof(m_lastFrame));
    
    setupWidgets();
    
    // Toast timer
    m_toastTimer = new QTimer(this);
    m_toastTimer->setInterval(100); // Update every 100ms
    connect(m_toastTimer, &QTimer::timeout, this, &QtUiContext::onProcessToasts);
    m_toastTimer->start();
}

QtUiContext::~QtUiContext()
{
    // Note: We don't own the attached resources, just clear the pointers
    m_themes = nullptr;
    m_sessions = nullptr;
    m_analytics = nullptr;
    m_database = nullptr;
    m_importExport = nullptr;
}

void QtUiContext::setupWidgets()
{
    // Create main widget container
    m_mainWidget = new QWidget();
    auto *mainLayout = new QVBoxLayout(m_mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create stacked widget for screens
    m_screenStack = new QStackedWidget(m_mainWidget);
    mainLayout->addWidget(m_screenStack);
    
    // Create screens
    m_studyScreen = new StudyScreenWidget(m_screenStack);
    m_screenStack->addWidget(m_studyScreen);
    
    m_analyticsScreen = new AnalyticsScreenWidget(m_screenStack);
    m_screenStack->addWidget(m_analyticsScreen);
    
    m_libraryScreen = new LibraryScreenWidget(m_screenStack);
    m_screenStack->addWidget(m_libraryScreen);
    
    // Set initial screen
    m_screenStack->setCurrentWidget(m_studyScreen);
    
    // Toast label (overlay)
    m_toastLabel = new QLabel(m_mainWidget);
    m_toastLabel->setStyleSheet("background-color: rgba(0, 0, 0, 180); color: white; padding: 10px; border-radius: 5px; font-size: 12pt;");
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->setWordWrap(true);
    m_toastLabel->setMaximumWidth(400);
    m_toastLabel->hide();
    m_toastLabel->raise();
}

void QtUiContext::attachThemeManager(struct HrThemeManager *themes)
{
    m_themes = themes;
    updateTheme();
}

void QtUiContext::attachSessionManager(struct SessionManager *sessions,
                                       const SessionCallbacks *callbacks)
{
    m_sessions = sessions;
    if (callbacks != nullptr) {
        m_chainedCallbacks = *callbacks;
    }
    
    if (m_studyScreen) {
        m_studyScreen->setSessionManager(sessions);
    }
}

void QtUiContext::attachAnalytics(struct AnalyticsHandle *analytics)
{
    m_analytics = analytics;
    
    if (m_analyticsScreen) {
        m_analyticsScreen->setAnalytics(analytics);
    }
}

void QtUiContext::attachDatabase(DatabaseHandle *database)
{
    m_database = database;
    
    if (m_libraryScreen) {
        m_libraryScreen->setDatabase(database);
    }
}

void QtUiContext::attachImportExport(struct ImportExportContext *io_context)
{
    m_importExport = io_context;
}

void QtUiContext::setFonts(const HrRenderFontSet * /* fonts */, float /* base_font_size */)
{
    // TODO: Implement font loading for Qt
}

const HrThemePalette *QtUiContext::activePalette() const
{
    if (m_themes != nullptr) {
        return theme_manager_active_palette(m_themes);
    }
    return nullptr;
}

bool QtUiContext::processFrame(const HrPlatformFrame *frame)
{
    if (frame != nullptr) {
        m_lastFrame = *frame;
        m_elapsedTime += frame->delta_time;
    }
    
    // Update screens if needed
    if (m_studyScreen && m_currentScreen == UI_SCREEN_STUDY) {
        m_studyScreen->update();
    } else if (m_analyticsScreen && m_currentScreen == UI_SCREEN_ANALYTICS) {
        m_analyticsScreen->update();
    } else if (m_libraryScreen && m_currentScreen == UI_SCREEN_LIBRARY) {
        m_libraryScreen->update();
    }
    
    return true;
}

void QtUiContext::pushToast(const char *message, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float duration_seconds)
{
    if (message == nullptr) {
        return;
    }
    
    ToastNotification toast;
    snprintf(toast.message, sizeof(toast.message), "%s", message);
    toast.r = r;
    toast.g = g;
    toast.b = b;
    toast.a = a;
    toast.ttl = duration_seconds;
    
    m_toasts.push_back(toast);
    
    // Show toast immediately
    if (m_toastLabel && m_toasts.size() == 1) {
        m_toastLabel->setText(message);
        m_toastLabel->setStyleSheet(QString("background-color: rgba(%1, %2, %3, %4); color: white; padding: 10px; border-radius: 5px; font-size: 12pt;")
                                    .arg(r).arg(g).arg(b).arg(a));
        m_toastLabel->adjustSize();
        
        // Position at bottom center
        if (m_mainWidget) {
            int x = (m_mainWidget->width() - m_toastLabel->width()) / 2;
            int y = m_mainWidget->height() - m_toastLabel->height() - 50;
            m_toastLabel->move(x, y);
        }
        
        m_toastLabel->show();
    }
}

void QtUiContext::showModal(const char *title, const char *body)
{
    if (title == nullptr || body == nullptr) {
        return;
    }
    
    QMessageBox::information(m_mainWidget, QString::fromUtf8(title), QString::fromUtf8(body));
}

void QtUiContext::closeModal()
{
    // Modals are blocking in Qt, so this is a no-op
}

void QtUiContext::toggleCommandPalette()
{
    // TODO: Implement command palette
    pushToast("Command palette not yet implemented", 200, 200, 200, 200, 2.0f);
}

void QtUiContext::requestScreen(UiScreenId screen)
{
    m_currentScreen = screen;
    
    switch (screen) {
        case UI_SCREEN_STUDY:
            m_screenStack->setCurrentWidget(m_studyScreen);
            break;
        case UI_SCREEN_ANALYTICS:
            m_screenStack->setCurrentWidget(m_analyticsScreen);
            break;
        case UI_SCREEN_LIBRARY:
            m_screenStack->setCurrentWidget(m_libraryScreen);
            break;
        default:
            break;
    }
    
    emit screenChanged(screen);
}

void QtUiContext::onProcessToasts()
{
    if (m_toasts.empty()) {
        if (m_toastLabel) {
            m_toastLabel->hide();
        }
        return;
    }
    
    // Update TTLs
    float deltaTime = 0.1f; // 100ms
    for (auto &toast : m_toasts) {
        toast.ttl -= deltaTime;
    }
    
    // Remove expired toasts
    m_toasts.erase(
        std::remove_if(m_toasts.begin(), m_toasts.end(),
                      [](const ToastNotification &t) { return t.ttl <= 0.0f; }),
        m_toasts.end()
    );
    
    // Update display
    if (!m_toasts.empty() && m_toastLabel) {
        const auto &toast = m_toasts.front();
        m_toastLabel->setText(toast.message);
        m_toastLabel->setStyleSheet(QString("background-color: rgba(%1, %2, %3, %4); color: white; padding: 10px; border-radius: 5px; font-size: 12pt;")
                                    .arg(toast.r).arg(toast.g).arg(toast.b).arg(toast.a));
    } else if (m_toastLabel) {
        m_toastLabel->hide();
    }
}

void QtUiContext::updateTheme()
{
    // TODO: Apply theme colors to Qt widgets
}

// C API implementation
extern "C" {

UiContext *ui_create(const UiConfig *config)
{
    // Note: The parent widget will be set by MainWindow
    auto *qtUi = new (std::nothrow) QtUiContext(config, nullptr);
    if (qtUi == nullptr) {
        return nullptr;
    }
    
    return reinterpret_cast<UiContext *>(qtUi);
}

void ui_destroy(UiContext *ui)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    delete qtUi;
}

void ui_attach_theme_manager(UiContext *ui, struct HrThemeManager *themes)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->attachThemeManager(themes);
}

void ui_attach_session_manager(UiContext *ui,
                               struct SessionManager *sessions,
                               const SessionCallbacks *forward_callbacks)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->attachSessionManager(sessions, forward_callbacks);
}

void ui_attach_analytics(UiContext *ui, struct AnalyticsHandle *analytics)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->attachAnalytics(analytics);
}

void ui_attach_database(UiContext *ui, DatabaseHandle *database)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->attachDatabase(database);
}

void ui_attach_import_export(UiContext *ui, struct ImportExportContext *io_context)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->attachImportExport(io_context);
}

void ui_set_fonts(UiContext *ui, const HrRenderFontSet *fonts, float base_font_size)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->setFonts(fonts, base_font_size);
}

const HrRenderContext *ui_render_context(const UiContext * /* ui */)
{
    // TODO: Implement render context for Qt
    return nullptr;
}

const HrThemePalette *ui_active_palette(const UiContext *ui)
{
    if (ui == nullptr) {
        return nullptr;
    }
    
    auto *qtUi = reinterpret_cast<const QtUiContext *>(ui);
    return qtUi->activePalette();
}

bool ui_process_frame(UiContext *ui, const HrPlatformFrame *frame)
{
    if (ui == nullptr) {
        return false;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    return qtUi->processFrame(frame);
}

void ui_toggle_command_palette(UiContext *ui)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->toggleCommandPalette();
}

void ui_push_toast(UiContext *ui, const char *message, Color background, float duration_seconds)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->pushToast(message, background.r, background.g, background.b, background.a, duration_seconds);
}

void ui_show_modal(UiContext *ui, const char *title, const char *body)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->showModal(title, body);
}

void ui_close_modal(UiContext *ui)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->closeModal();
}

void ui_request_screen(UiContext *ui, UiScreenId screen)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->requestScreen(screen);
}

} // extern "C"
