#ifndef HYPERRECALL_QT_UI_H
#define HYPERRECALL_QT_UI_H

extern "C" {
#include "../ui.h"
#include "../theme.h"
}

/**
 * @brief Qt-based implementation of the UI context.
 * 
 * This is a minimal implementation that provides stubs for the UI API
 * to allow the application to initialize and run with Qt.
 */
class QtUiContext {
public:
    explicit QtUiContext(const UiConfig *config);
    ~QtUiContext();

    void attachThemeManager(struct HrThemeManager *themes);
    void attachSessionManager(struct SessionManager *sessions,
                              const SessionCallbacks *callbacks);
    void attachAnalytics(struct AnalyticsHandle *analytics);
    void attachDatabase(DatabaseHandle *database);
    void setFonts(const HrRenderFontSet *fonts, float base_font_size);
    
    const HrThemePalette *activePalette() const;
    bool processFrame(const HrPlatformFrame *frame);
    void pushToast(const char *message, float duration_seconds);

private:
    struct HrThemeManager *m_themes;
    struct SessionManager *m_sessions;
    struct AnalyticsHandle *m_analytics;
    DatabaseHandle *m_database;
    bool m_enableDevtools;
};

#endif /* HYPERRECALL_QT_UI_H */
