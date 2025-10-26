#include "qt_ui.h"

#include <cstdlib>
#include <cstring>

extern "C" {
#include "../ui.h"
#include "../theme.h"
}

QtUiContext::QtUiContext(const UiConfig *config)
    : m_themes(nullptr)
    , m_sessions(nullptr)
    , m_analytics(nullptr)
    , m_database(nullptr)
    , m_enableDevtools(false)
{
    if (config != nullptr) {
        m_enableDevtools = config->enable_devtools;
    }
}

QtUiContext::~QtUiContext()
{
    // Note: We don't own the attached resources, just clear the pointers
    m_themes = nullptr;
    m_sessions = nullptr;
    m_analytics = nullptr;
    m_database = nullptr;
}

void QtUiContext::attachThemeManager(struct HrThemeManager *themes)
{
    m_themes = themes;
}

void QtUiContext::attachSessionManager(struct SessionManager *sessions,
                                       const SessionCallbacks * /* callbacks */)
{
    m_sessions = sessions;
}

void QtUiContext::attachAnalytics(struct AnalyticsHandle *analytics)
{
    m_analytics = analytics;
}

void QtUiContext::attachDatabase(DatabaseHandle *database)
{
    m_database = database;
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

bool QtUiContext::processFrame(const HrPlatformFrame * /* frame */)
{
    // Minimal frame processing - just return success
    // In a full implementation, this would handle UI updates and rendering
    return true;
}

void QtUiContext::pushToast(const char * /* message */, float /* duration_seconds */)
{
    // TODO: Implement toast notifications in Qt
}

// C API implementation
extern "C" {

UiContext *ui_create(const UiConfig *config)
{
    auto *qtUi = new (std::nothrow) QtUiContext(config);
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

void ui_attach_import_export(UiContext * /* ui */, struct ImportExportContext * /* io_context */)
{
    // TODO: Implement import/export attachment
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

void ui_toggle_command_palette(UiContext * /* ui */)
{
    // TODO: Implement command palette
}

void ui_push_toast(UiContext *ui, const char *message, Color /* background */, float duration_seconds)
{
    if (ui == nullptr) {
        return;
    }
    
    auto *qtUi = reinterpret_cast<QtUiContext *>(ui);
    qtUi->pushToast(message, duration_seconds);
}

void ui_show_modal(UiContext * /* ui */, const char * /* title */, const char * /* body */)
{
    // TODO: Implement modal dialogs
}

void ui_close_modal(UiContext * /* ui */)
{
    // TODO: Implement modal dialogs
}

void ui_request_screen(UiContext * /* ui */, UiScreenId /* screen */)
{
    // TODO: Implement screen navigation
}

} // extern "C"
