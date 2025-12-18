-- Temporium - СУБД Компьютерные Игры
-- Инициализация базы данных

-- Создание таблицы пользователей
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(64) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы игр
CREATE TABLE IF NOT EXISTS games (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    disk_space DOUBLE PRECISION NOT NULL,
    ram_usage DOUBLE PRECISION NOT NULL,
    vram_required DOUBLE PRECISION NOT NULL,
    genre VARCHAR(64) NOT NULL,
    completed BOOLEAN DEFAULT FALSE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(name, user_id)
);

-- Индексы
CREATE INDEX IF NOT EXISTS idx_games_user_id ON games(user_id);
CREATE INDEX IF NOT EXISTS idx_games_genre ON games(genre);
CREATE INDEX IF NOT EXISTS idx_games_completed ON games(completed);
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);

SELECT 'База данных Temporium готова!' AS status;
