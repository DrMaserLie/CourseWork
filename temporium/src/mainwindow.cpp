#include "mainwindow.h"
#include <QApplication>
#include <QStyle>
#include <QScreen>
#include <QFont>
#include <QInputDialog>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

namespace Temporium {

// Цвета темы
const QString DARK_BG = "#303030";
const QString DARK_LIGHTER = "#404040";
const QString DARK_BORDER = "#505050";
const QString ACCENT_COLOR = "#03fce8";
const QString ACCENT_DARKER = "#02d4c4";
const QString TEXT_COLOR = "#ffffff";
const QString TEXT_SECONDARY = "#b0b0b0";
const QString TEXT_PRIMARY  = "#ffffff";
const QString BORDER_COLOR  = "#444444";
static void setupSpinBox(QDoubleSpinBox* spinBox, double min, double max, double defaultVal = 0) {
    spinBox->setDecimals(1);
    spinBox->setRange(-99999, 99999);
    spinBox->setValue(defaultVal);
    
    QObject::connect(spinBox, &QDoubleSpinBox::editingFinished, [spinBox, min, max]() {
        double val = spinBox->value();
        if (val < min) spinBox->setValue(min);
        else if (val > max) spinBox->setValue(max);
    });
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , filterActive_(false)
    , lastClickedRow_(-1)
    , settings_("NSTU", "Temporium")
{
    setWindowTitle("Temporium - СУБД Компьютерные Игры");
    setMinimumSize(1200, 700);
    resize(1400, 800);
    
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    applyDarkTheme();
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupConnections();
    
    connectToDatabase();
    loadLastUsername();
    showLoginPage();
    
    statusBar()->showMessage("Добро пожаловать в Temporium!");
}

MainWindow::~MainWindow() {}

void MainWindow::applyDarkTheme() {
    QString styleSheet = QString(R"(
        QMainWindow, QWidget {
            background-color: %1;
            color: %2;
        }
        
        QMenuBar {
            background-color: %3;
            color: %2;
            border-bottom: 1px solid %4;
        }
        
        QMenuBar::item:selected {
            background-color: %5;
            color: #000000;
        }
        
        QMenu {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
        }
        
        QMenu::item:selected {
            background-color: %5;
            color: #000000;
        }
        
        QToolBar {
            background-color: %3;
            border: none;
            spacing: 5px;
            padding: 5px;
        }
        
        QPushButton {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 8px 16px;
            min-height: 20px;
        }
        
        QPushButton:hover {
            background-color: %5;
            border-color: %5;
            color: #000000;
        }
        
        QPushButton:pressed {
            background-color: %6;
            color: #000000;
        }
        
        QPushButton:disabled {
            background-color: %3;
            color: #606060;
        }
        
        QLineEdit, QComboBox {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 6px;
            selection-background-color: %5;
            selection-color: #000000;
        }
        
        QDoubleSpinBox, QSpinBox {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 6px;
            selection-background-color: %5;
            selection-color: #000000;
        }
        
        QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QSpinBox:focus {
            border-color: %5;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 20px;
            background-color: transparent;
        }
        
        QComboBox QAbstractItemView {
            background-color: %3;
            color: %2;
            selection-background-color: %5;
            selection-color: #000000;
        }
        
        QTableWidget {
            background-color: %3;
            color: %2;
            gridline-color: %4;
            border: 1px solid %4;
            border-radius: 4px;
            selection-background-color: %5;
            selection-color: #000000;
        }
        
        QTableWidget::item {
            padding: 5px;
            border-right: 1px solid %4;
        }
        
        QTableWidget::item:selected {
            background-color: %5;
            color: #000000;
        }
        
        QHeaderView::section {
            background-color: %1;
            color: %2;
            padding: 8px;
            border: none;
            border-right: 1px solid %4;
            border-bottom: 2px solid %5;
        }
        
        QGroupBox {
            color: %5;
            border: 1px solid %4;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 10px;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 5px;
            color: %5;
        }
        
        QCheckBox {
            color: %2;
            spacing: 8px;
        }
        
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 2px solid %4;
            border-radius: 3px;
            background-color: %3;
        }
        
        QCheckBox::indicator:hover {
            border-color: %5;
        }
        
        QCheckBox::indicator:checked {
            border: 4px solid %3;
            background-color: %5;
            outline: 2px solid %5;
        }
        
        QLabel {
            color: %2;
        }
        
        QStatusBar {
            background-color: %3;
            color: %7;
            border-top: 1px solid %4;
        }
        
        QScrollBar:vertical {
            background-color: %1;
            width: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:vertical {
            background-color: %4;
            border-radius: 6px;
            min-height: 20px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: %5;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        
        QScrollBar:horizontal {
            background-color: %1;
            height: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:horizontal {
            background-color: %4;
            border-radius: 6px;
            min-width: 20px;
        }
        
        QScrollBar::handle:horizontal:hover {
            background-color: %5;
        }
        
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        
        QDialog {
            background-color: %1;
        }
        
        QMessageBox {
            background-color: %1;
        }
        
        QMessageBox QLabel {
            color: %2;
        }
        
        QInputDialog {
            background-color: %1;
        }
    )").arg(DARK_BG, TEXT_COLOR, DARK_LIGHTER, DARK_BORDER, ACCENT_COLOR, ACCENT_DARKER, TEXT_SECONDARY);
    
    setStyleSheet(styleSheet);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Навигация стрелками на экране входа
        if (obj == usernameEdit_ || obj == passwordEdit_) {
            if (keyEvent->key() == Qt::Key_Down) {
                if (obj == usernameEdit_) {
                    passwordEdit_->setFocus();
                    return true;
                } else if (obj == passwordEdit_) {
                    loginButton_->setFocus();
                    return true;
                }
            } else if (keyEvent->key() == Qt::Key_Up) {
                if (obj == passwordEdit_) {
                    usernameEdit_->setFocus();
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::connectToDatabase() {
    QString host = qgetenv("DB_HOST");
    QString port = qgetenv("DB_PORT");
    QString dbname = qgetenv("DB_NAME");
    QString user = qgetenv("DB_USER");
    QString password = qgetenv("DB_PASSWORD");
    
    if (host.isEmpty()) host = "localhost";
    if (port.isEmpty()) port = "5432";
    if (dbname.isEmpty()) dbname = "gamedb";
    if (user.isEmpty()) user = "postgres";
    if (password.isEmpty()) password = "postgres";
    
    if (!dbManager_.connect(host.toStdString(), port.toInt(), 
                            dbname.toStdString(), user.toStdString(), 
                            password.toStdString())) {
        QMessageBox::critical(this, "Ошибка подключения",
            QString("Не удалось подключиться к базе данных:\n%1\n\n"
                    "Убедитесь, что PostgreSQL запущен:\n"
                    "./run.sh db-start")
                .arg(QString::fromStdString(dbManager_.getLastError())));
    }
}

void MainWindow::saveLastUsername() {
    if (rememberUserCheck_->isChecked()) {
        settings_.setValue("lastUsername", usernameEdit_->text());
        settings_.setValue("rememberUser", true);
    } else {
        settings_.remove("lastUsername");
        settings_.setValue("rememberUser", false);
    }
}

void MainWindow::loadLastUsername() {
    bool remember = settings_.value("rememberUser", false).toBool();
    rememberUserCheck_->setChecked(remember);
    if (remember) {
        usernameEdit_->setText(settings_.value("lastUsername", "").toString());
    }
}

void MainWindow::setupUI() {
    stackedWidget_ = new QStackedWidget(this);
    setCentralWidget(stackedWidget_);
    
    setupLoginPage();
    setupMainPage();
}

void MainWindow::setupLoginPage() {
    loginPage_ = new QWidget();
    
    QVBoxLayout* mainLayout = new QVBoxLayout(loginPage_);
    mainLayout->setAlignment(Qt::AlignCenter);
    
    // Песочные часы (Unicode) с цветом акцента
    QLabel* titleLabel = new QLabel("⏳ Temporium");
    QFont titleFont;
    titleFont.setPointSize(36);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString("color: %1;").arg(ACCENT_COLOR));
    
    QLabel* subtitleLabel = new QLabel("СУБД Компьютерные Игры");
    QFont subtitleFont;
    subtitleFont.setPointSize(14);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(QString("color: %1;").arg(TEXT_SECONDARY));
    
    QGroupBox* loginBox = new QGroupBox("Вход в систему");
    loginBox->setMinimumWidth(400);
    loginBox->setMaximumWidth(450);
    QFormLayout* formLayout = new QFormLayout(loginBox);
    formLayout->setSpacing(15);
    
    usernameEdit_ = new QLineEdit();
    usernameEdit_->setPlaceholderText("Введите имя пользователя");
    usernameEdit_->setMinimumHeight(35);
    usernameEdit_->installEventFilter(this);
    
    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("Введите пароль");
    passwordEdit_->setMinimumHeight(35);
    passwordEdit_->installEventFilter(this);
    
    rememberUserCheck_ = new QCheckBox("Запомнить имя пользователя");
    
    formLayout->addRow("Пользователь:", usernameEdit_);
    formLayout->addRow("Пароль:", passwordEdit_);
    formLayout->addRow("", rememberUserCheck_);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);
    
    loginButton_ = new QPushButton("Войти");
    loginButton_->setMinimumHeight(40);
    loginButton_->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: #000000; font-weight: bold; }"
        "QPushButton:hover { background-color: %2; }"
    ).arg(ACCENT_COLOR, ACCENT_DARKER));
    
    registerButton_ = new QPushButton("Регистрация");
    registerButton_->setMinimumHeight(40);
    
    buttonLayout->addWidget(loginButton_);
    buttonLayout->addWidget(registerButton_);
    
    mainLayout->addStretch();
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addSpacing(40);
    mainLayout->addWidget(loginBox, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
    
    stackedWidget_->addWidget(loginPage_);
}

void MainWindow::setupMainPage() {
    mainPage_ = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(mainPage_);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Левая панель с фильтрами
    QWidget* leftPanel = new QWidget();
    leftPanel->setMinimumWidth(280);
    leftPanel->setMaximumWidth(320);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Информация о пользователе
    userInfoLabel_ = new QLabel();
    userInfoLabel_->setStyleSheet(QString(
        "QLabel { background-color: %1; color: %2; padding: 10px; border-radius: 4px; font-weight: bold; }"
    ).arg(DARK_LIGHTER, ACCENT_COLOR));
    userInfoLabel_->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(userInfoLabel_);
    
    filterGroupBox_ = new QGroupBox("Фильтры");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterGroupBox_);
    filterLayout->setSpacing(8);
    
    // Фильтр по статусу
    QHBoxLayout* completedLayout = new QHBoxLayout();
    filterCompletedCheck_ = new QCheckBox("Статус:");
    filterCompletedCombo_ = new QComboBox();
    filterCompletedCombo_->addItem("Пройденные", true);
    filterCompletedCombo_->addItem("Не пройденные", false);
    filterCompletedCombo_->setEnabled(false);
    completedLayout->addWidget(filterCompletedCheck_);
    completedLayout->addWidget(filterCompletedCombo_, 1);
    filterLayout->addLayout(completedLayout);
    
    // Фильтр по жанру
    QHBoxLayout* genreLayout = new QHBoxLayout();
    filterGenreCheck_ = new QCheckBox("Жанр:");
    filterGenreCombo_ = new QComboBox();
    for (const auto& genre : GENRES) {
        filterGenreCombo_->addItem(QString::fromStdString(genre));
    }
    filterGenreCombo_->setEnabled(false);
    genreLayout->addWidget(filterGenreCheck_);
    genreLayout->addWidget(filterGenreCombo_, 1);
    filterLayout->addLayout(genreLayout);
    
    // Фильтр по месту на диске
    filterLayout->addWidget(new QLabel("Место на диске (ГБ):"));
    QHBoxLayout* diskMinLayout = new QHBoxLayout();
    filterDiskMinCheck_ = new QCheckBox("Мин:");
    filterDiskMinSpin_ = new QDoubleSpinBox();
    setupSpinBox(filterDiskMinSpin_, 0, MAX_DISK_SPACE, 0);
    filterDiskMinSpin_->setEnabled(false);
    diskMinLayout->addWidget(filterDiskMinCheck_);
    diskMinLayout->addWidget(filterDiskMinSpin_, 1);
    filterLayout->addLayout(diskMinLayout);
    
    QHBoxLayout* diskMaxLayout = new QHBoxLayout();
    filterDiskMaxCheck_ = new QCheckBox("Макс:");
    filterDiskMaxSpin_ = new QDoubleSpinBox();
    setupSpinBox(filterDiskMaxSpin_, 0, MAX_DISK_SPACE, MAX_DISK_SPACE);
    filterDiskMaxSpin_->setEnabled(false);
    diskMaxLayout->addWidget(filterDiskMaxCheck_);
    diskMaxLayout->addWidget(filterDiskMaxSpin_, 1);
    filterLayout->addLayout(diskMaxLayout);
    
    // Фильтр по RAM
    filterLayout->addWidget(new QLabel("ОЗУ (ГБ):"));
    QHBoxLayout* ramMinLayout = new QHBoxLayout();
    filterRamMinCheck_ = new QCheckBox("Мин:");
    filterRamMinSpin_ = new QDoubleSpinBox();
    setupSpinBox(filterRamMinSpin_, 0, MAX_RAM_USAGE, 0);
    filterRamMinSpin_->setEnabled(false);
    ramMinLayout->addWidget(filterRamMinCheck_);
    ramMinLayout->addWidget(filterRamMinSpin_, 1);
    filterLayout->addLayout(ramMinLayout);
    
    QHBoxLayout* ramMaxLayout = new QHBoxLayout();
    filterRamMaxCheck_ = new QCheckBox("Макс:");
    filterRamMaxSpin_ = new QDoubleSpinBox();
    setupSpinBox(filterRamMaxSpin_, 0, MAX_RAM_USAGE, MAX_RAM_USAGE);
    filterRamMaxSpin_->setEnabled(false);
    ramMaxLayout->addWidget(filterRamMaxCheck_);
    ramMaxLayout->addWidget(filterRamMaxSpin_, 1);
    filterLayout->addLayout(ramMaxLayout);
    
    // Фильтр по VRAM
    filterLayout->addWidget(new QLabel("Видеопамять (ГБ):"));
    QHBoxLayout* vramMinLayout = new QHBoxLayout();
    filterVramMinCheck_ = new QCheckBox("Мин:");
    filterVramMinSpin_ = new QDoubleSpinBox();
    setupSpinBox(filterVramMinSpin_, 0, MAX_VRAM_REQUIRED, 0);
    filterVramMinSpin_->setEnabled(false);
    vramMinLayout->addWidget(filterVramMinCheck_);
    vramMinLayout->addWidget(filterVramMinSpin_, 1);
    filterLayout->addLayout(vramMinLayout);
    
    QHBoxLayout* vramMaxLayout = new QHBoxLayout();
    filterVramMaxCheck_ = new QCheckBox("Макс:");
    filterVramMaxSpin_ = new QDoubleSpinBox();
    setupSpinBox(filterVramMaxSpin_, 0, MAX_VRAM_REQUIRED, MAX_VRAM_REQUIRED);
    filterVramMaxSpin_->setEnabled(false);
    vramMaxLayout->addWidget(filterVramMaxCheck_);
    vramMaxLayout->addWidget(filterVramMaxSpin_, 1);
    filterLayout->addLayout(vramMaxLayout);
    
    // Фильтр по тегам
    filterLayout->addWidget(new QLabel(""));
    QHBoxLayout* tagLayout = new QHBoxLayout();
    filterTagCheck_ = new QCheckBox("Тег:");
    filterTagCombo_ = new QComboBox();
    filterTagCombo_->addItem("Все теги");
    filterTagCombo_->setEnabled(false);
    tagLayout->addWidget(filterTagCheck_);
    tagLayout->addWidget(filterTagCombo_, 1);
    filterLayout->addLayout(tagLayout);
    
    // Фильтр по избранному
    QHBoxLayout* favoriteLayout = new QHBoxLayout();
    filterFavoriteCheck_ = new QCheckBox("Избранное:");
    filterFavoriteCombo_ = new QComboBox();
    filterFavoriteCombo_->addItem("Только избранное", true);
    filterFavoriteCombo_->addItem("Не избранное", false);
    filterFavoriteCombo_->setEnabled(false);
    favoriteLayout->addWidget(filterFavoriteCheck_);
    favoriteLayout->addWidget(filterFavoriteCombo_, 1);
    filterLayout->addLayout(favoriteLayout);
    
    // Фильтр по установленным
    QHBoxLayout* installedLayout = new QHBoxLayout();
    filterInstalledCheck_ = new QCheckBox("Установлено:");
    filterInstalledCombo_ = new QComboBox();
    filterInstalledCombo_->addItem("Только установленные", true);
    filterInstalledCombo_->addItem("Не установленные", false);
    filterInstalledCombo_->setEnabled(false);
    installedLayout->addWidget(filterInstalledCheck_);
    installedLayout->addWidget(filterInstalledCombo_, 1);
    filterLayout->addLayout(installedLayout);
    
    // Фильтр по оценке
    QHBoxLayout* ratingLayout = new QHBoxLayout();
    filterRatingCheck_ = new QCheckBox("Оценка:");
    filterRatingModeCombo_ = new QComboBox();
    filterRatingModeCombo_->addItem("не меньше", 0);
    filterRatingModeCombo_->addItem("не больше", 1);
    filterRatingModeCombo_->setEnabled(false);
    filterRatingSpin_ = new QSpinBox();
    filterRatingSpin_->setRange(0, 10);
    filterRatingSpin_->setValue(5);
    filterRatingSpin_->setEnabled(false);
    ratingLayout->addWidget(filterRatingCheck_);
    ratingLayout->addWidget(filterRatingModeCombo_);
    ratingLayout->addWidget(filterRatingSpin_);
    filterLayout->addLayout(ratingLayout);
    
    QHBoxLayout* filterButtonLayout = new QHBoxLayout();
    applyFilterButton_ = new QPushButton("Применить");
    resetFilterButton_ = new QPushButton("Сбросить");
    filterButtonLayout->addWidget(applyFilterButton_);
    filterButtonLayout->addWidget(resetFilterButton_);
    filterLayout->addLayout(filterButtonLayout);
    
    leftLayout->addWidget(filterGroupBox_);
    leftLayout->addStretch();
    
    // Правая панель с таблицей
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    gamesTable_ = new QTableWidget();
    gamesTable_->setColumnCount(12);
    gamesTable_->setHorizontalHeaderLabels({
        "ID", "Название", "Диск (ГБ)", "ОЗУ (ГБ)", "VRAM (ГБ)", "Жанр", "Пройдено", "Оценка", "★", "📥", "Теги", "Ссылка"
    });
    gamesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    gamesTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    gamesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    gamesTable_->setShowGrid(true);
    
    // Настройка заголовка для возможности изменения ширины столбцов
    gamesTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    gamesTable_->horizontalHeader()->setStretchLastSection(true);
    gamesTable_->horizontalHeader()->setMinimumSectionSize(40);
    
    // Установка начальных размеров столбцов
    resetTableColumnWidths();
    
    gamesTable_->verticalHeader()->setVisible(false);
    gamesTable_->setAlternatingRowColors(true);
    gamesTable_->setStyleSheet(QString(
        "QTableWidget { alternate-background-color: %1; }"
    ).arg(DARK_LIGHTER));
    
    QHBoxLayout* controlLayout = new QHBoxLayout();
    addButton_ = new QPushButton("➕ Добавить");
    editButton_ = new QPushButton("✏️ Редактировать");
    deleteButton_ = new QPushButton("🗑️ Удалить");
    notesButton_ = new QPushButton("📝 Заметки");
    refreshButton_ = new QPushButton("🔄 Обновить");
    
    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);
    notesButton_->setEnabled(false);
    notesButton_->setCheckable(true);  // Кнопка-переключатель
    
    controlLayout->addWidget(addButton_);
    controlLayout->addWidget(editButton_);
    controlLayout->addWidget(deleteButton_);
    controlLayout->addWidget(notesButton_);
    controlLayout->addStretch();
    controlLayout->addWidget(refreshButton_);
    
    // Панель заметок (раскрывающаяся)
    notesPanel_ = new QGroupBox("📝 Заметки");
    notesPanel_->setVisible(false);
    notesPanel_->setStyleSheet(QString(
        "QGroupBox { "
        "  background-color: %1; "
        "  border: 1px solid %2; "
        "  border-radius: 6px; "
        "  margin-top: 10px; "
        "  padding: 10px; "
        "} "
        "QGroupBox::title { "
        "  subcontrol-origin: margin; "
        "  left: 10px; "
        "  padding: 0 5px; "
        "  color: %3; "
        "}"
    ).arg(DARK_LIGHTER, BORDER_COLOR, ACCENT_COLOR));
    
    QVBoxLayout* notesPanelLayout = new QVBoxLayout(notesPanel_);
    
    notesPanelTitle_ = new QLabel("Выберите игру");
    notesPanelTitle_->setStyleSheet(QString("color: %1; font-weight: bold;").arg(TEXT_PRIMARY));
    
    notesPanelEdit_ = new QTextEdit();
    notesPanelEdit_->setPlaceholderText("Здесь можно записать заметки об игре...");
    notesPanelEdit_->setMinimumHeight(80);
    notesPanelEdit_->setMaximumHeight(120);
    notesPanelEdit_->setStyleSheet(QString(
        "QTextEdit { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: 4px; "
        "  padding: 5px; "
        "}"
    ).arg(DARK_BG, TEXT_PRIMARY, BORDER_COLOR));
    
    saveNotesButton_ = new QPushButton("💾 Сохранить заметки");
    saveNotesButton_->setStyleSheet(QString(
        "QPushButton { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: none; "
        "  padding: 8px 16px; "
        "  border-radius: 4px; "
        "} "
        "QPushButton:hover { background-color: #029a8a; }"
    ).arg(ACCENT_COLOR, DARK_BG));
    
    notesPanelLayout->addWidget(notesPanelTitle_);
    notesPanelLayout->addWidget(notesPanelEdit_);
    
    QHBoxLayout* notesButtonLayout = new QHBoxLayout();
    notesButtonLayout->addStretch();
    notesButtonLayout->addWidget(saveNotesButton_);
    notesPanelLayout->addLayout(notesButtonLayout);
    
    currentNotesGameId_ = -1;
    
    // Статистика внизу
    statsLabel_ = new QLabel();
    statsLabel_->setStyleSheet(QString(
        "QLabel { background-color: %1; color: %2; padding: 8px; border-radius: 4px; }"
    ).arg(DARK_LIGHTER, TEXT_SECONDARY));
    statsLabel_->setAlignment(Qt::AlignCenter);
    
    rightLayout->addWidget(gamesTable_);
    rightLayout->addLayout(controlLayout);
    rightLayout->addWidget(notesPanel_);
    rightLayout->addWidget(statsLabel_);
    
    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(rightPanel, 1);
    
    stackedWidget_->addWidget(mainPage_);
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    
    QMenu* fileMenu = menuBar->addMenu("Файл");
    
    loginAction_ = fileMenu->addAction("Войти");
    logoutAction_ = fileMenu->addAction("Выйти");
    logoutAction_->setEnabled(false);
    fileMenu->addSeparator();
    exitAction_ = fileMenu->addAction("Выход");
    exitAction_->setShortcut(QKeySequence::Quit);
    
    QMenu* gamesMenu = menuBar->addMenu("Игры");
    
    addAction_ = gamesMenu->addAction("Добавить игру");
    addAction_->setShortcut(QKeySequence::New);
    editAction_ = gamesMenu->addAction("Редактировать игру");
    deleteAction_ = gamesMenu->addAction("Удалить игру");
    deleteAction_->setShortcut(QKeySequence::Delete);
    
    QMenu* dataMenu = menuBar->addMenu("Данные");
    
    exportAction_ = dataMenu->addAction("Экспорт в файл...");
    exportFilteredAction_ = dataMenu->addAction("Экспорт с фильтром...");
    importAction_ = dataMenu->addAction("Импорт из файла...");
    dataMenu->addSeparator();
    viewExportedAction_ = dataMenu->addAction("Просмотр экспортированного файла...");
    
    // Меню администратора - будет показано/скрыто в зависимости от прав
    adminMenu_ = menuBar->addMenu("Администрирование");
    adminAction_ = adminMenu_->addAction("Панель администратора");
    adminMenu_->menuAction()->setVisible(false);  // Изначально скрыто
    
    QMenu* helpMenu = menuBar->addMenu("Справка");
    aboutAction_ = helpMenu->addAction("О программе");
}

void MainWindow::setupToolBar() {
    QToolBar* toolBar = addToolBar("Панель инструментов");
    toolBar->setMovable(false);
    toolBar->addAction(addAction_);
    toolBar->addAction(editAction_);
    toolBar->addAction(deleteAction_);
    toolBar->addSeparator();
    toolBar->addAction(exportAction_);
    toolBar->addAction(importAction_);
}

void MainWindow::setupConnections() {
    connect(filterCompletedCheck_, &QCheckBox::toggled, filterCompletedCombo_, &QComboBox::setEnabled);
    connect(filterGenreCheck_, &QCheckBox::toggled, filterGenreCombo_, &QComboBox::setEnabled);
    connect(filterDiskMinCheck_, &QCheckBox::toggled, filterDiskMinSpin_, &QDoubleSpinBox::setEnabled);
    connect(filterDiskMaxCheck_, &QCheckBox::toggled, filterDiskMaxSpin_, &QDoubleSpinBox::setEnabled);
    connect(filterRamMinCheck_, &QCheckBox::toggled, filterRamMinSpin_, &QDoubleSpinBox::setEnabled);
    connect(filterRamMaxCheck_, &QCheckBox::toggled, filterRamMaxSpin_, &QDoubleSpinBox::setEnabled);
    connect(filterVramMinCheck_, &QCheckBox::toggled, filterVramMinSpin_, &QDoubleSpinBox::setEnabled);
    connect(filterVramMaxCheck_, &QCheckBox::toggled, filterVramMaxSpin_, &QDoubleSpinBox::setEnabled);
    
    // Новые фильтры
    connect(filterTagCheck_, &QCheckBox::toggled, filterTagCombo_, &QComboBox::setEnabled);
    connect(filterFavoriteCheck_, &QCheckBox::toggled, filterFavoriteCombo_, &QComboBox::setEnabled);
    connect(filterInstalledCheck_, &QCheckBox::toggled, filterInstalledCombo_, &QComboBox::setEnabled);
    connect(filterRatingCheck_, &QCheckBox::toggled, filterRatingModeCombo_, &QComboBox::setEnabled);
    connect(filterRatingCheck_, &QCheckBox::toggled, filterRatingSpin_, &QSpinBox::setEnabled);
    
    connect(loginButton_, &QPushButton::clicked, this, &MainWindow::onLogin);
    connect(registerButton_, &QPushButton::clicked, this, &MainWindow::onRegister);
    connect(passwordEdit_, &QLineEdit::returnPressed, this, &MainWindow::onLogin);
    connect(usernameEdit_, &QLineEdit::returnPressed, [this]() { passwordEdit_->setFocus(); });
    
    connect(addButton_, &QPushButton::clicked, this, &MainWindow::onAddGame);
    connect(editButton_, &QPushButton::clicked, this, &MainWindow::onEditGame);
    connect(deleteButton_, &QPushButton::clicked, this, &MainWindow::onDeleteGame);
    connect(refreshButton_, &QPushButton::clicked, this, &MainWindow::onRefreshGames);
    connect(notesButton_, &QPushButton::clicked, this, &MainWindow::onToggleNotesPanel);
    connect(saveNotesButton_, &QPushButton::clicked, this, &MainWindow::onSaveNotes);
    
    connect(applyFilterButton_, &QPushButton::clicked, this, &MainWindow::onApplyFilter);
    connect(resetFilterButton_, &QPushButton::clicked, this, &MainWindow::onResetFilter);
    
    connect(loginAction_, &QAction::triggered, this, &MainWindow::showLoginPage);
    connect(logoutAction_, &QAction::triggered, this, &MainWindow::onLogout);
    connect(exitAction_, &QAction::triggered, this, &QWidget::close);
    connect(addAction_, &QAction::triggered, this, &MainWindow::onAddGame);
    connect(editAction_, &QAction::triggered, this, &MainWindow::onEditGame);
    connect(deleteAction_, &QAction::triggered, this, &MainWindow::onDeleteGame);
    connect(exportAction_, &QAction::triggered, this, &MainWindow::onExportToFile);
    connect(exportFilteredAction_, &QAction::triggered, this, &MainWindow::onExportFilteredToFile);
    connect(importAction_, &QAction::triggered, this, &MainWindow::onImportFromFile);
    connect(viewExportedAction_, &QAction::triggered, this, &MainWindow::onViewExportedFile);
    connect(aboutAction_, &QAction::triggered, this, &MainWindow::onAbout);
    connect(adminAction_, &QAction::triggered, this, &MainWindow::onAdminPanel);
    
    connect(gamesTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::onTableSelectionChanged);
    connect(gamesTable_, &QTableWidget::cellClicked, this, &MainWindow::onTableCellClicked);
    connect(gamesTable_, &QTableWidget::cellDoubleClicked, this, &MainWindow::onTableCellDoubleClicked);
}

void MainWindow::onTableCellClicked(int row, int column) {
    // Если клик на колонке "Ссылка" (тк индекс 11) - открыть URL
    if (column == 11) {
        QTableWidgetItem* item = gamesTable_->item(row, 11);
        if (item) {
            QString url = item->data(Qt::UserRole).toString();
            if (!url.isEmpty()) {
                // Добавляем схему если отсутствует
                if (!url.startsWith("http://") && !url.startsWith("https://")) {
                    url = "https://" + url;
                }
                QDesktopServices::openUrl(QUrl(url));
                // Не меняем выделение при клике на ссылку
                return;
            }
        }
    }
    
    // Toggle selection: повторный клик снимает выделение
    if (row == lastClickedRow_ && gamesTable_->selectionModel()->isRowSelected(row, QModelIndex())) {
        gamesTable_->clearSelection();
        lastClickedRow_ = -1;
    } else {
        lastClickedRow_ = row;
    }
    
    updateButtonStates();
}

void MainWindow::onTableCellDoubleClicked(int row, int column) {
    // Если двойной клик на колонке "Ссылка" - открыть URL
    if (column == 11) {
        QTableWidgetItem* item = gamesTable_->item(row, column);
        if (item && !item->text().isEmpty()) {
            QString url = item->data(Qt::UserRole).toString();
            if (!url.isEmpty()) {
                QDesktopServices::openUrl(QUrl(url));
                return;
            }
        }
    }
    // Иначе - редактирование
    onEditGame();
}

void MainWindow::onToggleNotesPanel() {
    int row = gamesTable_->currentRow();
    if (row < 0) {
        notesPanel_->setVisible(false);
        notesButton_->setChecked(false);
        return;
    }
    
    bool showPanel = notesButton_->isChecked();
    notesPanel_->setVisible(showPanel);
    
    if (showPanel) {
        // Загружаем заметки для выбранной игры
        QTableWidgetItem* idItem = gamesTable_->item(row, 0);
        if (idItem) {
            int gameId = idItem->text().toInt();
            QString gameName = gamesTable_->item(row, 1)->text();
            QString notes = idItem->data(Qt::UserRole + 1).toString();
            
            currentNotesGameId_ = gameId;
            notesPanelTitle_->setText(QString("📝 Заметки: %1").arg(gameName));
            notesPanelEdit_->setPlainText(notes);
            notesPanelEdit_->setFocus();
        }
    } else {
        currentNotesGameId_ = -1;
    }
}

void MainWindow::onSaveNotes() {
    if (currentNotesGameId_ <= 0) {
        QMessageBox::warning(this, "Ошибка", "Не выбрана игра для сохранения заметок.");
        return;
    }
    
    QString notes = notesPanelEdit_->toPlainText();
    
    if (dbManager_.updateGameNotes(currentNotesGameId_, currentUser_.id, notes.toStdString())) {
        // Обновляем данные в таблице
        int row = gamesTable_->currentRow();
        if (row >= 0) {
            QTableWidgetItem* idItem = gamesTable_->item(row, 0);
            if (idItem) {
                idItem->setData(Qt::UserRole + 1, notes);
            }
        }
        
        statusBar()->showMessage("Заметки сохранены", 3000);
    } else {
        QMessageBox::critical(this, "Ошибка", 
            QString("Не удалось сохранить заметки:\n%1")
                .arg(QString::fromStdString(dbManager_.getLastError())));
    }
}

void MainWindow::updateButtonStates() {
    bool hasSelection = gamesTable_->currentRow() >= 0 && 
                        gamesTable_->selectionModel()->hasSelection();
    editButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
    notesButton_->setEnabled(hasSelection);
    editAction_->setEnabled(hasSelection);
    deleteAction_->setEnabled(hasSelection);
    
    // Если выбор снят, закрыть панель заметок
    if (!hasSelection && notesPanel_->isVisible()) {
        notesPanel_->setVisible(false);
        notesButton_->setChecked(false);
        currentNotesGameId_ = -1;
    }
}

void MainWindow::showLoginPage() {
    stackedWidget_->setCurrentWidget(loginPage_);
    loginAction_->setEnabled(true);
    logoutAction_->setEnabled(false);
    addAction_->setEnabled(false);
    editAction_->setEnabled(false);
    deleteAction_->setEnabled(false);
    exportAction_->setEnabled(false);
    exportFilteredAction_->setEnabled(false);
    importAction_->setEnabled(false);
    viewExportedAction_->setEnabled(false);
    adminMenu_->menuAction()->setVisible(false);
    
    passwordEdit_->clear();
    if (!rememberUserCheck_->isChecked()) {
        usernameEdit_->clear();
    }
    usernameEdit_->setFocus();
}

void MainWindow::showMainPage() {
    stackedWidget_->setCurrentWidget(mainPage_);
    loginAction_->setEnabled(false);
    logoutAction_->setEnabled(true);
    addAction_->setEnabled(true);
    exportAction_->setEnabled(true);
    exportFilteredAction_->setEnabled(true);
    importAction_->setEnabled(true);
    viewExportedAction_->setEnabled(true);
    
    // Показываем меню администратора только для админов
    adminMenu_->menuAction()->setVisible(currentUser_.is_admin);
    
    // Обновляем информацию о пользователе
    QString userType = currentUser_.is_admin ? "👑 Администратор" : "👤 Пользователь";
    userInfoLabel_->setText(QString("%1: %2").arg(userType, QString::fromStdString(currentUser_.username)));
    
    lastClickedRow_ = -1;
    resetTableColumnWidths();
    updateTagsCombo();
    updateGamesTable();
    updateStatusBar();
    updateStats();
}

void MainWindow::onLogin() {
    QString username = usernameEdit_->text().trimmed();
    QString password = passwordEdit_->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя пользователя и пароль!");
        return;
    }
    
    if (!dbManager_.isConnected()) {
        connectToDatabase();
        if (!dbManager_.isConnected()) {
            return;
        }
    }
    
    std::string passwordHash = HashUtils::hashPassword(password.toStdString(), username.toStdString());
    currentUser_ = dbManager_.authenticateUser(username.toStdString(), passwordHash);
    
    if (currentUser_.id > 0) {
        saveLastUsername();
        showMainPage();
        QString msg = currentUser_.is_admin ? 
            QString("Добро пожаловать, администратор %1!").arg(username) :
            QString("Добро пожаловать, %1!").arg(username);
        statusBar()->showMessage(msg);
    } else {
        QMessageBox::warning(this, "Ошибка входа", 
            "Неверное имя пользователя или пароль!");
        passwordEdit_->clear();
        passwordEdit_->setFocus();
    }
}

void MainWindow::onRegister() {
    QString username = usernameEdit_->text().trimmed();
    QString password = passwordEdit_->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя пользователя и пароль!");
        return;
    }
    
    if (username.length() < 3) {
        QMessageBox::warning(this, "Ошибка", "Имя пользователя должно содержать минимум 3 символа!");
        return;
    }
    
    if (password.length() < 4) {
        QMessageBox::warning(this, "Ошибка", "Пароль должен содержать минимум 4 символа!");
        return;
    }
    
    if (!dbManager_.isConnected()) {
        connectToDatabase();
        if (!dbManager_.isConnected()) {
            return;
        }
    }
    
    if (dbManager_.userExists(username.toStdString())) {
        QMessageBox::warning(this, "Ошибка", "Пользователь с таким именем уже существует!");
        return;
    }
    
    std::string passwordHash = HashUtils::hashPassword(password.toStdString(), username.toStdString());
    
    if (dbManager_.registerUser(username.toStdString(), passwordHash)) {
        QMessageBox::information(this, "Успех", 
            "Регистрация успешна! Теперь вы можете войти в систему.");
    } else {
        QMessageBox::critical(this, "Ошибка", 
            QString("Ошибка регистрации: %1").arg(QString::fromStdString(dbManager_.getLastError())));
    }
}

void MainWindow::onLogout() {
    currentUser_ = User();
    filterActive_ = false;
    currentFilter_.reset();
    lastClickedRow_ = -1;
    
    // Закрываем панель заметок
    notesPanel_->setVisible(false);
    notesButton_->setChecked(false);
    currentNotesGameId_ = -1;
    
    showLoginPage();
    statusBar()->showMessage("Вы вышли из системы");
}

void MainWindow::onAddGame() {
    GameEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Game game = dialog.getGame();
        game.user_id = currentUser_.id;
        
        if (dbManager_.addGame(game)) {
            updateTagsCombo();
            updateGamesTable();
            updateStats();
            statusBar()->showMessage("Игра добавлена");
        } else {
            QMessageBox::critical(this, "Ошибка", 
                QString("Не удалось добавить игру: %1")
                    .arg(QString::fromStdString(dbManager_.getLastError())));
        }
    }
}

