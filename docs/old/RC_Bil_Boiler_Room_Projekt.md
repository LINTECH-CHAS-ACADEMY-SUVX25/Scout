# RC-Bil med Live-Telemetri

> **Boiler Room Projekt** — Inbyggda system och industriell programmering
> Kurs 4 - Systemutvecklare C/C++

|                 |                                                                                                                                             |
|-----------------|---------------------------------------------------------------------------------------------------------------------------------------------|
| **Projektnamn** | **RC-Bil med Live-Telemetri**                                                                                                               |
| **Arbetstid**   | 4 timmar Boiler Room (måndag) + minst 12 timmar extra grupparbete per vecka                                                                 |
| **Beskrivning** | WiFi-styrd RC-bil med realtidstelemetri. ESP32-S3 styr motorer och samlar sensordata som streamas live till en 7-tums pekskärm (dashboard). |

## PROJEKTBESKRIVNING

RC-Bil med Live-Telemetri är ett inbyggt system där en 2WD-robotplattform styrs trådlöst via WiFi från en ESP32-S3-Touch-LCD-7B-skärm. Bilen är utrustad med en ESP32-CAM som fungerar som co-processor: den streamar live-video och sensordata (hastighet, riktning, batteri) tillbaka till dashboarden via UART och WiFi.

Projektet kombinerar realtidsstyrning, trådlös kommunikation och hårdvarunära motorstyrning i ett system som är genuint kul att demonstrera. Tänk er en miniaturiserad drönarkontroll: dashboarden visar live-video, telemetri och styrsignaler, medan bilen hanterar motorstyrning, sensoravläsning och kommunikation i realtid.

Till skillnad från konventionella lab-projekt kräver detta system robusthet under verklig drift — WiFi-latens påverkar styrbarhet, motorstyrning har hårda tidskrav, och videoströmning konkurrerar med telemetridata om bandbredd. Det är exakt den typen av trade-offs som industriella inbyggda system kräver.

## HÅRDVARUÖVERSIKT

Projektet använder fyra hårdvarukomponenter som samverkar i ett distribuerat system.

#### ESP32-S3-Touch-LCD-7B — Dashboard-kontroller

7-tums pekskärm som fungerar som operatörsgränssnitt och styrcentral. Tar emot telemetri från bilen via WiFi, visar live-video och sensordata på skärmen, och skickar styrsignaler (riktning, hastighet) tillbaka till bilen. Kör FreeRTOS med separata tasks för WiFi-kommunikation, displayrendering och touch-input.

#### ESP32-CAM — Bil-kontroller och kameranod

Monterad på bilen. Styr motorerna via L298N-modulen via GPIO/PWM, läser av sensordata (hastighet via encoder, batteri via ADC), och kommunicerar med dashboarden via WiFi. ESP32-CAM:s inbyggda kamera streamar video. UART används för diagnostik och debuggning.

#### Robot Car Kit 2WD — Plattform

Robotchassi med två DC-motorer och hjul. Monterar ESP32-CAM, L298N-motorförare och batteri. Enkelt att modifiera och lägga till sensorer på.

#### L298N Dubbel H-Brygga — Motorförare

Styr de två DC-motorerna med PWM-signaler från ESP32-CAM. Hanterar riktning (framåt/bakåt) och hastighet (duty cycle) för varje hjul separat, vilket möjliggör sväng och kurskontroll. Spänningsområde 5-35V, max 2A per kanal.

|                   |                    |                    |                               |
|-------------------|--------------------|--------------------|-------------------------------|
| **Interface**     | **Signaler**       | **Komponent**      | **Notering**                  |
| WiFi (Styrning)   | UDP/TCP (802.11)   | Dashboard ↔ Bil    | Styrsignaler + telemetri      |
| PWM (Motorer)     | IN1, IN2, ENA, ENB | ESP32-CAM → L298N  | 25 kHz PWM för hastighet      |
| UART (Diagnostik) | TX, RX (USB)       | ESP32-CAM → PC     | 115200 baud, 8N1              |
| I²C (Sensor)      | SDA, SCL           | ESP32-CAM → IMU    | MPU6050 orientering (stretch) |
| SPI (Display)     | MOSI, SCK, CS      | S3 → LCD-7B intern | Hanteras av BSP-drivrutin     |

## SYSTEMKRAV

Följande krav är obligatoriska och måste uppfyllas för godkänt betyg.

#### Hårdvara

- ESP32-S3-Touch-LCD-7B som dashboard och styrcentral

- ESP32-CAM som bil-kontroller monterad på 2WD-chassit

- L298N H-brygga för motorstyrning

- UART via USB för diagnostikinterface på bil-sidan

#### Arkitektur

