$HOME
├─ dev/
│  ├─ flutter/                 # Flutter SDK (git clone here)
│  └─ flutter_projects/        # Your apps (keep repos here)
├─ Android/
│  └─ Sdk/                     # Android SDK root
│     ├─ cmdline-tools/
│     │  └─ tools/             # unzip commandlinetools-* into here
│     ├─ platform-tools/       # adb, fastboot
│     ├─ platforms/
│     │  └─ android-35/        # Android API 35 platform
│     ├─ build-tools/
│     │  └─ 35.0.0/
│     └─ emulator/             # emulator binaries
├─ apps/
│  └─ android-studio/studio/   # Optional: Studio unpacked here (if you use it)
├─ .android/
│  └─ avd/                     # AVDs live here (created by avdmanager)
├─ .pub-cache/                 # Dart/Flutter packages cache
├─ .config/flutter_tools/      # Flutter tool cache (includes Chromium for web)
└─ .dartServer/                # Dart analysis server cache