void MainWindow::onEditGame() {
    int currentRow = gamesTable_->currentRow();
    if (currentRow < 0 || !gamesTable_->selectionModel()->hasSelection()) {
        QMessageBox::warning(this, "Внимание", "Выберите игру для редактирования!");
        return;
    }
    
    int gameId = gamesTable_->item(currentRow, 0)->text().toInt();
    Game game = dbManager_.getGameById(gameId, currentUser_.id);
    
    if (game.id == 0) {
        QMessageBox::warning(this, "Ошибка", "Игра не найдена!");
        return;
    }
    
    GameEditDialog dialog(this, &game);
    if (dialog.exec() == QDialog::Accepted) {
        Game updatedGame = dialog.getGame();
        updatedGame.id = game.id;
        updatedGame.user_id = currentUser_.id;
        
        if (dbManager_.updateGame(updatedGame)) {
            updateTagsCombo();
            updateGamesTable();
            updateStats();
            statusBar()->showMessage("Игра обновлена");
        } else {
            QMessageBox::critical(this, "Ошибка", 
                QString("Не удалось обновить игру: %1")
                    .arg(QString::fromStdString(dbManager_.getLastError())));
        }
    }
}

void MainWindow::onDeleteGame() {
    int currentRow = gamesTable_->currentRow();
    if (currentRow < 0 || !gamesTable_->selectionModel()->hasSelection()) {
        QMessageBox::warning(this, "Внимание", "Выберите игру для удаления!");
        return;
    }
    
    QString gameName = gamesTable_->item(currentRow, 1)->text();
    int gameId = gamesTable_->item(currentRow, 0)->text().toInt();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение",
        QString("Вы уверены, что хотите удалить игру \"%1\"?").arg(gameName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (dbManager_.deleteGame(gameId, currentUser_.id)) {
            lastClickedRow_ = -1;
            updateTagsCombo();
            updateGamesTable();
            updateStats();
            statusBar()->showMessage(QString("Игра \"%1\" удалена").arg(gameName));
        } else {
            QMessageBox::critical(this, "Ошибка", 
                QString("Не удалось удалить игру: %1")
                    .arg(QString::fromStdString(dbManager_.getLastError())));
        }
    }
}

