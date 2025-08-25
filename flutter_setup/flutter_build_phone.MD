Klar! Vi gør to ting:

1. Kør din Flutter‑app på en fysisk Android‑telefon
2. Tilføj minimal BLE‑styring så “Test Alarm” tænder LED og “Hush” slukker den

Jeg holder det step‑by‑step og uden store code dumps—du skriver selv koden.

---

# 1) Kør appen på telefon (fra VS Code)

**Telefon**

1. Åbn Indstillinger → Om telefonen → tryk 7× på “Build‑nummer” → Udviklerindstillinger aktiveres.
2. I Udviklerindstillinger: slå **USB‑debugging** til.
3. Tilslut USB‑kabel → accepter RSA‑fingerprint på telefonen.

**PC / VS Code**

1. Sørg for at `platform-tools` (adb) er i din Android SDK (du har det via CLI tools).
2. I terminal (i projektmappen):

   * `flutter devices` → du skal se din telefon på listen.
   * Kør: `flutter run -d <deviceId>`
3. Alternativt release APK:

   * `flutter build apk --release`
   * `adb install -r build/app/outputs/flutter-apk/app-release.apk`

> Tip: BLE fungerer meget bedre på en fysisk telefon end i emulatoren.

---

# 2) BLE fra Flutter → nRF52840 LED

**Forudsætning på nRF‑siden**
Din firmware skal:

* Reklamere som fx `nRF52840-LED`
* Have en **128‑bit service UUID** og én **write characteristic** (1 byte): `0x01` = LED ON, `0x00` = LED OFF

## (a) Pubspec + Android tilladelser

`pubspec.yaml`:

```yaml
dependencies:
  flutter_blue_plus: ^latest
  permission_handler: ^latest
```

`android/app/src/main/AndroidManifest.xml` (øverst under `<manifest>` – Android 12+):

```xml
<uses-permission android:name="android.permission.BLUETOOTH"/>
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN"/>
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" android:usesPermissionFlags="neverForLocation"/>
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT"/>
<!-- For ældre Android-versioner: -->
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>
```

I din app: bed om runtime‑permissions for `BLUETOOTH_SCAN` og `BLUETOOTH_CONNECT` (og evt. location for <12).

## (b) Definér dine UUID’er (én gang)

Lav fx `lib/ble_ids.dart`:

```dart
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

final Guid serviceUuid = Guid('12345678-1234-5678-1234-56789abcdef0');
final Guid charUuid    = Guid('12345678-1234-5678-1234-56789abcC001'); // eksempel
const targetName = 'nRF52840-LED';
```

## (c) Minimal BLE‑klient (skelet)

Lav `lib/ble_client.dart` (kun kerne‑metoder—du fylder krop/logik ind):

```dart
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'ble_ids.dart';

class BleClient {
  BleClient._();
  static final instance = BleClient._();

  BluetoothDevice? _dev;
  BluetoothCharacteristic? _ledChar;

  Future<void> connect() async {
    // 1) Permissions (permission_handler) – kald før scanning
    // 2) Scan og find device (match på targetName eller serviceUuid)
    // 3) stop scan, connect til device
    // 4) discoverServices -> find serviceUuid -> find charUuid (WRITE/WRITE_NR)
    // 5) cache i _ledChar
  }

  Future<void> writeLed(bool on) async {
    // skriv [0x01] hvis on == true ellers [0x00]
    // fx: await _ledChar?.write([on ? 0x01 : 0x00], withoutResponse: true);
  }

  Future<void> disconnect() async {
    // disconnect pænt
  }
}
```

> Tjek: Når du scanner, kan du matche på `advertisementData.advName == targetName` eller på service UUID.

## (d) Kobling til din eksisterende UI

I **TestAlarmPage**:

* Tilføj en “Connect”‑knap der kalder `BleClient.instance.connect()`
* “Start Alarm”‑knappen kalder `BleClient.instance.writeLed(true)`

I **AlarmsPage (Hush)**:

