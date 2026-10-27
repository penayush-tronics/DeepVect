#pragma once

#include <QHash>
#include <QMainWindow>

class AppModel;
class GLWidget;
class Graph2DWidget;
class OperationCard;
class QAction;
class QComboBox;
class QLabel;
class QScrollArea;
class QSplitter;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;
class QWidget;
class VectorCard;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    QWidget* createToolbar();
    QWidget* createSidebar();
    void connectModel();

    void applyTheme();
    void updateToolbarState();
    void refreshGraph();
    void rebuildVectorCards();
    void updateCardSelection(int selectedId);

    AppModel* m_model = nullptr;
    Graph2DWidget* m_graph2D = nullptr;
    GLWidget* m_glWidget = nullptr;
    QStackedWidget* m_graphStack = nullptr;
    QSplitter* m_splitter = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_cardsContainer = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QHash<int, VectorCard*> m_cards;
    OperationCard* m_operationCard = nullptr;

    QToolButton* m_twoDButton = nullptr;
    QToolButton* m_threeDButton = nullptr;
    QLabel* m_planeLabel = nullptr;
    QComboBox* m_planeCombo = nullptr;
    QComboBox* m_formatCombo = nullptr;
    QToolButton* m_displayButton = nullptr;
    QAction* m_axisNumbersAction = nullptr;
    QAction* m_labelsAction = nullptr;
    QAction* m_headToTailAction = nullptr;
    QToolButton* m_themeButton = nullptr;
    QToolButton* m_fitButton = nullptr;
};
