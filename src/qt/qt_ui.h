#ifndef HYPERRECALL_QT_UI_H
#define HYPERRECALL_QT_UI_H

#include <QObject>
#include <QWidget>
#include <vector>

extern "C" {
#include "../ui.h"
#include "../theme.h"
#include "../sessions.h"
}

class QStackedWidget;
class QLabel;
class QTimer;
class StudyScreenWidget;
class AnalyticsScreenWidget;
class LibraryScreenWidget;

/**
 * @brief Represents a toast notification
 */
struct ToastNotification {
    char message[256];
    unsigned char r, g, b, a;
    float ttl;
};

/**
 * @brief Qt-based implementation of the UI context with full screen support
 */
class QtUiContext : public QObject {
    Q_OBJECT

public:
    explicit QtUiContext(const UiConfig *config, QWidget *parentWidget);
    ~QtUiContext();

    // Core UI API
    void attachThemeManager(struct HrThemeManager *themes);
    void attachSessionManager(struct SessionManager *sessions,
                              const SessionCallbacks *callbacks);
    void attachAnalytics(struct AnalyticsHandle *analytics);
    void attachDatabase(DatabaseHandle *database);
    void attachImportExport(struct ImportExportContext *io_context);
    void setFonts(const HrRenderFontSet *fonts, float base_font_size);
    
    const HrThemePalette *activePalette() const;
    bool processFrame(const HrPlatformFrame *frame);
    
    // UI Operations
    void pushToast(const char *message, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float duration_seconds);
    void showModal(const char *title, const char *body);
    void closeModal();
    void toggleCommandPalette();
    void requestScreen(UiScreenId screen);
    
    // Get the main UI widget
    QWidget *widget() const { return m_mainWidget; }

signals:
    void screenChanged(UiScreenId newScreen);

private slots:
    void onProcessToasts();

private:
    void setupWidgets();
    void updateTheme();

    // Core subsystems
    struct HrThemeManager *m_themes;
    struct SessionManager *m_sessions;
    struct AnalyticsHandle *m_analytics;
    DatabaseHandle *m_database;
    struct ImportExportContext *m_importExport;
    SessionCallbacks m_chainedCallbacks;
    
    bool m_enableDevtools;
    UiScreenId m_currentScreen;
    
    // UI Widgets
    QWidget *m_mainWidget;
    QStackedWidget *m_screenStack;
    
    // Screens
    StudyScreenWidget *m_studyScreen;
    AnalyticsScreenWidget *m_analyticsScreen;
    LibraryScreenWidget *m_libraryScreen;
    
    // Toast system
    std::vector<ToastNotification> m_toasts;
    QTimer *m_toastTimer;
    QLabel *m_toastLabel;
    
    // State
    double m_elapsedTime;
    HrPlatformFrame m_lastFrame;
};

#endif /* HYPERRECALL_QT_UI_H */
