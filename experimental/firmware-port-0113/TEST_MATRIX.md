# Test-Matrix: Port 02.01.01.07 → 02.01.01.13

Arbeitsdokument, nur lokal. Plant Emulations- und Hardware-Tests für den Port.
**Keine Patch-Bytes, keine Adressen final.** Adressen sind die aus der
lesenden Analyse belegten Werte (siehe `README.md`); sie werden im Test
bestätigt, nicht vorausgesetzt.

Grundlagen:
- Emulation nach Vorbild `experimental/firmware-port/test_*44_emulation.py`
  (Unicorn führt die echten ARM-Instruktionen aus, Vendor-Funktionen werden
  abgefangen, Kontrollfluss pro Maschinenstatus geprüft; schreibt Status nach
  `object+0xbbc` und einen Stale-Wert nach `0xbb4`).
- Hardware nach `docs/TESTING.md`, Abschnitt „Real-printer release checklist“.
- Ein bestandener Host-/Emulationstest beweist **keine** Hardware-Sicherheit.

## Patch-Kategorien

| Kat. | Bedeutung | Neuberechnung nach Verschiebung? |
|---|---|---|
| D | Reiner Datenpatch (`cmp`/`mov`/`ldr [r11,#…]`) | nein |
| N | NOP (Aufruf entfällt) | nein |
| L | Lageunabhängig / In-Funktion, Ziel `+0x7790` belegt | nein |
| P | Payload-abhängig (Ziel im Block `0xcd77e8`) | ja, nach Payload-Platzierung |
| X | Cross-Function-Branch (Ziel in anderer Funktion) | ja |
| G | PLT/GOT-Referenz | ja (neuer Stub) |
| F | DIFF-Block (rekompiliert) | ja, Disassemblierung |

## A. Emulations-Matrix

### Ausgeführt am 2026-11-10 (lokal, Unicorn 2.1.4)

Gegen den `corrected_v2`-Stand (die Bytes, die auf dem Testgerät laufen):
`elegoo_printer` `688b7bf8…`, `ec-eeb001-gui` `668ab8cb…`, `ai_camera`
`90da7634…`. Vendor-Funktionen sind dabei abgefangen; die Tests prüfen
Kontrollfluss, nicht Hardware.

| Script | Deckt ab | Ergebnis |
|---|---|---|
| `emulate_e1.py` | E1 | **PASS** — PC erreicht `0x74bd0`, `data_get_print_status` (`0xae640`) nicht aufgerufen |
| `emulate_e2.py` | E2 | **PASS** — erster Aufruf landet bei `resume` (`0x16a7f8`), nicht `pause` (`0x16a748`) |
| `emulate_e345.py` | E3/E4/E5 | **PASS** — 128/128 (32 Status × 4 Pfade) |
| `emulate_e6.py` | E6 | **PASS** — `stop_http_server_file` (`0x827148`) nicht aufgerufen, Funktion läuft bis Rücksprung |

Weiterhin nicht ausgeführt: E7, E8, E9, E10 (payload-/disassembly-abhängig,
in der Matrix unten als blockiert/offen markiert), E11 (Dual Trust /
Reactor byte-identisch, keine Emulation nötig).

