#ifndef HYPERRECALL_STUDY_SCREEN_H
#define HYPERRECALL_STUDY_SCREEN_H

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;
class QTextEdit;

extern "C" {
#include "../sessions.h"
}

/**
 * @brief Study screen widget for SRS card review
 */
class StudyScreenWidget : public QWidget {
    Q_OBJECT

public:
    explicit StudyScreenWidget(QWidget *parent = nullptr);
    
    void setSessionManager(struct SessionManager *sessions);
    void update();

signals:
    void sessionCompleted();

private slots:
    void onStartMasterySession();
    void onStartCramSession();
    void onAnswerEasy();
    void onAnswerGood();
    void onAnswerHard();
    void onAnswerAgain();

private:
    void setupUI();
    void showWelcomeScreen();
    void showCardReview();
    void showSessionComplete();
    
    struct SessionManager *m_sessions;
    
    QLabel *m_statusLabel;
    QTextEdit *m_cardDisplay;
    QPushButton *m_startMasteryBtn;
    QPushButton *m_startCramBtn;
    QPushButton *m_easyBtn;
    QPushButton *m_goodBtn;
    QPushButton *m_hardBtn;
    QPushButton *m_againBtn;
    
    QWidget *m_welcomeWidget;
    QWidget *m_reviewWidget;
    QWidget *m_completeWidget;
    
    bool m_sessionActive;
};

#endif /* HYPERRECALL_STUDY_SCREEN_H */
