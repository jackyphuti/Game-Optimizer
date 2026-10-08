const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
    scanHardware: () => ipcRenderer.invoke('scan-hardware'),
    getMetrics: () => ipcRenderer.invoke('get-metrics'),
    toggleTurbo: (enable) => ipcRenderer.invoke('toggle-turbo', enable),
    purgeRam: () => ipcRenderer.invoke('purge-ram'),
    restoreDefaults: () => ipcRenderer.invoke('restore-defaults'),
    getGames: () => ipcRenderer.invoke('get-games'),
    minimize: () => ipcRenderer.invoke('window-minimize'),
    maximize: () => ipcRenderer.invoke('window-maximize'),
    close: () => ipcRenderer.invoke('window-close')
});