* “Hush”‑knappen kalder `BleClient.instance.writeLed(false)`

Eksempel (kun knap‑callbacks, så du ser mønstret):

```dart
// I TestAlarmPage build():
ElevatedButton(
  onPressed: () async {
    await BleClient.instance.connect();
  },
  child: const Text('Connect'),
),

ElevatedButton(
  onPressed: () async {
    await BleClient.instance.writeLed(true); // ON
  },
  child: const Text('Start Alarm'),
);

// I AlarmsPage build():
ElevatedButton(
  onPressed: () async {
    await BleClient.instance.writeLed(false); // OFF (Hush)
  },
  child: const Text('Hush'),
),
```

---

# 3) Hurtig tjekliste (typiske fejl)

* Telefonen dukker ikke op i `flutter devices` → mangler USB‑debugging eller adb‑driver/autorisation.
* BLE finder ikke enheden → tjek at nRF reklamerer (nRF Connect på telefonen er god til at verificere).
* Write fejler → sikr at char har WRITE/WRITE\_NR og at du skriver **præcis 1 byte**.
* Android 12+ → husk runtime‑permissions.

---

Vil du have mig til at skitsere den konkrete `connect()`‑metode (10–15 linjer) med `flutter_blue_plus` scanning og service/char‑lookup, siger du til—så holder jeg den ultrakort og plug‑and‑play for dit setup.

Godt spørgsmål 👍

Der er to forskellige måder at “få appen over” på din telefon – og det afgør hvad der sker, når du tager USB-kablet ud:

---

### 1) **`flutter run` via USB (debug build)**

* Når du kører `flutter run -d <deviceId>`, kører appen i **debug-mode**.
* Appen installeres på telefonen, men forbindelsen til din PC bruges hele tiden for **hot reload, logs og debugging**.
* **Hvis du tager kablet ud**:

  * Appen kan stadig være installeret og ligge på startskærmen.
  * Men den kan crashe/lukke, fordi den prøver at tale med debug-processen på PC.
  * Nogle telefoner tillader at åbne den igen, men så starter den ikke rigtigt uden PC.

---

### 2) **`flutter build apk --release` + `adb install` (release build)**

* Du bygger en **release APK** (uden debug-binding til PC).
* Den installeres som en “rigtig” app på telefonen.
* Du kan tage kablet ud, og appen bliver liggende som en normal app, du kan åbne via app-ikonet.
* Ingen hot reload, ingen debug logs – men den fungerer “standalone” på telefonen.

---

✅ **Opsummering**

* **Debug build (`flutter run`)** → kræver ofte PC tilsluttet.
* **Release build (`flutter build apk --release`)** → fungerer som en almindelig app, også når USB-kablet er ude.

---

Vil du have, at jeg viser dig den helt korte kommando til at lave en **installerbar .apk**, som du kan finde i filhåndtering på telefonen og dele ligesom en normal app?


Super—her er den korte, sikre vej til en “standalone” release‑app på din telefon 👇

# Trin-for-trin (release APK + install)

1. I projektmappen (VS Code terminal):

```
flutter clean
flutter pub get
flutter build apk --release
```

2. Find APK’en (genereres her):

```
build/app/outputs/flutter-apk/app-release.apk
```

3. Tilslut telefonen via USB (USB‑debugging slået til) og tjek forbindelsen:

```
adb devices
```

Du skal se din enhed som “device”.

4. Installer APK’en:

```
adb install -r build/app/outputs/flutter-apk/app-release.apk
```

`-r` = “reinstall” (overskriv hvis den allerede findes).

5. Tag kablet ud → Åbn appen fra app‑menuen som en helt almindelig app.

---

## Typiske fejl og hurtige fixes

* **INSTALL\_FAILED\_VERSION\_DOWNGRADE**
  Din APK har lavere `versionCode` end den installerede.
  Løsning: Hæv `versionCode` i `android/app/build.gradle` (i `defaultConfig`) og byg igen, eller afinstaller først:

  ```
  adb uninstall <din.package.name>
  ```

  (find pakkenavnet i `android/app/src/main/AndroidManifest.xml` – fx `com.example.testapp`)

