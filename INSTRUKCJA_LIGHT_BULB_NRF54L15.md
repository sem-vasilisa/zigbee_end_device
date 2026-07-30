# Jak zbudować Zigbee Light Bulb (Router) na nRF54L15

Budujemy "żarówkę": urządzenie, które **dołącza** do sieci Coordinatora
(rola: **Router**), wystawia cluster **On/Off w roli server** i na komendę
Toggle zapala/gasi diodę LED0.

To jest wersja instrukcji na **nRF54L15** (DK albo własna płytka). Wersja pod
nRF52840 Dongle: [INSTRUKCJA_LIGHT_BULB.md](INSTRUKCJA_LIGHT_BULB.md).
Logika Zigbee jest **identyczna** — zmienia się wyłącznie warstwa sprzętowa.

Co się różni względem dongla:

| | nRF52840 Dongle | nRF54L15 |
|---|---|---|
| Pamięć programu | Flash 1 MB | **RRAM 1524 kB** |
| Timer dla ZBOSS | `timer2` | **`timer20`** |
| Logi | USB CDC ACM (wirtualny COM) | **UART20 → VCOM debuggera** |
| Mapa pamięci | `pm_static.yml` (ręczna, przez bootloader) | **generowana automatycznie** |
| Flashowanie | `nrfutil dfu usb-serial` + przycisk RESET | **`west flash` przez SWD** |
| Rdzeń radiowy | ten sam rdzeń | ten sam rdzeń (nie jak w 5340!) |
| Dedykowane RAM/RRAM dla FLPR | brak | **trzeba odzyskać w overlayu** |

Kod aplikacji (`src/main.c`, `src/zb_light_bulb.h`) — **bez jednej zmiany**.

---

## 1. Struktura projektu

```
zigbee_light_bulb/
├── CMakeLists.txt
├── prj.conf                                    ← wspólny dla obu płytek
├── pm_static_nrf52840dongle_nrf52840.yml       ← było: pm_static.yml
├── boards/
│   ├── nrf54l15dk_nrf54l15_cpuapp.overlay      ← NOWY: timer + RRAM/SRAM
│   ├── nrf52840dongle_nrf52840.overlay         ← stare, dongle
│   └── nrf52840dongle_nrf52840.conf            ← USB CDC tylko dla dongla
└── src/
    ├── main.c
    └── zb_light_bulb.h
```

**`pm_static.yml` przestaje obowiązywać.** Był potrzebny na donglu, bo tam za
flashem siedzi bootloader USB DFU i partycje trzeba było wbić ręcznie — z
adresami układu 52840, które na 54L15 nie mają sensu (RRAM, inny layout).
Na nRF54L15 nie ma bootloadera, a partycje `zboss_nvram` i
`zboss_product_config` Partition Manager dokłada sam — definicja jest w
`nrf/subsys/partition_manager/pm.yml.zboss` i mówi tylko "za `app`,
o rozmiarze `CONFIG_PM_PARTITION_SIZE_ZBOSS_NVRAM`".

Zamiast kasować plik, **zmień mu nazwę na
`pm_static_nrf52840dongle_nrf52840.yml`**. Partition Manager szuka najpierw
`pm_static_<board_target>.yml`, a dopiero potem ogólnego `pm_static.yml`, więc
dongle dalej dostaje swoją mapę, a 54L15 idzie ścieżką automatyczną. Ta sama
sztuczka co z plikiem `.conf` dla USB.

### 1.1 `CMakeLists.txt`

Bez zmian:

```cmake
cmake_minimum_required(VERSION 3.20.0)

list(APPEND EXTRA_ZEPHYR_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/../ncs-zigbee
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zigbee_light_bulb)

target_sources(app PRIVATE src/main.c)
```

### 1.2 `prj.conf`

```conf
# Zephyr defaults
CONFIG_NCS_SAMPLES_DEFAULTS=y

# Serial — wymagane przez libzboss.a
CONFIG_SERIAL=y
CONFIG_UART_INTERRUPT_DRIVEN=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
CONFIG_GPIO=y

# USB CDC ACM — tylko dongle, patrz boards/nrf52840dongle_nrf52840.conf
# (nRF54L15 nie ma USB — logi idą przez UART/VCOM debuggera)

# Logging
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3

# Pamięć
CONFIG_HEAP_MEM_POOL_SIZE=2048
CONFIG_MAIN_THREAD_PRIORITY=7
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048

# Zigbee — ROUTER (nie COORDINATOR!)
CONFIG_ZIGBEE_ADD_ON=y
CONFIG_ZIGBEE_APP_UTILS=y
CONFIG_ZIGBEE_ROLE_ROUTER=y

# Wyłącz Zephyr networking
CONFIG_NET_IPV6=n
CONFIG_NET_IP_ADDR_CHECK=n
CONFIG_NET_UDP=n
```

