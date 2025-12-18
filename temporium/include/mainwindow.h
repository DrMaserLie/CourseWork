#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QHeaderView>
#include <QFormLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QSettings>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QTextEdit>
#include <QSpinBox>

#include "database_manager.h"
#include "hash_utils.h"

namespace Temporium {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onLogin();
    void onRegister();
    void onLogout();
    
    void onAddGame();
    void onEditGame();
    void onDeleteGame();
    void onRefreshGames();
    
    void onApplyFilter();
    void onResetFilter();
    
    void onExportToFile();
    void onExportFilteredToFile();
    void onImportFromFile();
    void onViewExportedFile();
    
    void onTableSelectionChanged();
    void onTableCellClicked(int row, int column);
    void onTableCellDoubleClicked(int row, int column);
    void onToggleNotesPanel();
    void onSaveNotes();
    void onAbout();
    
    void onAdminPanel();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupLoginPage();
    void setupMainPage();
    void setupConnections();
    void applyDarkTheme();
    
    void showLoginPage();
    void showMainPage();
    void updateGamesTable();
    void updateGamesTable(const std::vector<Game>& games);
    void updateStatusBar();
    void updateButtonStates();
    void resetTableColumnWidths();
    void updateTagsCombo();
    void updateStats();
    
    void connectToDatabase();
    void saveLastUsername();
    void loadLastUsername();
    
    QStackedWidget* stackedWidget_;
    
    // Страница входа
    QWidget* loginPage_;
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QPushButton* loginButton_;
    QPushButton* registerButton_;
    QCheckBox* rememberUserCheck_;
    
    // Главная страница
    QWidget* mainPage_;
    QTableWidget* gamesTable_;
    QLabel* userInfoLabel_;
    
    // Панель фильтров
    QGroupBox* filterGroupBox_;
    QCheckBox* filterCompletedCheck_;
    QComboBox* filterCompletedCombo_;
    QCheckBox* filterGenreCheck_;
    QComboBox* filterGenreCombo_;
    QCheckBox* filterDiskMinCheck_;
    QDoubleSpinBox* filterDiskMinSpin_;
    QCheckBox* filterDiskMaxCheck_;
    QDoubleSpinBox* filterDiskMaxSpin_;
    QCheckBox* filterRamMinCheck_;
    QDoubleSpinBox* filterRamMinSpin_;
    QCheckBox* filterRamMaxCheck_;
    QDoubleSpinBox* filterRamMaxSpin_;
    QCheckBox* filterVramMinCheck_;
    QDoubleSpinBox* filterVramMinSpin_;
    QCheckBox* filterVramMaxCheck_;
    QDoubleSpinBox* filterVramMaxSpin_;
    
    // Новые фильтры
    QCheckBox* filterTagCheck_;
    QComboBox* filterTagCombo_;
    QCheckBox* filterFavoriteCheck_;
    QComboBox* filterFavoriteCombo_;
    QCheckBox* filterInstalledCheck_;
    QComboBox* filterInstalledCombo_;
    QCheckBox* filterRatingCheck_;
    QComboBox* filterRatingCombo_;  // Есть оценка / Нет оценки / Любая
    
    QPushButton* applyFilterButton_;
    QPushButton* resetFilterButton_;
    
    // Статистика внизу окна
    QLabel* statsLabel_;
    
    // Панель заметок (раскрывающаяся)
    QGroupBox* notesPanel_;
    QTextEdit* notesPanelEdit_;
    QLabel* notesPanelTitle_;
    QPushButton* saveNotesButton_;
    int currentNotesGameId_;  // ID игры, для которой открыты заметки
    
    // Кнопки управления
    QPushButton* addButton_;
    QPushButton* editButton_;
    QPushButton* deleteButton_;
    QPushButton* refreshButton_;
    QPushButton* notesButton_;  // Кнопка для раскрытия заметок
    
    // Действия меню
    QAction* loginAction_;
    QAction* logoutAction_;
    QAction* exitAction_;
    QAction* addAction_;
    QAction* editAction_;
    QAction* deleteAction_;
    QAction* exportAction_;
    QAction* exportFilteredAction_;
    QAction* importAction_;
    QAction* viewExportedAction_;
    QAction* aboutAction_;
    QAction* adminAction_;
    QMenu* adminMenu_;
    
    DatabaseManager dbManager_;
    User currentUser_;
    
    GameFilter currentFilter_;
    bool filterActive_;
    
    QString lastExportedFile_;
    
    int lastClickedRow_;
    
    QSettings settings_;
};

// Диалог редактирования игры
class GameEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit GameEditDialog(QWidget* parent = nullptr, const Game* game = nullptr);
    Game getGame() const;

private:
    QLineEdit* nameEdit_;
    QDoubleSpinBox* diskSpaceSpin_;
    QDoubleSpinBox* ramUsageSpin_;
    QDoubleSpinBox* vramRequiredSpin_;
    QComboBox* genreCombo_;
    QCheckBox* completedCheck_;
    QLineEdit* urlEdit_;
    
    // Новые поля
    QComboBox* ratingCombo_;         // Оценка: "Нет" или 0-10
    QCheckBox* favoriteCheck_;       // Избранное
    QCheckBox* installedCheck_;      // Установлено
    QLineEdit* tagsEdit_;            // Теги через запятую
    QTextEdit* notesEdit_;           // Заметки
    
    int gameId_;
    int userId_;
};

// Диалог просмотра бинарного файла
class BinaryFileViewDialog : public QDialog {
    Q_OBJECT

public:
    explicit BinaryFileViewDialog(const std::vector<Game>& games, 
                                   const QString& filename,
                                   QWidget* parent = nullptr);

private:
    QTableWidget* table_;
};

// Админская панель
class AdminPanelDialog : public QDialog {
    Q_OBJECT

public:
    explicit AdminPanelDialog(DatabaseManager* dbManager, int adminUserId, QWidget* parent = nullptr);
    
    // Возвращает новый логин, если он был изменен
    QString getNewUsername() const { return newUsername_; }

private slots:
    void onDeleteUser();
    void onRefresh();
    void onChangeUsername();
    void onChangePassword();
    void onResetAdmin();

private:
    void updateUsersList();
    
    DatabaseManager* dbManager_;
    int adminUserId_;
    QTableWidget* usersTable_;
    QPushButton* deleteButton_;
    QPushButton* refreshButton_;
    QPushButton* changeUsernameButton_;
    QPushButton* changePasswordButton_;
    QPushButton* resetAdminButton_;
    QString newUsername_;
};

} // namespace Temporium

#endif // MAINWINDOW_H
