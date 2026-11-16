<p align="center">
  <img src="program_info/io.github.artemii14.ElysiumLauncher.svg" width="112" alt="Логотип Elysium Launcher">
</p>

<h1 align="center">Elysium Launcher</h1>

<p align="center">
  Свои миры. Свои сборки. Свой интерфейс.<br>
  Minecraft-лаунчер на C++ и Qt 6 с темой Midnight и двумя режимами запуска.
</p>

<p align="center">
  <a href="https://github.com/artemii14/ubiquitous-happiness/releases/tag/v1.0"><strong>Скачать 1.0</strong></a> ·
  <a href="#возможности">Возможности</a> ·
  <a href="#документация">Документация</a> ·
  <a href="https://github.com/artemii14/ubiquitous-happiness/issues/new/choose">Сообщить об ошибке</a>
</p>

<p align="center">
  <a href="https://github.com/artemii14/ubiquitous-happiness/actions/workflows/windows.yml"><img src="https://github.com/artemii14/ubiquitous-happiness/actions/workflows/windows.yml/badge.svg?branch=main" alt="Сборка Windows"></a>
  <a href="https://github.com/artemii14/ubiquitous-happiness/actions/workflows/elysium.yml"><img src="https://github.com/artemii14/ubiquitous-happiness/actions/workflows/elysium.yml/badge.svg?branch=main" alt="Сборка Linux"></a>
</p>

