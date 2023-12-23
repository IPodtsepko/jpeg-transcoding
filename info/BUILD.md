# Сборка проекта

### Содержание
- [Установка CMake](#установка-cmake)
- [Сборка](#сборка)


Для сборки C++ части проекта необходимо использовать утилиту `cmake`.

## Установка CMake

На macOS для установки `cmake` требуется выполнить команду:
```
brew install cmake
```

На Ubuntu:
```
sudo apt update
sudo apt install cmake
```

На Windows требуется скачать и установить ее c [официального сайта](https://cmake.org/download/)

## Сборка

Для корректной работы Python-скриптов требуется выполнять сборку в директорию `build` в корневой папке репозитория. Для этого на macOS требуется выполнить следующие комманды:
```
mkdir build
cd build
cmake ..
make
```

На Windows требуется также установленный компилятор. Примерная последовательность комманд для компилятора Visual Studio:
```
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```