Jedyna zmiana względem wersji dongla: **`CONFIG_USB_DEVICE_STACK` /
`CONFIG_USB_CDC_ACM` wyleciały do pliku per-płytka**. nRF54L15 fizycznie nie
ma peryferium USB — z tymi opcjami build padnie na braku węzła devicetree.
Zephyr sam dociąga `boards/<nazwa_płytki>.conf` do konfiguracji, więc dongle
dostaje USB, a 54L15 nie, bez żadnych `#ifdef`.

`CONFIG_SERIAL=y` zostaje. Na 54L15 konsola idzie przez `uart20`, wystawiony
na DK jako VCOM (drugi port COM z J-Linka).

### 1.3 `boards/nrf54l15dk_nrf54l15_cpuapp.overlay`

To jest właściwa nowość. Pełny plik:
[boards/nrf54l15dk_nrf54l15_cpuapp.overlay](boards/nrf54l15dk_nrf54l15_cpuapp.overlay).
Trzy rzeczy, każda z konkretnego powodu:

```dts
/ {
    chosen {
        ncs,zigbee-timer = &timer20;   /* 1 */
    };
};

&cpuapp_rram { reg = <0x0 DT_SIZE_K(1524)>; };                        /* 2 */
&cpuapp_sram { reg = <0x20000000 DT_SIZE_K(256)>;
               ranges = <0x0 0x20000000 0x40000>; };

&uart20  { /delete-property/ hw-flow-control; };                       /* 3 */
&timer20 { status = "okay"; };
```

1. **`ncs,zigbee-timer`** — ZBOSS potrzebuje wyłącznego timera sprzętowego do
   odmierzania beacon intervals (802.15.4 liczy czas w kwantach ~15,36 ms).
   Na donglu to był `timer2`; na 54L15 peryferia są ponumerowane per domena
   zasilania i właściwy jest `timer20`. Bez tego `chosen` build przechodzi,
   a `zigbee_enable()` wywala się w runtime.
2. **Odzyskanie RRAM i SRAM.** nRF54L15 ma drugi rdzeń — **FLPR** (RISC-V
   coprocesor). Domyślne devicetree rezerwuje mu kawałek RRAM i SRAM, nawet
   jeśli go nie używasz. My go nie używamy, a stos Zigbee jest gruby, więc
   oddajemy sobie całość: 1524 kB RRAM i 256 kB SRAM. Objaw pominięcia to
   `region 'RRAM' overflowed by N bytes` przy linkowaniu.
3. **HWFC na uart20** — na DK sprzętowa kontrola przepływu jest domyślnie
   włączona i psuje konsolę (znane, w samplach Nordica jest ten sam
   `/delete-property/` z komentarzem "TODO: re-enable once fixed").

> Zwróć uwagę, czego tu **nie** ma: żadnej konfiguracji rdzenia sieciowego.
> Na nRF5340 radio 802.15.4 siedzi na osobnym rdzeniu i trzeba budować obraz
> `ipc_radio`. nRF54L15 ma radio na tym samym rdzeniu co aplikacja — jest
> pod tym względem prostszy, jak 52840.

### 1.4 `src/zb_light_bulb.h` i `src/main.c`

**Bez zmian.** Definicja urządzenia ZCL (On/Off Light, device ID `0x0100`,
EP 20, trzy clustery server), `zcl_device_cb`, `zboss_signal_handler`,
kolejność w `main()` — wszystko to jest warstwą ponad sprzętem i przenosi się
1:1. Omówienie w rozdziale 1.3–1.4 [instrukcji dla dongla](INSTRUKCJA_LIGHT_BULB.md).

Jedyny detal wart odnotowania: **LED na nRF54L15 DK jest active-HIGH**
(`gpios = <&gpio2 9 GPIO_ACTIVE_HIGH>`), a na donglu był active-LOW. Kod
tego nie zauważa, bo `gpio_pin_set_dt()` czyta polaryzację z devicetree —
`1` znaczy "świeć" na obu płytkach. To jest dokładnie ten powód, dla którego
warto używać `_dt` wariantów API zamiast `gpio_pin_set()` z gołym numerem.