- RTOS-baserad arkitektur med FreeRTOS på båda ESP32-noderna

- Minst 4 separata tasks med tydliga ansvarsområden och prioriteter

- Synkronisering via köer, mutex och semaforer

- Timer- eller avbrottsdriven PWM-generering (ej busy-wait)

- Hardware Abstraction Layer (HAL) för motor och sensorer

- Modulär komponentstruktur i ESP-IDF

#### Kommunikation

- WiFi-anslutning mellan dashboard och bil via UDP (låg latens) eller TCP

- PWM-kommunikation med L298N för motorstyrning

- UART-baserat diagnostikinterface på bil-sidan

- Korrekt felhantering för alla kommunikationsprotokoll

#### Robusthet och drift

- Watchdog-timer för övervakning av kritiska tasks

- Säker nödstopp: motorer stannar om WiFi-anslutning bryts \> 500 ms

- Felhantering vid kommunikationsavbrott och ogiltiga kommandon

- System som körs stabilt i minst 30 minuter utan krasch

- Loggning av systemhändelser och felsituationer via UART

## FUNKTIONELLA KRAV

#### Dashboard (ESP32-S3-Touch-LCD-7B)

- Visa live-telemetri: hastighet, riktning, batteri och WiFi-signalstyrka

- Pekskärmsstyrning: virtuell joystick för framåt/bakåt/vänster/höger

- Visa anslutningsstatus och latens (RTT) till bilen

- Uppdatering av displayen minst var 100 ms

- Nödstopp-knapp på pekskärmen

#### Bil-kontroller (ESP32-CAM)

- Ta emot styrsignaler från dashboard och omvandla till PWM-signaler

- Skicka telemetridata till dashboard med konfigurerbart intervall (50-500 ms)

- Säker fallback: automatisk nödstopp om styrkommando uteblir \> 500 ms

- Motorstyrning: framåt, bakåt, vänster, höger och variabel hastighet via PWM

#### UART-diagnostikinterface (på bil-sidan)

- STATUS: Visa systemtillstånd (WiFi, motorer, batteri, uptime)

- MOTOR: Visa aktuella PWM-värden och riktning per motor

- SENSOR: Visa sensoravläsningar

- CONFIG \<param\> \<value\>: Ändra konfigurationsparametrar (t.ex. maxhastighet)

- DIAG: Visa systemdiagnostik (task-statistik, stackanvändning)

- HELP: Visa tillgängliga kommandon

## STRETCH GOALS — UTÖKAD FUNKTIONALITET

|              |                                                                   |
|--------------|-------------------------------------------------------------------|
| **Kategori** | **Stretch Goals**                                                 |
| Live-video   | Streama MJPEG-video från ESP32-CAM till dashboardskärmen via WiFi |
|              | Visa video + telemetri i delad vy på 7-tums skärm                 |
| Sensorer     | MPU6050 IMU via I²C: orientering och g-krafter visas på dashboard |
|              | Varvtalsmätare via encoder-avbrott på hjulen                      |
| Styrning     | PID-reglering av rakkörning (kompensera för motorskillnader)      |
|              | Hastighetsbegränsning konfigurerbar från dashboard via pekskärm   |
| Test         | CI-pipeline med GitHub Actions för automatiska byggen             |
|              | Hardware-in-the-loop testskript som verifierar PWM-signaler       |
| Energi       | Batteriövervakning via ADC med larm vid låg spänning              |

## LEVERABLER

Vid kursens slut (vecka 12) ska följande levereras:

#### Kod och system

- Komplett ESP-IDF-projekt i Git-repository (separata komponenter för dashboard och bil)

- CMakeLists.txt och alla nödvändiga konfigurationsfiler

- Fungerande RC-bil med WiFi-styrning och live-telemetri på dashboard

- C++-klasser med RAII och HAL-abstraktion för motor och kommunikation

- Enhetstester och testskript

#### Dokumentation

- README med installations-, bygg- och driftsinstruktioner

- Arkitekturdokumentation med blockschema och signalflöden

- Sekvensdiagram för styrsignal (Touch → Dashboard → WiFi → Bil → PWM → Motor)

- HAL-gränssnittsdokumentation för motorstyrning

- Felsökningsrapport med identifierade och åtgärdade buggar

- Individuell skriftlig reflektion (per student)

#### Presentation

- Muntlig presentation (15-20 minuter)

- Live-demonstration: kör bilen i klassrummet med dashboard på skärmen

- Presentation av arkitektur och designbeslut

- Demonstration av UART-diagnostikinterface

- Diskussion av utmaningar: latens, nödstopp, motorstyrning

## DETALJERAD VECKOPLANERING

