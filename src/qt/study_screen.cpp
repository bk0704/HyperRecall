#include "study_screen.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QStackedWidget>

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
    // TODO: Sync with actual session state
}

void StudyScreenWidget::onStartMasterySession()
{
    // TODO: Actually start a mastery session via m_sessions
    showCardReview();
}

void StudyScreenWidget::onStartCramSession()
{
    // TODO: Actually start a cram session via m_sessions
    showCardReview();
}

void StudyScreenWidget::onAnswerEasy()
{
    // TODO: Submit response to session manager
    // For prototype, just simulate
    m_cardDisplay->setPlainText("Next card would appear here...");
}

void StudyScreenWidget::onAnswerGood()
{
    // TODO: Submit response to session manager
    m_cardDisplay->setPlainText("Next card would appear here...");
}

void StudyScreenWidget::onAnswerHard()
{
    // TODO: Submit response to session manager
    m_cardDisplay->setPlainText("Next card would appear here...");
}

void StudyScreenWidget::onAnswerAgain()
{
    // TODO: Submit response to session manager
    m_cardDisplay->setPlainText("Card will be shown again soon...");
}
