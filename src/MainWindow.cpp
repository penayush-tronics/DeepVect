#include "MainWindow.h"

#include "AppModel.h"
#include "GLWidget.h"
#include "Graph2DWidget.h"
#include "GraphTypes.h"
#include "OperationCard.h"
#include "Theme.h"
#include "VectorCard.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_model(new AppModel(this)) {
    setWindowTitle(QStringLiteral("Vectorial"));
    setMinimumSize(900, 640);

    buildUi();
    connectModel();
    rebuildVectorCards();
    updateToolbarState();
    applyTheme();
}

void MainWindow::buildUi() {
    auto* root = new QWidget(this);
    root->setObjectName(QStringLiteral("AppRoot"));

    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    rootLayout->addWidget(createToolbar());

    m_splitter = new QSplitter(Qt::Horizontal, root);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->addWidget(createSidebar());

    m_graphStack = new QStackedWidget(m_splitter);
    m_graphStack->setMinimumWidth(420);

    m_graph2D = new Graph2DWidget(m_graphStack);
    m_glWidget = new GLWidget(m_graphStack);
    m_graphStack->addWidget(m_graph2D);
    m_graphStack->addWidget(m_glWidget);

    m_splitter->addWidget(m_graphStack);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({390, 900});

    rootLayout->addWidget(m_splitter, 1);
    setCentralWidget(root);
}

QWidget* MainWindow::createToolbar() {
    auto* toolbar = new QFrame;
    toolbar->setObjectName(QStringLiteral("Toolbar"));

    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(8);

    auto* title = new QLabel(QStringLiteral("Vectorial"), toolbar);
    title->setObjectName(QStringLiteral("AppTitle"));
    layout->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("vector algebra"), toolbar);
    subtitle->setObjectName(QStringLiteral("Hint"));
    layout->addWidget(subtitle);
    layout->addStretch();

    m_twoDButton = new QToolButton(toolbar);
    m_twoDButton->setText(QStringLiteral("2D"));
    m_twoDButton->setCheckable(true);
    m_twoDButton->setAutoExclusive(true);
    m_twoDButton->setToolTip(QStringLiteral("Flat plane view"));
    layout->addWidget(m_twoDButton);

    m_threeDButton = new QToolButton(toolbar);
    m_threeDButton->setText(QStringLiteral("3D"));
    m_threeDButton->setCheckable(true);
    m_threeDButton->setAutoExclusive(true);
    m_threeDButton->setToolTip(QStringLiteral("Orbitable 3D view"));
    layout->addWidget(m_threeDButton);

    m_planeLabel = new QLabel(QStringLiteral("Plane"), toolbar);
    m_planeLabel->setObjectName(QStringLiteral("Hint"));
    layout->addWidget(m_planeLabel);

    m_planeCombo = new QComboBox(toolbar);
    m_planeCombo->addItem(QStringLiteral("xy"), static_cast<int>(Plane2D::XY));
    m_planeCombo->addItem(QStringLiteral("xz"), static_cast<int>(Plane2D::XZ));
    m_planeCombo->addItem(QStringLiteral("yz"), static_cast<int>(Plane2D::YZ));
    layout->addWidget(m_planeCombo);

    m_formatCombo = new QComboBox(toolbar);
    m_formatCombo->addItem(QStringLiteral("i, j, k"), static_cast<int>(InputFormat::IJK));
    m_formatCombo->addItem(QStringLiteral("Column"), static_cast<int>(InputFormat::Column));
    layout->addWidget(m_formatCombo);

    m_displayButton = new QToolButton(toolbar);
    m_displayButton->setText(QStringLiteral("Display"));
    m_displayButton->setPopupMode(QToolButton::InstantPopup);
    auto* displayMenu = new QMenu(m_displayButton);
    m_axisNumbersAction = displayMenu->addAction(QStringLiteral("Axis numbers"));
    m_axisNumbersAction->setCheckable(true);
    m_labelsAction = displayMenu->addAction(QStringLiteral("Vector labels"));
    m_labelsAction->setCheckable(true);
    m_headToTailAction = displayMenu->addAction(QStringLiteral("Head-to-tail"));
    m_headToTailAction->setCheckable(true);
    m_displayButton->setMenu(displayMenu);
    layout->addWidget(m_displayButton);

    m_themeButton = new QToolButton(toolbar);
    m_themeButton->setText(QStringLiteral("Dark mode"));
    m_themeButton->setCheckable(true);
    m_themeButton->setToolTip(QStringLiteral("Toggle dark mode"));
    layout->addWidget(m_themeButton);

    m_fitButton = new QToolButton(toolbar);
    m_fitButton->setText(QStringLiteral("Fit"));
    m_fitButton->setToolTip(QStringLiteral("Fit the graph to the vectors"));
    layout->addWidget(m_fitButton);

    connect(m_twoDButton, &QToolButton::clicked, this, [this]() {
        m_model->setViewMode(ViewMode::TwoD);
    });
    connect(m_threeDButton, &QToolButton::clicked, this, [this]() {
        m_model->setViewMode(ViewMode::ThreeD);
    });
    connect(m_planeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_model->setPlane2D(static_cast<Plane2D>(m_planeCombo->itemData(index).toInt()));
    });
    connect(m_formatCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_model->setInputFormat(static_cast<InputFormat>(m_formatCombo->itemData(index).toInt()));
    });
    connect(m_axisNumbersAction, &QAction::toggled, this, [this](bool checked) {
        m_model->setShowAxisNumbers(checked);
    });
    connect(m_labelsAction, &QAction::toggled, this, [this](bool checked) {
        m_model->setShowLabels(checked);
    });
    connect(m_headToTailAction, &QAction::toggled, this, [this](bool checked) {
        m_model->setArrowMode(checked ? ArrowMode::HeadToTail : ArrowMode::Origin);
    });
    connect(m_themeButton, &QToolButton::clicked, this, [this](bool checked) {
        m_model->setThemeMode(checked ? ThemeMode::Dark : ThemeMode::Light);
    });
    connect(m_fitButton, &QToolButton::clicked, this, [this]() {
        if (m_model->viewMode() == ViewMode::TwoD) {
            m_graph2D->fitToScene();
        } else {
            m_glWidget->fitToScene();
        }
    });

    return toolbar;
}