---

## 2. Budowanie i flashowanie

### 2.1 nRF54L15 DK

1. VS Code → nRF Connect → **Add build configuration** → board target
   **`nrf54l15dk/nrf54l15/cpuapp`** → Build.

   Z linii poleceń:
   ```bash
   west build -b nrf54l15dk/nrf54l15/cpuapp --sysbuild
   ```

2. Flash — DK ma wbudowany J-Link, więc żadnych zipów i przycisków RESET:
   ```bash
   west flash
   ```
   albo przycisk **Flash** w VS Code.

3. Logi: podłącz Serial Terminal do portu VCOM (nie tego od DFU — 54L15 go
   nie ma), **115200 8N1**.

To jest największa praktyczna wygoda względem dongla: **znika cała ceremonia
z `nrfutil pkg generate`, `--application-version` i licznikiem wersji per
urządzenie**. Flashujesz przez SWD, ile razy chcesz, tym samym obrazem.

Jeśli urządzenie nie chce się zaprogramować (fabrycznie zablokowany dostęp
debugowy):
```bash
nrfutil device recover
```

### 2.2 Własna płytka z nRF54L15

Overlay wyżej zakłada devicetree DK. Dla własnej płytki masz dwie drogi:

**Droga na skróty (na start, do sprawdzenia że radio działa):** buduj jako
`nrf54l15dk/nrf54l15/cpuapp` i dołóż własny overlay z Twoimi pinami:

```dts
/ {
    aliases { led0 = &my_led; };

    leds {
        compatible = "gpio-leds";
        my_led: led_0 {
            gpios = <&gpio1 11 GPIO_ACTIVE_HIGH>;   /* ← Twój pin */
        };
    };
};
```

Zadziała, o ile Twój układ to pełny nRF54L15 (nie L10/L05) i nie ruszałeś
UART-a. Do zabawy w sam Zigbee to wystarczy.

**Droga właściwa:** własna definicja płytki w `boards/` workspace'u —
`board.yml`, `<board>.dts`, `<board>_defconfig`. Skopiuj katalog
`zephyr/boards/nordic/nrf54l15dk` jako punkt startowy i wytnij, czego nie
masz. Wtedy `west build -b moja_plytka/nrf54l15/cpuapp` i overlay z tego
projektu przestaje być potrzebny w części pinowej.

Do sprawdzenia na własnej płytce, niezależnie od drogi:

- **Kwarc 32 kHz (LFXO)** — jeśli go nie zamontowałeś, w devicetree trzeba
  przełączyć źródło zegara na wewnętrzny RC (`&lfxo { status = "disabled"; }`
  + odpowiedni `CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC`). Zigbee działa na RC,
  ale rozjazd czasu psuje synchronizację przy dłuższych okresach uśpienia.
- **Antena i strojenie** — jeśli zasięg jest fatalny (kilkadziesiąt cm),
  to prawie zawsze matching network, nie firmware.
- **Pin do programowania** — SWDIO/SWDCLK wyprowadzone; można programować
  z nRF54L15 DK przez złącze **Debug Out**.

---

## 3. Test całości — Coordinator + Light Bulb

Przebieg identyczny jak w wersji dla dongla, bo protokół nie wie, na czym
działa. Coordinator może zostać na donglu — **nRF52840 i nRF54L15 gadają ze
sobą bez problemu**, to ta sama warstwa 802.15.4 i ten sam Zigbee R23.

1. Coordinator startuje i otwiera sieć (`open`, jeśli minęło >180 s).
2. Żarówka dołącza:
   ```
   I: Starting Zigbee Light Bulb (Router)
   I: Joining network for the first time...
   I: Joined network OK
   I: Our short addr: 0x1234
   ```
3. Na Coordinatorze: `name lampa`, potem `toggle lampa`.
4. Na żarówce:
   ```
   I: On/Off attribute changed to: 1
   I: Setting On/Off value: ON
   ```

### Najczęstsze problemy — specyficzne dla nRF54L15

- **`region 'RRAM' overflowed`** — brakuje sekcji `&cpuapp_rram` w overlayu
  (patrz 1.3, punkt 2).
- **Build pada na `CONFIG_USB_DEVICE_STACK`** — zostawiłeś opcje USB
  w `prj.conf`. Przenieś do `boards/nrf52840dongle_nrf52840.conf`.
