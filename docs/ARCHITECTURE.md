# Архитектура

Игра — один исполняемый файл на C++23 и raylib 5.5. Код устроен как набор
простых структур (`PlaneState`, `LevelState`, `WorldState`…) и свободных
функций над ними, без классов и слоёв абстракции. Один и тот же код
собирается под macOS (desktop) и Web (Emscripten); iOS — в планах.

## Модули

```mermaid
flowchart TD
    main["main.cpp<br/>игровой цикл, экраны, камера"]
    input["input.cpp<br/>клавиатура + тач → FlightInput"]
    flight["flight.cpp<br/>физика полёта, PlaneParams"]
    level["level.cpp<br/>чекпоинты, финиш, HUD, экраны краха и результатов"]
    world["objects/world.cpp<br/>рельеф, препятствия"]
    plane["objects/plane.cpp<br/>модель и анимация самолёта"]
    glb["objects/glb_nodes.cpp<br/>дерево узлов GLB"]
    audio["audio.cpp<br/>процедурный звук"]
    menu["menu.cpp<br/>главное меню"]
    settings["settings.cpp<br/>пресеты графики"]
    character["objects/character.cpp<br/>декоративные фигурки"]

    main --> input
    main --> flight
    main --> level
    main --> world
    main --> plane
    main --> audio
    main --> menu
    main --> settings
    main --> character
    plane --> glb
    world --> settings
```

| Модуль | Отвечает за | Ключевые типы и функции |
| --- | --- | --- |
| `src/main.cpp` | Цикл кадра, переключение экранов, камера, порядок отрисовки, загрузка ассетов | `Game`, `Screen`, `UpdateFrame`, `AssetPath` |
| `src/input.cpp` | Ввод с клавиатуры и тача в единую структуру; экранный стик и кнопки | `FlightInput`, `ReadFlightInput`, `DrawTouchOverlay` |
| `src/flight.cpp` | Физика: управляемость зависит от скорости, сопротивление, сваливание, разбег, взлёт, посадка | `PlaneParams`, `BiplaneParams`, `PlaneState`, `UpdatePlaneControls` |
| `src/level.cpp` | Цель уровня (1 км), чекпоинты-кольца, финишные ворота, HUD, экраны краха и результатов | `LevelState`, `UpdateLevel`, `DrawLevelHUD`, `DrawCrashScreen`, `DrawResultsScreen` |
| `src/objects/world.cpp` | Heightmap-рельеф с ровным коридором, жёсткие и мягкие препятствия, высота земли | `WorldState`, `GetGroundHeight`, `CheckObstacleHit` |
| `src/objects/plane.cpp` | Загрузка биплана, анимация пропеллера, рулей, элеронов, колёс | `PlaneModel`, `PlaneAnim`, `UpdatePlaneAnimation`, `DrawPlaneObject` |
| `src/objects/glb_nodes.cpp` | Чтение дерева узлов GLB (raylib его теряет) | `GlbNode`, `Mat4`, `LoadGlbNodes` |
| `src/audio.cpp` | Синтез звука в коде: гул двигателя, удар при краше, звон чекпоинта, ветер (зависит от скорости), глухой удар при мягкой посадке, щелчок кнопки UI (не обрывается сбросом `StopOneShotSounds`), глухой стук при мягком ударе о препятствие, общая громкость (`-`/`=`, шаг 0.1) | `EngineAudio`, `UpdateEngineAudio`, `UpdateWindAudio`, `PlayCrashSound`, `PlayChimeSound`, `PlayTouchdownSound`, `PlayClickSound`, `PlayDamageSound`, `SetMasterVolumeClamped` |
| `src/menu.cpp` | Главное меню из списка пунктов; клавиатура, мышь, тач | `MenuState`, `UpdateMenu`, `DrawMenu` |
| `src/settings.cpp` | Пресеты Low/Medium/High и переключатели необязательных эффектов | `GraphicsSettings`, `InitGraphicsSettings` |

## Экраны игры

```mermaid
stateDiagram-v2
    [*] --> Menu
    Menu --> Playing: Играть
    Playing --> Crashed: удар о препятствие / жёсткая посадка
    Crashed --> Playing: R — заново
    Crashed --> Menu: M — в меню
    Playing --> Finished: ворота 1000 м пройдены, посадка и скорость ≤ 8 м/с
    Finished --> Playing: R — заново
    Finished --> Menu: M — в меню
    Menu --> [*]: Выход (только desktop)
```

