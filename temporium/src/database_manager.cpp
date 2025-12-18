#include "database_manager.h"
#include "hash_utils.h"
#include <fstream>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>

namespace Temporium {

DatabaseManager::DatabaseManager() : conn_(nullptr) {}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect(const std::string& host, int port,
                              const std::string& dbname,
                              const std::string& user,
                              const std::string& password) {
    try {
        std::stringstream conn_str;
        conn_str << "host=" << host 
                 << " port=" << port 
                 << " dbname=" << dbname 
                 << " user=" << user 
                 << " password=" << password;
        
        conn_ = std::make_unique<pqxx::connection>(conn_str.str());
        
        if (conn_->is_open()) {
            if (initializeTables()) {
                ensureAdminExists();
                return true;
            }
        }
        
        last_error_ = "Failed to open database connection";
        return false;
    } catch (const std::exception& e) {
        last_error_ = std::string("Connection error: ") + e.what();
        return false;
    }
}

void DatabaseManager::disconnect() {
    if (conn_) {
        conn_.reset();
    }
}

bool DatabaseManager::isConnected() const {
    return conn_ && conn_->is_open();
}

bool DatabaseManager::initializeTables() {
    try {
        pqxx::work txn(*conn_);
        
        // Таблица пользователей с флагом админа
        txn.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "    id SERIAL PRIMARY KEY,"
            "    username VARCHAR(255) UNIQUE NOT NULL,"
            "    password_hash VARCHAR(64) NOT NULL,"
            "    is_admin BOOLEAN DEFAULT FALSE,"
            "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ")"
        );
        
        // Добавляем колонку is_admin если её нет (для миграции)
        txn.exec(
            "DO $$ BEGIN "
            "    ALTER TABLE users ADD COLUMN IF NOT EXISTS is_admin BOOLEAN DEFAULT FALSE; "
            "EXCEPTION WHEN others THEN NULL; END $$"
        );
        
        // Таблица игр с полем URL и новыми полями
        txn.exec(
            "CREATE TABLE IF NOT EXISTS games ("
            "    id SERIAL PRIMARY KEY,"
            "    name VARCHAR(255) NOT NULL,"
            "    disk_space DOUBLE PRECISION NOT NULL,"
            "    ram_usage DOUBLE PRECISION NOT NULL,"
            "    vram_required DOUBLE PRECISION NOT NULL,"
            "    genre VARCHAR(64) NOT NULL,"
            "    completed BOOLEAN DEFAULT FALSE,"
            "    url VARCHAR(512) DEFAULT '',"
            "    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,"
            "    rating INTEGER DEFAULT -1,"
            "    is_favorite BOOLEAN DEFAULT FALSE,"
            "    notes TEXT DEFAULT '',"
            "    tags VARCHAR(512) DEFAULT '',"
            "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
            "    UNIQUE(name, user_id)"
            ")"
        );
        
        // Добавляем новые колонки если их нет (для миграции)
        txn.exec(
            "DO $$ BEGIN "
            "    ALTER TABLE games ADD COLUMN IF NOT EXISTS url VARCHAR(512) DEFAULT ''; "
            "EXCEPTION WHEN others THEN NULL; END $$"
        );
        txn.exec(
            "DO $$ BEGIN "
            "    ALTER TABLE games ADD COLUMN IF NOT EXISTS rating INTEGER DEFAULT -1; "
            "EXCEPTION WHEN others THEN NULL; END $$"
        );
        txn.exec(
            "DO $$ BEGIN "
            "    ALTER TABLE games ADD COLUMN IF NOT EXISTS is_favorite BOOLEAN DEFAULT FALSE; "
            "EXCEPTION WHEN others THEN NULL; END $$"
        );
        txn.exec(
            "DO $$ BEGIN "
            "    ALTER TABLE games ADD COLUMN IF NOT EXISTS notes TEXT DEFAULT ''; "
            "EXCEPTION WHEN others THEN NULL; END $$"
        );
        txn.exec(
            "DO $$ BEGIN "
            "    ALTER TABLE games ADD COLUMN IF NOT EXISTS tags VARCHAR(512) DEFAULT ''; "
            "EXCEPTION WHEN others THEN NULL; END $$"
        );
        txn.exec(
            "DO $$ BEGIN "
            "    ALTER TABLE games ADD COLUMN IF NOT EXISTS is_installed BOOLEAN DEFAULT FALSE; "
            "EXCEPTION WHEN others THEN NULL; END $$"
        );
        