- **`zigbee_enable()` crashuje / brak reakcji radia** — brak
  `chosen { ncs,zigbee-timer = &timer20; }` albo `timer20` nie ma
  `status = "okay"`.
- **Błędy o adresach partycji / `zboss_nvram`** — nie usunąłeś
  `pm_static.yml` z dongla.
- **Cisza na konsoli** — zły port COM (na DK jest ich kilka) albo nie
  wyłączyłeś `hw-flow-control` na `uart20`.
- **`west flash` nie widzi urządzenia** — `nrfutil device list`, a jeśli
  urządzenie jest zablokowane: `nrfutil device recover`.

Pułapki wspólne z wersją dongla (brak `CONFIG_SERIAL=y`, brak
`#include <zb_mem_config_max.h>`, rejestracja device ctx po `zigbee_enable()`,
stary PAN ID w NVRAM po resecie Coordinatora) obowiązują nadal.

---

## 4. Rozbudowa

- **Level Control (0x0008) — ściemnianie.** Na 54L15 pamiętaj o ograniczeniu
  sprzętowym: PWM i GPIO muszą być w tej samej domenie zasilania, więc
  `pwm20` obsłuży tylko piny portu **P1**. Na DK z PWM działa LED1, nie LED0.
  Wzór w `ncs-zigbee/samples/light_bulb/boards/nrf54l15dk_nrf54l15_cpuapp.overlay`.
- **Lokalny przycisk** — `sw0` (P1.13, active-low z pull-upem) przełącza stan
  lokalnie i raportuje atrybut do sieci.
- **End Device na baterii** — patrz rozdział 5 (ogólnie) i **rozdział 6**
  (konkretnie: płytka `internal_btz`). **nRF54L15 to jedyny z tych układów,
  który się do tego naprawdę nadaje** (~1 µA w System OFF, sporo lepiej niż
  52840).

---

## 5. Wariant bateryjny (End Device)

Jeśli celem jest urządzenie na baterii, zmiany są zaskakująco małe — ale
**żarówka jest do tego złym kandydatem**. Sleepy End Device śpi i odbiera dane
dopiero gdy odpyta rodzica, więc "zapal" przychodzi z opóźnieniem 0,5–3 s.
Dlatego w Zigbee żarówki to zawsze Routery na zasilaniu sieciowym, a na
baterii chodzą **czujniki i przyciski**, które same nadają. Poniższe zmiany
mają sens dopiero po przerobieniu clusterów na coś nadającego.

**`prj.conf`:**
```conf
-CONFIG_ZIGBEE_ROLE_ROUTER=y
+CONFIG_ZIGBEE_ROLE_END_DEVICE=y

+CONFIG_RAM_POWER_DOWN_LIBRARY=y
+CONFIG_PM_DEVICE=y

# do pomiarów prądu — patrz snippet low_power w samplu light_switch
+CONFIG_SERIAL=n
+CONFIG_CONSOLE=n
+CONFIG_LOG=n
```

**`main.c`** — dopisz przed `zigbee_enable()`:
```c
zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(30000));
zigbee_configure_sleepy_behavior(true);        /* rx_on_when_idle = false */

if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
    power_down_unused_ram();
}
```
plus `zb_zdo_pim_set_long_poll_interval(3000);` do sterowania tempem
odpytywania i `power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY` w
`app_clusters_attr_init()`.

Warto też zamienić `zb_mem_config_max.h` na `zb_mem_config_min.h` — End
Device nie prowadzi tablic routingu, oszczędzasz kilkanaście kB RAM.

Reszta — clustery, `zcl_device_cb`, `zboss_signal_handler` — bez zmian;
sygnały ZBOSS dla ED są te same. Gotowy wzorzec z obsługą trybu low power:
`ncs-zigbee/samples/light_switch` (snippet `low_power`).

---

## 6. Sleepy End Device na płytce `internal_btz`

Płytka [`../internal_btz`](../internal_btz/Internal_NORDIC_-Project-) (GoodByte
Hardware, "BLE/Thread/Zigbee EndDevice", 35 × 35 mm) jest projektowana **dokładnie
pod ten scenariusz**. Poniżej wszystko wyciągnięte z netlisty KiCada
(`Internal_NORDIC_Project-netlist.net`) — nie z domysłów.

### 6.1 Co jest na płytce

