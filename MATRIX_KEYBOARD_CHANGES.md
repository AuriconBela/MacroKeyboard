# Mátrix Billentyűzet Módosítások

## Változtatások Összefoglalása

A makró billentyűzet kódja módosítva lett, hogy mátrix elrendezésű billentyűzetet támogasson:

### 1. Hardware Változások
- **Előtte**: 12 egyedi pin kellett minden billentyűhöz (pin 14-23, A0, A1)
- **Utána**: 7 analóg pin a 4×3 mátrix elrendezéshez
- **Pin kiosztás**:
  - Oszlop pinek (OUTPUT): A0, A1, A2, A3
  - Sor pinek (INPUT_PULLUP): A4, A5, A6

### 2. Szoftver Változások

#### main.cpp
- `keyPins[12]` array lecserélve `colPins[4]` és `rowPins[3]` array-ekre
- `handleKeys()` függvény teljesen átírva mátrix szkennelésre
- Pin inicializálás módosítva a setup()-ban
- Debug információk hozzáadva

#### State.cpp
- `NormalState::handleKeyPress()` módosítva:
  - Mindig küld `KEY_PRESSED:X` üzenetet a PC-nek minden billentyű lenyomásnál
  - Ha van konfigurált parancs, akkor végrehajtja azt is
- `NormalState::updateLCD()` módosítva 4×3 layout megjelenítésére

### 3. Serial Kommunikáció

A következő üzenetek kerülnek elküldésre a PC-nek:

```
KEY_PRESSED:0   # Billentyű 0 lenyomva (sor 0, oszlop 0)
KEY_PRESSED:1   # Billentyű 1 lenyomva (sor 0, oszlop 1)
...
KEY_PRESSED:11  # Billentyű 11 lenyomva (sor 2, oszlop 3)
```

Ha a billentyűhöz van konfigurált parancs:
```
KEY:X           # Parancs végrehajtási kérés X billentyűhöz
```

### 4. Billentyű Indexelés

Mátrix pozíció → Index számítás:
```cpp
int keyIndex = row * 4 + col;
```

Billentyű layout:
```
+---+---+---+---+
| 0 | 1 | 2 | 3 |  <- Sor 0
+---+---+---+---+
| 4 | 5 | 6 | 7 |  <- Sor 1  
+---+---+---+---+
| 8 | 9 |10 |11 |  <- Sor 2
+---+---+---+---+
```

### 5. Hardware Kapcsolás

```
Arduino Micro Analóg Pinek:
- A0 (Pin 18) → Oszlop 0
- A1 (Pin 19) → Oszlop 1  
- A2 (Pin 20) → Oszlop 2
- A3 (Pin 21) → Oszlop 3
- A4 (Pin 22) → Sor 0
- A5 (Pin 23) → Sor 1
- A6 (Pin 4)  → Sor 2

Mátrix kapcsolás:
- Oszlop pinek: OUTPUT, alapértelmezetten HIGH
- Sor pinek: INPUT_PULLUP
- Billentyű lenyomása: oszlop LOW + sor LOW olvasás
- Analóg pinek digitális módban használva (pinMode/digitalWrite/digitalRead)
```

## Tesztelés

1. Töltse fel a kódot az Arduino Micro-ra
2. Nyissa meg a Serial Monitor-t (9600 baud)
3. Nyomjon le billentyűket a mátrixon
4. Ellenőrizze a serial kimenetet:
   - Minden billentyű lenyomásnál `Matrix key pressed: X (row: Y, col: Z)` üzenet
   - `KEY_PRESSED:X` üzenet a PC-nek
   - Ha van konfigurált parancs, akkor `KEY:X` és állapotváltás Command módba

## Hibakeresés

- Ha egy billentyű nem működik: ellenőrizze a mátrix kapcsolásokat
- Ha több billentyű egyszerre aktiválódik: ellenőrizze a diódák jelenlétét (anti-ghosting)
- Serial kimenet segít a hibakeresésben

## Megjegyzések

- A kód kompatibilis a meglévő serial protokollal
- Az LCD kijelző frissítve a 4×3 layout megjelenítésére  
- Debug üzenetek ki/bekapcsolhatók a `USE_MINIMAL_DISPLAY` flaggal