Vecka 1 är introduktionsvecka utan Boiler Room-implementation. Från vecka 2 tillämpar vi mönstret där måndagens Boiler Room bygger på föregående veckas workshops.

### KURSVECKA 1: PROJEKTINTRODUKTION OCH HÅRDVARUÖVERSIKT

|           |                                               |
|-----------|-----------------------------------------------|
| **Fokus** | Introduktionsvecka — inget Boiler Room-arbete |

Under den första kursveckan introduceras projektet. Vi monterar 2WD-chassit, kopplar L298N till ESP32-CAM och testar att motorerna snurrar. Vi planerar WiFi-arkitekturen och bestämmer kommunikationsprotokoll mellan dashboard och bil.

#### Implementering

- Montera Robot Car Kit 2WD-chassit med motorer och hjul

- Koppla L298N till ESP32-CAM: IN1, IN2, ENA (vänster motor), IN3, IN4, ENB (höger motor)

- Testa manuell motorstyrning via enkel GPIO-kod

- Planera WiFi-arkitektur: UDP vs TCP, portval, meddelandeformat

- Skapa systemskiss med kommunikationsflöden

- Sätt upp Git-repository med grundläggande ESP-IDF-projektstruktur

#### Baskrav (måste uppfyllas)

- Monterat och fungerande 2WD-chassi

- L298N korrekt kopplad — motorer snurrar via enkel testskod

- Systemskiss som visar kommunikation mellan dashboard och bil

- Git-repository med grundläggande ESP-IDF-projektstruktur

- Alla gruppmedelmmars ESP-IDF-miljöer verifierade

- Preliminär backlog för 12 veckor

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 1: Förståelse för inbyggda systems särart</p>
<p>Kursmål 11: Dokumentera hårdvarunära design</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Presentation av systemskiss, monterat chassi och initial projektplanering</td>
</tr>
</tbody>
</table>

### KURSVECKA 2: ESP-IDF-PROJEKT OCH WIFI-ANSLUTNING

|           |                                                                  |
|-----------|------------------------------------------------------------------|
| **Fokus** | Tillämpa vecka 1: ESP-IDF projektstruktur och grundläggande WiFi |

Skapa det grundläggande ESP-IDF-projektet för båda noderna och etablera WiFi-anslutning. Målet är att dashboard-ESP32:n och bil-ESP32-CAM:n kan hitta varandra och skicka ett enkelt testmeddelande.

#### Implementering

- ESP-IDF-projekt med korrekt katalogstruktur för båda noder (dashboard/, car/)

- WiFi station mode på bil-sidan: anslut ESP32-CAM till lokalt nätverk

- WiFi AP eller station mode på dashboard-sidan

- Enkel UDP-socket: dashboard skickar 'PING', bil svarar 'PONG'

- ESP-loggning (ESP_LOGI, ESP_LOGW, ESP_LOGE) på båda noder

#### Baskrav (måste uppfyllas)

- Fungerande ESP-IDF-projekt för dashboard och bil som kompilerar och flashas

- Bil-ESP32-CAM ansluter till WiFi och loggar tilldelad IP-adress

- Lyckad UDP-kommunikation: PING-PONG verifierat med loggutskrift

- README med instruktioner för att bygga och flasha båda noder

#### Stretch Goals (valfritt)

- WiFi-reconnect vid tappat nätverk

- Konfigurerbar bil-IP via menuconfig

- Mät och logga RTT (round-trip time) för PING-PONG

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 1: Inbyggd utveckling vs traditionell</p>
<p>Kursmål 6: RTOS-baserat system</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av WiFi-ansluten bil och dashboard som kommunicerar</td>
</tr>
</tbody>
</table>

### KURSVECKA 3: MOTORSTYRNING OCH FREERTOS-TASKARKITEKTUR

|           |                                                                     |
|-----------|---------------------------------------------------------------------|
| **Fokus** | Tillämpa vecka 2: FreeRTOS tasks med tydliga ansvar och prioriteter |

Implementera terminalens taskarkitektur med FreeRTOS på bil-sidan. Skapa separata tasks för WiFi-mottagning, motorstyrning, telemetriinsamling och systemövervakning. Implementera grundläggande PWM-motorstyrning.

#### Implementering

- Motor-task: tar emot styrkommandon och genererar PWM-signaler till L298N

- Network-task: tar emot UDP-paket från dashboard och vidarebefordrar till motor-task

- Telemetri-task: samlar sensordata och skickar till dashboard

- Monitor-task: övervakar systemstatus och loggar via UART

- PWM-konfiguration: LEDC-drivrutin för hastighetsreglering (0-100% duty cycle)

#### Baskrav (måste uppfyllas)

- Minst 4 separata FreeRTOS-tasks med tydliga ansvarsområden