| Funkcja | Pin nRF54L15 | Szczegóły |
|---|---|---|
| I²C SDA | **P0.03** | pull-up R5 10k |
| I²C SCL | **P0.04** | pull-up R4 10k |
| BMI270 INT1 / INT2 | **P0.01 / P0.02** | IMU 6-osiowy, wake-on-motion |
| Załączanie szyny czujników | **P0.00** (`~VDD_susp_EN`) | P-MOSFET Q2, **aktywny stanem niskim** |
| Przycisk użytkownika | **P1.14** | SW2 do masy przez R9 1k, C18 10nF (debounce) → **active-low, wymaga pull-upa** |
| LED użytkownika | **P2.08** | anoda do pinu, katoda przez R3 3,3k do GND → **active-high** |
| Załączanie zasilania zewn. | **P2.06** (`~PW_SW_EN`) | P-MOSFET Q3 → wyjście na J4 |
| LFXO 32,768 kHz | **P1.00 / P1.01** | kwarc Y2, bez zewnętrznych kondensatorów |
| HFXO 32 MHz | XC1 / XC2 | kwarc Y1, bez zewnętrznych kondensatorów |
| DC/DC | DCC / DECD | dławik L1 4,7 µH **jest zamontowany** |
| NFC | P1.02 / P1.03 | złącze J5 |
| Wolne GPIO | P1.04–P1.13 (J2), P2.00–P2.05 (J4) | wyprowadzone na goldpiny |
| Debug | J3 | 10-pin Cortex SWD |
| Zasilanie | BT1 → Q1 | automatyczny power-path: gdy jest +3,3 V z debuggera, bateria jest odcięta |

Czujniki na szynie **`VDD_susp`** (przełączanej przez P0.00):

| Układ | Co mierzy | Sterownik w Zephyrze |
|---|---|---|
| **BMI270** (U2) | akcelerometr + żyroskop | ✅ `bosch,bmi270-i2c` |
| **STS4x** (U3) | temperatura ±0,2 °C | ⚠️ brak dedykowanego — patrz 6.6 |
| **LTR-329ALS-01** (U4) | natężenie światła 0,01–64k lux | ❌ **brak, trzeba napisać** |

### 6.2 Dlaczego ta płytka jest dobrym kandydatem

Cztery decyzje projektowe, które realnie decydują o tym, czy SED pociągnie
z baterii miesiącami czy tygodniami — i wszystkie są tu zrobione dobrze:

1. **Kwarc 32,768 kHz (Y2) jest zamontowany.** Sleepy End Device budzi się co
   kilka sekund, żeby odpytać rodzica. Na wewnętrznym RC zegar dryfuje i okno
   odbiorcze trzeba otwierać z zapasem — czyli radio siedzi włączone dłużej,
   niż musi. LFXO to jest ten element, który zamienia "działa" w "działa długo".
2. **DC/DC (L1) jest zamontowany.** Trzeba go **jawnie włączyć** w devicetree,
   inaczej układ jedzie na LDO i tracisz ~30–40 % prądu w trybie aktywnym za
   darmo. To najczęstszy pominięty krok przy własnej płytce.
3. **Czujniki na przełączanej szynie `VDD_susp`.** Spoczynkowy prąd BMI270 +
   LTR-329 + STS4x to jednostki µA — porównywalnie z całym uśpionym nRF54L15.
   Możliwość odcięcia im zasilania jednym GPIO to nie jest ozdobnik.
4. **I²C wylądowało na P0.03/P0.04, czyli na `gpio0`.** Na nRF54L15 peryferia
   mogą używać wyłącznie pinów ze swojej domeny zasilania, a `gpio0` (tylko
   5 pinów: P0.00–P0.04 — płytka zajmuje wszystkie) siedzi w domenie
   niskiego poboru razem z **`i2c30`**. To znaczy, że możesz odpytać czujnik
   bez wybudzania głównej domeny peryferiów. Gdyby I²C poszło na P1, byłoby
   drożej. Nie wiem, czy to była świadoma decyzja, ale wyszło korzystnie.

Konsekwencje, o których trzeba pamiętać:

- **P1.00 i P1.01 są zajęte przez LFXO** — nie są dostępne jako GPIO.
- **`gpio2` (P2.x) nie ma instancji GPIOTE** — na P2 nie zrobisz przerwania
  ani wybudzenia. LED (P2.08) to wyjście, więc bez znaczenia; ale przycisk
  jest na P1.14 (`gpiote20`) i to jest jedyny powód, dla którego może służyć
  jako źródło wybudzenia.