| # | Ziel | Betroffene Stelle | Kat. | Erwartetes Verhalten | Beobachtung | Pass/Fail | Rückfall bei Fail |
|---|---|---|---|---|---|---|---|
| E1 | GUI Z-Offset im Druck erreichbar | `set_event_cb` case5 @ `0x747c8` → `0x74bd0` | L | Sprung in den Z-Offset-Pfad unabhängig vom Druckstatus | PC landet bei `0x74bd0` (→ `setting_set_z_offset_create`) | PASS wenn Ziel erreicht, in jedem Status | Menü bleibt statusabhängig gesperrt |
| E2 | Kamera-Koexistenz | `LocalRtcMonitor` @ `0x13287c` | X | `bl` ruft `resume` (`0x16a7f8`), nicht `pause` | abgefangener Callee = `resume` | PASS wenn `resume` | lokaler MJPEG bricht bei WAN-RTC ab |
| E3 | HTTP-Upload Statuswächter | `handle_file_upload` Einsprünge `0x857100`/`0x8579c8`/`0x858138`/`0x857140` → `set_status`/`set_sub_status` (neue VAs) | L/X | nur erlaubte Status (1/11, bzw. PRINTING=2) führen fort, sonst Fallthrough | wie `test_http44_emulation.py`: pro Status 0–31 landet PC am erwarteten Ziel | PASS wenn alle 32 Status pro Einsprung korrekt | aktiver Druck wird fälschlich auf idle gezwungen |
| E4 | `device_status_offset` = `0xbbc` | Member-Zugriff in den HTTP-Statuswächtern | D | Status wird bei `object+0xbbc` gelesen, Stale `0xbb4` ignoriert | Test schreibt `0xbbc`/`0xbb4` getrennt; Verhalten folgt `0xbbc` | PASS wenn Entscheidung `0xbbc` folgt | altes Layout `0xbb4` aktiv → falscher Status |
| E5 | 409 statt Löschen bei Erst-Chunk | `handle_file_cancel`/`handle_file_upload` Reclaim | F | existierende Datei → HTTP 409, keine Löschung | R4==409 im Nicht-idle-Fall (wie bestehender Test) | PASS wenn 409 ohne Löschpfad | Datei wird gelöscht |
| E6 | LanStrategy::stop ohne HTTP-Stop | `0x826944` NOP | N | `stop()` ruft `stop_http_server_file` **nicht** | kein Aufruf des abgefangenen `0x827148` | PASS wenn kein Aufruf | HTTP-Server wird im LAN-Stop beendet |
| E7 | Payload-Koexistenzlogik | Block `0xcd77e8` (neue Position) | P | LAN/WAN-Koexistenz: korrekte Strategy-/Report-Pfade je Modus | Payload mit abgefangenen Vendor-Zielen ausführen, Pfad pro Modus prüfen | **blockiert** bis Payload platziert | — |
| E8 | Payload→Vendor-Branches | 13 Gruppe-B-Ziele | P/X/G | jeder Branch erreicht die belegte Zielfunktion | abgefangene Callees == erwartete Funktion | PASS je Ziel; `start+0x11c` offen | falscher Callee |
| E9 | PLT/GOT aus Payload | `strcmp`, `std::string`-Stubs | G | Aufruf erreicht libc/libstdc++ über neuen Stub | abgefangener Stub == erwartetes Symbol | PASS je Stub | Sprung ins Leere/falsche Funktion |
| E10 | DIFF-Blöcke | `set_mode`, `handle_file_upload`-Block, `AsyncMQTTConnector`, `check_token` | F | nach Neuherleitung: semantisch wie alt | pro Block eigener Guard-Test | **blockiert** bis disassembliert | — |
| E11 | Dual Trust / Reactor (byte-identisch) | `daemon-000`, `libcolib.so` | — | unverändertes Verhalten | Regression der bestehenden Dual-Tests | PASS wenn unverändert | — |

## B. Hardware-Matrix (`docs/TESTING.md`)

| # | Ziel | Betroffener Patch | Erwartetes Verhalten | Beobachtung | Pass/Fail | Rückfall |
|---|---|---|---|---|---|---|
| H1 | Z-Offset-Menü im laufenden Druck | GUI | Menü öffnet auch während des Drucks | Bedienung am Display | PASS wenn erreichbar, kein Absturz | gesperrt / UI hängt |
| H2 | Kamera + Remote gleichzeitig | Kamera | lokaler MJPEG bleibt, WAN-RTC steuert | Stream lokal + Cloud | PASS wenn beide laufen | lokaler Stream bricht |
| H3 | OrcaSlicer Upload / Start / Cancel | HTTP | Upload in Idle und Druck korrekt; Cancel-Guards | Upload großer Datei, Abbruch | PASS wenn Status korrekt, kein Datenverlust | Upload verwirft aktiven Druck |
| H4 | Dateiname mit Leerzeichen, Mehrfach-Ops | HTTP | korrekte Behandlung | mehrere Dateien | PASS wie Stock | Fehler/Hänger |
| H5 | LAN/WAN-Moduswechsel, MQTT-Koexistenz | Netzwerk + Payload | Wechsel ohne Serviceneustart | MQTT/Canvas Reconnect | PASS wenn stabil | Reconnect-Schleife |
| H6 | Gerätestatus-Meldungen | `device_status_offset` | korrekte Status/Substatus | Statusanzeige/Logs | PASS wenn plausibel | falscher Status |
| H7 | Boot, Rollback, Power-Cycle | uboot/boot0 + Gesamtpaket | sauberer Boot, Rollback möglich | Power-Cycle, Update/Downgrade | PASS wenn kein Brick | — (Abbruch, nicht flashen) |