        txn.exec("CREATE INDEX IF NOT EXISTS idx_games_user_id ON games(user_id)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_games_genre ON games(genre)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_games_completed ON games(completed)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_games_favorite ON games(is_favorite)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_games_rating ON games(rating)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_games_installed ON games(is_installed)");
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Table initialization error: ") + e.what();
        return false;
    }
}

void DatabaseManager::ensureAdminExists() {
    try {
        pqxx::work txn(*conn_);
        
        // Проверяем, есть ли админ
        pqxx::result r = txn.exec("SELECT COUNT(*) FROM users WHERE is_admin = TRUE");
        
        if (r[0][0].as<int>() == 0) {
            // Создаем администратора по умолчанию: admin / admin123
            std::string adminHash = HashUtils::hashPassword("admin123", "admin");
            txn.exec_params(
                "INSERT INTO users (username, password_hash, is_admin) VALUES ($1, $2, TRUE)",
                "admin", adminHash
            );
        }
        
        txn.commit();
    } catch (const std::exception& e) {
        // Игнорируем ошибки (админ может уже существовать)
    }
}

bool DatabaseManager::registerUser(const std::string& username, const std::string& password_hash, bool is_admin) {
    try {
        pqxx::work txn(*conn_);
        
        txn.exec_params(
            "INSERT INTO users (username, password_hash, is_admin) VALUES ($1, $2, $3)",
            username, password_hash, is_admin
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Registration error: ") + e.what();
        return false;
    }
}

User DatabaseManager::authenticateUser(const std::string& username, const std::string& password_hash) {
    User user;
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT id, username, password_hash, is_admin FROM users WHERE username = $1 AND password_hash = $2",
            username, password_hash
        );
        
        if (!r.empty()) {
            user.id = r[0]["id"].as<int>();
            user.username = r[0]["username"].as<std::string>();
            user.password_hash = r[0]["password_hash"].as<std::string>();
            user.is_admin = r[0]["is_admin"].as<bool>();
        }
        
        txn.commit();
    } catch (const std::exception& e) {
        last_error_ = std::string("Authentication error: ") + e.what();
    }
    
    return user;
}

bool DatabaseManager::userExists(const std::string& username) {
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT COUNT(*) FROM users WHERE username = $1",
            username
        );
        
        txn.commit();
        return r[0][0].as<int>() > 0;
    } catch (const std::exception& e) {
        last_error_ = std::string("User check error: ") + e.what();
        return false;
    }
}

std::vector<User> DatabaseManager::getAllUsers() {
    std::vector<User> users;
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec(
            "SELECT id, username, password_hash, is_admin FROM users ORDER BY username"
        );
        
        for (const auto& row : r) {
            User user;
            user.id = row["id"].as<int>();
            user.username = row["username"].as<std::string>();
            user.password_hash = row["password_hash"].as<std::string>();
            user.is_admin = row["is_admin"].as<bool>();
            users.push_back(user);
        }
        
        txn.commit();
    } catch (const std::exception& e) {
        last_error_ = std::string("Get all users error: ") + e.what();
    }
    
    return users;
}

bool DatabaseManager::deleteUser(int user_id) {
    try {
        pqxx::work txn(*conn_);
        
        // Не даём удалить администратора
        pqxx::result r = txn.exec_params(
            "SELECT is_admin FROM users WHERE id = $1",
            user_id
        );
        
        if (!r.empty() && r[0]["is_admin"].as<bool>()) {
            last_error_ = "Cannot delete admin user";
            return false;
        }
        
        txn.exec_params("DELETE FROM users WHERE id = $1", user_id);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Delete user error: ") + e.what();
        return false;
    }
}

