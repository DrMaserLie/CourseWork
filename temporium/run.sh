#!/bin/bash

# Temporium - СУБД Компьютерные Игры
# Курсовая работа по дисциплине "Программирование"

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Цвета
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                   ⏳ TEMPORIUM ⏳                            ║"
echo "║              СУБД Компьютерные Игры                          ║"
echo "║         Курсовая работа - Программирование                   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Определение docker compose
DOCKER_COMPOSE=""
detect_docker_compose() {
    if command -v docker-compose &> /dev/null; then
        DOCKER_COMPOSE="docker-compose"
    elif docker compose version &> /dev/null 2>&1; then
        DOCKER_COMPOSE="docker compose"
    else
        echo -e "${RED}Docker Compose не найден!${NC}"
        exit 1
    fi
}

show_help() {
    echo "Использование: $0 [команда]"
    echo ""
    echo "Команды:"
    echo "  install     - Установить зависимости (требуется sudo)"
    echo "  build       - Собрать приложение (+ создать ярлык)"
    echo "  run         - Запустить приложение (БД + GUI)"
    echo "  app         - Запустить только GUI (БД должна быть запущена)"
    echo "  db-start    - Запустить базу данных PostgreSQL"
    echo "  db-stop     - Остановить базу данных"
    echo "  db-shell    - Подключиться к БД через psql"
    echo "  desktop     - Пересоздать ярлык на рабочем столе"
    echo "  reset-admin - Сбросить админа к admin/admin123"
    echo "  deb         - Создать DEB-пакет для установки"
    echo "  clean       - Удалить сборку и данные БД"
    echo "  help        - Показать эту справку"
    echo ""
    echo "Быстрый старт:"
    echo "  $0 install   # Один раз - установка зависимостей"
    echo "  $0 build     # Сборка приложения + создание ярлыка"
    echo "  $0 run       # Запуск (БД + приложение)"
    echo ""
}

install_deps() {
    echo -e "${GREEN}Установка зависимостей...${NC}"
    
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        pkg-config \
        qtbase5-dev \
        libqt5svg5-dev \
        libpqxx-dev \
        libssl-dev \
        libpq-dev
    
    echo -e "${GREEN}✓ Зависимости установлены${NC}"
}

build_app() {
    echo -e "${GREEN}Сборка приложения...${NC}"
    
    # Проверка зависимостей
    MISSING=""
    command -v cmake &> /dev/null || MISSING="$MISSING cmake"
    pkg-config --exists libpqxx 2>/dev/null || MISSING="$MISSING libpqxx-dev"
    pkg-config --exists Qt5Widgets 2>/dev/null || MISSING="$MISSING qtbase5-dev"
    pkg-config --exists openssl 2>/dev/null || MISSING="$MISSING libssl-dev"
    
    if [ -n "$MISSING" ]; then
        echo -e "${RED}Отсутствуют зависимости:$MISSING${NC}"
        echo "Выполните: $0 install"
        exit 1
    fi
    
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    cd ..
    
    echo -e "${GREEN}✓ Сборка завершена!${NC}"
    echo ""
    echo "Исполняемый файл: ${SCRIPT_DIR}/build/Temporium"
    
    # Автоматическое создание ярлыка на рабочем столе
    echo ""
    install_desktop_auto
}

wait_for_db() {
    echo -e "${YELLOW}Ожидание готовности PostgreSQL...${NC}"
    
    local max_attempts=30
    local attempt=1
    
    while [ $attempt -le $max_attempts ]; do
        if docker exec temporium-db pg_isready -U postgres -d gamedb &>/dev/null; then
            echo -e "${GREEN}✓ PostgreSQL готов!${NC}"
            return 0
        fi
        
        echo -ne "\r  Попытка $attempt/$max_attempts..."
        sleep 1
        ((attempt++))
    done
    
    echo ""
    echo -e "${RED}PostgreSQL не запустился за $max_attempts секунд${NC}"
    echo "Проверьте логи: docker logs temporium-db"
    return 1
}

db_start() {
    echo -e "${GREEN}Запуск PostgreSQL...${NC}"
    detect_docker_compose
    
    cd docker
    $DOCKER_COMPOSE down --remove-orphans 2>/dev/null || true
    $DOCKER_COMPOSE up -d
    cd ..
    
    wait_for_db
}

db_stop() {
    echo -e "${YELLOW}Остановка PostgreSQL...${NC}"
    detect_docker_compose
    
    cd docker
    $DOCKER_COMPOSE down --remove-orphans
    cd ..
    
    echo -e "${GREEN}✓ PostgreSQL остановлен${NC}"
}

db_shell() {
    echo -e "${BLUE}Подключение к базе данных...${NC}"
    docker exec -it temporium-db psql -U postgres -d gamedb
}

run_app() {
    if [ ! -f "build/Temporium" ]; then
        echo -e "${YELLOW}Приложение не собрано. Собираем...${NC}"
        build_app
    fi
    
    db_start
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Не удалось запустить БД${NC}"
        exit 1
    fi
    
    sleep 2
    
    echo -e "${GREEN}Запуск Temporium...${NC}"
    ./build/Temporium
}

run_app_only() {
    if [ ! -f "build/Temporium" ]; then
        echo -e "${RED}Приложение не собрано!${NC}"
        echo "Выполните: $0 build"
        exit 1
    fi
    
    if ! docker exec temporium-db pg_isready -U postgres -d gamedb &>/dev/null; then
        echo -e "${YELLOW}БД не запущена. Запускаем...${NC}"
        db_start
        sleep 2
    fi
    
    echo -e "${GREEN}Запуск Temporium...${NC}"
    ./build/Temporium
}

