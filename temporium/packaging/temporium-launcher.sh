#!/bin/bash
# Temporium Launcher Script
# Запускает PostgreSQL в Docker и GUI приложение

SCRIPT_DIR="/usr/share/temporium"
DATA_DIR="$HOME/.local/share/temporium"

# Цвета
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Создаём директорию для данных
mkdir -p "$DATA_DIR"

# Проверка Docker
if ! command -v docker &> /dev/null; then
    echo -e "${RED}Docker не установлен!${NC}"
    echo "Установите Docker: sudo apt install docker.io"
    exit 1
fi

# Проверка прав на Docker
if ! docker info &> /dev/null; then
    echo -e "${YELLOW}Нет прав на Docker. Добавьте пользователя в группу docker:${NC}"
    echo "  sudo usermod -aG docker \$USER"
    echo "  (требуется перезайти в систему)"
    exit 1
fi

# Запуск PostgreSQL если не запущен
if ! docker ps --format '{{.Names}}' | grep -q "^temporium-db$"; then
    echo -e "${YELLOW}Запуск базы данных PostgreSQL...${NC}"
    
    # Удаляем старый контейнер если есть
    docker rm temporium-db 2>/dev/null || true
    
    docker run -d \
        --name temporium-db \
        -e POSTGRES_USER=postgres \
        -e POSTGRES_PASSWORD=postgres123 \
        -e POSTGRES_DB=gamedb \
        -p 5432:5432 \
        -v temporium-pgdata:/var/lib/postgresql/data \
        --restart unless-stopped \
        postgres:15-alpine
    
    # Ожидание готовности БД
    echo -n "Ожидание готовности БД"
    for i in {1..30}; do
        if docker exec temporium-db pg_isready -U postgres -d gamedb &>/dev/null; then
            echo -e " ${GREEN}OK${NC}"
            break
        fi
        echo -n "."
        sleep 1
    done
fi

# Переменные окружения для подключения к БД
export DB_HOST="localhost"
export DB_PORT="5432"
export DB_NAME="gamedb"
export DB_USER="postgres"
export DB_PASSWORD="postgres123"

# Запуск GUI
exec "$SCRIPT_DIR/Temporium" "$@"
