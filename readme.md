# PC Audio Mixer mit Arduino

Ein hardwarebasierter Audio-Mixer zur Steuerung der Lautstärke einzelner Programme unter Windows.

Das Projekt besteht aus einer Hardware mit mehreren Slidern, einem Arduino und der Windows-Anwendung **WinAudioMixer**. Der Arduino liest die Positionen der Slider aus und überträgt die ermittelten Werte über die serielle Schnittstelle an den PC.

**WinAudioMixer** empfängt die übertragenen Werte, filtert und verarbeitet diese und verwendet sie anschließend zur Steuerung der Lautstärke einzelner Programme.

Die Zuordnung und Verarbeitung der Slider wird über eine Konfigurationsdatei festgelegt. Dadurch kann die Belegung der einzelnen Slider angepasst werden.

## Inspiration

Die grundlegende Idee für den hardwarebasierten Audio-Mixer sowie die verwendete Arduino-Hardware orientieren sich am Open-Source-Projekt [deej](https://github.com/FabianA2001/deej).

Die Umsetzung der PC-seitigen Verarbeitung erfolgt über die eigene Anwendung **WinAudioMixer**.

### Beispiel der Hardware

![Hardware](docs/hardware.jpg)

## Konfiguration

Die Konfiguration von **WinAudioMixer** erfolgt über eine `config.ini` Datei.

Beispiel:

```ini
[Serial]
port=/dev/cu.usbmodem1301
baudrate=9600
slider_count=2

[Filter]
window_size=5

[DeadZone]
value=5

[Mapping]
slider_0=Spotify.exe
slider_1=unassigned
```

### Serial

Im Abschnitt `[Serial]` werden die Einstellungen für die serielle Kommunikation mit dem Arduino festgelegt.

| Einstellung    | Beschreibung                                         |
| -------------- | ---------------------------------------------------- |
| `port`         | Serieller Port, über den der Arduino verbunden ist   |
| `baudrate`     | Übertragungsgeschwindigkeit der seriellen Verbindung |
| `slider_count` | Anzahl der angeschlossenen Slider                    |

### Filter

Der Abschnitt `[Filter]` definiert die Verarbeitung der eingelesenen Slider-Werte.

| Einstellung   | Beschreibung                                                 |
| ------------- | ------------------------------------------------------------ |
| `window_size` | Größe des Filterfensters zur Glättung der eingelesenen Werte |

### DeadZone

Im Abschnitt `[DeadZone]` wird die Totzone für die Slider-Werte festgelegt.

| Einstellung | Beschreibung                                                           |
| ----------- | ---------------------------------------------------------------------- |
| `value`     | Bereich, in dem kleine Änderungen der Slider-Position ignoriert werden |

Dadurch können geringfügige Schwankungen der eingelesenen Werte verhindert werden.

### Mapping

Im Abschnitt `[Mapping]` wird festgelegt, welcher Slider welches Programm steuert.

```ini
[Mapping]
slider_0=Spotify
slider_1=unassigned
```

In diesem Beispiel steuert `slider_0` die Lautstärke von **Spotify**.

`slider_1` ist mit `unassigned` gekennzeichnet. Dieser Slider steuert dadurch die Lautstärke aller Programme, die nicht explizit einem anderen Slider zugeordnet wurden.



