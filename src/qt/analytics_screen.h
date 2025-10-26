#ifndef HYPERRECALL_ANALYTICS_SCREEN_H
#define HYPERRECALL_ANALYTICS_SCREEN_H

#include <QWidget>

class QLabel;
class QTableWidget;
class QChartView;

extern "C" {
#include "../analytics.h"
}

/**
 * @brief Analytics dashboard showing study statistics
 */
class AnalyticsScreenWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnalyticsScreenWidget(QWidget *parent = nullptr);
    
    void setAnalytics(struct AnalyticsHandle *analytics);
    void update();

private:
    void setupUI();
    void refreshStats();
    
    struct AnalyticsHandle *m_analytics;
    
    QLabel *m_totalReviewsLabel;
    QLabel *m_averageEaseLabel;
    QLabel *m_streakLabel;
    QTableWidget *m_recentActivityTable;
    QWidget *m_chartPlaceholder;
};

#endif /* HYPERRECALL_ANALYTICS_SCREEN_H */
