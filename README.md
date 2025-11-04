# OSISP - Операционные среды и системное программирование

Лабораторные работы по курсу "Операционные среды и системное программирование".

## Требования

- **mingw-w64** - кросс-компилятор для Windows
- **Wine** - для запуска Windows приложений на Linux
- **CMake** - система сборки (версия 3.10 или выше)

## Установка зависимостей

```bash
# Ubuntu/Debian
sudo apt install mingw-w64 wine cmake

# Проверка установки
x86_64-w64-mingw32-gcc --version
wine --version
cmake --version
```

## Сборка проекта

```bash
# Конфигурация с использованием CMake и toolchain файла
cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake

# Сборка всех лабораторных работ
cmake --build build

# Или сборка конкретной цели
cmake --build build --target self_restoring
```

## Структура проекта

```
OSISP/
├── CMakeLists.txt              # Главный CMake файл
├── toolchain-mingw64.cmake     # Toolchain для кросс-компиляции
├── Lab_1/                      # Лабораторная работа 1
│   ├── CMakeLists.txt
│   ├── Readme.md
│   └── task1_self_restoring/   # Задание 1: Самовосстанавливающийся процесс
│       ├── CMakeLists.txt
│       ├── README.md
│       ├── self_restoring.c
│       └── Makefile            # Альтернативная сборка
└── .vscode/                    # Настройки VS Code для IntelliSense
    ├── settings.json
    └── c_cpp_properties.json
```

## Лабораторные работы

### [Лабораторная работа 1: Управление процессами, потоками, нитями](Lab_1/)

#### ✅ Задание 1: Самовосстанавливающийся процесс
Приложение демонстрирует процесс, который перезапускается с сохранением состояния.

**Запуск:**
```bash
wine build/Lab_1/task1_self_restoring/self_restoring.exe
```

**Подробнее:** [Lab_1/task1_self_restoring/README.md](Lab_1/task1_self_restoring/README.md)

## Настройка VS Code

Проект настроен для работы с IntelliSense при кросс-компиляции для Windows:

- Используется MinGW-w64 компилятор
- Настроены пути к заголовочным файлам Windows
- Определены макросы для Windows API
- Интеграция с CMake Tools

При открытии проекта в VS Code IntelliSense должен автоматически распознать Windows API.

## Альтернативная сборка с Makefile

В каждой папке с заданием есть Makefile для быстрой сборки:

```bash
cd Lab_1/task1_self_restoring
make          # Сборка
make run      # Запуск через Wine
make clean    # Очистка
```

## Автор

BSUIR, курс OSISP
