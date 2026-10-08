// Canvas Automotive Cluster Gauge Renderer
class CarGauge {
    constructor(canvasId, options = {}) {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) return;
        this.ctx = this.canvas.getContext('2d');
        
        this.title = options.title || 'SPEED';
        this.unit = options.unit || '%';
        this.minVal = options.minVal || 0;
        this.maxVal = options.maxVal || 100;
        this.redlineVal = options.redlineVal || 85;
        this.glowColor = options.glowColor || '#00f3ff';
        this.accentColor = options.accentColor || '#00ff88';

        this.currentVal = 0;
        this.targetVal = 0;
        this.needleAngle = -Math.PI * 0.75;
        this.velocity = 0;

        this.resize();
        window.addEventListener('resize', () => this.resize());
    }

    resize() {
        if (!this.canvas) return;
        const rect = this.canvas.getBoundingClientRect();
        this.width = rect.width * (window.devicePixelRatio || 1);
        this.height = rect.height * (window.devicePixelRatio || 1);
        this.canvas.width = this.width;
        this.canvas.height = this.height;
        this.cx = this.width / 2;
        this.cy = this.height / 2;
        this.radius = Math.min(this.width, this.height) * 0.42;
    }

    setValue(val) {
        this.targetVal = Math.max(this.minVal, Math.min(this.maxVal, val));
    }

    update() {
        // Smooth spring physics for gauge needle
        const diff = this.targetVal - this.currentVal;
        this.velocity += diff * 0.12;
        this.velocity *= 0.75;
        this.currentVal += this.velocity;
    }

    draw(subtext = '', extraBadge = '') {
        if (!this.ctx) return;
        const ctx = this.ctx;
        ctx.clearRect(0, 0, this.width, this.height);

        const startAngle = Math.PI * 0.75; // 135 deg
        const endAngle = Math.PI * 2.25;   // 405 deg
        const totalAngle = endAngle - startAngle;

        const valRatio = (this.currentVal - this.minVal) / (this.maxVal - this.minVal);
        const redlineRatio = (this.redlineVal - this.minVal) / (this.maxVal - this.minVal);
        const currentAngle = startAngle + valRatio * totalAngle;

        // 1. Dark Gauge Outer Bezel
        ctx.beginPath();
        ctx.arc(this.cx, this.cy, this.radius + 12, 0, Math.PI * 2);
        ctx.strokeStyle = '#181e2b';
        ctx.lineWidth = 4;
        ctx.stroke();

        // 2. Background Track
        ctx.beginPath();
        ctx.arc(this.cx, this.cy, this.radius, startAngle, endAngle);
        ctx.strokeStyle = '#121722';
        ctx.lineWidth = 14;
        ctx.lineCap = 'round';
        ctx.stroke();

        // 3. Normal Active Gradient Arc
        const normalEndAngle = startAngle + Math.min(valRatio, redlineRatio) * totalAngle;
        if (valRatio > 0) {
            ctx.beginPath();
            ctx.arc(this.cx, this.cy, this.radius, startAngle, normalEndAngle);
            ctx.strokeStyle = this.glowColor;
            ctx.lineWidth = 14;
            ctx.lineCap = 'round';
            ctx.shadowColor = this.glowColor;
            ctx.shadowBlur = 12;
            ctx.stroke();
            ctx.shadowBlur = 0;
        }

        // 4. Redline Zone Arc
        ctx.beginPath();
        ctx.arc(this.cx, this.cy, this.radius, startAngle + redlineRatio * totalAngle, endAngle);
        ctx.strokeStyle = 'rgba(255, 0, 85, 0.25)';
        ctx.lineWidth = 14;
        ctx.stroke();

        if (valRatio > redlineRatio) {
            ctx.beginPath();
            ctx.arc(this.cx, this.cy, this.radius, startAngle + redlineRatio * totalAngle, currentAngle);
            ctx.strokeStyle = '#ff0055';
            ctx.lineWidth = 14;
            ctx.lineCap = 'round';
            ctx.shadowColor = '#ff0055';
            ctx.shadowBlur = 18;
            ctx.stroke();
            ctx.shadowBlur = 0;
        }

        // 5. Radial Tick Marks
        const tickCount = 20;
        for (let i = 0; i <= tickCount; i++) {
            const angle = startAngle + (i / tickCount) * totalAngle;
            const isMajor = (i % 5 === 0);
            const isRedline = (i / tickCount) >= redlineRatio;

            const innerR = isMajor ? this.radius - 20 : this.radius - 12;
            const outerR = this.radius - 4;

            const x1 = this.cx + Math.cos(angle) * innerR;
            const y1 = this.cy + Math.sin(angle) * innerR;
            const x2 = this.cx + Math.cos(angle) * outerR;
            const y2 = this.cy + Math.sin(angle) * outerR;

            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.strokeStyle = isRedline ? '#ff0055' : (isMajor ? '#e0e6ed' : '#3d4b66');
            ctx.lineWidth = isMajor ? 3 : 1.5;
            ctx.stroke();
        }

        // 6. Center Hub Circle
        ctx.beginPath();
        ctx.arc(this.cx, this.cy, 36, 0, Math.PI * 2);
        ctx.fillStyle = '#0e121a';
        ctx.fill();
        ctx.strokeStyle = this.currentVal >= this.redlineVal ? '#ff0055' : this.glowColor;
        ctx.lineWidth = 3;
        ctx.stroke();

        // 7. Glowing Needle
        ctx.save();
        ctx.translate(this.cx, this.cy);
        ctx.rotate(currentAngle);

        ctx.beginPath();
        ctx.moveTo(-16, -3);
        ctx.lineTo(this.radius - 6, 0);
        ctx.lineTo(-16, 3);
        ctx.closePath();

        const needleColor = this.currentVal >= this.redlineVal ? '#ff0055' : this.glowColor;
        ctx.fillStyle = needleColor;
        ctx.shadowColor = needleColor;
        ctx.shadowBlur = 16;
        ctx.fill();

        ctx.beginPath();
        ctx.arc(0, 0, 10, 0, Math.PI * 2);
        ctx.fillStyle = '#ffffff';
        ctx.fill();
        ctx.restore();

        // 8. Digital Readout HUD in Gauge Center Bottom
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';

        // Digital Value
        ctx.font = `900 ${Math.round(this.radius * 0.36)}px "Rajdhani", sans-serif`;
        ctx.fillStyle = this.currentVal >= this.redlineVal ? '#ff0055' : '#ffffff';
        ctx.fillText(Math.round(this.currentVal), this.cx, this.cy + this.radius * 0.42);

        // Unit label
        ctx.font = `700 ${Math.round(this.radius * 0.16)}px "Rajdhani", sans-serif`;
        ctx.fillStyle = this.glowColor;
        ctx.fillText(this.unit, this.cx, this.cy + this.radius * 0.62);

        // Subtext (e.g., GHz or GB)
        if (subtext) {
            ctx.font = `600 ${Math.round(this.radius * 0.14)}px "Rajdhani", sans-serif`;
            ctx.fillStyle = '#8b9bb4';
            ctx.fillText(subtext, this.cx, this.cy + this.radius * 0.78);
        }

        // Gauge Title at Top
        ctx.font = `800 ${Math.round(this.radius * 0.18)}px "Rajdhani", sans-serif`;
        ctx.fillStyle = '#8b9bb4';
        ctx.fillText(this.title, this.cx, this.cy - this.radius * 0.52);

        // Redline warning indicator
        if (this.currentVal >= this.redlineVal) {
            ctx.font = `900 ${Math.round(this.radius * 0.14)}px "Rajdhani", sans-serif`;
            ctx.fillStyle = '#ff0055';
            ctx.fillText('⚡ REDLINE', this.cx, this.cy - this.radius * 0.72);
        } else if (extraBadge) {
            ctx.font = `700 ${Math.round(this.radius * 0.13)}px "Rajdhani", sans-serif`;
            ctx.fillStyle = this.accentColor;
            ctx.fillText(extraBadge, this.cx, this.cy - this.radius * 0.72);
        }
    }
}

window.CarGauge = CarGauge;