bool DatabaseManager::isAdmin(int user_id) {
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT is_admin FROM users WHERE id = $1",
            user_id
        );
        
        txn.commit();
        return !r.empty() && r[0]["is_admin"].as<bool>();
    } catch (const std::exception& e) {
        return false;
    }
}

int DatabaseManager::getUserGamesCount(int user_id) {
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT COUNT(*) FROM games WHERE user_id = $1",
            user_id
        );
        
        txn.commit();
        return r[0][0].as<int>();
    } catch (const std::exception& e) {
        return 0;
    }
}

bool DatabaseManager::changeUsername(int user_id, const std::string& new_username, const std::string& current_password) {
    try {
        pqxx::work txn(*conn_);
        
        // Проверяем, не занято ли имя
        pqxx::result r = txn.exec_params(
            "SELECT COUNT(*) FROM users WHERE username = $1 AND id != $2",
            new_username, user_id
        );
        
        if (r[0][0].as<int>() > 0) {
            last_error_ = "Пользователь с таким именем уже существует";
            return false;
        }
        
        // Получаем старое имя пользователя
        r = txn.exec_params(
            "SELECT username FROM users WHERE id = $1",
            user_id
        );
        
        if (r.empty()) {
            last_error_ = "Пользователь не найден";
            return false;
        }
        
        // Меняем имя пользователя
        txn.exec_params(
            "UPDATE users SET username = $1 WHERE id = $2",
            new_username, user_id
        );
        
        // Пересчитываем хеш пароля с новым именем как солью
        std::string new_password_hash = HashUtils::hashPassword(current_password, new_username);
        txn.exec_params(
            "UPDATE users SET password_hash = $1 WHERE id = $2",
            new_password_hash, user_id
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Change username error: ") + e.what();
        return false;
    }
}

bool DatabaseManager::changePassword(int user_id, const std::string& new_password_hash) {
    try {
        pqxx::work txn(*conn_);
        
        txn.exec_params(
            "UPDATE users SET password_hash = $1 WHERE id = $2",
            new_password_hash, user_id
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Change password error: ") + e.what();
        return false;
    }
}

bool DatabaseManager::resetAdminCredentials() {
    try {
        pqxx::work txn(*conn_);
        
        // Сбрасываем админа к дефолтным значениям: admin / admin123
        std::string adminHash = HashUtils::hashPassword("admin123", "admin");
        
        txn.exec_params(
            "UPDATE users SET username = 'admin', password_hash = $1 WHERE is_admin = TRUE",
            adminHash
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Reset admin error: ") + e.what();
        return false;
    }
}

bool DatabaseManager::addGame(const Game& game) {
    try {
        pqxx::work txn(*conn_);
        
        txn.exec_params(
            "INSERT INTO games (name, disk_space, ram_usage, vram_required, genre, completed, url, user_id, rating, is_favorite, is_installed, notes, tags) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)",
            game.name, game.disk_space, game.ram_usage, game.vram_required,
            game.genre, game.completed, game.url, game.user_id,
            game.rating, game.is_favorite, game.is_installed, game.notes, game.tags
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Add game error: ") + e.what();
        return false;
    }
}

bool DatabaseManager::updateGame(const Game& game) {
    try {
        pqxx::work txn(*conn_);
        
        txn.exec_params(
            "UPDATE games SET name = $1, disk_space = $2, ram_usage = $3, "
            "vram_required = $4, genre = $5, completed = $6, url = $7, "
            "rating = $8, is_favorite = $9, is_installed = $10, notes = $11, tags = $12 "
            "WHERE id = $13 AND user_id = $14",
            game.name, game.disk_space, game.ram_usage, game.vram_required,
            game.genre, game.completed, game.url,
            game.rating, game.is_favorite, game.is_installed, game.notes, game.tags,
            game.id, game.user_id
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Update game error: ") + e.what();
        return false;
    }
}

bool DatabaseManager::deleteGame(int game_id, int user_id) {
    try {
        pqxx::work txn(*conn_);
        
        txn.exec_params(
            "DELETE FROM games WHERE id = $1 AND user_id = $2",
            game_id, user_id
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Delete game error: ") + e.what();
        return false;
    }
}

bool DatabaseManager::deleteGameByName(const std::string& name, int user_id) {
    try {
        pqxx::work txn(*conn_);
        
        txn.exec_params(
            "DELETE FROM games WHERE name = $1 AND user_id = $2",
            name, user_id
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Delete game by name error: ") + e.what();
        return false;
    }
}

std::vector<Game> DatabaseManager::getAllGames(int user_id) {
    std::vector<Game> games;
    
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT id, name, disk_space, ram_usage, vram_required, genre, completed, url, user_id, "
            "rating, is_favorite, is_installed, notes, tags "
            "FROM games WHERE user_id = $1 ORDER BY name",
            user_id
        );
        
        for (const auto& row : r) {
            Game game;
            game.id = row["id"].as<int>();
            game.name = row["name"].as<std::string>();
            game.disk_space = row["disk_space"].as<double>();
            game.ram_usage = row["ram_usage"].as<double>();
            game.vram_required = row["vram_required"].as<double>();
            game.genre = row["genre"].as<std::string>();
            game.completed = row["completed"].as<bool>();
            game.url = row["url"].is_null() ? "" : row["url"].as<std::string>();
            game.user_id = row["user_id"].as<int>();
            game.rating = row["rating"].is_null() ? -1 : row["rating"].as<int>();
            game.is_favorite = row["is_favorite"].is_null() ? false : row["is_favorite"].as<bool>();
            game.is_installed = row["is_installed"].is_null() ? false : row["is_installed"].as<bool>();
            game.notes = row["notes"].is_null() ? "" : row["notes"].as<std::string>();
            game.tags = row["tags"].is_null() ? "" : row["tags"].as<std::string>();
            games.push_back(game);
        }
        
        txn.commit();
    } catch (const std::exception& e) {
        last_error_ = std::string("Get all games error: ") + e.what();
    }
    
    return games;
}

std::string DatabaseManager::buildFilterCondition(const GameFilter& filter, int user_id) {
    std::stringstream ss;
    ss << "user_id = " << user_id;
    
    if (filter.filter_completed) {
        ss << " AND completed = " << (filter.completed_value ? "TRUE" : "FALSE");
    }
    
    if (filter.filter_genre && !filter.genre_value.empty()) {
        ss << " AND genre = '" << conn_->esc(filter.genre_value) << "'";
    }
    
    if (filter.filter_disk_space_min) {
        ss << " AND disk_space >= " << filter.disk_space_min;
    }
    
    if (filter.filter_disk_space_max) {
        ss << " AND disk_space <= " << filter.disk_space_max;
    }
    
    if (filter.filter_ram_min) {
        ss << " AND ram_usage >= " << filter.ram_min;
    }
    
    if (filter.filter_ram_max) {
        ss << " AND ram_usage <= " << filter.ram_max;
    }
    
    if (filter.filter_vram_min) {
        ss << " AND vram_required >= " << filter.vram_min;
    }
    
    if (filter.filter_vram_max) {
        ss << " AND vram_required <= " << filter.vram_max;
    }
    
    // Новые фильтры
    if (filter.filter_tag && !filter.tag_value.empty()) {
        // Ищем тег в списке тегов (через LIKE с разделителями)
        ss << " AND (tags LIKE '%" << conn_->esc(filter.tag_value) << "%')";
    }
    
    if (filter.filter_favorite) {
        ss << " AND is_favorite = " << (filter.favorite_value ? "TRUE" : "FALSE");
    }
    
    if (filter.filter_installed) {
        ss << " AND is_installed = " << (filter.installed_value ? "TRUE" : "FALSE");
    }
    
    if (filter.filter_rating_min) {
        ss << " AND rating >= " << filter.rating_min;
    }
    
    if (filter.filter_rating_max) {
        ss << " AND rating <= " << filter.rating_max << " AND rating >= 0";
    }
    
    if (filter.filter_has_rating) {
        if (filter.has_rating_value) {
            ss << " AND rating >= 0";  // Только с оценкой
        } else {
            ss << " AND rating = -1";  // Только без оценки
        }
    }
    
    return ss.str();
}

std::vector<Game> DatabaseManager::getFilteredGames(int user_id, const GameFilter& filter) {
    std::vector<Game> games;
    
    try {
        pqxx::work txn(*conn_);
        
        std::string query = 
            "SELECT id, name, disk_space, ram_usage, vram_required, genre, completed, url, user_id, "
            "rating, is_favorite, is_installed, notes, tags "
            "FROM games WHERE " + buildFilterCondition(filter, user_id) + " ORDER BY name";
        
        pqxx::result r = txn.exec(query);
        
        for (const auto& row : r) {
            Game game;
            game.id = row["id"].as<int>();
            game.name = row["name"].as<std::string>();
            game.disk_space = row["disk_space"].as<double>();
            game.ram_usage = row["ram_usage"].as<double>();
            game.vram_required = row["vram_required"].as<double>();
            game.genre = row["genre"].as<std::string>();
            game.completed = row["completed"].as<bool>();
            game.url = row["url"].is_null() ? "" : row["url"].as<std::string>();
            game.user_id = row["user_id"].as<int>();
            game.rating = row["rating"].is_null() ? -1 : row["rating"].as<int>();
            game.is_favorite = row["is_favorite"].is_null() ? false : row["is_favorite"].as<bool>();
            game.is_installed = row["is_installed"].is_null() ? false : row["is_installed"].as<bool>();
            game.notes = row["notes"].is_null() ? "" : row["notes"].as<std::string>();
            game.tags = row["tags"].is_null() ? "" : row["tags"].as<std::string>();
            games.push_back(game);
        }
        
        txn.commit();
    } catch (const std::exception& e) {
        last_error_ = std::string("Get filtered games error: ") + e.what();
    }
    
    return games;
}

Game DatabaseManager::getGameById(int game_id, int user_id) {
    Game game;
    
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT id, name, disk_space, ram_usage, vram_required, genre, completed, url, user_id, "
            "rating, is_favorite, is_installed, notes, tags "
            "FROM games WHERE id = $1 AND user_id = $2",
            game_id, user_id
        );
        
        if (!r.empty()) {
            game.id = r[0]["id"].as<int>();
            game.name = r[0]["name"].as<std::string>();
            game.disk_space = r[0]["disk_space"].as<double>();
            game.ram_usage = r[0]["ram_usage"].as<double>();
            game.vram_required = r[0]["vram_required"].as<double>();
            game.genre = r[0]["genre"].as<std::string>();
            game.completed = r[0]["completed"].as<bool>();
            game.url = r[0]["url"].is_null() ? "" : r[0]["url"].as<std::string>();
            game.user_id = r[0]["user_id"].as<int>();
            game.rating = r[0]["rating"].is_null() ? -1 : r[0]["rating"].as<int>();
            game.is_favorite = r[0]["is_favorite"].is_null() ? false : r[0]["is_favorite"].as<bool>();
            game.is_installed = r[0]["is_installed"].is_null() ? false : r[0]["is_installed"].as<bool>();
            game.notes = r[0]["notes"].is_null() ? "" : r[0]["notes"].as<std::string>();
            game.tags = r[0]["tags"].is_null() ? "" : r[0]["tags"].as<std::string>();
        }
        
        txn.commit();
    } catch (const std::exception& e) {
        last_error_ = std::string("Get game by ID error: ") + e.what();
    }
    
    return game;
}

Game DatabaseManager::getGameByName(const std::string& name, int user_id) {
    Game game;
    
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT id, name, disk_space, ram_usage, vram_required, genre, completed, url, user_id, "
            "rating, is_favorite, is_installed, notes, tags "
            "FROM games WHERE name = $1 AND user_id = $2",
            name, user_id
        );
        
        if (!r.empty()) {
            game.id = r[0]["id"].as<int>();
            game.name = r[0]["name"].as<std::string>();
            game.disk_space = r[0]["disk_space"].as<double>();
            game.ram_usage = r[0]["ram_usage"].as<double>();
            game.vram_required = r[0]["vram_required"].as<double>();
            game.genre = r[0]["genre"].as<std::string>();
            game.completed = r[0]["completed"].as<bool>();
            game.url = r[0]["url"].is_null() ? "" : r[0]["url"].as<std::string>();
            game.user_id = r[0]["user_id"].as<int>();
            game.rating = r[0]["rating"].is_null() ? -1 : r[0]["rating"].as<int>();
            game.is_favorite = r[0]["is_favorite"].is_null() ? false : r[0]["is_favorite"].as<bool>();
            game.is_installed = r[0]["is_installed"].is_null() ? false : r[0]["is_installed"].as<bool>();
            game.notes = r[0]["notes"].is_null() ? "" : r[0]["notes"].as<std::string>();
            game.tags = r[0]["tags"].is_null() ? "" : r[0]["tags"].as<std::string>();
        }
        
        txn.commit();
    } catch (const std::exception& e) {
        last_error_ = std::string("Get game by name error: ") + e.what();
    }
    
    return game;
}

bool DatabaseManager::writeGamesToFile(const std::string& filename, const std::vector<Game>& games) {
    try {
        std::vector<BinaryGameRecord> records;
        records.reserve(games.size());
        
        for (const auto& game : games) {
            BinaryGameRecord record;
            std::memset(&record, 0, sizeof(record));
            
            record.id = game.id;
            std::strncpy(record.name, game.name.c_str(), sizeof(record.name) - 1);
            record.disk_space = game.disk_space;
            record.ram_usage = game.ram_usage;
            record.vram_required = game.vram_required;
            std::strncpy(record.genre, game.genre.c_str(), sizeof(record.genre) - 1);
            record.completed = game.completed ? 1 : 0;
            std::strncpy(record.url, game.url.c_str(), sizeof(record.url) - 1);
            record.user_id = game.user_id;
            record.rating = game.rating;
            record.is_favorite = game.is_favorite ? 1 : 0;
            record.is_installed = game.is_installed ? 1 : 0;
            std::strncpy(record.notes, game.notes.c_str(), sizeof(record.notes) - 1);
            std::strncpy(record.tags, game.tags.c_str(), sizeof(record.tags) - 1);
            
            records.push_back(record);
        }
        
        std::string hash;
        if (!records.empty()) {
            hash = HashUtils::sha256(
                reinterpret_cast<const char*>(records.data()), 
                records.size() * sizeof(BinaryGameRecord)
            );
        } else {
            hash = HashUtils::sha256("", 0);
        }
        
        BinaryFileHeader header;
        header.magic = FILE_MAGIC;
        header.version = FILE_VERSION;
        header.record_count = static_cast<uint32_t>(games.size());
        std::memset(header.hash, 0, sizeof(header.hash));
        std::memcpy(header.hash, hash.c_str(), std::min(hash.length(), sizeof(header.hash)));
        
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            last_error_ = "Cannot open file for writing: " + filename;
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        
        if (!records.empty()) {
            file.write(reinterpret_cast<const char*>(records.data()), 
                       records.size() * sizeof(BinaryGameRecord));
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Write file error: ") + e.what();
        return false;
    }
}

bool DatabaseManager::exportToBinaryFile(const std::string& filename, int user_id) {
    std::vector<Game> games = getAllGames(user_id);
    return writeGamesToFile(filename, games);
}

bool DatabaseManager::exportFilteredToBinaryFile(const std::string& filename, int user_id,
                                                  const GameFilter& filter) {
    std::vector<Game> games = getFilteredGames(user_id, filter);
    return writeGamesToFile(filename, games);
}

FileVerificationResult DatabaseManager::verifyBinaryFile(const std::string& filename) {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return FileVerificationResult::FILE_NOT_FOUND;
        }
        
        BinaryFileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        if (!file.good()) {
            return FileVerificationResult::READ_ERROR;
        }
        
        if (header.magic != FILE_MAGIC) {
            return FileVerificationResult::INVALID_MAGIC;
        }
        
        // Поддержка версий 1 и 2
        if (header.version != FILE_VERSION && header.version != 1) {
            return FileVerificationResult::INVALID_VERSION;
        }
        
        std::vector<BinaryGameRecord> records(header.record_count);
        
        if (header.record_count > 0) {
            file.read(reinterpret_cast<char*>(records.data()), 
                      header.record_count * sizeof(BinaryGameRecord));
            
            if (!file.good()) {
                return FileVerificationResult::READ_ERROR;
            }
        }
        
        file.close();
        
        std::string calculated_hash;
        if (!records.empty()) {
            calculated_hash = HashUtils::sha256(
                reinterpret_cast<const char*>(records.data()), 
                records.size() * sizeof(BinaryGameRecord)
            );
        } else {
            calculated_hash = HashUtils::sha256("", 0);
        }
        
        std::string stored_hash(header.hash, sizeof(header.hash));
        
        if (calculated_hash != stored_hash) {
            return FileVerificationResult::HASH_MISMATCH;
        }
        
        return FileVerificationResult::OK;
    } catch (const std::exception& e) {
        last_error_ = std::string("Verification error: ") + e.what();
        return FileVerificationResult::READ_ERROR;
    }
}

