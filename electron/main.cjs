const { app, BrowserWindow, shell, dialog } = require("electron");
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


  // Allow JUUM's Web Serial bridge to choose a connected USB serial device.
  const ses = win.webContents.session;
  ses.setPermissionCheckHandler((_contents, permission) => permission === "serial");
  ses.setDevicePermissionHandler(details => details.deviceType === "serial");
  ses.on("select-serial-port", (event, _contents, deviceList, callback) => {
    event.preventDefault();
    if (!deviceList || deviceList.length === 0) {
      callback("");
      return;
    }
    const labels = deviceList.map((device, index) => {
      const name = device.displayName || device.portName || device.productName || "USB serial device";
      const vendor = device.vendorId ? " · VID " + device.vendorId : "";
      const product = device.productId ? " PID " + device.productId : "";
      return name + vendor + product;
    });
    dialog.showMessageBox(win, {
      type: "question",
      title: "Connect JUUM Hardware",
      message: "Choose the ESP32-S3 controller's USB serial port.",
      detail: "Close Arduino Serial Monitor before connecting. Select the board you want JUUM to control.",
      buttons: [...labels, "Cancel"],
      cancelId: labels.length,
      defaultId: 0,
      noLink: true
    }).then(({ response }) => {
      callback(response >= 0 && response < deviceList.length ? deviceList[response].portId : "");
    }).catch(() => callback(""));
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