install_desktop_auto() {
    echo -e "${YELLOW}Создание ярлыка на рабочем столе...${NC}"
    
    # Определение директории рабочего стола
    DESKTOP_DIR="$HOME/Desktop"
    if [ -f "$HOME/.config/user-dirs.dirs" ]; then
        source "$HOME/.config/user-dirs.dirs"
        DESKTOP_DIR="${XDG_DESKTOP_DIR:-$HOME/Desktop}"
    fi
    
    # Создание директорий
    mkdir -p "$HOME/.local/share/icons"
    mkdir -p "$HOME/.local/share/applications"
    mkdir -p "$DESKTOP_DIR"
    
    # Копирование иконки
    cp "$SCRIPT_DIR/resources/temporium.svg" "$HOME/.local/share/icons/temporium.svg"
    
    # Создание launcher скрипта
    LAUNCHER_SCRIPT="$HOME/.local/bin/temporium-launcher.sh"
    mkdir -p "$HOME/.local/bin"
    
    cat > "$LAUNCHER_SCRIPT" << LAUNCHER
#!/bin/bash
# Temporium Launcher
cd "$SCRIPT_DIR"
exec ./run.sh run
LAUNCHER
    chmod +x "$LAUNCHER_SCRIPT"
    
    # Создание .desktop файла
    DESKTOP_FILE="$DESKTOP_DIR/Temporium.desktop"
    cat > "$DESKTOP_FILE" << DESKTOP
[Desktop Entry]
Version=2.0
Type=Application
Name=Temporium
GenericName=Game Database
Comment=СУБД Компьютерные Игры
Exec=$LAUNCHER_SCRIPT
Icon=$HOME/.local/share/icons/temporium.svg
Terminal=false
Categories=Office;Database;Qt;
Keywords=games;database;collection;
StartupNotify=true
StartupWMClass=Temporium
DESKTOP
    chmod +x "$DESKTOP_FILE"
    
    # Для GNOME нужно пометить файл как доверенный
    if command -v gio &> /dev/null; then
        gio set "$DESKTOP_FILE" metadata::trusted true 2>/dev/null || true
    fi
    
    # Копирование в applications для меню приложений
    cp "$DESKTOP_FILE" "$HOME/.local/share/applications/temporium.desktop"
    
    # Обновление базы данных desktop файлов
    if command -v update-desktop-database &> /dev/null; then
        update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
    fi
    
    echo -e "${GREEN}✓ Ярлык создан на рабочем столе!${NC}"
    echo "  Файл: $DESKTOP_FILE"
}

install_desktop() {
    if [ ! -f "$SCRIPT_DIR/build/Temporium" ]; then
        echo -e "${RED}Приложение не собрано! Сначала выполните: ./run.sh build${NC}"
        exit 1
    fi
    
    install_desktop_auto
}

build_deb() {
    echo -e "${GREEN}Создание DEB-пакета...${NC}"
    
    if [ ! -f "$SCRIPT_DIR/build/Temporium" ]; then
        echo -e "${RED}Приложение не собрано! Сначала выполните: ./run.sh build${NC}"
        exit 1
    fi
    
    if [ ! -f "$SCRIPT_DIR/packaging/build-deb.sh" ]; then
        echo -e "${RED}Скрипт сборки DEB-пакета не найден!${NC}"
        exit 1
    fi
    
    chmod +x "$SCRIPT_DIR/packaging/build-deb.sh"
    bash "$SCRIPT_DIR/packaging/build-deb.sh"
}

clean_all() {
    echo -e "${RED}Очистка...${NC}"
    read -p "Удалить сборку и данные БД? (y/n) " -n 1 -r
    echo
    
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf build
        
        detect_docker_compose
        cd docker
        $DOCKER_COMPOSE down -v --remove-orphans 2>/dev/null || true
        cd ..
        
        docker rm -f jumandji-app jumandji-db 2>/dev/null || true
        docker volume rm docker_postgres_data docker_jumandji_data 2>/dev/null || true
        
        echo -e "${GREEN}✓ Очистка завершена${NC}"
    fi
}

reset_admin() {
    echo -e "${YELLOW}Сброс учётных данных администратора...${NC}"
    
    if ! docker exec temporium-db pg_isready -U postgres -d gamedb &>/dev/null; then
        echo -e "${YELLOW}БД не запущена. Запускаем...${NC}"
        db_start
        sleep 2
    fi
    
    # Вычисляем хеш пароля admin123 с солью admin
    # SHA256("admin123" + "admin") в hex
    ADMIN_HASH=$(echo -n "admin123admin" | sha256sum | cut -d' ' -f1)
    
    docker exec temporium-db psql -U postgres -d gamedb -c \
        "UPDATE users SET username = 'admin', password_hash = '$ADMIN_HASH' WHERE is_admin = TRUE;"
    
    echo -e "${GREEN}✓ Учётные данные администратора сброшены!${NC}"
    echo ""
    echo "Логин: admin"
    echo "Пароль: admin123"
}

# Обработка команд
case "${1:-help}" in
    install)
        install_deps
        ;;
    build)
        build_app
        ;;
    db-start)
        db_start
        ;;
    db-stop)
        db_stop
        ;;
    db-shell)
        db_shell
        ;;
    run)
        run_app
        ;;
    app)
        run_app_only
        ;;
    desktop)
        install_desktop
        ;;
    reset-admin)
        reset_admin
        ;;
    deb)
        build_deb
        ;;
    clean)
        clean_all
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo -e "${RED}Неизвестная команда: $1${NC}"
        show_help
        exit 1
        ;;
esac