void MainWindow::onRefreshGames() {
    resetTableColumnWidths();
    updateTagsCombo();
    updateGamesTable();
    updateStats();
    statusBar()->showMessage("Данные обновлены, настройки отображения сброшены");
}

void MainWindow::onApplyFilter() {
    currentFilter_.reset();
    
    if (filterCompletedCheck_->isChecked()) {
        currentFilter_.filter_completed = true;
        currentFilter_.completed_value = filterCompletedCombo_->currentData().toBool();
    }
    
    if (filterGenreCheck_->isChecked()) {
        currentFilter_.filter_genre = true;
        currentFilter_.genre_value = filterGenreCombo_->currentText().toStdString();
    }
    
    if (filterDiskMinCheck_->isChecked()) {
        currentFilter_.filter_disk_space_min = true;
        currentFilter_.disk_space_min = filterDiskMinSpin_->value();
    }
    
    if (filterDiskMaxCheck_->isChecked()) {
        currentFilter_.filter_disk_space_max = true;
        currentFilter_.disk_space_max = filterDiskMaxSpin_->value();
    }
    
    if (filterRamMinCheck_->isChecked()) {
        currentFilter_.filter_ram_min = true;
        currentFilter_.ram_min = filterRamMinSpin_->value();
    }
    
    if (filterRamMaxCheck_->isChecked()) {
        currentFilter_.filter_ram_max = true;
        currentFilter_.ram_max = filterRamMaxSpin_->value();
    }
    
    if (filterVramMinCheck_->isChecked()) {
        currentFilter_.filter_vram_min = true;
        currentFilter_.vram_min = filterVramMinSpin_->value();
    }
    
    if (filterVramMaxCheck_->isChecked()) {
        currentFilter_.filter_vram_max = true;
        currentFilter_.vram_max = filterVramMaxSpin_->value();
    }
    
    // Новые фильтры
    if (filterTagCheck_->isChecked() && filterTagCombo_->currentIndex() > 0) {
        currentFilter_.filter_tag = true;
        currentFilter_.tag_value = filterTagCombo_->currentData().toString().toStdString();
    }
    
    if (filterFavoriteCheck_->isChecked()) {
        currentFilter_.filter_favorite = true;
        currentFilter_.favorite_value = filterFavoriteCombo_->currentData().toBool();
    }
    
    if (filterInstalledCheck_->isChecked()) {
        currentFilter_.filter_installed = true;
        currentFilter_.installed_value = filterInstalledCombo_->currentData().toBool();
    }
    
    if (filterRatingCheck_->isChecked()) {
        int ratingValue = filterRatingSpin_->value();
        int mode = filterRatingModeCombo_->currentData().toInt();
        if (mode == 0) {
            // "не меньше" - filter_rating_min
            currentFilter_.filter_rating_min = true;
            currentFilter_.rating_min = ratingValue;
        } else {
            // "не больше" - filter_rating_max
            currentFilter_.filter_rating_max = true;
            currentFilter_.rating_max = ratingValue;
        }
    }
    
    filterActive_ = true;
    lastClickedRow_ = -1;
    updateGamesTable();
    statusBar()->showMessage("Фильтр применен");
}

