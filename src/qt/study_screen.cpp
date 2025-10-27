#include "study_screen.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QStackedWidget>
#include <QString>

extern "C" {
#include "../srs.h"
}

StudyScreenWidget::StudyScreenWidget(QWidget *parent)
    : QWidget(parent)
    , m_sessions(nullptr)
    , m_sessionActive(false)
{
    setupUI();
    showWelcomeScreen();
}

void StudyScreenWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    // Status label at top
    m_statusLabel = new QLabel("Ready to study", this);
    m_statusLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    mainLayout->addWidget(m_statusLabel);
    
    // Create stacked widget for different states
    auto *stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(stackedWidget, 1);
    
    // Welcome screen
    m_welcomeWidget = new QWidget(this);
    auto *welcomeLayout = new QVBoxLayout(m_welcomeWidget);
    welcomeLayout->addStretch();
    
    auto *titleLabel = new QLabel("HyperRecall Study", m_welcomeWidget);
    titleLabel->setStyleSheet("font-size: 24pt; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    welcomeLayout->addWidget(titleLabel);
    
    auto *subtitle = new QLabel("Start a study session", m_welcomeWidget);
    subtitle->setStyleSheet("font-size: 14pt; color: gray;");
    subtitle->setAlignment(Qt::AlignCenter);
    welcomeLayout->addWidget(subtitle);
    
    welcomeLayout->addSpacing(40);
    
    m_startMasteryBtn = new QPushButton("Start Mastery Session", m_welcomeWidget);
    m_startMasteryBtn->setMinimumHeight(50);
    m_startMasteryBtn->setStyleSheet("font-size: 14pt;");
    connect(m_startMasteryBtn, &QPushButton::clicked, this, &StudyScreenWidget::onStartMasterySession);
    welcomeLayout->addWidget(m_startMasteryBtn);
    
    m_startCramBtn = new QPushButton("Start Cram Session", m_welcomeWidget);
    m_startCramBtn->setMinimumHeight(50);
    m_startCramBtn->setStyleSheet("font-size: 14pt;");
    connect(m_startCramBtn, &QPushButton::clicked, this, &StudyScreenWidget::onStartCramSession);
    welcomeLayout->addWidget(m_startCramBtn);
    
    welcomeLayout->addStretch();
    stackedWidget->addWidget(m_welcomeWidget);
    
    // Review screen
    m_reviewWidget = new QWidget(this);
    auto *reviewLayout = new QVBoxLayout(m_reviewWidget);
    
    m_cardDisplay = new QTextEdit(m_reviewWidget);
    m_cardDisplay->setReadOnly(true);
    m_cardDisplay->setStyleSheet("font-size: 16pt; padding: 20px;");
    m_cardDisplay->setPlainText("Card content will appear here...\n\nThis is a prototype card display.");
    reviewLayout->addWidget(m_cardDisplay, 1);
    
    // Answer buttons
    auto *buttonLayout = new QHBoxLayout();
    
    m_againBtn = new QPushButton("Again", m_reviewWidget);
    m_againBtn->setMinimumHeight(50);
    m_againBtn->setStyleSheet("background-color: #e74c3c; color: white; font-size: 14pt;");
    connect(m_againBtn, &QPushButton::clicked, this, &StudyScreenWidget::onAnswerAgain);
    buttonLayout->addWidget(m_againBtn);
    
    m_hardBtn = new QPushButton("Hard", m_reviewWidget);
    m_hardBtn->setMinimumHeight(50);
    m_hardBtn->setStyleSheet("background-color: #e67e22; color: white; font-size: 14pt;");
    connect(m_hardBtn, &QPushButton::clicked, this, &StudyScreenWidget::onAnswerHard);
    buttonLayout->addWidget(m_hardBtn);
    
    m_goodBtn = new QPushButton("Good", m_reviewWidget);
    m_goodBtn->setMinimumHeight(50);
    m_goodBtn->setStyleSheet("background-color: #27ae60; color: white; font-size: 14pt;");
    connect(m_goodBtn, &QPushButton::clicked, this, &StudyScreenWidget::onAnswerGood);
    buttonLayout->addWidget(m_goodBtn);
    
    m_easyBtn = new QPushButton("Easy", m_reviewWidget);
    m_easyBtn->setMinimumHeight(50);
    m_easyBtn->setStyleSheet("background-color: #2ecc71; color: white; font-size: 14pt;");
    connect(m_easyBtn, &QPushButton::clicked, this, &StudyScreenWidget::onAnswerEasy);
    buttonLayout->addWidget(m_easyBtn);
    
    reviewLayout->addLayout(buttonLayout);
    stackedWidget->addWidget(m_reviewWidget);
    
    // Complete screen
    m_completeWidget = new QWidget(this);
    auto *completeLayout = new QVBoxLayout(m_completeWidget);
    completeLayout->addStretch();
    
    auto *completeLabel = new QLabel("Session Complete!", m_completeWidget);
    completeLabel->setStyleSheet("font-size: 24pt; font-weight: bold;");
    completeLabel->setAlignment(Qt::AlignCenter);
    completeLayout->addWidget(completeLabel);
    
    auto *statsLabel = new QLabel("Great work! Check analytics for details.", m_completeWidget);
    statsLabel->setStyleSheet("font-size: 14pt; color: gray;");
    statsLabel->setAlignment(Qt::AlignCenter);
    completeLayout->addWidget(statsLabel);
    
    completeLayout->addStretch();
    stackedWidget->addWidget(m_completeWidget);
}