> Независимый форк [ElyPrismLauncher](https://github.com/ElyPrismLauncher/Launcher),
> основанного на [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher).
> Не является официальным продуктом Prism Launcher, Ely.by или Mojang/Microsoft.
> Minecraft не входит в комплект; для игры через Microsoft-аккаунт требуется право на игру.

## Скачать

### Elysium Launcher 1.0

| Платформа | Скачать | Как запустить |
|---|---|---|
| **Windows · установщик** | **[EXE](https://github.com/artemii14/ubiquitous-happiness/releases/download/v1.0/ElysiumLauncher-Windows.exe)** | Запустите установщик и следуйте его инструкциям |
| **Windows x64 · portable** | **[ZIP](https://github.com/artemii14/ubiquitous-happiness/releases/download/v1.0/ElysiumLauncher-Windows-x64-portable.zip)** | Распакуйте **весь архив**, запустите `elysiumlauncher.exe` |
| **Linux x64** | **[AppImage](https://github.com/artemii14/ubiquitous-happiness/releases/download/v1.0/ElysiumLauncher-Linux-x86_64.AppImage)** | Разрешите исполнение и запустите файл |

[Описание релиза](https://github.com/artemii14/ubiquitous-happiness/releases/tag/v1.0) ·
[Контрольные суммы SHA-256](https://github.com/artemii14/ubiquitous-happiness/releases/download/v1.0/SHA256SUMS.txt)

Linux: Ubuntu 24.04 / glibc 2.39 или совместимая система новее.

```sh
chmod +x ElysiumLauncher-Linux-x86_64.AppImage
./ElysiumLauncher-Linux-x86_64.AppImage
```

> [!WARNING]
> Бинарники не подписаны. Portable и AppImage прошли CI; отдельно загруженный автором
> установщик EXE в CI не проверялся. Контрольная сумма не заменяет подпись и антивирусную проверку.
> Не отключайте защиту системы ради запуска. Перед тестированием сохраните резервные копии миров и сборок.

<details>
<summary>Нужны самые свежие экспериментальные сборки?</summary>

Откройте [Windows Actions](https://github.com/artemii14/ubiquitous-happiness/actions/workflows/windows.yml)
или [Linux Actions](https://github.com/artemii14/ubiquitous-happiness/actions/workflows/elysium.yml),
выберите успешный запуск нужной ветки/коммита и скачайте файл из **Artifacts**.
GitHub может потребовать вход. Артефакты Actions хранятся 14 дней;
файлы Releases публикуются отдельно. Скачанный архив сам по себе не обновляется.

[Подробнее о Windows-сборках](docs/WINDOWS-GITHUB.md).

</details>

## Возможности

### Интерфейс, который можно настроить

| | |
|---|---|
| **Modern** | Библиотека с поиском, сортировкой и панелью действий справа |
| **Legacy** | Компактный выбор сборки и запуск без лишних панелей |
| **Midnight** | Тёмная тема, собственные SVG-значки и настраиваемый акцент |
| **Hyprland-inspired** | Графитовые панели, бирюзовые рамки и отдельные тонкие cyan-иконки |
| **Живой фон** | Северное сияние, звёздное небо или волны; анимацию можно выключить |
| **Своя раскладка** | Перестановка кнопок и целых блоков с сохранением порядка |

Всё оформление — в **Настройки → Внешний вид**. Чтобы переставлять блоки,
включите **«Разрешить перемещение»**, нажмите **«Применить»** и тяните заголовок **↕**.
Выключите переключатель, чтобы заблокировать раскладку. Стандартные темы Prism сохранены.

Поиск: **Ctrl+K**, очистка: **Esc**. Иконку выбранной сборки можно изменить нажатием на неё справа.
Переводы пока частичные; после смены языка рекомендуется перезапуск.

### От vanilla до модифицированной сборки

- **Vanilla:** выберите Minecraft — новый экземпляр получит автоматическое имя.
- **Sodium · Fabric / NeoForge:** каталог Modrinth, Sodium, Iris
  и Zoomify либо Ok Zoomer. Fabric API обязателен для Fabric; Sodium Extra и
  официальный Mod Menu добавляются при наличии совместимых версий.
- **Проверка загрузок:** зависимости по метаданным и SHA-512 файлов модов.
- **OptiFine:** локальный импорт совместимого LaunchWrapper-профиля,
  без раздачи и автоматического скачивания OptiFine.
- **Привычные инструменты Prism/ElyPrism:** аккаунты Ely.by и Microsoft,
  Modrinth/CurseForge, загрузчики модов, Java, миры, ресурс-паки, шейдеры, логи,
  импорт и экспорт экземпляров. Доступность внешних сервисов зависит от их API и условий.

**Sodium-рецепты создают только новые экземпляры — существующие сборки автоматически не обновляются.**
Проверка метаданных не гарантирует игровой запуск каждой комбинации модов.

### Виртуальная локальная сеть

Панель управления **отдельно установленным ZeroTier One**: подключение по Network ID,
статус сетей, копирование IP и отключение. Установка необязательна; доступ к локальной
службе может требовать настройки. Адрес `ZT IP:порт` передаётся друзьям вручную —
**публикации и поиска хостов нет**.

## Документация

- [Режимы интерфейса, оформление и OptiFine](docs/INTERFACE-MODES.md)
- [Sodium: состав рецептов и ограничения](docs/SODIUM-PRESETS.md)
- [ZeroTier: подключение и безопасность](docs/ZEROTIER.md)
- [Windows: сборка и диагностика](docs/WINDOWS-GITHUB.md)
- [Примечания к релизу 1.0](docs/RELEASE-v1.0.md)

### Продолжить разработку в новом чате

**[CHAT_HANDOFF.md](CHAT_HANDOFF.md)** в корне содержит историю изменений, решения,
расположение кода, проверки и оставшиеся задачи. Напишите новому помощнику:

> Прочитай CHAT_HANDOFF.md в корне репозитория и продолжи работу.

## Поддержка и предложения

[Создайте Issue](https://github.com/artemii14/ubiquitous-happiness/issues/new/choose)
по-русски или по-английски. Укажите ОС, версию/коммит, шаги воспроизведения
и ожидаемый результат. Перед публикацией логов удалите токены, пароли, email
и другие личные данные. Ошибки этого форка не отправляйте разработчикам Prism/ElyPrism.

[Участие в разработке](CONTRIBUTING.md) · [Правила сообщества](CODE_OF_CONDUCT.md)

## Сборка из исходников

Нужны CMake ≥ 3.25, C++20, Ninja, Qt ≥ 6.8 (Core, Widgets, Concurrent, Network,
Test, Xml, NetworkAuth, OpenGL), Extra CMake Modules, cmark, libarchive,
libqrencode, zlib, toml++ ≥ 3.2. На Linux также нужен development-пакет gamemode.
Для `CMakePresets.json` требуется CMake с поддержкой presets schema 8.
Точные наборы зависимостей CI: [Windows](.github/workflows/windows.yml) и [Linux](.github/workflows/elysium.yml).

```sh
git clone --recurse-submodules https://github.com/artemii14/ubiquitous-happiness.git
cd ubiquitous-happiness
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
cmake --install build
```

При отдельной установке Qt передайте `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64`
(либо путь к Qt для вашей платформы). На Linux установленный файл — `install/bin/elysiumlauncher`.
Структурные проверки: `python3 scripts/check_elysium.py`.
Нативные тесты и сборка не заменяют ручную проверку авторизации и запуска Minecraft.

## Происхождение и лицензии

Спасибо разработчикам **ElyPrismLauncher, Prism Launcher и MultiMC**, а также авторам
используемых библиотек и модов. Интерфейсы XMCL и Legacy использовались как ориентир
сценариев; это собственная Qt-реализация, а не объединение их приложений.

Код лаунчера: **GPL-3.0-only** — [LICENSE](LICENSE).
Лицензии и уведомления отдельных компонентов и ресурсов: [COPYING.md](COPYING.md),
[program_info/LICENSE](program_info/LICENSE) и файлы библиотек.
Авторские уведомления исходных проектов сохранены. Моды скачиваются из источника при установке
и не включены в репозиторий; у них собственные лицензии.

Исходная ревизия ElyPrism: `1c8bdd4dd72da77024a5dbb8bd8ef863540e3364`.
[Оригинальное описание проекта](docs/UPSTREAM-README.md) · [Чек-лист публичного релиза](docs/RELEASE-CHECKLIST.md)