std::string DatabaseManager::getVerificationErrorText(FileVerificationResult result) {
    switch (result) {
        case FileVerificationResult::OK:
            return "Файл корректен";
        case FileVerificationResult::FILE_NOT_FOUND:
            return "Файл не найден";
        case FileVerificationResult::INVALID_MAGIC:
            return "Неверный формат файла (не является файлом Temporium)";
        case FileVerificationResult::INVALID_VERSION:
            return "Неподдерживаемая версия формата файла";
        case FileVerificationResult::HASH_MISMATCH:
            return "Файл поврежден или модифицирован (контрольная сумма не совпадает)";
        case FileVerificationResult::READ_ERROR:
            return "Ошибка чтения файла";
        default:
            return "Неизвестная ошибка";
    }
}

bool DatabaseManager::importFromBinaryFile(const std::string& filename, int user_id) {
    FileVerificationResult verification = verifyBinaryFile(filename);
    if (verification != FileVerificationResult::OK) {
        last_error_ = getVerificationErrorText(verification);
        return false;
    }
    
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            last_error_ = "Cannot open file for reading: " + filename;
            return false;
        }
        
        BinaryFileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        for (uint32_t i = 0; i < header.record_count; ++i) {
            BinaryGameRecord record;
            file.read(reinterpret_cast<char*>(&record), sizeof(record));
            
            Game game;
            game.name = record.name;
            game.disk_space = record.disk_space;
            game.ram_usage = record.ram_usage;
            game.vram_required = record.vram_required;
            game.genre = record.genre;
            game.completed = record.completed != 0;
            game.url = record.url;
            game.user_id = user_id;
            
            addGame(game);
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Import error: ") + e.what();
        return false;
    }
}

