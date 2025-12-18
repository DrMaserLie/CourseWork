#!/bin/bash
# Temporium DEB Package Builder

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
PKG_DIR="$PROJECT_DIR/temporium-deb"
VERSION="4.1.0"

# Цвета
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║              Создание DEB-пакета Temporium                   ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# Проверка сборки
if [ ! -f "$BUILD_DIR/Temporium" ]; then
    echo -e "${RED}Приложение не собрано! Сначала выполните: ./run.sh build${NC}"
    exit 1
fi

# Очистка старой директории
rm -rf "$PKG_DIR"

# Создание структуры директорий
echo -e "${YELLOW}Создание структуры пакета...${NC}"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/usr/share/temporium"
mkdir -p "$PKG_DIR/usr/share/applications"
mkdir -p "$PKG_DIR/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$PKG_DIR/usr/share/doc/temporium"

# Копирование файлов DEBIAN
cp "$SCRIPT_DIR/debian/DEBIAN/control" "$PKG_DIR/DEBIAN/"
cp "$SCRIPT_DIR/debian/DEBIAN/postinst" "$PKG_DIR/DEBIAN/"
cp "$SCRIPT_DIR/debian/DEBIAN/prerm" "$PKG_DIR/DEBIAN/"
chmod 755 "$PKG_DIR/DEBIAN/postinst"
chmod 755 "$PKG_DIR/DEBIAN/prerm"

# Обновление версии в control
sed -i "s/Version:.*/Version: $VERSION/" "$PKG_DIR/DEBIAN/control"

# Определение архитектуры
ARCH=$(dpkg --print-architecture)
sed -i "s/Architecture:.*/Architecture: $ARCH/" "$PKG_DIR/DEBIAN/control"

# Автоопределение версии libpqxx
PQXX_PKG=$(dpkg -l | grep -oP 'libpqxx-\d+\.\d+' | head -1)
if [ -n "$PQXX_PKG" ]; then
    echo -e "${YELLOW}Обнаружен пакет: $PQXX_PKG${NC}"
    # Добавляем текущую версию первой в список зависимостей
    sed -i "s/libpqxx-7.9 | libpqxx-7.8 | libpqxx-6.4/$PQXX_PKG | libpqxx-7.9 | libpqxx-7.8 | libpqxx-6.4/" "$PKG_DIR/DEBIAN/control"
fi

# Копирование исполняемого файла
echo -e "${YELLOW}Копирование файлов приложения...${NC}"
cp "$BUILD_DIR/Temporium" "$PKG_DIR/usr/share/temporium/"
chmod 755 "$PKG_DIR/usr/share/temporium/Temporium"

# Копирование launcher скрипта
cp "$SCRIPT_DIR/temporium-launcher.sh" "$PKG_DIR/usr/bin/temporium"
chmod 755 "$PKG_DIR/usr/bin/temporium"

# Копирование desktop файла
cp "$SCRIPT_DIR/temporium.desktop" "$PKG_DIR/usr/share/applications/"
chmod 644 "$PKG_DIR/usr/share/applications/temporium.desktop"

# Копирование иконки
cp "$PROJECT_DIR/resources/temporium.svg" "$PKG_DIR/usr/share/icons/hicolor/scalable/apps/"
chmod 644 "$PKG_DIR/usr/share/icons/hicolor/scalable/apps/temporium.svg"

# Создание документации
cat > "$PKG_DIR/usr/share/doc/temporium/README" << 'EOF'
Temporium - СУБД Компьютерные Игры
==================================

Десктопное приложение для управления коллекцией компьютерных игр.

Курсовая работа по дисциплине "Программирование"
ФГБОУ ВО "Новосибирский государственный технический университет"
Кафедра "Защиты информации" 
Группа АБ-422, Тюриков Максим Олегович

Возможности:
- Аутентификация пользователей с хэшированием паролей (SHA-256)
- CRUD операции для игр
- Фильтрация и сортировка
- Экспорт/импорт в бинарный формат
- База данных PostgreSQL через Docker
- Тёмная тема интерфейса

Первый запуск:
  Логин: admin
  Пароль: admin123

Для работы требуется Docker.
EOF

# Создание файла copyright
cat > "$PKG_DIR/usr/share/doc/temporium/copyright" << 'EOF'
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: temporium
Source: https://github.com/nstu/temporium

Files: *
Copyright: 2025 NSTU Tyurikov Maxim Olegovich
License: MIT

License: MIT
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:
 .
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 .
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
EOF

# Установка правильных прав
find "$PKG_DIR" -type d -exec chmod 755 {} \;
find "$PKG_DIR/usr" -type f -exec chmod 644 {} \;
chmod 755 "$PKG_DIR/usr/bin/temporium"
chmod 755 "$PKG_DIR/usr/share/temporium/Temporium"

# Вычисление установленного размера
INSTALLED_SIZE=$(du -sk "$PKG_DIR/usr" | cut -f1)
sed -i "/^Depends:/a Installed-Size: $INSTALLED_SIZE" "$PKG_DIR/DEBIAN/control"

# Сборка пакета
echo -e "${YELLOW}Сборка DEB-пакета...${NC}"
DEB_FILE="$PROJECT_DIR/temporium_${VERSION}_${ARCH}.deb"
dpkg-deb --build "$PKG_DIR" "$DEB_FILE"

# Проверка пакета
echo -e "${YELLOW}Проверка пакета...${NC}"
if command -v lintian &> /dev/null; then
    lintian --no-tag-display-limit "$DEB_FILE" 2>/dev/null || true
fi

# Очистка
rm -rf "$PKG_DIR"

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║              DEB-пакет создан успешно!                       ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "Файл: ${CYAN}$DEB_FILE${NC}"
echo ""
echo "Установка:"
echo "  sudo dpkg -i $DEB_FILE"
echo "  sudo apt install -f  # для установки зависимостей"
echo ""
echo "Удаление:"
echo "  sudo dpkg -r temporium"
echo ""