- Bilen rör sig framåt, bakåt, vänster och höger via enkla WiFi-kommandon

- Korrekt prioritetsordning: motor \> network \> telemetri \> monitor

- PWM-hastighetsreglering med minst 3 hastighetsnivåer

- Alla tasks loggar sin aktivitet via ESP_LOGI

#### Stretch Goals (valfritt)

- Task-statistik med vTaskGetRunTimeStats()

- Mjuk acceleration/deceleration för smidigare körning

- Watchdog-övervakning av motor-task

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 2: RTOS-principer och schemaläggning</p>
<p>Kursmål 6: RTOS-baserat system</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av WiFi-styrd bil med FreeRTOS multitask-arkitektur</td>
</tr>
</tbody>
</table>

### KURSVECKA 4: RTOS-SYNKRONISERING OCH NÖDSTOPP

|           |                                                                   |
|-----------|-------------------------------------------------------------------|
| **Fokus** | Tillämpa vecka 3: Mutex, semaforer och köer för säker datadelning |

Implementera synkroniseringsmekanismer och kritisk säkerhetsfunktion: automatisk nödstopp om WiFi-anslutningen bryts. Data från nätverket måste delas säkert med motor-tasken.

#### Implementering

- FreeRTOS-kö för styrkommandon från network-task till motor-task

- FreeRTOS-kö för telemetridata från telemetri-task till network-task

- Mutex för skydd av delad WiFi-resurs och konfigurationsdata

- Nödstopp-logik: motor-task stannar motorer om kö är tom \> 500 ms

- Heartbeat-mekanism: dashboard skickar keepalive-paket var 200 ms

#### Baskrav (måste uppfyllas)

- Styrkommandon överförs via kö från network-task till motor-task

- Automatisk nödstopp verifierat: bryt WiFi → bilen stannar inom 500 ms

- Korrekt mutex-användning för delad konfiguration

- Timeout-hantering på alla blockerande kö-operationer

- Dokumentation av vilka resurser som skyddas och varför

#### Stretch Goals (valfritt)

- Prioritetsinversion-skydd med priority inheritance mutex

- Konfigurerbar nödstoppstimeout via UART CONFIG-kommando

- Dubbelbuffring av telemetridata för konsistent display-uppdatering

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 2: Synkronisering och realtidskrav</p>
<p>Kursmål 6: Determinism och robusthet</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av synkroniserat system med verifierat nödstopp</td>
</tr>
</tbody>
</table>

### KURSVECKA 5: AVBROTT OCH TIMERDRIVEN MOTORSTYRNING

|           |                                                          |
|-----------|----------------------------------------------------------|
| **Fokus** | Tillämpa vecka 4: Avbrottshantering, timrar och watchdog |

Ersätt polling-baserad motorstyrning med timerdriven PWM och implementera avbrottsbaserad telemetriinsamling. Watchdog-timers övervakar systemets hälsa.

#### Implementering

- Hårdvarutimer (GPTIMER) som triggar periodisk telemetrisampling

- ISR som notifierar telemetri-task via semafor (deferred processing)

- FreeRTOS software timer för heartbeat-generering till dashboard

- Task watchdog timer (TWDT) för motor-task och network-task

- FromISR-varianter av FreeRTOS-API:er i avbrottskontext

#### Baskrav (måste uppfyllas)

- Timer-driven telemetrisampling med konfigurerbart intervall

- Korrekt deferred interrupt processing (ISR → task via semafor)

- FreeRTOS software timer för heartbeat

- Task watchdog aktiverad för motor-task och network-task

- Dokumentation av avbrottsprioriteter och timing-krav

#### Stretch Goals (valfritt)

- Mätning av ISR-exekveringstid och verifiering mot worst-case

- Adaptiv telemetrifrekvens baserat på nätverkslast

- Encoder-avbrott för varvtalsmätning (om encoder finns tillgänglig)

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 2: Avbrottshantering och realtidskrav</p>
<p>Kursmål 6: Realtid och determinism</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av timerdriven arkitektur med watchdog-övervakning</td>
</tr>
</tbody>
</table>

### KURSVECKA 6: UART-DIAGNOSTIKINTERFACE OCH BITMANIPULATION

|           |                                                                           |
|-----------|---------------------------------------------------------------------------|
| **Fokus** | Tillämpa vecka 5: Bitmanipulation, registeråtkomst och UART-kommunikation |

Implementera ett UART-baserat diagnostikinterface för bil-sidan. Via seriell anslutning ska vi kunna inspektera bilens tillstånd, kontrollera motorer, läsa telemetri och konfigurera systemet.

#### Implementering

- UART-drivrutin med konfigurerad baudrate, databitar och paritet

