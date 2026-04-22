/**
 * @file    apptheme.cpp
 * @brief   A fully modern, 2026-era Dark/Light QSS stylesheet for Qt C++.
 * Clean, professional, and strictly uses supported Qt properties.
 * @author  Ali Sakkaf
 */
#include "apptheme.h"

QString AppTheme::darkQss()
{
    return R"QSS(
/* ==========================================================
   GLOBAL RESET & TYPOGRAPHY
   ========================================================== */
* {
    font-family: "Segoe UI", "SF Pro Text", Roboto, Arial, sans-serif;
    font-size: 9pt; /* Natural and comfortable reading size */
    outline: none;  /* Remove dotted outline on focus */
}

/* Fix transparent Taskbar issue: background applied only to main windows */
QMainWindow, QDialog {
    background-color: #18181B; /* Zinc 900 - Highly professional dark color */
    color: #E4E4E7;            /* Zinc 200 - Clear and non-glaring text */
}

QWidget {
    color: #E4E4E7; /* Force text color inheritance for all elements */
}

/* ==========================================================
   HEADER & STATUS BAR
   ========================================================== */
QFrame#headerFrame {
    background-color: #27272A; /* Zinc 800 */
    border-bottom: 1px solid #3F3F46;
}
QLabel#appTitle {
    font-size: 11pt;
    font-weight: bold;
    color: #FFFFFF;
}
QLabel#versionLabel {
    font-size: 8pt;
    color: #A1A1AA;
}
QLabel#liveBadge {
    background-color: rgba(16, 185, 129, 0.15);
    color: #34D399;
    font-weight: bold;
    border: 1px solid rgba(16, 185, 129, 0.3);
    border-radius: 4px;
    padding: 2px 8px;
}

QFrame#statusBar {
    background-color: #27272A;
    border-top: 1px solid #3F3F46;
}
QLabel#statusMsg  { color: #A1A1AA; }
QLabel#rxLabel    { color: #60A5FA; font-weight: bold; }
QLabel#txLabel    { color: #34D399; font-weight: bold; }
QLabel#cpuLabel   { color: #FBBF24; font-weight: bold; }
QLabel#ramLabel   { color: #A78BFA; font-weight: bold; }

/* ==========================================================
   TABS (MAIN & SUB)
   ========================================================== */
QTabWidget::pane {
    border: none;
    background-color: transparent;
}
QTabBar::tab {
    background: transparent;
    color: #A1A1AA;
    padding: 8px 16px;
    font-weight: bold;
    /* font-size: 9.5pt;
    border-bottom: 2px solid transparent;*/
}
QTabBar::tab:selected {
    color: #60A5FA; /* Soft blue */
    border-bottom: 2px solid #60A5FA;
}
QTabBar::tab:hover:!selected {
    color: #E4E4E7;
    border-bottom: 2px solid #52525B;
}

/* Sub-tabs specific override */
QTabWidget#subTabs::pane {
    border: 1px solid #3F3F46;
    border-radius: 6px;
    background-color: #27272A;
    top: -1px;
}
QTabWidget#subTabs > QTabBar::tab {
    background-color: #18181B;
    border: 1px solid #3F3F46;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 2px;
    padding: 6px 14px;
}
QTabWidget#subTabs > QTabBar::tab:selected {
    background-color: #27272A;
    color: #FFFFFF;
    border-top: 2px solid #60A5FA;
}

/* ==========================================================
   CARDS & DASHBOARD ELEMENTS
   ========================================================== */
QFrame#rxCard, QFrame#txCard, QFrame#pktCard, QFrame#speedCard {
    background-color: #27272A;
    border: 1px solid #3F3F46;
    border-radius: 8px;
}
QLabel#cardTitle {
    color: #A1A1AA;
    font-size: 8.5pt;
    font-weight: 800; /* Extra bold for professional card titles */
}
QLabel#cardTitle2 {
    color: #A1A1AA;
    font-size: 8.5pt;
    font-weight: 800; /* Extra bold for professional card titles */
}
QLabel#cardTitle3 {
    color: #A1A1AA;
    font-size: 8.5pt;
    font-weight: 800; /* Extra bold for professional card titles */
}
/* Unified size and extra bold weight for large numbers */
QLabel#lblPackets, QLabel#lblRxVal, QLabel#lblTxVal {
    font-size: 17pt;
    font-weight: 900; /* Ultra bold for high-tech aesthetic */
}
/* Individual colors applied cleanly */
QLabel#lblPackets { color: #FFFFFF; }
QLabel#lblRxVal   { color: #3B82F6; } /* Vibrant premium blue for Download */
QLabel#lblTxVal   { color: #10B981; } /* Vibrant emerald green for Upload */