### 6.3 Definicja płytki

Overlay na `nrf54l15dk` tu nie wystarczy — pinout jest inny na tyle, że robi
się bałagan. Zrób własną definicję w `zigbee/boards/goodbyte/internal_btz/`
(katalog `boards/` obok `zephyr/` i `nrf/`, żeby west go widział):

```
boards/goodbyte/internal_btz/
├── board.yml
├── Kconfig.internal_btz
├── internal_btz_nrf54l15_cpuapp.dts
├── internal_btz_nrf54l15_cpuapp.yaml
└── internal_btz_nrf54l15_cpuapp_defconfig
```

Skopiuj `zephyr/boards/nordic/nrf54l15dk` jako punkt startowy i podmień
warstwę sprzętową. Sedno `.dts`:

```dts
/ {
    aliases {
        led0 = &user_led;
        sw0  = &user_sw;
    };

    leds {
        compatible = "gpio-leds";
        user_led: led_0 {
            gpios = <&gpio2 8 GPIO_ACTIVE_HIGH>;    /* P2.08, anoda do pinu */
        };
    };

    buttons {
        compatible = "gpio-keys";
        user_sw: button_0 {
            gpios = <&gpio1 14 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;  /* P1.14 */
            zephyr,code = <INPUT_KEY_0>;
        };
    };

    /* Szyna czujników — P0.00 aktywny stanem NISKIM (bramka P-MOSFETa Q2) */
    vdd_susp: vdd_susp {
        compatible = "regulator-fixed";
        regulator-name = "VDD_susp";
        enable-gpios = <&gpio0 0 GPIO_ACTIVE_LOW>;
        startup-delay-us = <2000>;
        regulator-boot-on;
    };
};

/* DC/DC — L1 jest zamontowany, WŁĄCZ GO */
&vregmain {
    status = "okay";
    regulator-initial-mode = <NRF5X_REG_MODE_DCDC>;
};

/* Kwarce bez zewnętrznych kondensatorów → wewnętrzne load caps.
 * Wartości poniżej są z DK — SPRAWDŹ W KARCIE KATALOGOWEJ SWOICH kwarców.
 */
&lfxo { load-capacitors = "internal"; load-capacitance-femtofarad = <15500>; };
&hfxo { load-capacitors = "internal"; load-capacitance-femtofarad = <15000>; };

/* I²C w domenie niskiego poboru — patrz 6.2 punkt 4 */
&i2c30 {
    status = "okay";
    pinctrl-0 = <&i2c30_default>;
    pinctrl-1 = <&i2c30_sleep>;
    pinctrl-names = "default", "sleep";

    bmi270: bmi270@68 {                 /* SDO do GND → adres 0x68 */
        compatible = "bosch,bmi270";
        reg = <0x68>;
        irq-gpios = <&gpio0 1 GPIO_ACTIVE_HIGH>;   /* INT1 = P0.01 */
    };

    /* LTR-329 = 0x29, STS4x = 0x4A (potwierdź w kartach katalogowych) */
};

&pinctrl {
    i2c30_default: i2c30_default {
        group1 {
            psels = <NRF_PSEL(TWIM_SDA, 0, 3)>, <NRF_PSEL(TWIM_SCL, 0, 4)>;
        };
    };
    i2c30_sleep: i2c30_sleep {
        group1 {
            psels = <NRF_PSEL(TWIM_SDA, 0, 3)>, <NRF_PSEL(TWIM_SCL, 0, 4)>;
            low-power-enable;           /* ← krytyczne, patrz niżej */
        };
    };
};

/* Zigbee — jak na DK */
/ { chosen { ncs,zigbee-timer = &timer20; }; };
&timer20 { status = "okay"; };

&cpuapp_rram { reg = <0x0 DT_SIZE_K(1524)>; };
&cpuapp_sram { reg = <0x20000000 DT_SIZE_K(256)>;
               ranges = <0x0 0x20000000 0x40000>; };
```

> **`low-power-enable` w stanie `sleep` to nie jest optymalizacja, tylko
> warunek poprawności.** Rezystory podciągające I²C (R4, R5) wiszą na
> `VDD_susp`, czyli na szynie, którą wyłączasz. Jeśli po odcięciu szyny
> nRF dalej trzyma SDA/SCL w stanie wysokim, prąd popłynie przez diody ESD
> czujników do martwej szyny — grzejesz baterię i częściowo zasilasz układy,
> które właśnie próbowałeś wyłączyć. Piny muszą zostać odłączone.