- Kommandotolk som parsar textkommandon från seriell port

- Kommandon: STATUS, MOTOR, SENSOR, CONFIG, DIAG, HELP

- Bitflaggor för systemtillstånd (WiFi, motorer aktiva, nödstopp, batteri)

- Bitmanipulation för att packa och presentera statusregister

#### Baskrav (måste uppfyllas)

- Fungerande UART-kommunikation med konfigurerbar baudrate

- Minst 5 kommandon implementerade och testade

- STATUS-kommando visar systemtillstånd med bitflaggor

- MOTOR-kommando visar aktuella PWM-värden per motor

- Felhantering vid ogiltiga kommandon och felaktig indata

#### Stretch Goals (valfritt)

- JSON-formaterad utdata för maskinläsbar diagnostik

- Loggning av UART-kommandon med tidsstämpel till NVS

- Kommando för att trigga nödstopp manuellt via UART

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 3: UART-kommunikation</p>
<p>Kursmål 7: Implementera UART-kommunikation</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av UART-diagnostikinterface med live-kommandon</td>
</tr>
</tbody>
</table>

### KURSVECKA 7: DASHBOARD-DISPLAY OCH KOMMUNIKATIONSPROTOKOLL

|           |                                                                    |
|-----------|--------------------------------------------------------------------|
| **Fokus** | Tillämpa vecka 6: SPI- och I²C-kommunikationsprotokoll i praktiken |

Nu bygger vi dashboarden! Pekskärmen på ESP32-S3-Touch-LCD-7B används för att visa live-telemetri och styra bilen. Touch-input implementeras via I²C och displayrendering via SPI/RGB. Vi definierar ett binärt telemetriprotokoll för effektiv WiFi-kommunikation.

#### Implementering

- Display-task på dashboard: rendera telemetridata med LVGL eller direkt framebuffer

- Touch-task: läs pekskärmskoordinater via I²C och omvandla till styrsignaler

- Binärt telemetriprotokoll: kompakt struct med hastighet, riktning, batteri, tidsstämpel

- Virtuell joystick på pekskärmen: touch-koordinater → PWM-värden

- Statusvisning: WiFi-ikon, batterinivå, latens (RTT)

#### Baskrav (måste uppfyllas)

- Pekskärmen visar live-telemetri från bilen (uppdateras minst var 100 ms)

- Touch-input styr bilen: joystick-yta på skärmen ger korrekt rörelse

- Binärt telemetriprotokoll implementerat och dokumenterat

- Visuell indikation av anslutningsstatus och latens

- Felhantering vid I²C-touchfel (NACK, timeout)

#### Stretch Goals (valfritt)

- Animerad hastighetsmätare och riktningsindikator

- Konfigurerbar layout på dashboard via pekskärmsnavigering

- Live-logg av senaste 10 systemhändelserna på skärmen

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 3: SPI och I²C i industriella miljöer</p>
<p>Kursmål 7: Implementera I²C och SPI</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av fungerande dashboard som styr bilen via pekskärm</td>
</tr>
</tbody>
</table>

### KURSVECKA 8: C++ ABSTRAKTIONER FÖR HÅRDVARA

|           |                                                                       |
|-----------|-----------------------------------------------------------------------|
| **Fokus** | Tillämpa vecka 7: C++ för inbyggda system med RAII och resurskontroll |

Refaktorera hårdvarunära kod till C++ med RAII-mönster. Skapa C++-klasser för motorstyrning, WiFi-kommunikation, UART-diagnostik och display-rendering. Fokus på resurshantering utan dynamisk allokering.

#### Implementering

- C++-klass MotorController med RAII (hanterar L298N-resurser)

- C++-klass WifiLink för WiFi-socket-kommunikation med anslutningshantering

- C++-klass DiagConsole för UART-diagnostikinterfacet

- C++-klass TelemetryPacket med serialisering/deserialisering

- Statiska buffertar istället för heap-allokering, constexpr för konstanter

#### Baskrav (måste uppfyllas)

- Minst 3 C++-klasser med korrekt resurshantering via RAII

- Inga dynamiska allokeringar (new/delete) i produktionskoden

- Korrekt användning av volatile för hårdvaruregister

- constexpr för kompileringstidskonstanter (PWM-frekvens, portval, etc.)

- Fungerande integration mellan C och C++-kod i ESP-IDF

#### Stretch Goals (valfritt)

- Template-baserat motorinterface för utbytbara motorförare

- CRTP för compile-time polymorfism i sensorabstraktion

- Statisk minnespool för telemetrimeddelanden

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 4: C++ i resursbegränsade system</p>
<p>Kursmål 8: C++-komponenter med resursbegränsning</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av C++-refaktorerat system med RAII-klasser</td>
</tr>
</tbody>
</table>