QLabel#lblRxUnit, QLabel#lblTxUnit {
    font-size: 9.5pt;
    font-weight: bold;
    color: #A1A1AA;
}

/* ==========================================================
   BUTTONS
   ========================================================== */
QPushButton {
    background-color: #3F3F46;
    color: #FAFAFA;
    border: 1px solid #52525B;
    border-radius: 4px;
    padding: 6px 14px;
    font-weight: 500;
}
QPushButton:hover { background-color: #52525B; border-color: #71717A; }
QPushButton:pressed { background-color: #27272A; }
QPushButton:disabled { background-color: #27272A; color: #71717A; border-color: #3F3F46; }

QPushButton#btnStartStop { background-color: #2563EB; color: #FFFFFF; border: 1px solid #1D4ED8; }
QPushButton#btnStartStop:hover { background-color: #3B82F6; border-color: #2563EB; }

QPushButton#dangerBtn { background-color: rgba(220, 38, 38, 0.15); color: #F87171; border: 1px solid rgba(220, 38, 38, 0.3); }
QPushButton#dangerBtn:hover { background-color: rgba(220, 38, 38, 0.25); border-color: #F87171; }

QPushButton#successBtn { background-color: rgba(16, 185, 129, 0.15); color: #34D399; border: 1px solid rgba(16, 185, 129, 0.3); }
QPushButton#successBtn:hover { background-color: rgba(16, 185, 129, 0.25); border-color: #34D399; }

QPushButton#btnAddBlock { background-color: rgba(220, 38, 38, 0.15); color: #F87171; border: 1px solid rgba(220, 38, 38, 0.3); }
QPushButton#btnAddBlock:hover { background-color: rgba(220, 38, 38, 0.25); border-color: #F87171; }

QPushButton#btnAddAllow { background-color: rgba(16, 185, 129, 0.15); color: #34D399; border: 1px solid rgba(16, 185, 129, 0.3); }
QPushButton#btnAddAllow:hover { background-color: rgba(16, 185, 129, 0.25); border-color: #34D399; }

QPushButton#iconBtn { background: transparent; border: none; font-size: 12pt; color: #A1A1AA; padding: 4px; }
QPushButton#iconBtn:hover { background-color: #3F3F46; color: #FFFFFF; border-radius: 4px; }

/* ==========================================================
   INPUTS & COMBOBOX
   ========================================================== */
QLineEdit, QComboBox, QSpinBox {
    background-color: #18181B;
    border: 1px solid #3F3F46;
    border-radius: 4px;
    padding: 5px 10px;
    color: #FAFAFA;
    selection-background-color: #2563EB;
}
QLineEdit:focus, QComboBox:focus { border: 1px solid #60A5FA; }
QLineEdit::placeholder { color: #71717A; }

QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
    background-color: #27272A;
    border: 1px solid #3F3F46;
    selection-background-color: #3B82F6;
    selection-color: #FFFFFF;
    color: #FAFAFA;
}

/* ==========================================================
   TABLES & TREES (FIXED COLORS)
   ========================================================== */
QTableView, QTreeView {
    background-color: #27272A;
    alternate-background-color: #18181B;
    border: 1px solid #3F3F46;
    border-radius: 6px;
    gridline-color: transparent;
    color: #E4E4E7; /* Prevents black text issues */
    selection-background-color: #3B82F6; /* Distinct blue on selection */
    selection-color: #FFFFFF;
}
QHeaderView::section {
    background-color: #18181B;
    color: #A1A1AA;
    border: none;
    border-bottom: 1px solid #3F3F46;
    border-right: 1px solid #27272A;
    padding: 6px 8px;
    font-weight: bold;
    font-size: 8.5pt;
}
QTableView::item, QTreeView::item {
    padding: 4px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.02);
}
QTableView::item:hover, QTreeView::item:hover {
    background-color: #3F3F46; /* Hover background color */
    color: #FFFFFF;
}

/* ==========================================================
   SCROLLBARS
   ========================================================== */
QScrollBar:vertical { background: transparent; width: 8px; margin: 0px; }
QScrollBar::handle:vertical { background-color: #52525B; border-radius: 4px; min-height: 20px; }
QScrollBar::handle:vertical:hover { background-color: #71717A; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }

QScrollBar:horizontal { background: transparent; height: 8px; margin: 0px; }
QScrollBar::handle:horizontal { background-color: #52525B; border-radius: 4px; min-width: 20px; }
QScrollBar::handle:horizontal:hover { background-color: #71717A; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }

/* ==========================================================
   MENU & SYSTEM TRAY (DARK)
   ========================================================== */
QMenu {
    background-color: #18181B;
    border: 1px solid #3F3F46;
    border-radius: 6px;
    padding: 4px 0px;
}
QMenu::item {
    padding: 6px 24px;
    color: #E4E4E7;
    font-weight: bold;
}
QMenu::item:selected {
    background-color: #2563EB;
    color: #FFFFFF;
}
QMenu::separator {
    height: 1px;
    background-color: #3F3F46;
    margin: 4px 0px;
}

/* ==========================================================
   MISCELLANEOUS (GROUPS, LABELS, PROGRESS BARS)
   ========================================================== */
QGroupBox {
    border: 1px solid #3F3F46;
    border-radius: 6px;
    margin-top: 10px;
    padding-top: 14px;
    color: #A1A1AA;
    font-weight: bold;
}
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; color: #60A5FA; }

QCheckBox { spacing: 6px; }
QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; border: 1px solid #71717A; background-color: #18181B; }
QCheckBox::indicator:checked { background-color: #3B82F6; border-color: #3B82F6; }

QLabel#sectionHeading { font-size: 13pt; font-weight: bold; color: #FFFFFF; }
QLabel#groupLabel { color: #A1A1AA; font-weight: bold; }
QLabel#subDetail  { color: #A1A1AA; }
QLabel#dropHint { color: #71717A; border: 2px dashed #52525B; border-radius: 6px; font-weight: bold; background-color: #18181B; }
QLabel#dropHint:hover { border-color: #60A5FA; color: #60A5FA; }

QProgressBar { background-color: #3F3F46; border: none; border-radius: 3px; text-align: right; color: #FFFFFF; font-weight: bold; }
QProgressBar::chunk { background-color: #3B82F6; border-radius: 3px; }
)QSS";
}

QString AppTheme::lightQss()
{
    return R"QSS(
/* ==========================================================
   GLOBAL RESET & TYPOGRAPHY
   ========================================================== */
* {
    font-family: "Segoe UI", "SF Pro Text", Roboto, Arial, sans-serif;
    font-size: 9pt;
    outline: none;
}

QMainWindow, QDialog {
    background-color: #F4F4F5; /* Zinc 100 - Comfortable light color */
    color: #27272A;            /* Zinc 800 - Dark text for readability */
}

QWidget {
    color: #27272A; /* Force color on all widgets to prevent invisible numbers in Hardware tab */
}

/* ==========================================================
   HEADER & STATUS BAR
   ========================================================== */
QFrame#headerFrame {
    background-color: #FFFFFF;
    border-bottom: 1px solid #E4E4E7;
}
QLabel#appTitle {
    font-size: 11pt;
    font-weight: bold;
    color: #18181B;
}
QLabel#versionLabel {
    font-size: 8pt;
    color: #71717A;
}
QLabel#liveBadge {
    background-color: rgba(16, 185, 129, 0.1);
    color: #059669;
    font-weight: bold;
    border: 1px solid rgba(16, 185, 129, 0.25);
    border-radius: 4px;
    padding: 2px 8px;
}

QFrame#statusBar {
    background-color: #FFFFFF;
    border-top: 1px solid #E4E4E7;
}
QLabel#statusMsg  { color: #71717A; }
QLabel#rxLabel    { color: #2563EB; font-weight: bold; }
QLabel#txLabel    { color: #059669; font-weight: bold; }
QLabel#cpuLabel   { color: #D97706; font-weight: bold; }
QLabel#ramLabel   { color: #7C3AED; font-weight: bold; }

/* ==========================================================
   TABS (MAIN & SUB)
   ========================================================== */
QTabWidget::pane {
    border: none;
    background-color: transparent;
}
QTabBar::tab {
    background: transparent;
    color: #71717A;
    padding: 8px 16px;
    font-weight: bold;
   /* font-size: 9.5pt;
    border-bottom: 2px solid transparent;*/
}
QTabBar::tab:selected {
    color: #2563EB;
    border-bottom: 2px solid #2563EB;
}
QTabBar::tab:hover:!selected {
    color: #27272A;
    border-bottom: 2px solid #D4D4D8;
}

QTabWidget#subTabs::pane {
    border: 1px solid #E4E4E7;
    border-radius: 6px;
    background-color: #FFFFFF;
    top: -1px;
}
QTabWidget#subTabs > QTabBar::tab {
    background-color: #F4F4F5;
    border: 1px solid #E4E4E7;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    margin-right: 2px;
    padding: 6px 14px;
}
QTabWidget#subTabs > QTabBar::tab:selected {
    background-color: #FFFFFF;
    color: #18181B;
    border-top: 2px solid #2563EB;
}

/* ==========================================================
   CARDS & DASHBOARD ELEMENTS
   ========================================================== */
QFrame#rxCard, QFrame#txCard, QFrame#pktCard, QFrame#speedCard {
    background-color: #FFFFFF;
    border: 1px solid #E4E4E7;
    border-radius: 8px;
}
QLabel#cardTitle {
    color: #71717A;
    font-size: 8.5pt;
    font-weight: 800; /* Extra bold for professional card titles */
}
QLabel#cardTitle2 {
    color: #71717A;
    font-size: 8.5pt;
    font-weight: 800; /* Extra bold for professional card titles */
}
QLabel#cardTitle3 {
    color: #71717A;
    font-size: 8.5pt;
    font-weight: 800; /* Extra bold for professional card titles */
}
/* Unified size and extra bold weight for large numbers */
QLabel#lblPackets, QLabel#lblRxVal, QLabel#lblTxVal {
    font-size: 17pt;
    font-weight: 900; /* Ultra bold for high-tech aesthetic */
}
/* Individual colors applied cleanly */
QLabel#lblPackets { color: #18181B; } /* Solid black for standard numbers */
QLabel#lblRxVal   { color: #2563EB; } /* Darker premium blue for Download */
QLabel#lblTxVal   { color: #059669; } /* Darker emerald green for Upload */