### Ergebnisse auf echter Hardware

Ein Testgerät, 02.01.01.13 mit den Ständen aus `HARDWARE_CONFIRMATION_0113.md`
(`elegoo_printer` `688b7bf8…`, GUI `668ab8cb…`, Kamera `90da7634…`), Reactor
deaktiviert, kein `LD_PRELOAD`. Gerätezeit (CST).

| # | Datum | Ergebnis | Durchführung und Beobachtung | Belege |
|---|---|---|---|---|
| H1 | 2026-11-10 | **PASS** | Datei `ECC2_0.6_first layer_Elegoo PLA+ _0.3_4m57s.gcode` (eine Schicht 0,3 mm, 80 × 80 mm, PLA+, 0,6-mm-Düse), Start ca. 05:54. Während des Drucks ließ sich das Z-Offset-Menü öffnen und bedienen; der Wert wurde mehrfach zwischen +0,05 und −0,05 mm verstellt, die Z-Achse reagierte hörbar. Druck regulär beendet. | Bediener-Beobachtung für Menü und Achse. `elegoo_printer` PID 2658 vor (05:52, 05:54) und nach dem Druck (06:08) gleich, Uptime durchgehend gestiegen: kein Absturz, kein Neustart. `GET :8081/api/printer` danach: `print.state = complete`, `z_offset.value = 0.035`. |

