cmake_minimum_required(VERSION 3.16)
project(Temporium VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Поиск Qt5
find_package(Qt5 COMPONENTS Widgets Svg REQUIRED)

# Поиск OpenSSL для хэширования
find_package(OpenSSL REQUIRED)

# Поиск libpqxx для PostgreSQL
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
pkg_check_modules(PQ REQUIRED libpq)

# Включение директорий
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${PQXX_INCLUDE_DIRS}
    ${PQ_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/database_manager.cpp
)

# Заголовочные файлы
set(HEADERS
    include/mainwindow.h
    include/database_manager.h
    include/types.h
    include/hash_utils.h
)

# Ресурсы
set(RESOURCES
    resources/resources.qrc
)

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

# Линковка библиотек
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Svg
    ${PQXX_LIBRARIES}
    ${PQ_LIBRARIES}
    OpenSSL::Crypto
)

# Установка
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES resources/temporium.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES packaging/temporium.desktop DESTINATION share/applications)
