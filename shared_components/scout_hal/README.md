# HAL — Hardware Abstraction Layer

Separerar hårdvarulogik från applikationslogik. All hårdvarunära kod implementerar ett interface härifrån — resten av systemet pratar bara med interfacet.

## Struktur

```
components/hal/
  IMotor.hpp       — motorstyrning (riktning + hastighet)
  ICamera.hpp      — kamerafångst
  INetwork.hpp     — TCP-transport
  IDisplay.hpp     — skärmpanel
  mock/            — testimplementationer utan hårdvara
```

## Två implementationer per interface

| Variant | Var | Kräver hårdvara |
|---------|-----|-----------------|
| `MotorMock`, `CameraMock` etc. | `mock/` | Nej — kör på valfri dator |
| `MotorEsp32`, `CameraEsp32` etc. | `firmware/cam/` eller `firmware/screen/` | Ja — ESP-IDF + fysisk hårdvara |

`main.cpp` väljer vilken variant som skickas in. Resten av koden ser bara interfacet.

## Köra tester

Testerna ligger i `test/` i projektroten och bygger utan ESP-IDF:

```bash
cd test
cmake -B build -S .
cmake --build build
./build/scout_hal_tests
```