void MainWindow::onResetFilter() {
    filterCompletedCheck_->setChecked(false);
    filterGenreCheck_->setChecked(false);
    filterDiskMinCheck_->setChecked(false);
    filterDiskMaxCheck_->setChecked(false);
    filterRamMinCheck_->setChecked(false);
    filterRamMaxCheck_->setChecked(false);
    filterVramMinCheck_->setChecked(false);
    filterVramMaxCheck_->setChecked(false);
    
    // Новые фильтры
    filterTagCheck_->setChecked(false);
    filterFavoriteCheck_->setChecked(false);
    filterInstalledCheck_->setChecked(false);
    filterRatingCheck_->setChecked(false);
    filterTagCombo_->setCurrentIndex(0);
    filterRatingModeCombo_->setCurrentIndex(0);
    filterRatingSpin_->setValue(5);
    
    currentFilter_.reset();
    filterActive_ = false;
    lastClickedRow_ = -1;
    updateGamesTable();
    statusBar()->showMessage("Фильтр сброшен");
}

void MainWindow::onExportToFile() {
    QString filename = QFileDialog::getSaveFileName(this, "Экспорт в файл",
        QDir::homePath() + "/games_export.bin", "Бинарные файлы (*.bin)");
    
    if (filename.isEmpty()) return;
    
    if (dbManager_.exportToBinaryFile(filename.toStdString(), currentUser_.id)) {
        lastExportedFile_ = filename;
        QMessageBox::information(this, "Успех", 
            "Данные успешно экспортированы!\n\nФайл защищен контрольной суммой SHA-256.");
    } else {
        QMessageBox::critical(this, "Ошибка", 
            QString("Ошибка экспорта: %1").arg(QString::fromStdString(dbManager_.getLastError())));
    }
}