### KURSVECKA 9: HAL-DESIGN OCH TESTBAR ARKITEKTUR

|           |                                                                       |
|-----------|-----------------------------------------------------------------------|
| **Fokus** | Tillämpa vecka 8: Hardware Abstraction Layer och dependency injection |

Implementera ett Hardware Abstraction Layer som separerar applikationslogiken från hårdvaran. Med HAL kan motorstyrningslogiken testas på host-datorn utan fysisk L298N eller motorer.

#### Implementering

- HAL-interface för motorstyrning (IMotorDriver) och WiFi-kommunikation (IWifiLink)

- Konkreta HAL-implementationer för ESP32-hårdvaran

- Mock-implementationer för testning på host (simulera motorkommandon och WiFi-svar)

- Defensive programming med assertions i kritiska kodsökvägar

- Felhantering utan exceptions — felkoder genom hela kodbasen

#### Baskrav (måste uppfyllas)

- HAL-interface som abstraherar minst 2 hårdvarukomponenter

- Motorstyrningslogiken fungerar med både riktig och mockad hårdvara

- Mock som simulerar telemetridata för test utan bil

- Assertions för att fånga programmeringsfel tidigt

- Dokumentation av HAL-gränssnitt och designbeslut

#### Stretch Goals (valfritt)

- Statisk analys med cppcheck eller clang-tidy

- MISRA C++-kontroll av säkerhetskritiska moduler (nödstopp)

- Compile-time interface-verifiering med static_assert

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 4: Strukturerad C++</p>
<p>Kursmål 8: Testbarhet och säkerhet</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av HAL-separerad arkitektur med mock-test på host</td>
</tr>
</tbody>
</table>

### KURSVECKA 10: FELSÖKNING OCH SYSTEMVERIFIERING

|           |                                                          |
|-----------|----------------------------------------------------------|
| **Fokus** | Tillämpa vecka 9: Debugger, mätinstrument och diagnostik |

Systematisk felsökning och verifiering av hela systemet. Mät PWM-signaler med oscilloskop, analysera WiFi-latens, och använd GDB för att debugga race conditions och nödstopp-logik.

#### Implementering

- GDB-debugging med breakpoints och watchpoints i motorstyrnings-logik

- Runtime task-statistik: stackanvändning och CPU-tid per task

- Utökat DIAG-kommando med detaljerad systemstatus

- Identifiera och åtgärda minst 2 buggar i befintlig kod

- Mätning av WiFi-latens (RTT) under körning med loggning

#### Baskrav (måste uppfyllas)

- Dokumenterad felsökningssession med GDB (screenshots/logg)

- Runtime diagnostik tillgänglig via UART DIAG-kommando

- Minst 2 identifierade och åtgärdade buggar dokumenterade

- Stackanvändningsrapport för alla tasks

- Verifiering av nödstopp-timing: \< 500 ms från WiFi-avbrott till motorstopp

#### Stretch Goals (valfritt)

- Coredump-analys vid systemkrasch

- Profiling av task-exekveringstider och nätverkslatens

- Custom panic handler med diagnostikinformation via UART

|             |                                                      |                                                                            |
|-------------|------------------------------------------------------|----------------------------------------------------------------------------|
| **Kursmål** | Kursmål 9: Felsökning med debugger och mätinstrument | **Checkpoint**                                                             |
|             |                                                      | Presentation av felsökningsrapport med identifierade och åtgärdade problem |

### KURSVECKA 11: TESTAUTOMATION OCH SLUTDOKUMENTATION

|           |                                                    |
|-----------|----------------------------------------------------|
| **Fokus** | Tillämpa vecka 10: Enhetstester, testskript och CI |

Skriv automatiserade tester och färdigställ all teknisk dokumentation. Skapa enhetstester för telemetriprotokoll och motorstyrningslogik, testskript som verifierar UART-interface och nödstopp-beteende.

#### Implementering

- Enhetstester för telemetri-serialisering/deserialisering och PWM-beräkningar

- Python-testskript för UART-kommandoverifiering

- Integrationstester som verifierar styrkommando → PWM-flödet

- Komplett teknisk dokumentation med arkitekturdiagram

- Sekvensdiagram: Touch → Dashboard → WiFi → Bil → PWM → Motor

#### Baskrav (måste uppfyllas)

- Minst 5 enhetstester för kritisk applikationslogik

- Minst 1 testskript som automatiserar UART-kommandon och verifierar svar

- Komplett arkitekturdokumentation med blockschema för båda noder

- README med installations- och driftsinstruktioner

- Sekvensdiagram för styrning och nödstopp