* **INSTALL\_FAILED\_UPDATE\_INCOMPATIBLE (signatur mismatch)**
  Du har tidligere installeret en app med anden signatur (fx debug vs release).
  Løsning: afinstaller:

  ```
  adb uninstall <din.package.name>
  ```

  og kør install igen.

* **Enheden vises ikke i `adb devices`**
  – USB‑debugging ikke slået til / ikke godkendt RSA‑fingerprint
  – Brug andet kabel/port
  – `adb kill-server && adb start-server` og prøv igen

* **Vil du dele APK’en uden ADB?**
  Kopiér `app-release.apk` til telefonens lager (fx via USB MTP eller fil‑deling) og installer den derfra (tillad “ukendte kilder”). ADB er dog lettest.

---

## Bonus (valgfrit)

* **Mindsk størrelse**:

  ```
  flutter build apk --release --split-per-abi
  ```

  Det laver flere APK’er (arm64‑v8a, armeabi‑v7a m.fl.). Installer den der matcher telefonens CPU.

* **Til Play Store**: Brug `flutter build appbundle --release` og sæt *app‑signing* korrekt op (keystore). Til sideload/testing er trin ovenfor nok.

Sig til hvis du vil have en mini‑tjekliste for `versionCode`/`versionName` og hurtig `packageName`‑ændring 💡


Super—her er den korte, sikre vej til en “standalone” release‑app på din telefon 👇

# Trin-for-trin (release APK + install)

1. I projektmappen (VS Code terminal):

```
flutter clean
flutter pub get
flutter build apk --release
```

2. Find APK’en (genereres her):

```
build/app/outputs/flutter-apk/app-release.apk
```

3. Tilslut telefonen via USB (USB‑debugging slået til) og tjek forbindelsen:

```
adb devices
```

Du skal se din enhed som “device”.

4. Installer APK’en:

```
adb install -r build/app/outputs/flutter-apk/app-release.apk
```

`-r` = “reinstall” (overskriv hvis den allerede findes).

5. Tag kablet ud → Åbn appen fra app‑menuen som en helt almindelig app.

---

## Typiske fejl og hurtige fixes

* **INSTALL\_FAILED\_VERSION\_DOWNGRADE**
  Din APK har lavere `versionCode` end den installerede.
  Løsning: Hæv `versionCode` i `android/app/build.gradle` (i `defaultConfig`) og byg igen, eller afinstaller først:

  ```
  adb uninstall <din.package.name>
  ```

  (find pakkenavnet i `android/app/src/main/AndroidManifest.xml` – fx `com.example.testapp`)

* **INSTALL\_FAILED\_UPDATE\_INCOMPATIBLE (signatur mismatch)**
  Du har tidligere installeret en app med anden signatur (fx debug vs release).
  Løsning: afinstaller:

  ```
  adb uninstall <din.package.name>
  ```

  og kør install igen.

* **Enheden vises ikke i `adb devices`**
  – USB‑debugging ikke slået til / ikke godkendt RSA‑fingerprint
  – Brug andet kabel/port
  – `adb kill-server && adb start-server` og prøv igen

* **Vil du dele APK’en uden ADB?**
  Kopiér `app-release.apk` til telefonens lager (fx via USB MTP eller fil‑deling) og installer den derfra (tillad “ukendte kilder”). ADB er dog lettest.

---

## Bonus (valgfrit)

* **Mindsk størrelse**:

  ```
  flutter build apk --release --split-per-abi
  ```

  Det laver flere APK’er (arm64‑v8a, armeabi‑v7a m.fl.). Installer den der matcher telefonens CPU.

* **Til Play Store**: Brug `flutter build appbundle --release` og sæt *app‑signing* korrekt op (keystore). Til sideload/testing er trin ovenfor nok.

Sig til hvis du vil have en mini‑tjekliste for `versionCode`/`versionName` og hurtig `packageName`‑ændring 💡