void MainWindow::onExportFilteredToFile() {
    if (!filterActive_) {
        QMessageBox::information(this, "Информация", 
            "Сначала примените фильтр для экспорта отфильтрованных данных.");
        return;
    }
    
    QString filename = QFileDialog::getSaveFileName(this, "Экспорт отфильтрованных данных",
        QDir::homePath() + "/games_filtered_export.bin", "Бинарные файлы (*.bin)");
    
    if (filename.isEmpty()) return;
    
    if (dbManager_.exportFilteredToBinaryFile(filename.toStdString(), currentUser_.id, currentFilter_)) {
        lastExportedFile_ = filename;
        QMessageBox::information(this, "Успех", 
            "Отфильтрованные данные успешно экспортированы!\n\nФайл защищен контрольной суммой SHA-256.");
    } else {
        QMessageBox::critical(this, "Ошибка", 
            QString("Ошибка экспорта: %1").arg(QString::fromStdString(dbManager_.getLastError())));
    }
}

void MainWindow::onImportFromFile() {
    QString filename = QFileDialog::getOpenFileName(this, "Импорт из файла",
        QDir::homePath(), "Бинарные файлы (*.bin)");
    
    if (filename.isEmpty()) return;
    
    FileVerificationResult verification = dbManager_.verifyBinaryFile(filename.toStdString());
    
    if (verification != FileVerificationResult::OK) {
        QMessageBox::critical(this, "Ошибка верификации",
            QString("Файл не прошел проверку:\n\n%1\n\nИмпорт отменён.")
                .arg(QString::fromStdString(DatabaseManager::getVerificationErrorText(verification))));
        return;
    }
    
    if (dbManager_.importFromBinaryFile(filename.toStdString(), currentUser_.id)) {
        updateGamesTable();
        QMessageBox::information(this, "Успех", 
            "Данные успешно импортированы!\n\nКонтрольная сумма файла подтверждена.");
    } else {
        QMessageBox::critical(this, "Ошибка", 
            QString("Ошибка импорта: %1").arg(QString::fromStdString(dbManager_.getLastError())));
    }
}

void MainWindow::onViewExportedFile() {
    QString filename = QFileDialog::getOpenFileName(this, "Открыть бинарный файл",
        lastExportedFile_.isEmpty() ? QDir::homePath() : lastExportedFile_, 
        "Бинарные файлы (*.bin)");
    
    if (filename.isEmpty()) return;
    
    FileVerificationResult verification = dbManager_.verifyBinaryFile(filename.toStdString());
    
    if (verification != FileVerificationResult::OK) {
        QMessageBox::warning(this, "Предупреждение",
            QString("Файл не прошел проверку:\n%1\n\nПросмотр может быть некорректным.")
                .arg(QString::fromStdString(DatabaseManager::getVerificationErrorText(verification))));
    }
    
    std::vector<Game> games = dbManager_.readBinaryFile(filename.toStdString());
    
    if (games.empty() && verification == FileVerificationResult::OK) {
        QMessageBox::information(this, "Информация", "Файл пуст.");
        return;
    }
    
    BinaryFileViewDialog dialog(games, filename, this);
    dialog.exec();
}

