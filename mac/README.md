# MOON MIC — сборка на Mac

Собирает VST3 и AU (универсальные: Apple Silicon + Intel, macOS 11+), запускает тесты и проверяет AU через `auval`.

## Один раз подготовить Mac
1. Xcode из App Store или только инструменты разработчика:
   `xcode-select --install`
2. CMake: `brew install cmake` (нужен Homebrew: https://brew.sh)

JUCE 9.0.4 скрипт скачает сам в `~/JUCE`, если его там нет.

## Собрать
```
git clone https://github.com/AlLayan-cyber/Moon-MIC.git
cd Moon-MIC
bash mac/build_mac.sh --install
```
- без `--install` — только сборка и тесты;
- с `--install` — ещё копирует плагины в `~/Library/Audio/Plug-Ins/VST3` и `~/Library/Audio/Plug-Ins/Components` и запускает `auval`.

Готовые файлы: `plugin/build-mac/MoonMic_artefacts/Release/VST3/MOON MIC.vst3` и `.../AU/MOON MIC.component`.

## Если macOS не пускает плагин
Сборка без подписи Apple. При первом открытии в DAW macOS может предупредить о неизвестном разработчике:
Системные настройки → Конфиденциальность и безопасность → «Разрешить».
Для продажи понадобится подпись и нотаризация (аккаунт Apple Developer).

## Для чата Claude на Mac
> Склонируй https://github.com/AlLayan-cyber/Moon-MIC и выполни `bash mac/build_mac.sh --install`. Если что-то упадёт, покажи ошибку.