QWidget* MainWindow::createSidebar() {
    auto* sidebar = new QFrame;
    sidebar->setObjectName(QStringLiteral("Sidebar"));
    sidebar->setMinimumWidth(340);
    sidebar->setMaximumWidth(520);

    auto* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("VECTORS"), sidebar);
    title->setObjectName(QStringLiteral("SectionTitle"));
    auto* addButton = new QPushButton(QStringLiteral("+ Add"), sidebar);
    addButton->setToolTip(QStringLiteral("Add a vector"));
    header->addWidget(title);
    header->addStretch();
    header->addWidget(addButton);
    layout->addLayout(header);

    m_cardsContainer = new QWidget(sidebar);
    m_cardsContainer->setObjectName(QStringLiteral("CardsContainer"));
    m_cardsContainer->setAttribute(Qt::WA_StyledBackground, true);
    m_cardsLayout = new QVBoxLayout(m_cardsContainer);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(8);
    m_cardsLayout->addStretch();

    m_scrollArea = new QScrollArea(sidebar);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->viewport()->setObjectName(QStringLiteral("CardsViewport"));
    m_scrollArea->setWidget(m_cardsContainer);
    layout->addWidget(m_scrollArea, 1);

    m_operationCard = new OperationCard(m_model, sidebar);
    layout->addWidget(m_operationCard);

    connect(addButton, &QPushButton::clicked, this, [this]() {
        m_model->addVector();
    });

    return sidebar;
}

void MainWindow::connectModel() {
    connect(m_model, &AppModel::vectorsChanged, this, [this]() {
        rebuildVectorCards();
        refreshGraph();
    });
    connect(m_model, &AppModel::vectorUpdated, this, [this](int) {
        refreshGraph();
    });
    connect(m_model, &AppModel::selectionChanged, this, [this](int id) {
        updateCardSelection(id);
        refreshGraph();
    });
    connect(m_model, &AppModel::settingsChanged, this, [this]() {
        applyTheme();
        updateToolbarState();
    });
    connect(m_model, &AppModel::operationChanged, this, [this]() {
        refreshGraph();
    });
}

