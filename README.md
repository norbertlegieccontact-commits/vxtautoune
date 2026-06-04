# VoxTune Autotune — VST3 Plugin

Premium real-time pitch correction by **VoxTune** (vox-tune.com)

---

## ⚡ Szybki start — GitHub Actions (bez instalacji kompilatora)

### 1. Utwórz repo na GitHub
```
1. Wejdź na github.com → New repository
2. Nazwa: VoxTuneAutotune
3. Public lub Private
4. Kliknij "Create repository"
```

### 2. Wgraj ten projekt
```bash
# W terminalu, w folderze projektu:
git init
git add .
git commit -m "Initial commit - VoxTune Autotune"
git branch -M main
git remote add origin https://github.com/TWOJA_NAZWA/VoxTuneAutotune.git
git push -u origin main
```

### 3. Poczekaj ~15 minut
GitHub automatycznie skompiluje plugin na Windows i macOS.

### 4. Pobierz gotowy .vst3
```
GitHub → Actions → Build VoxTune Autotune → Artifacts
Pobierz: VoxTuneAutotune-Windows-VST3.zip lub macOS
```

### 5. Zainstaluj
- **Windows:** Wypakuj `.vst3` do `C:\Program Files\Common Files\VST3\`
- **macOS:** Wypakuj `.vst3` do `/Library/Audio/Plug-Ins/VST3/`

---

## 🎛️ Parametry

| Parametr | Zakres | Opis |
|----------|--------|------|
| Speed | 0–100 | 0 = T-Pain (robot), 100 = naturalna korekcja |
| Humanize | 0–100 | Subtelna losowa modulacja jak prawdziwy wokalista |
| Mix | 0–100 | Wet/dry blend |
| Key | C–B | Tonacja |
| Scale | 6 opcji | Chromatic, Major, Minor, Pentatonic, Blues, Dorian |
| Pitch | ±12 st. | Manualne przesunięcie wysokości (sekcja FORMANT) |
| Formant | ±12 st. | Korekcja formantu (sekcja FORMANT) |

---

## 🔧 Budowanie lokalnie (opcjonalne)

**Wymagania:** CMake 3.22+, Visual Studio 2022 (Win) lub Xcode 14+ (Mac)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Plugin pojawi się w: `build/VoxTuneAutotune_artefacts/Release/VST3/`

---

## 📄 Licencja

MIT License — możesz sprzedawać ten plugin komercyjnie.

---

*Built with JUCE 8 | Inspired by bemtorres/opentune (MIT)*
