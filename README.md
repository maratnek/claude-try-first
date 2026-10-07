# Flight Game

Аркадная 3D-игра про полёты на бипланах 1920-х: взлёт, чекпоинты, финиш
через 1 км, посадка. C++23 и raylib 5.5; desktop (macOS) и Web, дальше iOS.

## Сборка и запуск

Desktop (нужны CMake и Conan, raylib подтягивается автоматически):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/FlightGame
ctest --test-dir build --output-on-failure   # физика полёта без окна
```

Web (нужен Emscripten):

```
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
```

Управление: стрелки — тангаж и крен, A/D — руль направления, W/S — тяга,
R — заново, M — меню, F1 — пресет графики. На тач-экране — виртуальный
стик слева и кнопки тяги справа.

## Документы

- [Архитектура](docs/ARCHITECTURE.md) — модули, экраны, сборка, ассеты.
- [Процесс работы](docs/PROCESS.md) — как агенты и владелец ставят и
  выполняют задачи, ветки, выпуск.
- [История создания](docs/DEVELOPMENT_HISTORY.md) — вехи и решения.
- [Журнал прогонов агентов](PROGRESS_LOG.md).
- [План релиза](design-notes/release-plan.md).
- [CLAUDE.md](CLAUDE.md) — правила и roadmap для агентов.