std::vector<Game> DatabaseManager::readBinaryFile(const std::string& filename) {
    std::vector<Game> games;
    
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            last_error_ = "Cannot open file for reading: " + filename;
            return games;
        }
        
        BinaryFileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        if (header.magic != FILE_MAGIC) {
            last_error_ = "Invalid file format";
            file.close();
            return games;
        }
        
        for (uint32_t i = 0; i < header.record_count; ++i) {
            BinaryGameRecord record;
            file.read(reinterpret_cast<char*>(&record), sizeof(record));
            
            Game game;
            game.id = record.id;
            game.name = record.name;
            game.disk_space = record.disk_space;
            game.ram_usage = record.ram_usage;
            game.vram_required = record.vram_required;
            game.genre = record.genre;
            game.completed = record.completed != 0;
            game.url = record.url;
            game.user_id = record.user_id;
            game.rating = record.rating;
            game.is_favorite = record.is_favorite != 0;
            game.is_installed = record.is_installed != 0;
            game.notes = record.notes;
            game.tags = record.tags;
            
            games.push_back(game);
        }
        
        file.close();
    } catch (const std::exception& e) {
        last_error_ = std::string("Read binary file error: ") + e.what();
    }
    
    return games;
}

std::vector<std::string> DatabaseManager::getUserTags(int user_id) {
    std::vector<std::string> tags;
    std::set<std::string> uniqueTags;
    
    try {
        pqxx::work txn(*conn_);
        
        pqxx::result r = txn.exec_params(
            "SELECT DISTINCT tags FROM games WHERE user_id = $1 AND tags != ''",
            user_id
        );
        
        for (const auto& row : r) {
            std::string tagStr = row["tags"].as<std::string>();
            // Разбиваем теги по запятой
            std::stringstream ss(tagStr);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                // Trim whitespace
                size_t start = tag.find_first_not_of(" \t");
                size_t end = tag.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    uniqueTags.insert(tag.substr(start, end - start + 1));
                }
            }
        }
        
        txn.commit();
        
        // Копируем в вектор
        tags.assign(uniqueTags.begin(), uniqueTags.end());
    } catch (const std::exception& e) {
        last_error_ = std::string("Get user tags error: ") + e.what();
    }
    
    return tags;
}

