#include "library_screen.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>

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
    refreshTopics();
    refreshCards();
}

void LibraryScreenWidget::update()
{
    // TODO: Sync with database
}

void LibraryScreenWidget::onTopicSelected()
{
    refreshCards();
    m_addCardBtn->setEnabled(true);
}

void LibraryScreenWidget::onAddTopic()
{
    QMessageBox::information(this, "Add Topic", "Add topic dialog would appear here\n(Prototype placeholder)");
}

void LibraryScreenWidget::onAddCard()
{
    QMessageBox::information(this, "Add Card", "Add card dialog would appear here\n(Prototype placeholder)");
}

void LibraryScreenWidget::onEditCard()
{
    QMessageBox::information(this, "Edit Card", "Edit card dialog would appear here\n(Prototype placeholder)");
}

void LibraryScreenWidget::onDeleteCard()
{
    QMessageBox::question(this, "Delete Card", "Delete this card?\n(Prototype placeholder)", 
                         QMessageBox::Yes | QMessageBox::No);
}

void LibraryScreenWidget::refreshTopics()
{
    // TODO: Load actual topics from database
}

void LibraryScreenWidget::refreshCards()
{
    // Add sample cards for prototype
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
    
    // Enable edit/delete buttons when row is selected
    connect(m_cardTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = !m_cardTable->selectedItems().isEmpty();
        m_editCardBtn->setEnabled(hasSelection);
        m_deleteCardBtn->setEnabled(hasSelection);
    });
}
