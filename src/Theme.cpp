#include "Theme.h"

namespace {

ThemeColors lightColors() {
    ThemeColors colors;
    colors.appBackground = QColor(0xF6, 0xF7, 0xF9);
    colors.panel = QColor(0xFF, 0xFF, 0xFF);
    colors.canvas = QColor(0xFF, 0xFF, 0xFF);
    colors.border = QColor(0xE4, 0xE7, 0xEC);
    colors.text = QColor(0x1F, 0x23, 0x28);
    colors.mutedText = QColor(0x6B, 0x72, 0x80);
    colors.accent = QColor(0x2D, 0x70, 0xB3);
    colors.accentSoft = QColor(0xE8, 0xF0, 0xF9);
    colors.gridMinor = QColor(0xEC, 0xEF, 0xF3);
    colors.gridMajor = QColor(0xDA, 0xDF, 0xE6);
    colors.axis = QColor(0x2B, 0x2F, 0x34);
    colors.axisX = QColor(0xCB, 0x3F, 0x3A);
    colors.axisY = QColor(0x22, 0x7E, 0x53);
    colors.axisZ = QColor(0x1D, 0x5F, 0xA8);
    colors.selection = QColor(0xD9, 0xE8, 0xF8);
    colors.error = QColor(0xC0, 0x38, 0x2B);
    colors.result = QColor(0x63, 0x40, 0x99);
    colors.vectorColors = {
        QColor(0x1F, 0x63, 0xB0),
        QColor(0x0E, 0x85, 0x7B),
        QColor(0xC2, 0x3B, 0x2E),
        QColor(0x6B, 0x46, 0xA8),
        QColor(0xB2, 0x6A, 0x00),
        QColor(0x2A, 0x7D, 0x4F),
        QColor(0xA6, 0x3A, 0x66),
        QColor(0x44, 0x51, 0x5F),
    };
    return colors;
}

ThemeColors darkColors() {
    ThemeColors colors;
    colors.appBackground = QColor(0x15, 0x16, 0x19);
    colors.panel = QColor(0x1E, 0x1F, 0x24);
    colors.canvas = QColor(0x1B, 0x1C, 0x20);
    colors.border = QColor(0x2E, 0x31, 0x38);
    colors.text = QColor(0xE8, 0xEA, 0xED);
    colors.mutedText = QColor(0x9A, 0xA0, 0xA6);
    colors.accent = QColor(0x6F, 0xA8, 0xDC);
    colors.accentSoft = QColor(0x26, 0x35, 0x44);
    colors.gridMinor = QColor(0x27, 0x29, 0x2F);
    colors.gridMajor = QColor(0x36, 0x39, 0x42);
    colors.axis = QColor(0xC9, 0xCE, 0xD7);
    colors.axisX = QColor(0xF0, 0x7A, 0x7A);
    colors.axisY = QColor(0x79, 0xDD, 0xA5);
    colors.axisZ = QColor(0x80, 0xB8, 0xEB);
    colors.selection = QColor(0x25, 0x32, 0x40);
    colors.error = QColor(0xF0, 0x7A, 0x7A);
    colors.result = QColor(0xD2, 0xBB, 0xF5);
    colors.vectorColors = {
        QColor(0x74, 0xB9, 0xF0),
        QColor(0x5F, 0xE0, 0xD1),
        QColor(0xFF, 0x9A, 0x8F),
        QColor(0xC8, 0xB3, 0xF4),
        QColor(0xFF, 0xC1, 0x68),
        QColor(0x7D, 0xE0, 0xA9),
        QColor(0xF2, 0x9C, 0xC7),
        QColor(0xC8, 0xCE, 0xD6),
    };
    return colors;
}

} // namespace

namespace Theme {

ThemeColors colors(ThemeMode mode) {
    return mode == ThemeMode::Dark ? darkColors() : lightColors();
}

QString styleSheet(ThemeMode mode) {
    const ThemeColors c = colors(mode);

    return QStringLiteral(R"QSS(
QMainWindow, QWidget#AppRoot {
    background-color: %1;
    color: %5;
}
QFrame#Toolbar {
    background-color: %2;
    border-bottom: 1px solid %4;
}
QFrame#Sidebar {
    background-color: %2;
    border-right: 1px solid %4;
}
QLabel {
    color: %5;
}
QLabel#AppTitle {
    font-size: 17px;
    font-weight: 600;
}
QLabel#SectionTitle {
    font-size: 11px;
    font-weight: 600;
    color: %6;
}
QLabel#Hint {
    color: %6;
    font-size: 11px;
}
QLabel#ErrorLabel {
    color: %9;
    font-size: 11px;
}
QFrame#Card {
    background-color: %2;
    border: 1px solid %4;
    border-radius: 10px;
}
QFrame#Card[selected="true"] {
    border: 1px solid %7;
    background-color: %8;
}
QFrame#Card[error="true"] {
    border: 1px solid %9;
}
QLineEdit, QDoubleSpinBox, QComboBox {
    background-color: %3;
    border: 1px solid %4;
    border-radius: 6px;
    padding: 4px 8px;
    color: %5;
    selection-background-color: %7;
    selection-color: white;
}
QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus {
    border: 1px solid %7;
}
QLineEdit#InlineName {
    background: transparent;
    border: 1px solid transparent;
    font-weight: 600;
    padding: 2px 4px;
}
QLineEdit#InlineName:focus {
    background-color: %3;
    border: 1px solid %7;
}
QPushButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 5px 10px;
    color: %7;
    font-weight: 500;
}
QPushButton:hover {
    background-color: %8;
}
QPushButton:pressed {
    background-color: %8;
}
QPushButton#PrimaryButton {
    background-color: %7;
    color: white;
    border: none;
    padding: 6px 12px;
}
QPushButton#PrimaryButton:hover {
    background-color: %7;
}
QPushButton#DangerButton {
    color: %9;
}
QPushButton#DangerButton:hover {
    background-color: %8;
}
QToolButton {
    background-color: %2;
    border: 1px solid %4;
    border-radius: 6px;
    padding: 4px 9px;
    color: %5;
}
QToolButton:hover {
    background-color: %8;
}
QToolButton:checked {
    background-color: %7;
    border-color: %7;
    color: white;
}
QCheckBox {
    spacing: 6px;
    color: %5;
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid %4;
    border-radius: 4px;
    background-color: %3;
}
QCheckBox::indicator:hover {
    border-color: %7;
}
QCheckBox::indicator:checked {
    border-color: %7;
    background-color: %7;
    image: url(:/icons/check.png);
}
QMenu {
    background-color: %2;
    border: 1px solid %4;
}
QMenu::item {
    padding: 5px 18px;
    color: %5;
}
QMenu::item:selected {
    background-color: %8;
}
QScrollArea {
    border: none;
    background-color: %2;
}
QWidget#CardsViewport, QWidget#CardsContainer {
    background-color: %2;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background-color: %4;
    border-radius: 5px;
    min-height: 30px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}
QToolTip {
    background-color: %2;
    color: %5;
    border: 1px solid %4;
}
)QSS")
        .arg(c.appBackground.name())
        .arg(c.panel.name())
        .arg(c.canvas.name())
        .arg(c.border.name())
        .arg(c.text.name())
        .arg(c.mutedText.name())
        .arg(c.accent.name())
        .arg(c.accentSoft.name())
        .arg(c.error.name());
}

} // namespace Theme