bool DatabaseManager::updateGameNotes(int game_id, int user_id, const std::string& notes) {
    try {
        pqxx::work txn(*conn_);
        
        txn.exec_params(
            "UPDATE games SET notes = $1 WHERE id = $2 AND user_id = $3",
            notes, game_id, user_id
        );
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        last_error_ = std::string("Update notes error: ") + e.what();
        return false;
    }
}

GameStats DatabaseManager::getGameStats(int user_id) {
    GameStats stats;
    
    try {
        pqxx::work txn(*conn_);
        
        // Общее количество игр
        pqxx::result r = txn.exec_params(
            "SELECT COUNT(*) FROM games WHERE user_id = $1",
            user_id
        );
        stats.total_games = r[0][0].as<int>();
        
        // Количество избранных
        r = txn.exec_params(
            "SELECT COUNT(*) FROM games WHERE user_id = $1 AND is_favorite = TRUE",
            user_id
        );
        stats.favorites_count = r[0][0].as<int>();
        
        // Количество пройденных
        r = txn.exec_params(
            "SELECT COUNT(*) FROM games WHERE user_id = $1 AND completed = TRUE",
            user_id
        );
        stats.completed_count = r[0][0].as<int>();
        
        // Количество без оценки
        r = txn.exec_params(
            "SELECT COUNT(*) FROM games WHERE user_id = $1 AND rating = -1",
            user_id
        );
        stats.no_rating_count = r[0][0].as<int>();
        
        // Количество установленных
        r = txn.exec_params(
            "SELECT COUNT(*) FROM games WHERE user_id = $1 AND is_installed = TRUE",
            user_id
        );
        stats.installed_count = r[0][0].as<int>();
        
        // Занимаемое место установленными играми
        r = txn.exec_params(
            "SELECT COALESCE(SUM(disk_space), 0) FROM games WHERE user_id = $1 AND is_installed = TRUE",
            user_id
        );
        stats.installed_disk_space = r[0][0].as<double>();
        
        // Количество без ссылки
        r = txn.exec_params(
            "SELECT COUNT(*) FROM games WHERE user_id = $1 AND (url IS NULL OR url = '')",
            user_id
        );
        stats.no_url_count = r[0][0].as<int>();
        
        txn.commit();
    } catch (const std::exception& e) {
        last_error_ = std::string("Get stats error: ") + e.what();
    }
    
    return stats;
}

std::string DatabaseManager::getLastError() const {
    return last_error_;
}

} // namespace Temporium
