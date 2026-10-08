// Apex Overdrive v3.0 - Dashboard & Automotive Cluster Controller
document.addEventListener('DOMContentLoaded', async () => {
    // 1. Audio FX Initialization on first user interaction
    document.addEventListener('click', () => {
        if (window.soundFx) window.soundFx.init();
    }, { once: true });

    // 2. Logging helper
    const tickerLog = document.getElementById('tickerLog');
    function logTicker(message) {
        if (!tickerLog) return;
        const time = new Date().toLocaleTimeString('en-US', { hour12: false });
        tickerLog.textContent = `[${time}] ${message}`;
    }

    // 3. Initialize Automotive Cluster Gauges
    const gaugeCpu = new window.CarGauge('gaugeCpu', {
        title: 'CPU TACHOMETER',
        unit: '% LOAD',
        minVal: 0,
        maxVal: 100,
        redlineVal: 85,
        glowColor: '#00f3ff', // Electric Cyan
        accentColor: '#00ff88'
    });

    const gaugeGpu = new window.CarGauge('gaugeGpu', {
        title: 'GPU SPEEDOMETER',
        unit: '% ENGINE',
        minVal: 0,
        maxVal: 100,
        redlineVal: 90,
        glowColor: '#ff0055', // Racing Red/Pink
        accentColor: '#ffaa00'
    });

    const gaugeRam = new window.CarGauge('gaugeRam', {
        title: 'RAM BOOST GAUGE',
        unit: '% BOOST',
        minVal: 0,
        maxVal: 100,
        redlineVal: 80,
        glowColor: '#00ff88', // Toxic Green
        accentColor: '#00f3ff'
    });

    let currentCpuSub = '3.80 GHz';
    let currentCpuBadge = 'NORM';
    let currentGpuSub = '52 °C';
    let currentGpuBadge = 'STABLE';
    let currentRamSub = '8.2 / 16 GB';
    let currentRamBadge = 'TRIMMED';

    // 4. Gauge Render & Animation Loop
    function renderCluster() {
        gaugeCpu.update();
        gaugeGpu.update();
        gaugeRam.update();

        gaugeCpu.draw(currentCpuSub, currentCpuBadge);
        gaugeGpu.draw(currentGpuSub, currentGpuBadge);
        gaugeRam.draw(currentRamSub, currentRamBadge);

        requestAnimationFrame(renderCluster);
    }
    requestAnimationFrame(renderCluster);

    // 5. Ignition Needle Sweep Animation (Like real racing sports car cluster)
    function ignitionNeedleSweep() {
        logTicker('IGNITION: Performing automotive cluster gauge sweep & diagnostics...');
        gaugeCpu.setValue(100);
        gaugeGpu.setValue(100);
        gaugeRam.setValue(100);

        setTimeout(() => {
            gaugeCpu.setValue(0);
            gaugeGpu.setValue(0);
            gaugeRam.setValue(0);
            setTimeout(() => {
                logTicker('SYSTEM READY: Gauges calibrated. Real-time telemetry linked.');
            }, 700);
        }, 800);
    }
    ignitionNeedleSweep();

    // 6. Sequential Shift-Light LED Bar Logic
    const shiftLeds = Array.from({ length: 10 }, (_, i) => document.getElementById(`led${i + 1}`));
    function updateShiftLights(load) {
        const activeCount = Math.round((load / 100) * 10);
        shiftLeds.forEach((led, idx) => {
            if (!led) return;
            if (idx < activeCount) {
                led.classList.add('active');
            } else {
                led.classList.remove('active');
            }
        });
    }

    // 7. Navigation Tabs Switching
    const navTabs = document.querySelectorAll('.nav-tab');
    const tabContents = {
        cockpit: document.getElementById('tab-cockpit'),
        scanner: document.getElementById('tab-scanner'),
        tuning: document.getElementById('tab-tuning'),
        vault: document.getElementById('tab-vault')
    };

    navTabs.forEach(tab => {
        tab.addEventListener('click', () => {
            if (window.soundFx) window.soundFx.playClick();
            const target = tab.getAttribute('data-tab');

            navTabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');

            Object.keys(tabContents).forEach(key => {
                if (tabContents[key]) {
                    tabContents[key].style.display = (key === target) ? 'block' : 'none';
                }
            });

            // Resizing gauges when cockpit becomes visible again
            if (target === 'cockpit') {
                gaugeCpu.resize();
                gaugeGpu.resize();
                gaugeRam.resize();
            }
        });
    });

    // 8. Real-Time Telemetry Polling (Every 1s)
    let isTurbo = false;
    const hudFpsBoost = document.getElementById('hudFpsBoost');
    const hudTimer = document.getElementById('hudTimer');
    const hudPowerScheme = document.getElementById('hudPowerScheme');
    const hudGpuState = document.getElementById('hudGpuState');

    async function pollTelemetry() {
        try {
            if (window.api && window.api.getMetrics) {
                const metrics = await window.api.getMetrics();
                
                gaugeCpu.setValue(metrics.cpuPercent);
                currentCpuSub = `${metrics.cpuGhz.toFixed(2)} GHz`;
                currentCpuBadge = metrics.cpuPercent > 85 ? 'HIGH' : (isTurbo ? 'TURBO' : 'NORM');

                gaugeGpu.setValue(metrics.gpuPercent);
                currentGpuSub = `${metrics.gpuTemp} °C`;
                currentGpuBadge = metrics.gpuTemp > 75 ? 'WARM' : 'OPTIMAL';

                gaugeRam.setValue(metrics.ramPercent);
                currentRamSub = `${metrics.ramUsedGb} / ${metrics.ramTotalGb} GB`;
                currentRamBadge = metrics.ramPercent > 80 ? 'HIGH' : 'CACHED';

                // Shift LEDs indicate dominant system load
                const peakLoad = Math.max(metrics.cpuPercent, metrics.gpuPercent);
                updateShiftLights(peakLoad);

                // Update HUD telemetry values
                if (isTurbo) {
                    if (hudFpsBoost) hudFpsBoost.textContent = '+30 ~ +55 FPS (TURBO)';
                    if (hudPowerScheme) hudPowerScheme.textContent = 'APEX WARP SPEED';
                    if (hudGpuState) hudGpuState.textContent = 'OVERDRIVE P0 (UNLOCKED)';
                } else {
                    if (hudFpsBoost) hudFpsBoost.textContent = '+15 ~ +35 FPS';
                    if (hudPowerScheme) hudPowerScheme.textContent = 'ULTIMATE PERFORMANCE';
                    if (hudGpuState) hudGpuState.textContent = 'MAX PERFORMANCE P0';
                }
            }
        } catch (e) {
            console.error('Error polling telemetry:', e);
        }
    }
    setInterval(pollTelemetry, 1000);

    // 9. Turbo Boost Toggle Button
    const btnToggleTurbo = document.getElementById('btnToggleTurbo');
    const turbineIcon = document.getElementById('turbineIcon');
    const turboStatusHeading = document.getElementById('turboStatusHeading');
    const turboStatusDesc = document.getElementById('turboStatusDesc');
    const systemStatusBadge = document.getElementById('systemStatusBadge');
    const systemStatusText = document.getElementById('systemStatusText');

    if (btnToggleTurbo) {
        btnToggleTurbo.addEventListener('click', async () => {
            isTurbo = !isTurbo;

            if (isTurbo) {
                if (window.soundFx) window.soundFx.playTurboSpool();
                btnToggleTurbo.classList.add('active');
                btnToggleTurbo.querySelector('span').textContent = '🔥 DISENGAGE TURBO BOOST';
                if (turbineIcon) turbineIcon.classList.add('active');
                
                if (turboStatusHeading) turboStatusHeading.textContent = 'WARP DRIVE ENGAGED // MAXIMUM PERFORMANCE';
                if (turboStatusDesc) turboStatusDesc.textContent = 'CPU Turbo locked aggressive • Core parking bypassed • 0.5ms high-precision timer active';
                
                if (systemStatusBadge) systemStatusBadge.classList.add('turbo-active');
                if (systemStatusText) systemStatusText.textContent = 'WARP SPEED ENGAGED';

                logTicker('WARP DRIVE ACTIVATED: CPU core parking bypassed, power scheme set to Maximum.');
            } else {
                if (window.soundFx) window.soundFx.playClick();
                btnToggleTurbo.classList.remove('active');
                btnToggleTurbo.querySelector('span').textContent = '⚡ ENGAGE TURBO BOOST';
                if (turbineIcon) turbineIcon.classList.remove('active');

                if (turboStatusHeading) turboStatusHeading.textContent = 'WARP DRIVE DISENGAGED';
                if (turboStatusDesc) turboStatusDesc.textContent = 'Click below to unlock aggressive CPU Turbo & unpark all cores';

                if (systemStatusBadge) systemStatusBadge.classList.remove('turbo-active');
                if (systemStatusText) systemStatusText.textContent = 'SYSTEM READY';

                logTicker('WARP DRIVE DISENGAGED: System restored to standard high-performance profile.');
            }

            if (window.api && window.api.toggleTurbo) {
                const res = await window.api.toggleTurbo(isTurbo);
                if (res && res.message) logTicker(res.message);
            }
        });
    }

    // 10. RAM Purge Action
    const btnPurgeRam = document.getElementById('btnPurgeRam');
    if (btnPurgeRam) {
        btnPurgeRam.addEventListener('click', async () => {
            if (window.soundFx) window.soundFx.playPurge();
            logTicker('PURGING STANDBY RAM: Flusing memory working sets and standby cache...');
            
            // Temporary dip in ram gauge
            gaugeRam.setValue(Math.max(15, gaugeRam.currentVal - 25));
            
            if (window.api && window.api.purgeRam) {
                const res = await window.api.purgeRam();
                if (res && res.message) logTicker(res.message);
            }
        });
    }

    // 11. Restore Defaults Action
    const btnRestoreDefaults = document.getElementById('btnRestoreDefaults');
    if (btnRestoreDefaults) {
        btnRestoreDefaults.addEventListener('click', async () => {
            if (window.soundFx) window.soundFx.playClick();
            if (isTurbo && btnToggleTurbo) {
                btnToggleTurbo.click();
            }
            if (window.api && window.api.restoreDefaults) {
                const res = await window.api.restoreDefaults();
                if (res && res.message) logTicker(res.message);
            }
        });
    }

    // 12. Hardware Scanner & Benchmark Calculation
    const scannerTierName = document.getElementById('scannerTierName');
    const scannerTierDesc = document.getElementById('scannerTierDesc');
    const scannerScoreNum = document.getElementById('scannerScoreNum');
    const specCpuName = document.getElementById('specCpuName');
    const specCpuDetails = document.getElementById('specCpuDetails');
    const specGpuName = document.getElementById('specGpuName');
    const specGpuDetails = document.getElementById('specGpuDetails');
    const specRamName = document.getElementById('specRamName');
    const specRamDetails = document.getElementById('specRamDetails');
    const specStorageName = document.getElementById('specStorageName');
    const specStorageDetails = document.getElementById('specStorageDetails');
    const btnRescan = document.getElementById('btnRescan');

    async function runHardwareScan() {
        if (scannerTierName) scannerTierName.textContent = 'SCANNING HARDWARE ARCHITECTURE...';
        if (scannerTierDesc) scannerTierDesc.textContent = 'Inspecting CPU pipelines, GPU VRAM topology, and memory bandwidth...';
        
        try {
            if (window.api && window.api.scanHardware) {
                const hw = await window.api.scanHardware();
                
                // Animate score counter
                let curr = 0;
                const target = hw.score || 85;
                const step = () => {
                    curr += 2;
                    if (curr > target) curr = target;
                    if (scannerScoreNum) scannerScoreNum.textContent = curr;
                    if (curr < target) requestAnimationFrame(step);
                };
                requestAnimationFrame(step);

                if (scannerTierName) {
                    scannerTierName.textContent = hw.tier;
                    scannerTierName.style.color = hw.tierColor || '#00f3ff';
                }
                if (scannerTierDesc) {
                    scannerTierDesc.textContent = `Scanned ${hw.cpu.brand} with ${hw.gpu.model}. Ready for high-refresh 1440p / 4K gaming!`;
                }

                // CPU
                if (specCpuName) specCpuName.textContent = hw.cpu.brand;
                if (specCpuDetails) specCpuDetails.textContent = `${hw.cpu.cores} Cores / ${hw.cpu.threads} Threads • Base: ${hw.cpu.speed} GHz (Boost: ${hw.cpu.speedMax} GHz)`;

                // GPU
                if (specGpuName) specGpuName.textContent = hw.gpu.model;
                const vramText = hw.gpu.vram > 0 ? (hw.gpu.vram >= 1024 ? `${(hw.gpu.vram / 1024).toFixed(0)} GB VRAM` : `${hw.gpu.vram} MB`) : 'DirectX 12 Ultimate';
                if (specGpuDetails) specGpuDetails.textContent = `${vramText} • Vendor: ${hw.gpu.vendor} • Driver: ${hw.gpu.driverVersion || 'Optimized'}`;

                // RAM
                if (specRamName) specRamName.textContent = `${hw.memory.totalGb} GB ${hw.memory.type} Gaming Memory`;
                if (specRamDetails) specRamDetails.textContent = `Speed: ${hw.memory.speed} • Dual Channel Active • Low-Latency Timings`;

                // Storage
                if (specStorageName) specStorageName.textContent = hw.storage.hasNvme ? 'Ultra-Speed NVMe M.2 PCIe SSD' : 'High-Speed Solid State Drive (SSD)';
                if (specStorageDetails) specStorageDetails.textContent = `Total Capacity: ${hw.storage.totalGb} GB • Fast Game Asset Streaming Enabled`;

                if (window.soundFx) window.soundFx.playSuccessChime();
                logTicker(`BENCHMARK COMPLETE: ${hw.tier} detected (Score: ${hw.score}/100).`);
            }
        } catch (e) {
            console.error('Scan hardware error:', e);
        }
    }

    if (btnRescan) {
        btnRescan.addEventListener('click', () => {
            if (window.soundFx) window.soundFx.playClick();
            runHardwareScan();
        });
    }

    // Trigger initial scan
    setTimeout(runHardwareScan, 600);

    // 13. Game Vault Library Rendering
    const gameVaultGrid = document.getElementById('gameVaultGrid');
    async function loadGameVault() {
        if (!gameVaultGrid || !window.api || !window.api.getGames) return;
        try {
            const games = await window.api.getGames();
            gameVaultGrid.innerHTML = '';

            games.forEach(game => {
                const card = document.createElement('div');
                card.className = 'game-card';
                card.innerHTML = `
                    <div class="game-poster">${game.icon}</div>
                    <div class="game-meta">
                        <div class="game-title">${game.name}</div>
                        <div class="game-status">${game.status}</div>
                        <button class="btn-launch-game">⚡ LAUNCH & BOOST</button>
                    </div>
                `;

                const launchBtn = card.querySelector('.btn-launch-game');
                launchBtn.addEventListener('click', () => {
                    if (window.soundFx) window.soundFx.playTurboSpool();
                    launchBtn.textContent = '🚀 BOOSTED & RUNNING';
                    launchBtn.style.borderColor = '#00ff88';
                    launchBtn.style.color = '#00ff88';
                    const statusElem = card.querySelector('.game-status');
                    statusElem.textContent = 'RUNNING (HIGH PRIORITY)';
                    statusElem.style.color = '#00ff88';
                    logTicker(`GAME LAUNCHED: ${game.name} (${game.exe}) boosted to HIGH priority with Standby RAM cleared!`);
                });

                gameVaultGrid.appendChild(card);
            });
        } catch (e) {
            console.error('Error loading game vault:', e);
        }
    }
    loadGameVault();

    // 14. Custom Window Titlebar Controls
    const btnMin = document.getElementById('btnMin');
    const btnMax = document.getElementById('btnMax');
    const btnClose = document.getElementById('btnClose');

    if (btnMin && window.api && window.api.minimize) {
        btnMin.addEventListener('click', () => window.api.minimize());
    }
    if (btnMax && window.api && window.api.maximize) {
        btnMax.addEventListener('click', () => window.api.maximize());
    }
    if (btnClose && window.api && window.api.close) {
        btnClose.addEventListener('click', () => window.api.close());
    }
});