| H2 | 2026-11-10 | **PASS** | Nach dem Wechsel aus dem LAN-Modus (siehe H5a) Kamerabild gleichzeitig offen in CC2 Control (lokal), ElegooSlicer und ELEGOO-App; alle drei laufen parallel mit minimaler Verzögerung. Vor dem Wechsel lief die Kamera lokal über Stunden stabil. | Bediener-Beobachtung. `GET :8081/api/printer`: `hardware.camera = true`, aktiver `camera_viewer`. Ob die App über den Cloud-Weg (WAN-RTC) verbunden war, ist nicht aus einem Log belegt; im Nicht-LAN-Modus ist das ihr vorgesehener Weg. |
| H5a | 2026-11-10 | **PASS** | Netzwerkmodus vom Bediener von LAN auf Nicht-LAN umgestellt, am Gerät. Dieser Wechsel war der Auslöser des ursprünglichen Absturzes (`SIGSEGV`, `pc=0`), den der Netzwerk-Patch beheben soll. Kein Absturz, MQTT neu verbunden. | `elegoo_printer` PID 2658 vor (06:08) und nach dem Wechsel (06:34) gleich, Uptime 7381 s → 8941 s, kein Geräte-Neustart. CC2 Control danach: Meldung `code 2002` „connect mqtt success", `mqtt_connected` und `mqtt_registered` true. |
| H3 | 2026-11-10 | **PARTIAL PASS** | Über die Community-Oberfläche ließen sich während eines laufenden Drucks Dateien hochladen und löschen; der Druck lief dabei ungestört weiter. Druckablauf per Live-Abfrage lückenlos mitgeschrieben: `printing` (ab 23:47) → `paused` (23:51:35) → `cancelled` (23:51:41) → Leerlauf (ab 23:51:45). | Bediener-Beobachtung für Upload/Löschen; `GET :8081/api/printer`-Mitschrift für den Zustandsverlauf. `elegoo_printer` PID 2658 vor und nach dem gesamten Ablauf gleich, Uptime durchgehend gestiegen. |
| H3 (Port-80-Pfad) | – | NOT RUN | Der HTTP-Upload-Pfad in `elegoo_printer` (Port 80), in dem die Status-Wächter des Patches sitzen, wurde nicht erreicht: ElegooSlicer verweigert den Upload selbst, sobald der Drucker nicht idle ist („The printer may not be able to start transmission/printing…"), schon vor dem Senden. Diese Meldung erscheint laut Bediener auch auf Originalfirmware und bei früheren Versionen, ist also nicht portierungsbedingt. Ein gezielter Test bräuchte einen direkten Upload an Port 80. | — |
| H5b | 2026-11-10 | **PASS (berichtet)** | Rückwechsel Nicht-LAN → LAN vom Bediener ausgeführt und als funktionierend bestätigt. | Bediener-Bestätigung. `GET :8081/api/printer` danach: Drucker idle und ansprechbar, Kamera vorhanden, keine neue Fehlermeldung (letzte weiterhin `code 2002` „connect mqtt success" aus dem H5a-Wechsel). Die `elegoo_printer`-PID wurde beim Rückwechsel nicht gegengeprüft; Absturzfreiheit daher nur indirekt (kein neuer Fehler-Report), nicht per PID wie bei H5a. |

Zu H1 nicht belegt:
- **Einzelne Offset-Werte während des Drucks:** nicht protokolliert. Das Log
  des Druckerprozesses ging nach `/dev/null` (`/proc/2658/fd/1` und `fd/2`),
  weil der Prozess zuvor mit `run_printer.sh start >/dev/null 2>&1` neu
  gestartet worden war.
- **Ausgangswert vor dem Test:** nicht notiert; nach dem Test steht der Offset
  auf `0.035`.
- **Wirkung im Druckbild:** Nach dem Entnehmen der Fläche berichtet der
  Bediener eine erkennbar unterschiedliche Haftung der Schicht in den
  Bereichen, in denen der Offset verstellt war. Das ist eine Sicht- und
  Tastprüfung ohne Messung, und welche Stelle zu welchem Wert gehört, ist
  nicht zugeordnet.

Nachtest im Leerlauf, nach dem Druck (Druckstatus `complete`, kein Teil
von H1): Änderungen am Display erschienen innerhalb des 2-s-Abfrageintervalls
in `GET :8081/api/printer`, ohne `pending` oder `timed_out`. Die Werte liefen
von `0.035` über `-0.015`, `-0.04`, `-0.015` und `0.005` nach `-0.015`. Damit
ist belegt, dass die Firmware Änderungen aus dem Menü übernimmt und der Wert
live auslesbar ist; während eines Drucks wurde das nicht mitgeschrieben.

Nebenbefund: CC2 Control meldete zur Druckzeit `code 1301`, `level 3`,
„ai spaghetti detection error". Das Druckergebnis war davon nicht betroffen;
die Ursache ist ungeklärt und gehört zur Kamera-Prüfung (H2).

Für weitere Tests:
- **Log erhalten:** Neustarts so ausführen, dass das Log erhalten bleibt,
  zum Beispiel `run_printer.sh start > /opt/usr/elegoo-test.log 2>&1`.
- **Live-Werte:** Z-Offset und Druckstatus lassen sich ohne SSH über
  `GET http://<Drucker>:8081/api/printer` (Felder `z_offset`, `print.state`)
  mitschreiben.

## Reihenfolge

1. E1, E2, E6 (lageunabhängig/einfach) — sofort emulierbar.
2. E3, E4, E5 (`test_http44_emulation.py` auf neue Adressen anpassen).
3. E11 Regression.
4. E7–E10 erst nach Payload-Platzierung bzw. DIFF-Disassemblierung.
5. Hardware H1–H7 zuletzt, nach bestandener Emulation.

## Offene Abhängigkeiten

- E7, E8 (teilw.), E10 hängen an Payload-Platzierung bzw. Disassemblierung.
- `LanStrategy::start+0x11c` (Gruppe B) bleibt bis zur Disassemblierung offen.
- Alle Hardware-Tests setzen einen vollständig portierten, emulations­geprüften
  Patch-Satz voraus.
