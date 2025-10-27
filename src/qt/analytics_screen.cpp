#include "analytics_screen.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QString>
#include <ctime>

extern "C" {
#include "../analytics.h"
}

AnalyticsScreenWidget::AnalyticsScreenWidget(QWidget *parent)
    : QWidget(parent)
    , m_analytics(nullptr)
{
    setupUI();
}

void AnalyticsScreenWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    // Title
    auto *titleLabel = new QLabel("Analytics Dashboard", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // Stats cards
    auto *statsLayout = new QHBoxLayout();
    
    // Total reviews card
    auto *reviewsBox = new QGroupBox("Total Reviews", this);
    auto *reviewsLayout = new QVBoxLayout(reviewsBox);
    m_totalReviewsLabel = new QLabel("142", reviewsBox);
    m_totalReviewsLabel->setStyleSheet("font-size: 32pt; font-weight: bold; color: #3498db;");
    m_totalReviewsLabel->setAlignment(Qt::AlignCenter);
    reviewsLayout->addWidget(m_totalReviewsLabel);
    statsLayout->addWidget(reviewsBox);
    
    // Average ease card
    auto *easeBox = new QGroupBox("Average Ease", this);
    auto *easeLayout = new QVBoxLayout(easeBox);
    m_averageEaseLabel = new QLabel("2.5", easeBox);
    m_averageEaseLabel->setStyleSheet("font-size: 32pt; font-weight: bold; color: #2ecc71;");
    m_averageEaseLabel->setAlignment(Qt::AlignCenter);
    easeLayout->addWidget(m_averageEaseLabel);
    statsLayout->addWidget(easeBox);
    
    // Streak card
    auto *streakBox = new QGroupBox("Current Streak", this);
    auto *streakLayout = new QVBoxLayout(streakBox);
    m_streakLabel = new QLabel("7 days", streakBox);
    m_streakLabel->setStyleSheet("font-size: 32pt; font-weight: bold; color: #e67e22;");
    m_streakLabel->setAlignment(Qt::AlignCenter);
    streakLayout->addWidget(m_streakLabel);
    statsLayout->addWidget(streakBox);
    
    mainLayout->addLayout(statsLayout);
    
    // Chart placeholder
    auto *chartBox = new QGroupBox("Activity Heatmap", this);
    auto *chartLayout = new QVBoxLayout(chartBox);
    m_chartPlaceholder = new QWidget(chartBox);
    m_chartPlaceholder->setMinimumHeight(200);
    m_chartPlaceholder->setStyleSheet("background-color: #ecf0f1; border: 2px dashed #bdc3c7;");
    
    auto *chartLabel = new QLabel("Chart visualization would appear here\n(Requires Qt Charts module)", m_chartPlaceholder);
    chartLabel->setStyleSheet("color: #7f8c8d; font-size: 12pt;");
    chartLabel->setAlignment(Qt::AlignCenter);
    auto *chartInnerLayout = new QVBoxLayout(m_chartPlaceholder);
    chartInnerLayout->addWidget(chartLabel);
    
    chartLayout->addWidget(m_chartPlaceholder);
    mainLayout->addWidget(chartBox);
    
    // Recent activity table
    auto *activityBox = new QGroupBox("Recent Activity", this);
    auto *activityLayout = new QVBoxLayout(activityBox);
    
    m_recentActivityTable = new QTableWidget(5, 4, activityBox);
    m_recentActivityTable->setHorizontalHeaderLabels({"Date", "Reviews", "Avg. Ease", "Time Spent"});
    m_recentActivityTable->horizontalHeader()->setStretchLastSection(true);
    m_recentActivityTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Sample data
    QStringList sampleData[] = {
        {"Today", "23", "2.6", "15 min"},
        {"Yesterday", "31", "2.4", "20 min"},
        {"2 days ago", "18", "2.7", "12 min"},
        {"3 days ago", "27", "2.5", "18 min"},
        {"4 days ago", "22", "2.3", "14 min"}
    };
    
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 4; ++col) {
            m_recentActivityTable->setItem(row, col, new QTableWidgetItem(sampleData[row][col]));
        }
    }
    
    activityLayout->addWidget(m_recentActivityTable);
    mainLayout->addWidget(activityBox);
}

void AnalyticsScreenWidget::setAnalytics(struct AnalyticsHandle *analytics)
{
    m_analytics = analytics;
    refreshStats();
}

void AnalyticsScreenWidget::update()
{
    refreshStats();
}

void AnalyticsScreenWidget::refreshStats()
{
    if (!m_analytics) {
        // Show placeholder data
        m_totalReviewsLabel->setText("142");
        m_averageEaseLabel->setText("2.5");
        m_streakLabel->setText("7 days");
        return;
    }
    
    // Get dashboard data
    const HrAnalyticsDashboard *dashboard = analytics_dashboard(m_analytics);
    if (!dashboard) {
        return;
    }
    
    // Update total reviews
    m_totalReviewsLabel->setText(QString::number(dashboard->reviews.total_reviews));
    
    // Update average ease (calculated from review data)
    if (dashboard->reviews.total_reviews > 0) {
        // Calculate average from rating distribution
        size_t total = 0;
        size_t weighted = 0;
        for (size_t i = 0; i < HR_ANALYTICS_RATING_BUCKETS && i < 5; i++) {
            total += dashboard->reviews.rating_counts[i];
            weighted += dashboard->reviews.rating_counts[i] * (i + 1);
        }
        double avg = total > 0 ? (double)weighted / total : 2.5;
        m_averageEaseLabel->setText(QString::number(avg, 'f', 2));
    } else {
        m_averageEaseLabel->setText("N/A");
    }
    
    // Update streak
    if (dashboard->streaks.current_streak > 0) {
        m_streakLabel->setText(QString("%1 days").arg(dashboard->streaks.current_streak));
    } else {
        m_streakLabel->setText("0 days");
    }
    
    // Update recent activity table with heatmap data
    m_recentActivityTable->setRowCount(0);
    
    // Show last 10 days from heatmap
    size_t rows = dashboard->heatmap_count > 10 ? 10 : dashboard->heatmap_count;
    for (size_t i = 0; i < rows; i++) {
        int row = m_recentActivityTable->rowCount();
        m_recentActivityTable->insertRow(row);
        
        const HrAnalyticsHeatmapSample *sample = &dashboard->heatmap[dashboard->heatmap_count - rows + i];
        
        // Format date
        time_t day = sample->day_start_utc;
        struct tm *tm_info = localtime(&day);
        char date_buf[32];
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tm_info);
        
        m_recentActivityTable->setItem(row, 0, new QTableWidgetItem(QString::fromUtf8(date_buf)));
        m_recentActivityTable->setItem(row, 1, new QTableWidgetItem(QString::number(sample->total_reviews)));
        m_recentActivityTable->setItem(row, 2, new QTableWidgetItem("N/A"));  // Avg ease not in heatmap
        m_recentActivityTable->setItem(row, 3, new QTableWidgetItem("N/A"));  // Time not tracked yet
    }
    
    // If no data, show placeholder
    if (m_recentActivityTable->rowCount() == 0) {
        m_recentActivityTable->insertRow(0);
        auto *item = new QTableWidgetItem("(No review data yet - start studying!)");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        m_recentActivityTable->setItem(0, 0, item);
    }
}
