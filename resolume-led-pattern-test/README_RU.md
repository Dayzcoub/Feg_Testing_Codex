# PACK.IT LED Pattern v0.2a — FFGL Source для Resolume Arena/Avenue

Процедурный генератор бесконечных LED-мозаик. Плагин сам создаёт изображение и не требует видеоклипа.

## Реализовано в v0.2a

- 12 паттернов: Kaleidoscope, Rings, Star Burst, Checker, Waves, Tunnel, Diamond Grid, Hex Flower, Plasma, Orbit, Pulse Rings, Pixel Tunnel;
- 11 форм пикселя: Circle, Square, Rounded Square, Diamond, Triangle, Hexagon, Star, Clover, Cross, Capsule, Ring;
- Pixel Density, Pixel Size, Rotation, Rotation Speed, Softness, Roundness, Inner Cut, Stretch X/Y;
- 2/3/4 активных цвета, Palette Shift, Palette Speed, Colour Blend, Saturation, Brightness;
- режимы движения Free, BPM и Paused;
- BPM Division, BPM Multiplier, Phase Offset и Beat Pulse с выбором цели;
- Smooth или ступенчатые 60/50/30/25/24/15/12/10/8/5 FPS;
- Frame Interpolation;
- Motion Blur с 4/8/12 сэмплами;
- Glow, Glow Size, Contrast, Invert, Mirror X/Y;
- прозрачный, чёрный или пользовательский фон;
- логическая группировка параметров в Resolume;
- сохранение исходных ID параметров v0.1 для совместимости композиций.

## Установка готовой сборки

### Windows

Скопируйте `PackItLEDPattern.dll` в папку FFGL-плагинов, обычно:

```text
Документы\Resolume\Extra Effects
```

### macOS

Скопируйте целиком `PackItLEDPattern.bundle` в папку, указанную в:

```text
Resolume Arena → Preferences → Video → FFGL Directories
```

На разных установках она может называться:

```text
~/Documents/Resolume/Extra Effects
```

или:

```text
~/Documents/Resolume Arena/Extra Effects
```

После замены плагина полностью перезапустите Resolume.

## Запуск

1. Откройте панель **Sources**.
2. Найдите `PK LED Pattern`.
3. Перетащите Source в свободную ячейку клипа.
4. Настройки появятся в панели Clip.

## Первый тест v0.2a

```text
Pattern: Hex Flower
Pixel Shape: Clover
Pixel Density: 55–75
Pixel Size: 70–85%
Motion Mode: Free
Animation FPS: 30
Motion Blur: 15–25%
Glow: 30–45%
```

Для BPM:

```text
Motion Mode: BPM
BPM Division: 1
Beat Pulse: 20–35%
Beat Pulse Target: Pixel Size или All
```

## Известные ограничения v0.2a

- Это первая alpha-сборка новой архитектуры; все паттерны и формы нужно визуально проверить внутри Resolume на реальной GPU.
- Motion Blur остаётся временным multisample blur без feedback-буфера и длинных trails.
- Цвета по-прежнему представлены отдельными R/G/B-параметрами, но теперь собраны в группу Colours.
- Preset-файлы Resolume пока не входят в архив; рекомендуемые комбинации приведены выше.
- FFT-аудиореактивность и feedback trails перенесены в v0.2b.

## Самостоятельная сборка Windows

Нужны Visual Studio/Build Tools 2022 с workload **Desktop development with C++** и Python 3.

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build_and_install.ps1
```

Скрипт скачает официальный FFGL SDK Resolume, соберёт DLL и установит её в стандартную папку Extra Effects.

## Удаление

Закройте Resolume и удалите `PackItLEDPattern.dll` или `PackItLEDPattern.bundle` из выбранной FFGL Directory.