void MainWindow::onTableSelectionChanged() {
    updateButtonStates();
    
    // Обновляем панель заметок если она открыта
    if (notesPanel_->isVisible()) {
        int row = gamesTable_->currentRow();
        if (row >= 0) {
            QTableWidgetItem* idItem = gamesTable_->item(row, 0);
            if (idItem) {
                int gameId = idItem->text().toInt();
                // Если выбрана другая игра, обновляем заметки
                if (gameId != currentNotesGameId_) {
                    QString gameName = gamesTable_->item(row, 1)->text();
                    QString notes = idItem->data(Qt::UserRole + 1).toString();
                    
                    currentNotesGameId_ = gameId;
                    notesPanelTitle_->setText(QString("📝 Заметки: %1").arg(gameName));
                    notesPanelEdit_->setPlainText(notes);
                }
            }
        }
    }
}

void MainWindow::onAdminPanel() {
    if (!currentUser_.is_admin) {
        QMessageBox::warning(this, "Доступ запрещен", "Эта функция доступна только администраторам.");
        return;
    }
    
    AdminPanelDialog dialog(&dbManager_, currentUser_.id, this);
    dialog.exec();
    
    // Если логин был изменен, обновляем отображение
    QString newUsername = dialog.getNewUsername();
    if (!newUsername.isEmpty()) {
        currentUser_.username = newUsername.toStdString();
        QString userType = currentUser_.is_admin ? "👑 Администратор" : "👤 Пользователь";
        userInfoLabel_->setText(QString("%1: %2").arg(userType, newUsername));
    }
}

void MainWindow::onAbout() {
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("О программе");
    aboutBox.setIconPixmap(QPixmap());
    aboutBox.setText(QString(
        "<h2 style='color: %1;'>⏳ Temporium</h2>"
        "<p>СУБД Компьютерные Игры</p>"
        "<p>Версия 4.1.0</p>"
        "<hr>"
        "<p>Курсовая работа по дисциплине «Программирование»</p>"
        "<p>ФГБОУ ВО «Новосибирский государственный технический университет»</p>"
        "<p>Кафедра «Защита информации»</p>"
        "<p>Тюриков Максим Олегович</p>"
        "<hr>"
        "<p><b>Возможности:</b></p>"
        "<ul>"
        "<li>Управление коллекцией игр</li>"
        "<li>Многопользовательский режим</li>"
        "<li>Администрирование пользователей</li>"
        "<li>Фильтрация по параметрам</li>"
        "<li>Экспорт/импорт в бинарный формат</li>"
        "<li>Защита от SQL-инъекций</li>"
        "<li>Проверка целостности файлов (SHA-256)</li>"
        "</ul>"
    ).arg(ACCENT_COLOR));
    aboutBox.exec();
}

void MainWindow::resetTableColumnWidths() {
    gamesTable_->setColumnWidth(0, 40);   // ID
    gamesTable_->setColumnWidth(1, 160);  // Название
    gamesTable_->setColumnWidth(2, 65);   // Диск
    gamesTable_->setColumnWidth(3, 65);   // ОЗУ
    gamesTable_->setColumnWidth(4, 65);   // VRAM
    gamesTable_->setColumnWidth(5, 85);   // Жанр
    gamesTable_->setColumnWidth(6, 70);   // Пройдено
    gamesTable_->setColumnWidth(7, 55);   // Оценка
    gamesTable_->setColumnWidth(8, 30);   // Избранное
    gamesTable_->setColumnWidth(9, 30);   // Установлено
    gamesTable_->setColumnWidth(10, 100); // Теги
}

void MainWindow::updateGamesTable() {
    std::vector<Game> games;
    
    if (filterActive_) {
        games = dbManager_.getFilteredGames(currentUser_.id, currentFilter_);
    } else {
        games = dbManager_.getAllGames(currentUser_.id);
    }
    
    updateGamesTable(games);
}

void MainWindow::updateGamesTable(const std::vector<Game>& games) {
    gamesTable_->setRowCount(0);
    gamesTable_->clearSelection();
    
    for (const auto& game : games) {
        int row = gamesTable_->rowCount();
        gamesTable_->insertRow(row);
        
        gamesTable_->setItem(row, 0, new QTableWidgetItem(QString::number(game.id)));
        
        // Название с индикатором заметок
        QString gameName = QString::fromStdString(game.name);
        if (!game.notes.empty()) {
            gameName += " 📝";  // Индикатор наличия заметок
        }
        QTableWidgetItem* nameItem = new QTableWidgetItem(gameName);
        if (!game.notes.empty()) {
            nameItem->setToolTip("Есть заметки: " + QString::fromStdString(game.notes).left(100) + "...");
        }
        gamesTable_->setItem(row, 1, nameItem);
        
        gamesTable_->setItem(row, 2, new QTableWidgetItem(QString::number(game.disk_space, 'f', 1)));
        gamesTable_->setItem(row, 3, new QTableWidgetItem(QString::number(game.ram_usage, 'f', 1)));
        gamesTable_->setItem(row, 4, new QTableWidgetItem(QString::number(game.vram_required, 'f', 1)));
        gamesTable_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(game.genre)));
        gamesTable_->setItem(row, 6, new QTableWidgetItem(game.completed ? "Да ✓" : "Нет"));
        
        // Оценка (колонка 7)
        QString ratingStr = (game.rating < 0) ? "—" : QString::number(game.rating);
        QTableWidgetItem* ratingItem = new QTableWidgetItem(ratingStr);
        ratingItem->setTextAlignment(Qt::AlignCenter);
        if (game.rating >= 8) {
            ratingItem->setForeground(QColor("#4CAF50"));  
        } else if (game.rating >= 5 && game.rating < 8) {
            ratingItem->setForeground(QColor("#FFC107"));  
        } else if (game.rating >= 0) {
            ratingItem->setForeground(QColor("#F44336"));  
        }
        gamesTable_->setItem(row, 7, ratingItem);
        
        // Избранное (колонка 8)
        QTableWidgetItem* favItem = new QTableWidgetItem(game.is_favorite ? "★" : "");
        favItem->setTextAlignment(Qt::AlignCenter);
        if (game.is_favorite) {
            favItem->setForeground(QColor("#FFD700"));  
            QFont favFont = favItem->font();
            favFont.setPointSize(14);
            favItem->setFont(favFont);
        }
        gamesTable_->setItem(row, 8, favItem);
        
        // Установлено (колонка 9)
        QTableWidgetItem* installedItem = new QTableWidgetItem(game.is_installed ? "📥" : "");
        installedItem->setTextAlignment(Qt::AlignCenter);
        if (game.is_installed) {
            installedItem->setForeground(QColor("#2196F3"));  
            QFont instFont = installedItem->font();
            instFont.setPointSize(12);
            installedItem->setFont(instFont);
        }
        gamesTable_->setItem(row, 9, installedItem);
        
        // Теги (колонка 10)
        QTableWidgetItem* tagsItem = new QTableWidgetItem(QString::fromStdString(game.tags));
        tagsItem->setForeground(QColor(TEXT_SECONDARY));
        gamesTable_->setItem(row, 10, tagsItem);
        
        // Ссылка (колонка 11)
        QTableWidgetItem* urlItem = new QTableWidgetItem();
        if (!game.url.empty()) {
            urlItem->setText("🔗 Открыть");
            urlItem->setData(Qt::UserRole, QString::fromStdString(game.url));
            urlItem->setForeground(QColor(ACCENT_COLOR));
            urlItem->setToolTip(QString::fromStdString(game.url));
            QFont linkFont = urlItem->font();
            linkFont.setUnderline(true);
            urlItem->setFont(linkFont);
        }
        gamesTable_->setItem(row, 11, urlItem);
        
        // Сохраняем ID игры для редактирования заметок
        gamesTable_->item(row, 0)->setData(Qt::UserRole + 1, QString::fromStdString(game.notes));
        
        if (game.completed) {
            QColor completedColor(30, 60, 30, 180);
            for (int col = 0; col < gamesTable_->columnCount(); ++col) {
                QTableWidgetItem* item = gamesTable_->item(row, col);
                if (item) {
                    item->setBackground(completedColor);
                }
            }
        }
        
        if (game.is_favorite && !game.completed) {
            QColor favoriteColor(60, 50, 20, 150);
            for (int col = 0; col < gamesTable_->columnCount(); ++col) {
                QTableWidgetItem* item = gamesTable_->item(row, col);
                if (item) {
                    item->setBackground(favoriteColor);
                }
            }
        }
    }
    
    updateButtonStates();
    updateStatusBar();
    updateStats();
}

void MainWindow::updateStatusBar() {
    QString status = QString("Игр в коллекции: %1").arg(gamesTable_->rowCount());
    
    if (filterActive_) {
        status += " (фильтр активен)";
    }
    
    statusBar()->showMessage(status);
}

void MainWindow::updateStats() {
    if (currentUser_.id == 0) return;
    
    GameStats stats = dbManager_.getGameStats(currentUser_.id);
    
    QString statsText = QString(
        "★ Избранное: %1  |  ✓ Пройдено: %2  |  📊 Без оценки: %3  |  "
        "📥 Установлено: %4 (%5 ГБ)  |  🔗 Без ссылки: %6")
        .arg(stats.favorites_count)
        .arg(stats.completed_count)
        .arg(stats.no_rating_count)
        .arg(stats.installed_count)
        .arg(stats.installed_disk_space, 0, 'f', 1)
        .arg(stats.no_url_count);
    
    statsLabel_->setText(statsText);
}

void MainWindow::updateTagsCombo() {
    filterTagCombo_->clear();
    filterTagCombo_->addItem("Все теги", "");
    
    if (currentUser_.id == 0) return;
    
    std::vector<std::string> tags = dbManager_.getUserTags(currentUser_.id);
    for (const auto& tag : tags) {
        filterTagCombo_->addItem(QString::fromStdString(tag), QString::fromStdString(tag));
    }
}


