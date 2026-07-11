# PACK.IT LED Pattern — тестовый FFGL Source для Resolume Arena/Avenue

Это первая тестовая версия процедурного генератора рисунков из светящихся LED-точек.

## Что уже есть

- 6 фигур: Kaleidoscope, Rings, Star, Checker, Waves, Tunnel;
- 4 настраиваемых цвета;
- плотность LED-точек и их размер;
- скорость движения и вращения;
- масштаб рисунка;
- симметрия 2/4/6/8/10/12;
- внутренняя частота анимации 60/50/30/25/15/10/5 FPS;
- Motion Blur;
- LED Glow.

Это именно **Source**, то есть он создаёт изображение сам и не требует загруженного видео.

## Самый простой способ собрать и установить

Нужны Windows 10/11 и Resolume Arena/Avenue 7.

1. Установите **Visual Studio 2022 Community** или **Build Tools 2022**.
2. В установщике обязательно отметьте workload **Desktop development with C++ / Разработка классических приложений на C++**.
3. Установите Python 3 и включите пункт **Add Python to PATH**.
4. Распакуйте этот архив в обычную папку без кириллицы в пути, например `C:\PackItLEDPattern`.
5. Нажмите правой кнопкой на `build_and_install.ps1` → **Запустить с помощью PowerShell**.
6. Если Windows блокирует запуск, откройте PowerShell в папке и выполните:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build_and_install.ps1
```

Скрипт скачает официальный FFGL SDK Resolume, соберёт 64-битную DLL и положит её в:

```text
Документы\Resolume\Extra Effects\PackItLEDPattern.dll
```

## Как добавить генератор в Resolume Arena

1. Запустите или перезапустите Resolume Arena.
2. Справа откройте панель **Sources**.
3. В поиске введите `PK LED Pattern`.
4. Перетащите источник в свободную ячейку клипа.
5. Выберите созданный клип.
6. Параметры генератора появятся на панели Clip.

Если источник не появился:

1. Откройте `Application → Preferences → Video`.
2. В разделе **FFGL Directories** убедитесь, что добавлена папка:
   `Документы\Resolume\Extra Effects`.
3. Нажмите rescan/reload plugins либо перезапустите Resolume.
4. Проверьте, что используется 64-битная версия Resolume 7.

## Рекомендуемый первый тест

- Pattern: `Kaleidoscope`;
- LED Density: около 60–80;
- Dot Size: 70–85%;
- Symmetry: 8;
- Animation FPS: 30;
- Motion Speed: немного правее центра;
- Rotation Speed: немного правее центра;
- Motion Blur: 15–30%;
- LED Glow: 30–50%.

## Ограничения тестовой версии

- Motion Blur сейчас сделан как восемь временных сэмплов внутри шейдера, без отдельного feedback-буфера.
- Нет аудиореактивности, BPM Sync, сохранённых пресетов и настоящих длинных trails.
- Цвета показаны в Resolume как отдельные R/G/B-параметры; в следующей версии их можно объединить и аккуратнее сгруппировать.
- DLL должна быть собрана на Windows.

## Удаление

Закройте Resolume и удалите:

```text
Документы\Resolume\Extra Effects\PackItLEDPattern.dll
```
