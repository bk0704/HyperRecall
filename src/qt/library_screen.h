#ifndef HYPERRECALL_LIBRARY_SCREEN_H
#define HYPERRECALL_LIBRARY_SCREEN_H

#include <QWidget>

class QTreeWidget;
class QTableWidget;
class QPushButton;
class QSplitter;

extern "C" {
#include "../db.h"
}

/**
 * @brief Library management screen for topics and cards
 */
class LibraryScreenWidget : public QWidget {
    Q_OBJECT

public:
    explicit LibraryScreenWidget(QWidget *parent = nullptr);
    
    void setDatabase(DatabaseHandle *database);
    void update();

private slots:
    void onTopicSelected();
    void onAddTopic();
    void onAddCard();
    void onEditCard();
    void onDeleteCard();

private:
    void setupUI();
    void refreshTopics();
    void refreshCards();
    
    DatabaseHandle *m_database;
    
    QTreeWidget *m_topicTree;
    QTableWidget *m_cardTable;
    QPushButton *m_addTopicBtn;
    QPushButton *m_addCardBtn;
    QPushButton *m_editCardBtn;
    QPushButton *m_deleteCardBtn;
};

#endif /* HYPERRECALL_LIBRARY_SCREEN_H */