GameEditDialog::GameEditDialog(QWidget* parent, const Game* game)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint)
    , gameId_(0)
    , userId_(0)
{
    setWindowTitle(game ? "Редактирование игры" : "Добавление игры");
    setMinimumWidth(500);
    setMinimumHeight(550);
    setModal(true);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QFormLayout* layout = new QFormLayout();
    layout->setSpacing(10);
    
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Введите название игры");
    
    diskSpaceSpin_ = new QDoubleSpinBox();
    diskSpaceSpin_->setRange(-99999, 99999);
    diskSpaceSpin_->setSuffix(" ГБ");
    diskSpaceSpin_->setDecimals(1);
    diskSpaceSpin_->setValue(10);
    connect(diskSpaceSpin_, &QDoubleSpinBox::editingFinished, [this]() {
        double val = diskSpaceSpin_->value();
        if (val < MIN_DISK_SPACE) diskSpaceSpin_->setValue(MIN_DISK_SPACE);
        else if (val > MAX_DISK_SPACE) diskSpaceSpin_->setValue(MAX_DISK_SPACE);
    });
    
    ramUsageSpin_ = new QDoubleSpinBox();
    ramUsageSpin_->setRange(-99999, 99999);
    ramUsageSpin_->setSuffix(" ГБ");
    ramUsageSpin_->setDecimals(1);
    ramUsageSpin_->setValue(4);
    connect(ramUsageSpin_, &QDoubleSpinBox::editingFinished, [this]() {
        double val = ramUsageSpin_->value();
        if (val < MIN_RAM_USAGE) ramUsageSpin_->setValue(MIN_RAM_USAGE);
        else if (val > MAX_RAM_USAGE) ramUsageSpin_->setValue(MAX_RAM_USAGE);
    });
    
    vramRequiredSpin_ = new QDoubleSpinBox();
    vramRequiredSpin_->setRange(-99999, 99999);
    vramRequiredSpin_->setSuffix(" ГБ");
    vramRequiredSpin_->setDecimals(1);
    vramRequiredSpin_->setValue(2);
    connect(vramRequiredSpin_, &QDoubleSpinBox::editingFinished, [this]() {
        double val = vramRequiredSpin_->value();
        if (val < MIN_VRAM_REQUIRED) vramRequiredSpin_->setValue(MIN_VRAM_REQUIRED);
        else if (val > MAX_VRAM_REQUIRED) vramRequiredSpin_->setValue(MAX_VRAM_REQUIRED);
    });
    
    genreCombo_ = new QComboBox();
    for (const auto& genre : GENRES) {
        genreCombo_->addItem(QString::fromStdString(genre));
    }
    
    completedCheck_ = new QCheckBox("Игра пройдена");
    
    urlEdit_ = new QLineEdit();
    urlEdit_->setPlaceholderText("https://store.steampowered.com/app/...");
    
    ratingCombo_ = new QComboBox();
    ratingCombo_->addItem("Нет оценки", -1);
    for (int i = 0; i <= 10; ++i) {
        ratingCombo_->addItem(QString::number(i), i);
    }
    
    favoriteCheck_ = new QCheckBox("★ Избранное");
    favoriteCheck_->setStyleSheet("QCheckBox { color: #FFD700; font-weight: bold; }");
    
    installedCheck_ = new QCheckBox("📥 Установлено");
    installedCheck_->setStyleSheet("QCheckBox { color: #2196F3; font-weight: bold; }");
    
    tagsEdit_ = new QLineEdit();
    tagsEdit_->setPlaceholderText("Теги через запятую: RPG, Open World, Co-op");
    
    layout->addRow("Название:", nameEdit_);
    layout->addRow("Место на диске:", diskSpaceSpin_);
    layout->addRow("Потребление ОЗУ:", ramUsageSpin_);
    layout->addRow("Видеопамять:", vramRequiredSpin_);
    layout->addRow("Жанр:", genreCombo_);
    layout->addRow("Оценка (0-10):", ratingCombo_);
    layout->addRow("Теги:", tagsEdit_);
    layout->addRow("Ссылка:", urlEdit_);
    
    QHBoxLayout* checksLayout = new QHBoxLayout();
    checksLayout->addWidget(completedCheck_);
    checksLayout->addWidget(installedCheck_);
    checksLayout->addStretch();
    checksLayout->addWidget(favoriteCheck_);
    layout->addRow("", checksLayout);
    
    mainLayout->addLayout(layout);
    
    QGroupBox* notesGroup = new QGroupBox("📝 Заметки");
    notesGroup->setCheckable(true);
    notesGroup->setChecked(false);
    QVBoxLayout* notesLayout = new QVBoxLayout(notesGroup);
    
    notesEdit_ = new QTextEdit();
    notesEdit_->setPlaceholderText("Здесь можно записать свои заметки об игре...");
    notesEdit_->setMinimumHeight(100);
    notesEdit_->setMaximumHeight(150);
    notesLayout->addWidget(notesEdit_);
    
    connect(notesGroup, &QGroupBox::toggled, notesEdit_, &QTextEdit::setVisible);
    notesEdit_->setVisible(false);
    
    mainLayout->addWidget(notesGroup);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (nameEdit_->text().trimmed().isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Ошибка");
            msgBox.setText("Введите название игры!");
            msgBox.exec();
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    if (game) {
        gameId_ = game->id;
        userId_ = game->user_id;
        nameEdit_->setText(QString::fromStdString(game->name));
        diskSpaceSpin_->setValue(game->disk_space);
        ramUsageSpin_->setValue(game->ram_usage);
        vramRequiredSpin_->setValue(game->vram_required);
        
        int genreIndex = genreCombo_->findText(QString::fromStdString(game->genre));
        if (genreIndex >= 0) {
            genreCombo_->setCurrentIndex(genreIndex);
        }
        
        completedCheck_->setChecked(game->completed);
        urlEdit_->setText(QString::fromStdString(game->url));
        
        int ratingIndex = ratingCombo_->findData(game->rating);
        if (ratingIndex >= 0) {
            ratingCombo_->setCurrentIndex(ratingIndex);
        }
        
        favoriteCheck_->setChecked(game->is_favorite);
        installedCheck_->setChecked(game->is_installed);
        tagsEdit_->setText(QString::fromStdString(game->tags));
        
        if (!game->notes.empty()) {
            notesEdit_->setPlainText(QString::fromStdString(game->notes));
            notesGroup->setChecked(true);
            notesEdit_->setVisible(true);
        }
    }
}

Game GameEditDialog::getGame() const {
    Game game;
    game.id = gameId_;
    game.name = nameEdit_->text().trimmed().toStdString();
    game.disk_space = diskSpaceSpin_->value();
    game.ram_usage = ramUsageSpin_->value();
    game.vram_required = vramRequiredSpin_->value();
    game.genre = genreCombo_->currentText().toStdString();
    game.completed = completedCheck_->isChecked();
    game.url = urlEdit_->text().trimmed().toStdString();
    game.user_id = userId_;
    
    game.rating = ratingCombo_->currentData().toInt();
    game.is_favorite = favoriteCheck_->isChecked();
    game.is_installed = installedCheck_->isChecked();
    game.tags = tagsEdit_->text().trimmed().toStdString();
    game.notes = notesEdit_->toPlainText().toStdString();
    
    return game;
}


BinaryFileViewDialog::BinaryFileViewDialog(const std::vector<Game>& games, 
                                           const QString& filename,
                                           QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint)
{
    setWindowTitle("Просмотр бинарного файла");
    setMinimumSize(900, 550);
    setModal(true);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* fileLabel = new QLabel(QString("Файл: %1").arg(QFileInfo(filename).fileName()));
    QLabel* infoLabel = new QLabel(QString("Записей в файле: %1").arg(games.size()));
    layout->addWidget(fileLabel);
    layout->addWidget(infoLabel);
    
    table_ = new QTableWidget();
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels({
        "Название", "Диск (ГБ)", "ОЗУ (ГБ)", "VRAM (ГБ)", "Жанр", "Пройдено", "Ссылка"
    });
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 220);
    table_->verticalHeader()->setVisible(false);
    
    for (const auto& game : games) {
        int row = table_->rowCount();
        table_->insertRow(row);
        
        table_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(game.name)));
        table_->setItem(row, 1, new QTableWidgetItem(QString::number(game.disk_space, 'f', 1)));
        table_->setItem(row, 2, new QTableWidgetItem(QString::number(game.ram_usage, 'f', 1)));
        table_->setItem(row, 3, new QTableWidgetItem(QString::number(game.vram_required, 'f', 1)));
        table_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(game.genre)));
        table_->setItem(row, 5, new QTableWidgetItem(game.completed ? "Да" : "Нет"));
        table_->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(game.url)));
    }
    
    layout->addWidget(table_);
    
    QPushButton* closeButton = new QPushButton("Закрыть");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeButton);
}


