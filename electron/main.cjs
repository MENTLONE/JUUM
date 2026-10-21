const { app, BrowserWindow, shell } = require("electron");
const path = require("node:path");
const { pathToFileURL } = require("node:url");

function createWindow() {
  const indexPath = path.join(__dirname, "..", "index.html");
  const appUrl = pathToFileURL(indexPath).href;

  const win = new BrowserWindow({
    width: 1440,
    height: 960,
    minWidth: 900,
    minHeight: 650,
    title: "JUUM — Strumplate Synth",
    backgroundColor: "#03080d",
    autoHideMenuBar: true,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: true
    }
  });

  win.loadURL(appUrl);
  win.webContents.setWindowOpenHandler(({ url }) => {
    if (url.startsWith("https://") || url.startsWith("http://")) {
      shell.openExternal(url);
    }
    return { action: "deny" };
  });
  win.webContents.on("will-navigate", (event, url) => {
    if (url !== appUrl && !url.startsWith("file://")) {
      event.preventDefault();
    }
  });
}

app.whenReady().then(() => {
  app.setAppUserModelId("com.mentlone.juum");
  createWindow();
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});