void MainWindow::applyTheme() {
    const ThemeColors colors = Theme::colors(m_model->themeMode());

    qApp->setStyleSheet(Theme::styleSheet(m_model->themeMode()));

    QPalette palette;
    palette.setColor(QPalette::Window, colors.appBackground);
    palette.setColor(QPalette::WindowText, colors.text);
    palette.setColor(QPalette::Base, colors.panel);
    palette.setColor(QPalette::AlternateBase, colors.canvas);
    palette.setColor(QPalette::ToolTipBase, colors.panel);
    palette.setColor(QPalette::ToolTipText, colors.text);
    palette.setColor(QPalette::Text, colors.text);
    palette.setColor(QPalette::Button, colors.panel);
    palette.setColor(QPalette::ButtonText, colors.text);
    palette.setColor(QPalette::BrightText, colors.error);
    palette.setColor(QPalette::Link, colors.accent);
    palette.setColor(QPalette::Highlight, colors.accent);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, colors.mutedText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, colors.mutedText);
    qApp->setPalette(palette);

    refreshGraph();
}

void MainWindow::updateToolbarState() {
    const bool twoD = m_model->viewMode() == ViewMode::TwoD;

    {
        const QSignalBlocker blockTwoD(m_twoDButton);
        const QSignalBlocker blockThreeD(m_threeDButton);
        m_twoDButton->setChecked(twoD);
        m_threeDButton->setChecked(!twoD);
    }

    m_planeLabel->setVisible(twoD);
    m_planeCombo->setVisible(twoD);
    {
        const QSignalBlocker block(m_planeCombo);
        const int index = m_planeCombo->findData(static_cast<int>(m_model->plane2D()));
        if (index >= 0) {
            m_planeCombo->setCurrentIndex(index);
        }
    }

    {
        const QSignalBlocker block(m_formatCombo);
        const int index = m_formatCombo->findData(static_cast<int>(m_model->inputFormat()));
        if (index >= 0) {
            m_formatCombo->setCurrentIndex(index);
        }
    }

    {
        const QSignalBlocker blockNumbers(m_axisNumbersAction);
        const QSignalBlocker blockLabels(m_labelsAction);
        const QSignalBlocker blockHeadToTail(m_headToTailAction);
        m_axisNumbersAction->setChecked(m_model->showAxisNumbers());
        m_labelsAction->setChecked(m_model->showLabels());
        m_headToTailAction->setChecked(m_model->arrowMode() == ArrowMode::HeadToTail);
    }

    {
        const QSignalBlocker block(m_themeButton);
        m_themeButton->setChecked(m_model->themeMode() == ThemeMode::Dark);
    }
}

void MainWindow::refreshGraph() {
    const ThemeColors colors = Theme::colors(m_model->themeMode());

    m_graph2D->setTheme(colors);
    m_glWidget->setTheme(colors);

    m_graph2D->setPlane(m_model->plane2D());
    m_graph2D->setShowAxisNumbers(m_model->showAxisNumbers());
    m_graph2D->setShowLabels(m_model->showLabels());
    m_glWidget->setShowAxisNumbers(m_model->showAxisNumbers());
    m_glWidget->setShowLabels(m_model->showLabels());

    const QList<GraphVector> vectors = m_model->graphVectors(colors);
    m_graph2D->setVectors(vectors);
    m_glWidget->setGraphVectors(vectors);

    m_graphStack->setCurrentWidget(
        m_model->viewMode() == ViewMode::TwoD ? static_cast<QWidget*>(m_graph2D)
                                              : static_cast<QWidget*>(m_glWidget));
}

void MainWindow::rebuildVectorCards() {
    const QList<VectorCard*> oldCards = m_cards.values();
    for (VectorCard* card : oldCards) {
        m_cardsLayout->removeWidget(card);
        card->hide();
        card->deleteLater();
    }
    m_cards.clear();

    for (const VectorEntry& entry : m_model->vectors()) {
        auto* card = new VectorCard(m_model, entry.id, m_cardsContainer);
        m_cardsLayout->insertWidget(m_cardsLayout->count() - 1, card);
        connect(card, &VectorCard::activated, this, [this](int id) {
            m_model->selectVector(id);
        });
        m_cards.insert(entry.id, card);
    }

    updateCardSelection(m_model->selectedId());
}

void MainWindow::updateCardSelection(int selectedId) {
    for (auto it = m_cards.cbegin(); it != m_cards.cend(); ++it) {
        it.value()->setSelected(it.key() == selectedId);
    }

    const auto scrollToSelected = [this, selectedId]() {
        VectorCard* card = m_cards.value(selectedId, nullptr);
        if (card == nullptr) {
            return;
        }
        if (card->layout() != nullptr) {
            card->layout()->activate();
        }
        m_cardsLayout->activate();
        m_scrollArea->ensureWidgetVisible(card, 0, 8);
    };
    QTimer::singleShot(0, this, scrollToSelected);
    QTimer::singleShot(50, this, scrollToSelected);
}
