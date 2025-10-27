#include "library_screen.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QMap>
#include <QString>
#include <QVariant>
#include <ctime>

extern "C" {
#include <sqlite3.h>
}

LibraryScreenWidget::LibraryScreenWidget(QWidget *parent)
    : QWidget(parent)
    , m_database(nullptr)
{
    setupUI();
}

void LibraryScreenWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    
    // Title
    auto *titleLabel = new QLabel("Library", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold;");
    mainLayout->addWidget(titleLabel);
    
    // Splitter for topics and cards
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left side: Topic tree
    auto *topicWidget = new QWidget(splitter);
    auto *topicLayout = new QVBoxLayout(topicWidget);
    
    auto *topicLabel = new QLabel("Topics", topicWidget);
    topicLabel->setStyleSheet("font-weight: bold;");
    topicLayout->addWidget(topicLabel);
    
    m_topicTree = new QTreeWidget(topicWidget);
    m_topicTree->setHeaderLabel("Topic Hierarchy");
    connect(m_topicTree, &QTreeWidget::itemSelectionChanged, this, &LibraryScreenWidget::onTopicSelected);
    topicLayout->addWidget(m_topicTree);
    
    m_addTopicBtn = new QPushButton("Add Topic", topicWidget);
    connect(m_addTopicBtn, &QPushButton::clicked, this, &LibraryScreenWidget::onAddTopic);
    topicLayout->addWidget(m_addTopicBtn);
    
    // Add sample topics
    auto *rootItem = new QTreeWidgetItem(m_topicTree);
    rootItem->setText(0, "Programming");
    auto *child1 = new QTreeWidgetItem(rootItem);
    child1->setText(0, "C++");
    auto *child2 = new QTreeWidgetItem(rootItem);
    child2->setText(0, "Python");
    auto *child3 = new QTreeWidgetItem(rootItem);
    child3->setText(0, "JavaScript");
    
    auto *mathItem = new QTreeWidgetItem(m_topicTree);
    mathItem->setText(0, "Mathematics");
    auto *mathChild1 = new QTreeWidgetItem(mathItem);
    mathChild1->setText(0, "Algebra");
    auto *mathChild2 = new QTreeWidgetItem(mathItem);
    mathChild2->setText(0, "Calculus");
    
    m_topicTree->expandAll();
    
    splitter->addWidget(topicWidget);
    
    // Right side: Card table
    auto *cardWidget = new QWidget(splitter);
    auto *cardLayout = new QVBoxLayout(cardWidget);
    
    auto *cardLabel = new QLabel("Cards", cardWidget);
    cardLabel->setStyleSheet("font-weight: bold;");
    cardLayout->addWidget(cardLabel);
    
    m_cardTable = new QTableWidget(0, 4, cardWidget);
    m_cardTable->setHorizontalHeaderLabels({"Prompt", "Type", "Due Date", "Ease"});
    m_cardTable->horizontalHeader()->setStretchLastSection(true);
    m_cardTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cardTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    cardLayout->addWidget(m_cardTable);
    
    // Card action buttons
    auto *cardButtonLayout = new QHBoxLayout();
    
    m_addCardBtn = new QPushButton("Add Card", cardWidget);
    connect(m_addCardBtn, &QPushButton::clicked, this, &LibraryScreenWidget::onAddCard);
    cardButtonLayout->addWidget(m_addCardBtn);
    
    m_editCardBtn = new QPushButton("Edit Card", cardWidget);
    m_editCardBtn->setEnabled(false);
    connect(m_editCardBtn, &QPushButton::clicked, this, &LibraryScreenWidget::onEditCard);
    cardButtonLayout->addWidget(m_editCardBtn);
    
    m_deleteCardBtn = new QPushButton("Delete Card", cardWidget);
    m_deleteCardBtn->setEnabled(false);
    connect(m_deleteCardBtn, &QPushButton::clicked, this, &LibraryScreenWidget::onDeleteCard);
    cardButtonLayout->addWidget(m_deleteCardBtn);
    
    cardButtonLayout->addStretch();
    cardLayout->addLayout(cardButtonLayout);
    
    splitter->addWidget(cardWidget);
    
    // Set splitter sizes
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    
    mainLayout->addWidget(splitter);
    
    // Populate with sample cards
    refreshCards();
}

void LibraryScreenWidget::setDatabase(DatabaseHandle *database)
{
    m_database = database;
    if (m_database) {
        refreshTopics();
        refreshCards();
    }
}

void LibraryScreenWidget::update()
{
    if (m_database) {
        refreshTopics();
        refreshCards();
    }
}

void LibraryScreenWidget::onTopicSelected()
{
    refreshCards();
    m_addCardBtn->setEnabled(true);
}

void LibraryScreenWidget::onAddTopic()
{
    QMessageBox::information(this, "Add Topic", "Add topic dialog would appear here\n(Database integration complete - UI TODO)");
}

void LibraryScreenWidget::onAddCard()
{
    QMessageBox::information(this, "Add Card", "Add card dialog would appear here\n(Database integration complete - UI TODO)");
}

void LibraryScreenWidget::onEditCard()
{
    QMessageBox::information(this, "Edit Card", "Edit card dialog would appear here\n(Database integration complete - UI TODO)");
}