#### Stretch Goals (valfritt)

- CI-pipeline med GitHub Actions för automatiska byggen

- Hardware-in-the-loop (HIL) testskript som kör bilen automatiserat

- Mock-WiFi-server för automatiserad integrationstestning

<table>
<colgroup>
<col style="width: 24%" />
<col style="width: 51%" />
<col style="width: 24%" />
</colgroup>
<tbody>
<tr class="odd">
<td><strong>Kursmål</strong></td>
<td><p>Kursmål 5: Automatisering och CI</p>
<p>Kursmål 10: Testautomation</p>
<p>Kursmål 11: Dokumentation</p></td>
<td><strong>Checkpoint</strong></td>
</tr>
<tr class="even">
<td></td>
<td></td>
<td>Demo av testsvit och presentation av dokumentation</td>
</tr>
</tbody>
</table>

### KURSVECKA 12: EXAMINATION

|           |                                |
|-----------|--------------------------------|
| **Fokus** | Slutpresentation och inlämning |

Examinationsvecka. RC-bilen ska vara körredo och presentationsredo. Varje grupp presenterar sitt system med live-demonstration: bilen körs i klassrummet och dashboarden visar live-telemetri på 7-tums pekskärmen.

#### Implementering

- Slutfinish av all kod och dokumentation

- Förbered presentationsmaterial (15-20 minuter)

- Rehearsal av live-demo: kör bilen med dashboard

- Individuell skriftlig reflektion färdigställd

#### Baskrav (måste uppfyllas)

- Komplett fungerande RC-bil med WiFi-styrning och live-telemetri

- All dokumentation färdigställd och inskickad

- Förberedd presentation (15-20 minuter)

- Live-demonstration: bil körs i klassrummet med dashboard på skärmen

- Individuell skriftlig reflektion inlämnad

|             |                  |                                       |
|-------------|------------------|---------------------------------------|
| **Kursmål** | Samtliga kursmål | **Checkpoint**                        |
|             |                  | Slutpresentation och projektinlämning |

## EXAMINATION

Projektet examineras genom tre moment som tillsammans täcker samtliga kursmål.

|                         |                                                                                                                         |             |                    |
|-------------------------|-------------------------------------------------------------------------------------------------------------------------|-------------|--------------------|
| **Moment**              | **Beskrivning**                                                                                                         | **Kursmål** | **Deadline**       |
| Skriftligt kunskapstest | Individuell prövning av teoretiska kunskaper om inbyggda system, RTOS, kommunikationsprotokoll, C++ och testautomation. | 1-5         | Vecka 12 (torsdag) |
| Skriftlig reflektion    | Individuell redogörelse för gruppens lösningar, designval och egna lärdomar.                                            | 1-5         | Vecka 12 (torsdag) |
| Projektinlämning        | Gruppinlämning med kod, dokumentation, tester och live-demonstration.                                                   | 6-11        | Vecka 12 (onsdag)  |

#### Betygskriterier

Icke godkänt (IG): Samtliga kursmål är inte uppfyllda.

Godkänt (G): Samtliga kursmål är uppfyllda.

Väl godkänt (VG): Alla lärandemål är uppfyllda på en tillfredsställande nivå. Utöver detta uppvisar studenten:

- Hög precision att utveckla RTOS-baserade inbyggda system med fokus på realtid, determinism och robusthet

- Stor skicklighet att implementera kommunikation via UART, SPI och I²C i hårdvarunära kod

- Hög precision att utveckla C++-komponenter med fokus på resursbegränsning, säkerhet och testbarhet

## TEKNISKA TIPS OCH RESURSER

#### L298N och motorstyrning

- ENA/ENB är PWM-ingångar för hastighetsreglering — anslut till LEDC-kapabla GPIO

- IN1/IN2 styr riktning för vänster motor, IN3/IN4 för höger motor

- 5V-logik: L298N accepterar 3.3V GPIO-signaler från ESP32 i de flesta fall

- Börja med låg duty cycle (30-40%) — motorer och chassit är känsliga för överspänning

- Lägg till en liten fördröjning vid riktningsbyte för att skydda motorerna

#### WiFi-latens och realtidsstyrning

- UDP föredras för styrkommandon — lägre latens men ingen leveransgaranti

- Typisk RTT på lokalt WiFi: 5-20 ms — tillräckligt för RC-bil-styrning

- Heartbeat-paket var 200 ms + 500 ms nödstoppstimeout ger god säkerhetsmarginal

- Undvik att skicka stora paket (t.ex. video) i samma task som styrkommandon

#### ESP32-CAM specifikt

- ESP32-CAM har begränsad GPIO — kontrollera pinout noggrant mot L298N-kopplingen