### 6.4 `prj.conf`

Nad tym, co w rozdziale 5:

```conf
CONFIG_ZIGBEE_ROLE_END_DEVICE=y
CONFIG_ZIGBEE_ADD_ON=y
CONFIG_ZIGBEE_APP_UTILS=y

# Low power
CONFIG_PM=y
CONFIG_PM_DEVICE=y
CONFIG_PM_DEVICE_RUNTIME=y          # i2c30 ma zephyr,pm-device-runtime-auto
CONFIG_RAM_POWER_DOWN_LIBRARY=y
CONFIG_SERIAL=n
CONFIG_CONSOLE=n
CONFIG_LOG=n

# Czujniki
CONFIG_I2C=y
CONFIG_SENSOR=y
CONFIG_BMI270=y
CONFIG_BMI270_TRIGGER_GLOBAL_THREAD=y

# Regulator szyny czujników
CONFIG_REGULATOR=y
CONFIG_REGULATOR_FIXED=y

CONFIG_NET_IPV6=n
CONFIG_NET_IP_ADDR_CHECK=n
CONFIG_NET_UDP=n
```

`CONFIG_PM_DEVICE_RUNTIME=y` jest tu istotne: węzeł `i2c30` w Zephyrze ma
`zephyr,pm-device-runtime-auto`, więc TWIM sam się wyłącza między transakcjami
i wchodzi w stan `sleep` pinctrl. Bez tego sterownik trzyma peryferium
włączone cały czas.

### 6.5 Aplikacja

Zamiast On/Off servera (żarówka) wystawiasz **czujnik**. Endpoint z clusterami:

| Cluster | ID | Rola | Po co |
|---|---|---|---|
| Basic | 0x0000 | server | `power_source = BATTERY` |
| Identify | 0x0003 | server | wymagany |
| **Power Configuration** | 0x0001 | server | stan baterii — patrz uwaga w 6.6 |
| **Temperature Measurement** | 0x0402 | server | STS4x |
| **Illuminance Measurement** | 0x0400 | server | LTR-329 |

Szkielet pętli pomiarowej — sedno to **włącz szynę, poczekaj, zmierz, wyłącz**:

```c
static const struct device *vdd_susp = DEVICE_DT_GET(DT_NODELABEL(vdd_susp));
static const struct device *bmi270   = DEVICE_DT_GET(DT_NODELABEL(bmi270));

static void measure_and_report(zb_uint8_t param)
{
    regulator_enable(vdd_susp);
    k_sleep(K_MSEC(10));                  /* czas startu czujników */

    struct sensor_value temp;
    sensor_sample_fetch(sts4x);
    sensor_channel_get(sts4x, SENSOR_CHAN_AMBIENT_TEMP, &temp);

    regulator_disable(vdd_susp);

    /* ZCL: temperatura w setnych stopnia, int16 */
    zb_int16_t zcl_temp = temp.val1 * 100 + temp.val2 / 10000;
    ZB_ZCL_SET_ATTRIBUTE(SENSOR_ENDPOINT,
        ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, (zb_uint8_t *)&zcl_temp, ZB_FALSE);

    /* Następny pomiar za 60 s — przez scheduler ZBOSS, nie k_sleep! */
    ZB_SCHEDULE_APP_ALARM(measure_and_report, 0,
        ZB_MILLISECONDS_TO_BEACON_INTERVAL(60000));
}
```

Zapis atrybutu przez `ZB_ZCL_SET_ATTRIBUTE` uruchamia mechanizm **attribute
reporting** — nie wysyłasz nic ręcznie, ZBOSS sam wyśle raport zgodnie
z konfiguracją, którą Coordinator ustawił komendą Configure Reporting. To jest
właściwy sposób; ręczne `ZB_ZCL_SEND_REPORT` omija ustawienia koordynatora.

W `main()`, przed `zigbee_enable()` — jak w rozdziale 5:

```c
zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(30000));
zigbee_configure_sleepy_behavior(true);
if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) power_down_unused_ram();
```