void StudyScreenWidget::showWelcomeScreen()
{
    auto *stackedWidget = findChild<QStackedWidget*>();
    if (stackedWidget) {
        stackedWidget->setCurrentWidget(m_welcomeWidget);
    }
    m_statusLabel->setText("Ready to study");
    m_sessionActive = false;
}

void StudyScreenWidget::showCardReview()
{
    auto *stackedWidget = findChild<QStackedWidget*>();
    if (stackedWidget) {
        stackedWidget->setCurrentWidget(m_reviewWidget);
    }
    m_statusLabel->setText("Study Session Active - Card 1 of 10");
    m_sessionActive = true;
}

void StudyScreenWidget::showSessionComplete()
{
    auto *stackedWidget = findChild<QStackedWidget*>();
    if (stackedWidget) {
        stackedWidget->setCurrentWidget(m_completeWidget);
    }
    m_statusLabel->setText("Session completed!");
    m_sessionActive = false;
    emit sessionCompleted();
}

void StudyScreenWidget::setSessionManager(struct SessionManager *sessions)
{
    m_sessions = sessions;
}

void StudyScreenWidget::update()
{
    if (!m_sessions) {
        return;
    }
    
    // Check if there's an active session
    const SessionCard *current = session_manager_current(m_sessions);
    if (current != nullptr) {
        // Update UI to show current card
        size_t remaining = session_manager_remaining(m_sessions);
        m_statusLabel->setText(
            QString("Study Session Active - Card %1 of %2")
                .arg(1)  // Position would need tracking
                .arg(remaining)
        );
        
        // Display card content (for now, just show the card ID)
        m_cardDisplay->setPlainText(
            QString("Card ID: %1\n\n"
                   "Ease: %2\n"
                   "Interval: %3 days\n"
                   "Mode: %4")
                .arg(current->card_id)
                .arg(current->state.ease_factor, 0, 'f', 2)
                .arg(current->state.interval_days)
                .arg(current->state.mode == SRS_MODE_MASTERY ? "Mastery" : "Cram")
        );
        
        showCardReview();
    } else {
        showWelcomeScreen();
    }
}

void StudyScreenWidget::onStartMasterySession()
{
    if (!m_sessions) {
        showCardReview();  // Show placeholder for now
        return;
    }
    
    // TODO: Get cards from database
    // For now, start an empty session to test the flow
    bool started = session_manager_begin(m_sessions, SESSION_MODE_MASTERY, nullptr, 0);
    if (started) {
        update();
    } else {
        showCardReview();  // Fallback to placeholder
    }
}

void StudyScreenWidget::onStartCramSession()
{
    if (!m_sessions) {
        showCardReview();  // Show placeholder for now
        return;
    }
    
    // TODO: Get cards from database  
    // For now, start an empty session to test the flow
    bool started = session_manager_begin(m_sessions, SESSION_MODE_CRAM, nullptr, 0);
    if (started) {
        update();
    } else {
        showCardReview();  // Fallback to placeholder
    }
}

void StudyScreenWidget::onAnswerEasy()
{
    if (!m_sessions) {
        m_cardDisplay->setPlainText("Next card would appear here...");
        return;
    }
    
    SRSReviewResult result;
    bool graded = session_manager_grade(m_sessions, SRS_RESPONSE_EASY, nullptr, &result);
    if (graded) {
        update();
    } else {
        // Session complete
        showSessionComplete();
    }
}

void StudyScreenWidget::onAnswerGood()
{
    if (!m_sessions) {
        m_cardDisplay->setPlainText("Next card would appear here...");
        return;
    }
    
    SRSReviewResult result;
    bool graded = session_manager_grade(m_sessions, SRS_RESPONSE_GOOD, nullptr, &result);
    if (graded) {
        update();
    } else {
        showSessionComplete();
    }
}

void StudyScreenWidget::onAnswerHard()
{
    if (!m_sessions) {
        m_cardDisplay->setPlainText("Next card would appear here...");
        return;
    }
    
    SRSReviewResult result;
    bool graded = session_manager_grade(m_sessions, SRS_RESPONSE_HARD, nullptr, &result);
    if (graded) {
        update();
    } else {
        showSessionComplete();
    }
}

void StudyScreenWidget::onAnswerAgain()
{
    if (!m_sessions) {
        m_cardDisplay->setPlainText("Card will be shown again soon...");
        return;
    }
    
    SRSReviewResult result;
    bool graded = session_manager_grade(m_sessions, SRS_RESPONSE_FAIL, nullptr, &result);
    if (graded) {
        update();
    } else {
        showSessionComplete();
    }
}