- GPIO 0 används av kameran — använd ej för motorstyrning

- PSRAM finns tillgängligt för framebuffer om video-streaming implementeras

- Flash-knapp (GPIO 0 till GND) krävs vid flashning — bygg in i montaget

#### FreeRTOS best practices

- Motor-task behöver hög prioritet och liten stack (motorstyrning är enkel men tidskritisk)

- Network-task behöver stor stack (minst 4096 bytes) pga WiFi/TCP

- Använd xQueueSend/xQueueReceive med timeout — aldrig portMAX_DELAY i produktion

- Aktivera configCHECK_FOR_STACK_OVERFLOW (nivå 2) under utveckling

#### Viktiga resurser

- ESP32-CAM Pinout: https://randomnerdtutorials.com/esp32-cam-ai-thinker-pinout/

- L298N datablad: https://www.st.com/resource/en/datasheet/l298.pdf

- ESP-IDF LEDC (PWM): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/ledc.html

- FreeRTOS API Reference: https://www.freertos.org/a00106.html

- LVGL för ESP32-S3: https://docs.lvgl.io/master/get-started/platforms/espressif.html

## KURSMÅL — REFERENS

Följande kursmål ska uppfyllas genom projektarbetet och de teoretiska examinationsmomenten.

#### Kunskaper

- 1\. Förklara hur utveckling för inbyggda system skiljer sig från traditionell programutveckling gällande resurser, realtid och driftsäkerhet.

- 2\. Redogöra för RTOS-principer, schemaläggning, avbrottshantering och realtidskrav.

- 3\. Förklara hur kommunikationsprotokoll som UART, SPI och I²C används i industriella miljöer.

- 4\. Förklara hur C++ kan användas för strukturerad och prestandaeffektiv kod i resursbegränsade system.

- 5\. Redogöra för hur automatisering och CI bidrar till stabilitet, testbarhet och driftsäkerhet i inbyggda system.

#### Färdigheter

- 6\. Utveckla ett RTOS-baserat inbyggt system med fokus på realtid, determinism och robusthet.

- 7\. Implementera kommunikation via UART, SPI och I²C i hårdvarunära kod.

- 8\. Utveckla komponenter i C++ med fokus på resursbegränsning, säkerhet och testbarhet.

- 9\. Utföra felsökning med debugger, mätinstrument och hårdvarunära verktyg.

- 10\. Skriva skript för enkel testautomation och verifiering.

- 11\. Dokumentera hårdvarunära design, systemflöden och integration enligt industriell standard.

|            |             |                                           |
|------------|-------------|-------------------------------------------|
| **Veckor** | **Kursmål** | **Fokusområde**                           |
| Vecka 1-2  | 1, 6, 11    | Hårdvarumontage, ESP-IDF, WiFi-anslutning |
| Vecka 3-4  | 2, 6        | FreeRTOS tasks, motorstyrning, nödstopp   |
| Vecka 5    | 2, 6        | Avbrott, timrar, watchdog                 |
| Vecka 6    | 3, 7        | UART-diagnostik, bitmanipulation          |
| Vecka 7    | 3, 7        | Dashboard-display via SPI, touch via I²C  |
| Vecka 8-9  | 4, 8        | C++ RAII, HAL, testbar design             |
| Vecka 10   | 9           | Felsökning, debugging, signalanalys       |
| Vecka 11   | 5, 10, 11   | Testautomation, CI, dokumentation         |
| Vecka 12   | Samtliga    | Examination och presentation              |

## AVSLUTANDE ORD

RC-Bil med Live-Telemetri är ett projekt som kombinerar det bästa av inbyggda system med något genuint kul att demonstrera. En bil som körs i klassrummet — styrd från en 7-tums pekskärm med live-telemetri — är ett demo som fastnar i minnet.

Projektet täcker samtliga kursmål på ett naturligt sätt: WiFi-kommunikationen ger verkliga latenskrav, nödstopp-logiken kräver korrekt RTOS-synkronisering, och motorstyrningen är hårdvarunärt nog att kräva oscilloskop och debugger för att verifiera.

Kom ihåg:

- Börja med låg hastighet — det är lättare att felsöka en bil som rör sig sakta

- Implementera nödstoppet tidigt — det skyddar hårdvaran under hela projektet

- Testa WiFi-latensen under verkliga förhållanden — klassrumsmiljöer kan vara brusiga

- Dokumentera löpande — inte bara i slutet

- Organisera hårdvaruåtkomsten inom gruppen tidigt

- Fråga om hjälp vid checkpoints — vi löser hårdvaruproblem tillsammans

**Lycka till med projektet!**

CHAS ACADEMY AB

Arenavägen 61, 121 77 Johanneshov, Sweden
