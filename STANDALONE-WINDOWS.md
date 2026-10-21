# JUUM Standalone for Windows

This project packages the existing JUUM browser synth into an offline desktop app using Electron. The desktop app loads the local `index.html` and `manual.html` files; it does not need GitHub Pages or an internet connection to run after installation.

## Build locally

Requirements: Windows 10/11, Node.js LTS, and npm.

1. Install Node.js from https://nodejs.org/.
2. Download or clone this repository.
3. Open a terminal in the repository folder.
4. Run:

   ```powershell
   npm install
   npm start
   ```

To build a Windows installer and portable executable:

```powershell
npm run dist
```

Build outputs appear in `dist\`. The installer is named similar to `JUUM-1.0.0-x64.exe`; the portable version can run without installation.

## Build from GitHub Actions

After the workflow completes, open the repository's Actions tab, select **Build JUUM for Windows**, open the successful run, and download the **JUUM-Windows** artifact. It contains the installer and portable executable.

## Offline and MIDI notes

The app loads the synth from local files. Presets are stored in the app's local browser storage on that Windows user account. MIDI support depends on the MIDI devices, drivers, and Web MIDI support available to the embedded Chromium runtime; verify MIDI I/O on the target computer. The current app is a standalone desktop app, not a VST3 plugin.

## Troubleshooting

- If the app does not start, install a current Node.js LTS version and rerun `npm install`.
- The first build needs internet access to download Electron and build dependencies.
- Once built, the app itself can run offline.
