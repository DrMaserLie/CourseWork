#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <pqxx/pqxx>
#include "types.h"

namespace Temporium {

// Результат проверки файла при импорте
enum class FileVerificationResult {
    OK,
    FILE_NOT_FOUND,
    INVALID_MAGIC,
    INVALID_VERSION,
    HASH_MISMATCH,
    READ_ERROR
};

// Статистика игр для отображения в статусбаре
struct GameStats {
    int total_games = 0;
    int favorites_count = 0;
    int completed_count = 0;
    int no_rating_count = 0;
    int installed_count = 0;
    double installed_disk_space = 0.0;  // ГБ
    int no_url_count = 0;
};

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();
    
    // Подключение к базе данных
    bool connect(const std::string& host, int port, 
                 const std::string& dbname, 
                 const std::string& user, 
                 const std::string& password);
    
    void disconnect();
    bool isConnected() const;
    
    // Инициализация таблиц
    bool initializeTables();
    
    // Операции с пользователями
    bool registerUser(const std::string& username, const std::string& password_hash, bool is_admin = false);
    User authenticateUser(const std::string& username, const std::string& password_hash);
    bool userExists(const std::string& username);
    
    // Админские функции
    std::vector<User> getAllUsers();
    bool deleteUser(int user_id);
    bool isAdmin(int user_id);
    int getUserGamesCount(int user_id);
    bool changeUsername(int user_id, const std::string& new_username, const std::string& current_password);
    bool changePassword(int user_id, const std::string& new_password_hash);
    bool resetAdminCredentials();  // Сброс админа к admin/admin123
    
    // CRUD операции с играми
    bool addGame(const Game& game);
    bool updateGame(const Game& game);
    bool deleteGame(int game_id, int user_id);
    bool deleteGameByName(const std::string& name, int user_id);
    
    // Получение игр
    std::vector<Game> getAllGames(int user_id);
    std::vector<Game> getFilteredGames(int user_id, const GameFilter& filter);
    Game getGameById(int game_id, int user_id);
    Game getGameByName(const std::string& name, int user_id);
    
    // Получение списка уникальных тегов пользователя
    std::vector<std::string> getUserTags(int user_id);
    
    // Обновление заметок для игры
    bool updateGameNotes(int game_id, int user_id, const std::string& notes);
    
    // Получение статистики игр
    GameStats getGameStats(int user_id);
    
    // Экспорт в бинарный файл (с хешем для проверки целостности)
    bool exportToBinaryFile(const std::string& filename, int user_id);
    bool exportFilteredToBinaryFile(const std::string& filename, int user_id, 
                                     const GameFilter& filter);
    
    // Верификация файла перед импортом
    FileVerificationResult verifyBinaryFile(const std::string& filename);
    
    // Импорт из бинарного файла (с проверкой хеша)
    bool importFromBinaryFile(const std::string& filename, int user_id);
    
    // Чтение бинарного файла (для просмотра)
    std::vector<Game> readBinaryFile(const std::string& filename);
    
    // Получение последней ошибки
    std::string getLastError() const;
    
    // Получение текстового описания ошибки верификации
    static std::string getVerificationErrorText(FileVerificationResult result);
    
private:
    std::unique_ptr<pqxx::connection> conn_;
    std::string last_error_;
    
    // Построение WHERE условия для фильтра
    std::string buildFilterCondition(const GameFilter& filter, int user_id);
    
    // Запись игр в файл
    bool writeGamesToFile(const std::string& filename, const std::vector<Game>& games);
    
    // Создание администратора по умолчанию
    void ensureAdminExists();
};

} // namespace Temporium

#endif // DATABASE_MANAGER_H