**Wake-on-motion.** BMI270 ma dwa wyprowadzone przerwania (P0.01, P0.02) na
`gpio0`, czyli z GPIOTE30 — potrafią wybudzić układ z uśpienia. Jeśli
urządzenie ma reagować na ruch, skonfiguruj trigger BMI270 zamiast odpytywania
z timera. To zmienia charakterystykę poboru z "budzę się co minutę" na
"śpię, dopóki nic się nie dzieje".

### 6.6 Do sprawdzenia i do napisania

Rzeczy, które trzeba rozwiązać, zanim to pojedzie:

- **Sterownik LTR-329 nie istnieje w Zephyrze.** Jest `ltrf216a` — to **inny
  układ** (adres 0x53, inna mapa rejestrów, gotowe luksy na wyjściu), nie
  podmienisz. Ale **pełny sterownik Zephyra nie jest tu potrzebny**: czujnik
  obsłużysz bezpośrednio przez `i2c_reg_write_byte` / `i2c_burst_read`
  z kodu aplikacji, ~40 linii. Dla porównania `ltrf216a.c` ma 176 linii i to
  jest cena za API `sensor_*`, którego przy jednym czujniku nie wykorzystasz —
  tym bardziej że czujnik siedzi na przełączanej szynie, więc cyklem jego
  zasilania i tak sterujesz ręcznie.

  LTR-329 zwraca **dwa kanały 16-bit**: CH0 (widzialne + IR) i CH1 (samo IR).
  Luksy liczy się z `ratio = CH1/(CH0+CH1)` wzorem odcinkowym z appendiksu
  karty katalogowej — bez tego zgłaszasz surowe zliczenia, nie luksy.
  Czytaj **CH1 przed CH0** (jeden burst od 0x88): odczyt pierwszego bajtu
  zatrzaskuje komplet danych, odwrotna kolejność miesza dwa pomiary.
- **STS4x nie ma dedykowanego bindingu.** Jest `sensirion,sht4x` (SHT4x =
  temperatura + wilgotność, STS4x = sama temperatura). Zestaw komend jest
  w dużej mierze wspólny, więc prawdopodobnie zadziała dla kanału
  temperatury — ale **zweryfikuj na sprzęcie**, nie zakładaj. Kanał
  wilgotności będzie zwracał śmieci.
- **Nie ma dzielnika napięcia baterii na ADC.** W netliście nie ma nic, co
  łączyłoby `VBat` z pinem SAADC — R1 i R7 to rezystory podciągające bramki
  MOSFET-ów. Skutkiem jest to, że cluster Power Configuration nie ma czym
  wypełnić `BatteryVoltage`. Opcje: sprawdź w dokumentacji nRF54L15, czy
  SAADC ma wewnętrzny kanał pomiaru VDD (na nRF52 był); jeśli nie — zostaje
  zgłaszanie samego `BatteryAlarmMask` albo dzielnik w rewizji 2 płytki.
- **Wartości `load-capacitance-femtofarad`** w `.dts` wpisałem z DK. Zależą od
  konkretnego kwarcu — weź je z karty katalogowej Y1 i Y2, bo zły dobór to
  rozjazd częstotliwości i problemy z zasięgiem.
- **Adresy I²C** (0x68 / 0x29 / 0x4A) wynikają ze sposobu podłączenia pinów
  adresowych — BMI270 ma SDO do masy, więc 0x68 jest pewne; pozostałe dwa
  potwierdź skanem magistrali.

### 6.7 Kolejność uruchamiania

Nie rób wszystkiego naraz — na własnej płytce każdy z tych kroków może
wysypać się z innego powodu:

1. **Blinky** — LED na P2.08, bez Zigbee. Sprawdza zasilanie, zegar, SWD.
2. **Skan I²C** — potwierdź adresy czujników przy włączonej szynie `VDD_susp`.
   Od razu widać, czy sterowanie P0.00 działa i czy nie odwróciłeś polaryzacji.
3. **Zigbee jako zwykły Router** (kod z tej instrukcji, tylko z nowym boardem)
   — sprawdza radio, antenę, kwarce, partycje. Jeszcze bez oszczędzania.
4. **Przełączenie na End Device**, dalej z logami i UART-em.
5. **Dopiero na końcu low power** — `CONFIG_SERIAL=n`, uśpienia, pomiar prądu.
   Jak zrobisz to wcześniej, stracisz jedyne narzędzie diagnostyczne, jakie
   masz, i nie będziesz wiedział, na którym z czterech poprzednich kroków
   coś nie zadziałało.