QLabel#lblRxUnit, QLabel#lblTxUnit {
    font-size: 9.5pt;
    font-weight: bold;
    color: #71717A;
}

/* ==========================================================
   MENU & SYSTEM TRAY (LIGHT)
   ========================================================== */
QMenu {
    background-color: #FFFFFF;
    border: 1px solid #D4D4D8;
    border-radius: 6px;
    padding: 4px 0px;
}
QMenu::item {
    padding: 6px 24px;
    color: #27272A;
    font-weight: bold;
}
QMenu::item:selected {
    background-color: #2563EB;
    color: #FFFFFF;
}
QMenu::separator {
    height: 1px;
    background-color: #E4E4E7;
    margin: 4px 0px;
}

/* ==========================================================
   BUTTONS
   ========================================================== */
QPushButton {
    background-color: #FFFFFF;
    color: #27272A;
    border: 1px solid #D4D4D8;
    border-radius: 4px;
    padding: 6px 14px;
    font-weight: 500;
}
QPushButton:hover { background-color: #F4F4F5; border-color: #A1A1AA; }
QPushButton:pressed { background-color: #E4E4E7; }
QPushButton:disabled { background-color: #F4F4F5; color: #A1A1AA; border-color: #E4E4E7; }

QPushButton#btnStartStop { background-color: #2563EB; color: #FFFFFF; border: 1px solid #1D4ED8; }
QPushButton#btnStartStop:hover { background-color: #3B82F6; border-color: #2563EB; }

QPushButton#dangerBtn { background-color: rgba(220, 38, 38, 0.05); color: #DC2626; border: 1px solid rgba(220, 38, 38, 0.2); }
QPushButton#dangerBtn:hover { background-color: rgba(220, 38, 38, 0.1); border-color: #DC2626; }

QPushButton#successBtn { background-color: rgba(16, 185, 129, 0.05); color: #059669; border: 1px solid rgba(16, 185, 129, 0.2); }
QPushButton#successBtn:hover { background-color: rgba(16, 185, 129, 0.1); border-color: #059669; }

QPushButton#btnAddBlock { background-color: rgba(220, 38, 38, 0.05); color: #DC2626; border: 1px solid rgba(220, 38, 38, 0.2); }
QPushButton#btnAddBlock:hover { background-color: rgba(220, 38, 38, 0.1); border-color: #DC2626; }

QPushButton#btnAddAllow { background-color: rgba(16, 185, 129, 0.05); color: #059669; border: 1px solid rgba(16, 185, 129, 0.2); }
QPushButton#btnAddAllow:hover { background-color: rgba(16, 185, 129, 0.1); border-color: #059669; }

QPushButton#iconBtn { background: transparent; border: none; font-size: 12pt; color: #71717A; padding: 4px; }
QPushButton#iconBtn:hover { background-color: #E4E4E7; color: #18181B; border-radius: 4px; }

/* ==========================================================
   INPUTS & COMBOBOX
   ========================================================== */
QLineEdit, QComboBox, QSpinBox {
    background-color: #FFFFFF;
    border: 1px solid #D4D4D8;
    border-radius: 4px;
    padding: 5px 10px;
    color: #27272A;
    selection-background-color: #BFDBFE;
    selection-color: #1E3A8A;
}
QLineEdit:focus, QComboBox:focus { border: 1px solid #2563EB; }
QLineEdit::placeholder { color: #A1A1AA; }

QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
    background-color: #FFFFFF;
    border: 1px solid #E4E4E7;
    selection-background-color: #EFF6FF;
    selection-color: #1D4ED8;
    color: #27272A;
}

/* ==========================================================
   TABLES & TREES (FIXED COLORS)
   ========================================================== */
QTableView, QTreeView {
    background-color: #FFFFFF;
    alternate-background-color: #F8FAFC;
    border: 1px solid #E4E4E7;
    border-radius: 6px;
    gridline-color: transparent;
    color: #27272A; /* Black text for readability */
    selection-background-color: #DBEAFE; /* Very light blue for selection */
    selection-color: #1E3A8A; /* Dark blue for selected text readability */
}
QHeaderView::section {
    background-color: #F4F4F5;
    color: #52525B;
    border: none;
    border-bottom: 1px solid #D4D4D8;
    border-right: 1px solid #F4F4F5;
    padding: 6px 8px;
    font-weight: bold;
    font-size: 8.5pt;
}
QTableView::item, QTreeView::item {
    padding: 4px;
    border-bottom: 1px solid #F1F5F9;
}
QTableView::item:hover, QTreeView::item:hover {
    background-color: #F1F5F9;
    color: #27272A;
}

/* ==========================================================
   SCROLLBARS
   ========================================================== */
QScrollBar:vertical { background: transparent; width: 8px; margin: 0px; }
QScrollBar::handle:vertical { background-color: #D4D4D8; border-radius: 4px; min-height: 20px; }
QScrollBar::handle:vertical:hover { background-color: #A1A1AA; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }

QScrollBar:horizontal { background: transparent; height: 8px; margin: 0px; }
QScrollBar::handle:horizontal { background-color: #D4D4D8; border-radius: 4px; min-width: 20px; }
QScrollBar::handle:horizontal:hover { background-color: #A1A1AA; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }

/* ==========================================================
   MISCELLANEOUS (GROUPS, LABELS, PROGRESS BARS)
   ========================================================== */
QGroupBox {
    border: 1px solid #E4E4E7;
    border-radius: 6px;
    margin-top: 10px;
    padding-top: 14px;
    color: #52525B;
    font-weight: bold;
}
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; color: #2563EB; }

QCheckBox { spacing: 6px; }
QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; border: 1px solid #A1A1AA; background-color: #FFFFFF; }
QCheckBox::indicator:checked { background-color: #2563EB; border-color: #2563EB; }

QLabel#sectionHeading { font-size: 13pt; font-weight: bold; color: #18181B; }
QLabel#groupLabel { color: #52525B; font-weight: bold; }
QLabel#subDetail  { color: #71717A; }
QLabel#dropHint { color: #71717A; border: 2px dashed #D4D4D8; border-radius: 6px; font-weight: bold; background-color: #F8FAFC; }
QLabel#dropHint:hover { border-color: #2563EB; color: #2563EB; }

QProgressBar { background-color: #E4E4E7; border: none; border-radius: 3px; text-align: right; color: #27272A; font-weight: bold; }
QProgressBar::chunk { background-color: #2563EB; border-radius: 3px; }
)QSS";
}

void AppTheme::apply(QApplication *app, Mode m)
{
    app->setStyleSheet(m == Dark ? darkQss() : lightQss());
}

void AppTheme::toggle(QApplication *app, Mode &current)
{
    current = (current == Dark) ? Light : Dark;
    apply(app, current);
}