Пока игра не в экране `Playing`, двигатель молчит; при входе в `Crashed`
один раз звучит удар. После ворот время останавливается, самолёт остаётся управляемым, на HUD баннер
«GATE! Land to finish», за воротами зона посадки; жёсткая посадка или удар
после ворот ведёт в `Crashed` со строкой «Gate reached in X.XX s».
`kRequireLandingAfterGate = false` в `level.cpp` возвращает мгновенный финиш
на воротах. На экране `Finished` физика заморожена, показываются
время, медаль, разница с лучшим временем («+0.84 s» или «NEW BEST»),
чеклист «Finished / All rings / Clean» (невыполненный пункт приглушён
оранжевым), строка со следующей целью и 1–3 звезды (3 = без повреждений и все
чекпоинты, 2 = одно из двух, 1 = просто финиш). Медаль по времени у ворот
(`ComputeMedal`, пороги `kMedalGold/Silver/Bronze` = 24/30/40 с в начале
`level.cpp`). Раскладка экрана масштабируется от высоты окна; лучшее время
хранится только в памяти на время сессии.

## Кадр

1. `ReadFlightInput` собирает ввод (клавиши + тач).
2. В зависимости от экрана: меню, перезапуск или шаг физики
   (`UpdatePlaneControls`) и уровня (`UpdateLevel`), проверка столкновений.
3. Анимация самолёта и звук подстраиваются под состояние.
4. Камера следует сзади-сверху за самолётом.
5. Отрисовка: мир → финиш → чекпоинты → самолёт → фигурки → HUD → оверлеи.

## Самолёт и физика

- Все константы полёта лежат в `PlaneParams` (скорость, разгон,
  сопротивление, сваливание, взлёт, посадка). Новый тип самолёта = новый
  набор параметров, без правки кода физики.
- Знаки: положительный `pitch` — нос вверх, положительный `roll` — крен
  вправо, `yaw` растёт при повороте вправо. Отрисовка (`plane.cpp`)
  инвертирует pitch и roll под правило `rlRotatef` — это задокументировано
  в коде.
- Модель биплана (`assets/models/biplane-1920.glb`) — 74 меша с
  именованными узлами-шарнирами. raylib при загрузке «запекает» узлы в
  вершины, поэтому `glb_nodes.cpp` читает дерево узлов отдельно, и каждая
  подвижная часть рисуется со своей матрицей.

## Ассеты

- Модели лежат в `assets/models/`. CMake после сборки копирует `assets/`
  рядом с исполняемым файлом, а код грузит их по пути от
  `GetApplicationDirectory()`. Поэтому игра запускается из любой рабочей
  папки (терминал, VS Code).
- В Web-сборке ассеты упаковываются в виртуальную ФС Emscripten
  (`--preload-file`), путь — `assets/...`.
- Web-сборка использует свою HTML-оболочку `web/shell.html` (холст на весь экран, без скролла/зума на телефонах, кнопка fullscreen, подсказка про альбомную ориентацию) вместо `minshell.html` из raylib.
- Звуки генерируются в коде, аудиофайлов нет.
- Новые 3D-модели заказываются через `asset-requests/` (промт пишет агент,
  генерирует человек). Процедурный генератор самолётов живёт в ветке
  `design` (`tools/aircraft-gen/`) и в `dev` пока не входит.

## Сборка

| Цель | Как собирается | Зависимости |
| --- | --- | --- |
| macOS desktop | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build` | raylib из Conan через cmake-conan provider (ставится автоматически) |
| Web | `emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release && cmake --build build-web` | raylib 5.5 через CMake FetchContent; Conan при Emscripten отключён |
| iOS | ещё нет | — |

CI (`.github/workflows/build.yml`) собирает обе цели на каждый push и PR. Web-job дополнительно выкладывает артефакт `FlightGame-web` (`index.html`, `index.js`, `index.wasm`, `index.data` в корне архива) — его можно сразу загружать на itch.io.

## Производительность

Всё, что не влияет на игровой процесс (столбики-маркеры, каркасы
препятствий, фигурки, диск пропеллера, а в будущем погода, частицы,
декор), отключается через `GraphicsSettings`. Исключение — `blobShadow`
(полупрозрачная тень самолёта на земле, `DrawBlobShadow` в `world.cpp`,
рисуется после мира и до самолёта без записи в глубину): она почти бесплатна
и помогает оценить высоту, поэтому включена во всех пресетах, включая Low. На Web и мобильных по
умолчанию пресет Low; цель — iPhone 11 и слабые ПК.