void LibraryScreenWidget::onDeleteCard()
{
    QMessageBox::question(this, "Delete Card", "Delete this card?\n(Database integration complete - UI TODO)", 
                         QMessageBox::Yes | QMessageBox::No);
}

void LibraryScreenWidget::refreshTopics()
{
    if (!m_database) {
        return;
    }
    
    m_topicTree->clear();
    
    // Query all topics from database
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT id, title, parent_id FROM topics ORDER BY parent_id, title";
    
    if (db_prepare(m_database, &stmt, sql) == SQLITE_OK) {
        // First pass: create all items
        QMap<sqlite3_int64, QTreeWidgetItem*> itemMap;
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            sqlite3_int64 id = sqlite3_column_int64(stmt, 0);
            const char *title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            sqlite3_int64 parent_id = sqlite3_column_int64(stmt, 2);
            
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, QString::fromUtf8(title ? title : "Unknown"));
            item->setData(0, Qt::UserRole, QVariant::fromValue(id));
            
            itemMap[id] = item;
            
            // If has parent, add as child; otherwise add to root
            if (parent_id != 0 && itemMap.contains(parent_id)) {
                itemMap[parent_id]->addChild(item);
            } else {
                m_topicTree->addTopLevelItem(item);
            }
        }
        
        sqlite3_finalize(stmt);
        m_topicTree->expandAll();
    }
    
    // If no topics exist, show a message
    if (m_topicTree->topLevelItemCount() == 0) {
        auto *item = new QTreeWidgetItem(m_topicTree);
        item->setText(0, "(No topics - add one to get started)");
        item->setDisabled(true);
    }
}

void LibraryScreenWidget::refreshCards()
{
    if (!m_database) {
        // Show sample data if no database
        m_cardTable->setRowCount(8);
        
        QStringList sampleCards[] = {
            {"What is a pointer in C++?", "Short Answer", "Today", "2.5"},
            {"Define polymorphism", "Short Answer", "Tomorrow", "2.3"},
            {"[...] is a memory leak", "Cloze", "In 2 days", "2.7"},
            {"Which is faster: vector or list?", "Multiple Choice", "Today", "2.1"},
            {"Explain virtual functions", "Short Answer", "In 3 days", "2.6"},
            {"const vs constexpr", "Compare", "Today", "2.4"},
            {"RAII stands for [...]", "Cloze", "Tomorrow", "2.8"},
            {"What does auto keyword do?", "Short Answer", "In 4 days", "2.2"}
        };
        
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 4; ++col) {
                m_cardTable->setItem(row, col, new QTableWidgetItem(sampleCards[row][col]));
            }
        }
        return;
    }
    
    m_cardTable->setRowCount(0);
    
    // Get selected topic
    sqlite3_int64 topic_id = 0;
    QTreeWidgetItem *selected = m_topicTree->currentItem();
    if (selected && !selected->isDisabled()) {
        topic_id = selected->data(0, Qt::UserRole).toLongLong();
    }
    
    // Query cards from database
    sqlite3_stmt *stmt = nullptr;
    const char *sql = topic_id > 0 
        ? "SELECT id, prompt, type, due_at, ease_factor FROM cards WHERE topic_id = ? LIMIT 100"
        : "SELECT id, prompt, type, due_at, ease_factor FROM cards LIMIT 100";
    
    if (db_prepare(m_database, &stmt, sql) == SQLITE_OK) {
        if (topic_id > 0) {
            sqlite3_bind_int64(stmt, 1, topic_id);
        }
        
        int row = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            m_cardTable->insertRow(row);
            
            const char *prompt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int card_type = sqlite3_column_int(stmt, 2);
            time_t due_at = static_cast<time_t>(sqlite3_column_int64(stmt, 3));
            int ease = sqlite3_column_int(stmt, 4);
            
            // Prompt
            m_cardTable->setItem(row, 0, new QTableWidgetItem(
                QString::fromUtf8(prompt ? prompt : "")
            ));
            
            // Type (simplified - would need proper card type enum)
            m_cardTable->setItem(row, 1, new QTableWidgetItem(
                QString("Type %1").arg(card_type)
            ));
            
            // Due date
            QString dueStr;
            if (due_at > 0) {
                time_t now = time(nullptr);
                double diff_days = difftime(due_at, now) / (24.0 * 3600.0);
                if (diff_days < 0) {
                    dueStr = "Overdue";
                } else if (diff_days < 1) {
                    dueStr = "Today";
                } else {
                    dueStr = QString("In %1 days").arg(static_cast<int>(diff_days));
                }
            } else {
                dueStr = "New";
            }
            m_cardTable->setItem(row, 2, new QTableWidgetItem(dueStr));
            
            // Ease
            m_cardTable->setItem(row, 3, new QTableWidgetItem(
                QString::number(ease / 100.0, 'f', 2)
            ));
            
            row++;
        }
        
        sqlite3_finalize(stmt);
    }
    
    // If no cards, show message
    if (m_cardTable->rowCount() == 0) {
        m_cardTable->insertRow(0);
        auto *item = new QTableWidgetItem("(No cards - add one to get started)");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        m_cardTable->setItem(0, 0, item);
    }
}