AdminPanelDialog::AdminPanelDialog(DatabaseManager* dbManager, int adminUserId, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint)
    , dbManager_(dbManager)
    , adminUserId_(adminUserId)
{
    setWindowTitle("Панель администратора");
    setMinimumSize(800, 600);
    setModal(true);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel("👑 Управление пользователями");
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    titleLabel->setStyleSheet(QString("color: %1;").arg(ACCENT_COLOR));
    layout->addWidget(titleLabel);
    
    QGroupBox* adminSettingsBox = new QGroupBox("Настройки администратора");
    QHBoxLayout* adminLayout = new QHBoxLayout(adminSettingsBox);
    
    changeUsernameButton_ = new QPushButton("✏️ Изменить логин");
    changePasswordButton_ = new QPushButton("🔑 Изменить пароль");
    resetAdminButton_ = new QPushButton("⚠️ Сбросить к admin/admin123");
    resetAdminButton_->setStyleSheet("QPushButton { color: #ff6b6b; }");
    
    adminLayout->addWidget(changeUsernameButton_);
    adminLayout->addWidget(changePasswordButton_);
    adminLayout->addStretch();
    adminLayout->addWidget(resetAdminButton_);
    
    layout->addWidget(adminSettingsBox);
    
    QLabel* usersLabel = new QLabel("Зарегистрированные пользователи:");
    layout->addWidget(usersLabel);
    
    usersTable_ = new QTableWidget();
    usersTable_->setColumnCount(4);
    usersTable_->setHorizontalHeaderLabels({"ID", "Имя пользователя", "Роль", "Игр"});
    usersTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    usersTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    usersTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    usersTable_->horizontalHeader()->setStretchLastSection(true);
    usersTable_->setColumnWidth(0, 50);
    usersTable_->setColumnWidth(1, 200);
    usersTable_->setColumnWidth(2, 150);
    usersTable_->verticalHeader()->setVisible(false);
    
    layout->addWidget(usersTable_);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    deleteButton_ = new QPushButton("🗑️ Удалить пользователя");
    deleteButton_->setEnabled(false);
    
    refreshButton_ = new QPushButton("🔄 Обновить");
    
    QPushButton* closeButton = new QPushButton("Закрыть");
    
    buttonLayout->addWidget(deleteButton_);
    buttonLayout->addWidget(refreshButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    
    layout->addLayout(buttonLayout);
    
    connect(deleteButton_, &QPushButton::clicked, this, &AdminPanelDialog::onDeleteUser);
    connect(refreshButton_, &QPushButton::clicked, this, &AdminPanelDialog::onRefresh);
    connect(changeUsernameButton_, &QPushButton::clicked, this, &AdminPanelDialog::onChangeUsername);
    connect(changePasswordButton_, &QPushButton::clicked, this, &AdminPanelDialog::onChangePassword);
    connect(resetAdminButton_, &QPushButton::clicked, this, &AdminPanelDialog::onResetAdmin);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(usersTable_, &QTableWidget::itemSelectionChanged, [this]() {
        int row = usersTable_->currentRow();
        if (row >= 0) {
            QString role = usersTable_->item(row, 2)->text();
            deleteButton_->setEnabled(role != "Администратор");
        } else {
            deleteButton_->setEnabled(false);
        }
    });
    
    updateUsersList();
}

void AdminPanelDialog::updateUsersList() {
    usersTable_->setRowCount(0);
    
    std::vector<User> users = dbManager_->getAllUsers();
    
    for (const auto& user : users) {
        int row = usersTable_->rowCount();
        usersTable_->insertRow(row);
        
        usersTable_->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        usersTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(user.username)));
        usersTable_->setItem(row, 2, new QTableWidgetItem(user.is_admin ? "Администратор" : "Пользователь"));
        
        int gamesCount = dbManager_->getUserGamesCount(user.id);
        usersTable_->setItem(row, 3, new QTableWidgetItem(QString::number(gamesCount)));
        
        if (user.is_admin) {
            for (int col = 0; col < usersTable_->columnCount(); ++col) {
                usersTable_->item(row, col)->setForeground(QColor(ACCENT_COLOR));
            }
        }
    }
}

void AdminPanelDialog::onDeleteUser() {
    int row = usersTable_->currentRow();
    if (row < 0) return;
    
    QString username = usersTable_->item(row, 1)->text();
    int userId = usersTable_->item(row, 0)->text().toInt();
    int gamesCount = usersTable_->item(row, 3)->text().toInt();
    
    QString message = QString("Вы уверены, что хотите удалить пользователя \"%1\"?").arg(username);
    if (gamesCount > 0) {
        message += QString("\n\nВнимание: будут также удалены все %1 игр(ы) этого пользователя!").arg(gamesCount);
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение удаления",
        message, QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (dbManager_->deleteUser(userId)) {
            updateUsersList();
            QMessageBox::information(this, "Успех", 
                QString("Пользователь \"%1\" удален").arg(username));
        } else {
            QMessageBox::critical(this, "Ошибка", 
                QString("Не удалось удалить пользователя: %1")
                    .arg(QString::fromStdString(dbManager_->getLastError())));
        }
    }
}

void AdminPanelDialog::onRefresh() {
    updateUsersList();
}

void AdminPanelDialog::onChangeUsername() {
    QDialog dialog(this);
    dialog.setWindowTitle("Изменение логина");
    dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);
    dialog.setMinimumWidth(350);
    
    QFormLayout* layout = new QFormLayout(&dialog);
    
    QLineEdit* newUsernameEdit = new QLineEdit();
    newUsernameEdit->setPlaceholderText("Новый логин");
    
    QLineEdit* passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText("Текущий пароль для подтверждения");
    
    layout->addRow("Новый логин:", newUsernameEdit);
    layout->addRow("Текущий пароль:", passwordEdit);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(buttons);
    
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    QString newUsername = newUsernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    
    if (newUsername.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля!");
        return;
    }
    
    if (newUsername.length() < 3) {
        QMessageBox::warning(this, "Ошибка", "Логин должен содержать минимум 3 символа!");
        return;
    }
    
    std::vector<User> users = dbManager_->getAllUsers();
    QString adminUsername;
    for (const auto& user : users) {
        if (user.id == adminUserId_) {
            adminUsername = QString::fromStdString(user.username);
            break;
        }
    }
    
    std::string currentHash = HashUtils::hashPassword(password.toStdString(), adminUsername.toStdString());
    User verifyUser = dbManager_->authenticateUser(adminUsername.toStdString(), currentHash);
    
    if (verifyUser.id == 0) {
        QMessageBox::warning(this, "Ошибка", "Неверный пароль!");
        return;
    }
    
    if (dbManager_->changeUsername(adminUserId_, newUsername.toStdString(), password.toStdString())) {
        newUsername_ = newUsername;
        updateUsersList();
        QMessageBox::information(this, "Успех", 
            QString("Логин успешно изменен на \"%1\".\n\nПри следующем входе используйте новый логин.").arg(newUsername));
    } else {
        QMessageBox::critical(this, "Ошибка", 
            QString::fromStdString(dbManager_->getLastError()));
    }
}

void AdminPanelDialog::onChangePassword() {
    QDialog passwordDialog(this);
    passwordDialog.setWindowTitle("Изменение пароля");
    passwordDialog.setWindowFlags(passwordDialog.windowFlags() | Qt::WindowStaysOnTopHint);
    passwordDialog.setMinimumWidth(350);
    
    QFormLayout* layout = new QFormLayout(&passwordDialog);
    
    QLineEdit* currentPasswordEdit = new QLineEdit();
    currentPasswordEdit->setEchoMode(QLineEdit::Password);
    currentPasswordEdit->setPlaceholderText("Текущий пароль");
    
    QLineEdit* newPasswordEdit = new QLineEdit();
    newPasswordEdit->setEchoMode(QLineEdit::Password);
    newPasswordEdit->setPlaceholderText("Новый пароль");
    
    QLineEdit* confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setPlaceholderText("Повторите новый пароль");
    
    layout->addRow("Текущий пароль:", currentPasswordEdit);
    layout->addRow("Новый пароль:", newPasswordEdit);
    layout->addRow("Подтверждение:", confirmPasswordEdit);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(buttons);
    
    connect(buttons, &QDialogButtonBox::accepted, &passwordDialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &passwordDialog, &QDialog::reject);
    
    if (passwordDialog.exec() != QDialog::Accepted) {
        return;
    }
    
    QString currentPassword = currentPasswordEdit->text();
    QString newPassword = newPasswordEdit->text();
    QString confirmPassword = confirmPasswordEdit->text();
    
    if (currentPassword.isEmpty() || newPassword.isEmpty() || confirmPassword.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля!");
        return;
    }
    
    if (newPassword != confirmPassword) {
        QMessageBox::warning(this, "Ошибка", "Новый пароль и подтверждение не совпадают!");
        return;
    }
    
    if (newPassword.length() < 4) {
        QMessageBox::warning(this, "Ошибка", "Пароль должен содержать минимум 4 символа!");
        return;
    }
    
    std::vector<User> users = dbManager_->getAllUsers();
    QString adminUsername;
    for (const auto& user : users) {
        if (user.id == adminUserId_) {
            adminUsername = QString::fromStdString(user.username);
            break;
        }
    }
    
    std::string currentHash = HashUtils::hashPassword(currentPassword.toStdString(), adminUsername.toStdString());
    User verifyUser = dbManager_->authenticateUser(adminUsername.toStdString(), currentHash);
    
    if (verifyUser.id == 0) {
        QMessageBox::warning(this, "Ошибка", "Неверный текущий пароль!");
        return;
    }
    
    std::string newHash = HashUtils::hashPassword(newPassword.toStdString(), adminUsername.toStdString());
    
    if (dbManager_->changePassword(adminUserId_, newHash)) {
        QMessageBox::information(this, "Успех", "Пароль успешно изменен!");
    } else {
        QMessageBox::critical(this, "Ошибка", 
            QString("Не удалось изменить пароль: %1")
                .arg(QString::fromStdString(dbManager_->getLastError())));
    }
}

void AdminPanelDialog::onResetAdmin() {
    QMessageBox::StandardButton reply = QMessageBox::warning(this, "Подтверждение сброса",
        "Вы уверены, что хотите сбросить учётные данные администратора?\n\n"
        "Логин: admin\n"
        "Пароль: admin123\n\n"
        "После сброса необходимо перезайти в систему!",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (dbManager_->resetAdminCredentials()) {
            newUsername_ = "admin";
            updateUsersList();
            QMessageBox::information(this, "Успех", 
                "Учётные данные администратора сброшены!\n\n"
                "Логин: admin\n"
                "Пароль: admin123\n\n"
                "Пожалуйста, перезайдите в систему.");
        } else {
            QMessageBox::critical(this, "Ошибка", 
                QString("Не удалось сбросить учётные данные: %1")
                    .arg(QString::fromStdString(dbManager_->getLastError())));
        }
    }
}

}