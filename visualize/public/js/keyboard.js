const KeyboardHeatmap = {
  container: null,
  viewportElement: null,
  stageElement: null,
  statusElement: null,
  totalElement: null,
  rangeElement: null,
  tooltipElement: null,
  resizeObserver: null,
  data: [],
  mode: 'standard',
  rangeTotal: 0,
  requestVersion: 0,
  currentRange: { start: null, end: null },
  numpadGap: 30,
  layout: [
    // 坐标需与 src/keyboard_layout.cpp 保持一致。
    { code: 27, label: 'Esc', x: 50, y: 20, width: 50, height: 50 },
    { code: 112, label: 'F1', x: 135, y: 20, width: 50, height: 50 },
    { code: 113, label: 'F2', x: 195, y: 20, width: 50, height: 50 },
    { code: 114, label: 'F3', x: 255, y: 20, width: 50, height: 50 },
    { code: 115, label: 'F4', x: 315, y: 20, width: 50, height: 50 },
    { code: 116, label: 'F5', x: 400, y: 20, width: 50, height: 50 },
    { code: 117, label: 'F6', x: 460, y: 20, width: 50, height: 50 },
    { code: 118, label: 'F7', x: 520, y: 20, width: 50, height: 50 },
    { code: 119, label: 'F8', x: 580, y: 20, width: 50, height: 50 },
    { code: 120, label: 'F9', x: 665, y: 20, width: 50, height: 50 },
    { code: 121, label: 'F10', x: 725, y: 20, width: 50, height: 50 },
    { code: 122, label: 'F11', x: 785, y: 20, width: 50, height: 50 },
    { code: 123, label: 'F12', x: 845, y: 20, width: 50, height: 50 },
    { code: 44, label: 'PrtSc', x: 970, y: 20, width: 50, height: 50 },
    { code: 145, label: 'ScrLk', x: 1030, y: 20, width: 50, height: 50 },
    { code: 19, label: 'Pause', x: 1090, y: 20, width: 50, height: 50 },

    { code: 192, label: '`', x: 50, y: 80, width: 50, height: 50 },
    { code: 49, label: '1', x: 110, y: 80, width: 50, height: 50 },
    { code: 50, label: '2', x: 170, y: 80, width: 50, height: 50 },
    { code: 51, label: '3', x: 230, y: 80, width: 50, height: 50 },
    { code: 52, label: '4', x: 290, y: 80, width: 50, height: 50 },
    { code: 53, label: '5', x: 350, y: 80, width: 50, height: 50 },
    { code: 54, label: '6', x: 410, y: 80, width: 50, height: 50 },
    { code: 55, label: '7', x: 470, y: 80, width: 50, height: 50 },
    { code: 56, label: '8', x: 530, y: 80, width: 50, height: 50 },
    { code: 57, label: '9', x: 590, y: 80, width: 50, height: 50 },
    { code: 48, label: '0', x: 650, y: 80, width: 50, height: 50 },
    { code: 189, label: '-', x: 710, y: 80, width: 50, height: 50 },
    { code: 187, label: '=', x: 770, y: 80, width: 50, height: 50 },
    { code: 8, label: 'Backspace', x: 830, y: 80, width: 110, height: 50 },
    { code: 45, label: 'Ins', x: 970, y: 80, width: 50, height: 50 },
    { code: 36, label: 'Home', x: 1030, y: 80, width: 50, height: 50 },
    { code: 33, label: 'PgUp', x: 1090, y: 80, width: 50, height: 50 },
    { code: 144, label: 'Num', x: 1140, y: 80, width: 50, height: 50 },
    { code: 111, label: '/', x: 1200, y: 80, width: 50, height: 50 },
    { code: 106, label: '*', x: 1260, y: 80, width: 50, height: 50 },
    { code: 109, label: '-', x: 1320, y: 80, width: 50, height: 50 },

    { code: 9, label: 'Tab', x: 50, y: 140, width: 75, height: 50 },
    { code: 81, label: 'Q', x: 135, y: 140, width: 50, height: 50 },
    { code: 87, label: 'W', x: 195, y: 140, width: 50, height: 50 },
    { code: 69, label: 'E', x: 255, y: 140, width: 50, height: 50 },
    { code: 82, label: 'R', x: 315, y: 140, width: 50, height: 50 },
    { code: 84, label: 'T', x: 375, y: 140, width: 50, height: 50 },
    { code: 89, label: 'Y', x: 435, y: 140, width: 50, height: 50 },
    { code: 85, label: 'U', x: 495, y: 140, width: 50, height: 50 },
    { code: 73, label: 'I', x: 555, y: 140, width: 50, height: 50 },
    { code: 79, label: 'O', x: 615, y: 140, width: 50, height: 50 },
    { code: 80, label: 'P', x: 675, y: 140, width: 50, height: 50 },
    { code: 219, label: '[', x: 735, y: 140, width: 50, height: 50 },
    { code: 221, label: ']', x: 795, y: 140, width: 50, height: 50 },
    { code: 220, label: '\\', x: 855, y: 140, width: 85, height: 50 },
    { code: 46, label: 'Del', x: 970, y: 140, width: 50, height: 50 },
    { code: 35, label: 'End', x: 1030, y: 140, width: 50, height: 50 },
    { code: 34, label: 'PgDn', x: 1090, y: 140, width: 50, height: 50 },
    { code: 103, label: '7', x: 1140, y: 140, width: 50, height: 50 },
    { code: 104, label: '8', x: 1200, y: 140, width: 50, height: 50 },
    { code: 105, label: '9', x: 1260, y: 140, width: 50, height: 50 },
    { code: 107, label: '+', x: 1320, y: 140, width: 50, height: 110 },

    { code: 20, label: 'Caps', x: 50, y: 200, width: 90, height: 50 },
    { code: 65, label: 'A', x: 150, y: 200, width: 50, height: 50 },
    { code: 83, label: 'S', x: 210, y: 200, width: 50, height: 50 },
    { code: 68, label: 'D', x: 270, y: 200, width: 50, height: 50 },
    { code: 70, label: 'F', x: 330, y: 200, width: 50, height: 50 },
    { code: 71, label: 'G', x: 390, y: 200, width: 50, height: 50 },
    { code: 72, label: 'H', x: 450, y: 200, width: 50, height: 50 },
    { code: 74, label: 'J', x: 510, y: 200, width: 50, height: 50 },
    { code: 75, label: 'K', x: 570, y: 200, width: 50, height: 50 },
    { code: 76, label: 'L', x: 630, y: 200, width: 50, height: 50 },
    { code: 186, label: ';', x: 690, y: 200, width: 50, height: 50 },
    { code: 222, label: "'", x: 750, y: 200, width: 50, height: 50 },
    { code: 13, label: 'Enter', x: 810, y: 200, width: 130, height: 50 },
    { code: 100, label: '4', x: 1140, y: 200, width: 50, height: 50 },
    { code: 101, label: '5', x: 1200, y: 200, width: 50, height: 50 },
    { code: 102, label: '6', x: 1260, y: 200, width: 50, height: 50 },

    { code: 160, label: 'Shift', x: 50, y: 260, width: 110, height: 50 },
    { code: 90, label: 'Z', x: 170, y: 260, width: 50, height: 50 },
    { code: 88, label: 'X', x: 230, y: 260, width: 50, height: 50 },
    { code: 67, label: 'C', x: 290, y: 260, width: 50, height: 50 },
    { code: 86, label: 'V', x: 350, y: 260, width: 50, height: 50 },
    { code: 66, label: 'B', x: 410, y: 260, width: 50, height: 50 },
    { code: 78, label: 'N', x: 470, y: 260, width: 50, height: 50 },
    { code: 77, label: 'M', x: 530, y: 260, width: 50, height: 50 },
    { code: 188, label: ',', x: 590, y: 260, width: 50, height: 50 },
    { code: 190, label: '.', x: 650, y: 260, width: 50, height: 50 },
    { code: 191, label: '/', x: 710, y: 260, width: 50, height: 50 },
    { code: 161, label: 'Shift', x: 770, y: 260, width: 170, height: 50 },
    { code: 38, label: 'Up', x: 1030, y: 260, width: 50, height: 50 },
    { code: 97, label: '1', x: 1140, y: 260, width: 50, height: 50 },
    { code: 98, label: '2', x: 1200, y: 260, width: 50, height: 50 },
    { code: 99, label: '3', x: 1260, y: 260, width: 50, height: 50 },

    { code: 162, label: 'Ctrl', x: 50, y: 320, width: 70, height: 50 },
    { code: 164, label: 'Alt', x: 210, y: 320, width: 70, height: 50 },
    { code: 32, label: 'Space', x: 290, y: 320, width: 300, height: 50 },
    { code: 165, label: 'Alt', x: 600, y: 320, width: 70, height: 50 },
    { code: 163, label: 'Ctrl', x: 840, y: 320, width: 100, height: 50 },
    { code: 37, label: 'Left', x: 970, y: 320, width: 50, height: 50 },
    { code: 40, label: 'Down', x: 1030, y: 320, width: 50, height: 50 },
    { code: 39, label: 'Right', x: 1090, y: 320, width: 50, height: 50 },
    { code: 96, label: '0', x: 1140, y: 320, width: 110, height: 50 },
    { code: 110, label: '.', x: 1260, y: 320, width: 50, height: 50 }
  ],

  init() {
    this.container = document.getElementById('keyboard-heatmap');
    this.viewportElement = document.getElementById('keyboard-viewport');
    this.stageElement = document.getElementById('keyboard-stage');
    this.statusElement = document.getElementById('keyboard-status');
    this.totalElement = document.getElementById('keyboard-total');
    this.rangeElement = document.getElementById('keyboard-range');

    if (!this.container) {
      console.error('键盘热力图容器未找到');
      return;
    }

    this.layout = this.layout.map((key) => ({ ...key, region: this.getKeyRegion(key) }));
    this.renderKeyboardLayout();
    this.initModeControls();
    this.initFixedLayout();
    const refreshButton = document.getElementById('keyboard-refresh');
    if (refreshButton) refreshButton.addEventListener('click', () => this.refresh());
    console.log('✓ 键盘热力图初始化完成');
  },

  renderKeyboardLayout() {
    const fragment = document.createDocumentFragment();
    const layout = this.getActiveLayout();

    layout.forEach((key) => {
      const keyElement = document.createElement('button');
      keyElement.type = 'button';
      keyElement.className = 'keyboard-key';
      keyElement.dataset.vkCode = String(key.code);
      keyElement.dataset.region = key.region;
      keyElement.style.left = key.x + 'px';
      keyElement.style.top = key.y + 'px';
      keyElement.style.width = key.width + 'px';
      keyElement.style.height = key.height + 'px';
      keyElement.textContent = key.label;
      keyElement.setAttribute('aria-label', key.label + ' 按键');
      keyElement.addEventListener('mouseenter', (event) => this.showKeyTooltip(event, key));
      keyElement.addEventListener('mouseleave', () => this.hideKeyTooltip());
      fragment.appendChild(keyElement);
    });

    this.container.innerHTML = '';
    this.container.appendChild(fragment);

    const dimensions = this.getDimensions();
    this.container.style.width = dimensions.width + 'px';
    this.container.style.height = dimensions.height + 'px';
    this.stageElement.style.width = dimensions.width + 'px';
    this.stageElement.style.height = dimensions.height + 'px';

    if (!this.tooltipElement) {
      this.tooltipElement = document.createElement('div');
      this.tooltipElement.className = 'tooltip keyboard-tooltip';
      this.tooltipElement.style.display = 'none';
      document.body.appendChild(this.tooltipElement);
    }
    this.renderHeatmapData();
  },

  getKeyRegion(key) {
    if (key.y === 20) return 'function';
    if (key.x >= 1140) return 'numpad';
    if (key.x >= 970) return 'navigation';
    if ([160, 161, 162, 163, 164, 165, 20].includes(key.code)) return 'modifier';
    if (key.code >= 65 && key.code <= 90) return 'letters';
    if (key.code >= 48 && key.code <= 57) return 'digits';
    return 'primary';
  },

  getActiveLayout() {
    if (this.mode === 'standard') {
      // 数字区独立右移，保留方向键原坐标和数字区内部间距。
      return this.layout.map((key) => key.region === 'numpad' ? { ...key, x: key.x + this.numpadGap } : key);
    }
    return this.layout
      .filter((key) => key.x < 950 && key.y >= 80)
      .map((key) => ({ ...key, x: key.x - 30, y: key.y - 60 }));
  },

  getDimensions() {
    return this.mode === 'compact' ? { width: 980, height: 330 } : { width: 1430, height: 410 };
  },

  initModeControls() {
    ['standard', 'compact'].forEach((mode) => {
      const button = document.getElementById('keyboard-mode-' + mode);
      if (button) button.addEventListener('click', () => this.setMode(mode));
    });
  },

  setMode(mode) {
    if (mode !== 'standard' && mode !== 'compact') return;
    this.mode = mode;
    ['standard', 'compact'].forEach((value) => {
      const button = document.getElementById('keyboard-mode-' + value);
      if (!button) return;
      const selected = value === mode;
      button.classList.toggle('is-selected', selected);
      button.setAttribute('aria-pressed', String(selected));
    });
    this.renderKeyboardLayout();
    requestAnimationFrame(() => this.fitToViewport());
  },

  initFixedLayout() {
    if (!this.viewportElement || !this.stageElement) return;
    if (window.ResizeObserver) {
      this.resizeObserver = new ResizeObserver(() => this.fitToViewport());
      this.resizeObserver.observe(this.viewportElement);
    } else {
      window.addEventListener('resize', () => this.fitToViewport());
    }
    requestAnimationFrame(() => this.fitToViewport());
  },

  fitToViewport() {
    if (!this.viewportElement || !this.stageElement) return;
    const dimensions = this.getDimensions();
    const scale = Math.min(1, (this.viewportElement.clientWidth - 24) / dimensions.width);
    const offsetX = Math.max(0, (this.viewportElement.clientWidth - dimensions.width * scale) / 2);
    const offsetY = Math.max(0, (this.viewportElement.clientHeight - dimensions.height * scale) / 2);
    this.stageElement.style.transform = 'translate(' + offsetX + 'px,' + offsetY + 'px) scale(' + scale + ')';
  },

  async loadKeyboardHeatmap(startDate, endDate) {
    if (!this.container) return;

    const url = this.buildUrl(startDate, endDate);
    const version = ++this.requestVersion;
    this.currentRange = { start: startDate, end: endDate };
    this.setStatus('加载中...');
    this.setRangeLabel(startDate, endDate);

    try {
      const rows = await ApiCache.fetchJson(url);
      if (version !== this.requestVersion) return;
      this.data = Array.isArray(rows) ? rows : [];
      this.renderHeatmapData();
      this.setStatus(this.data.length > 0 ? '已加载' : '暂无可映射按键数据');
      console.log('✓ 键盘热力图数据加载完成: ' + this.data.length + ' 个按键');
    } catch (error) {
      if (version !== this.requestVersion) return;
      console.error('加载键盘热力图失败:', error);
      this.data = [];
      this.renderHeatmapData();
      this.setStatus('加载失败: ' + error.message);
    }
  },

  async refresh() {
    ApiCache.invalidateMatching('/api/heatmap');
    await this.loadKeyboardHeatmap(this.currentRange.start, this.currentRange.end);
  },

  buildUrl(startDate, endDate) {
    if (startDate && endDate) {
      return '/api/heatmap?start=' + encodeURIComponent(startDate) + '&end=' + encodeURIComponent(endDate);
    }

    if (startDate) {
      return '/api/heatmap?date=' + encodeURIComponent(startDate);
    }

    return '/api/heatmap';
  },

  renderHeatmapData() {
    const visibleCodes = new Set(this.getActiveLayout().map((key) => String(key.code)));
    const visibleData = this.data.filter((item) => visibleCodes.has(String(item.vk_code)));
    const max = visibleData.length > 0 ? Math.max(...visibleData.map((item) => Number(item.count) || 0), 1) : 1;

    if (this.totalElement) this.totalElement.textContent = String(visibleData.length);
    this.updateKeyStates(max);
  },

  getHeatChannels(heat) {
    const value = Math.min(1, Math.max(0, Number(heat) || 0));
    const stops = [
      { at: 0, color: [238, 241, 255] },
      { at: 0.36, color: [205, 209, 255] },
      { at: 0.7, color: [239, 190, 222] },
      { at: 1, color: [255, 116, 151] }
    ];
    const upperIndex = stops.findIndex((stop) => value <= stop.at);
    const upper = stops[upperIndex < 0 ? stops.length - 1 : upperIndex];
    const lower = stops[Math.max(0, (upperIndex < 0 ? stops.length - 1 : upperIndex) - 1)];
    const progress = upper.at === lower.at ? 0 : (value - lower.at) / (upper.at - lower.at);
    return lower.color.map((channel, index) => Math.round(channel + (upper.color[index] - channel) * progress));
  },

  formatHeatColor(channels) {
    return 'rgb(' + channels.join(', ') + ')';
  },

  // 使用同一热度两侧的颜色构造完整键帽渐变，避免恢复圆形热点。
  getHeatGradientColors(heat) {
    const value = Math.min(1, Math.max(0, Number(heat) || 0));
    const startChannels = this.getHeatChannels(Math.max(0, value - 0.12));
    const endChannels = this.getHeatChannels(Math.min(1, value + 0.18));
    const highlightChannels = startChannels.map((channel) => Math.round(channel + (255 - channel) * 0.18));
    return {
      highlight: this.formatHeatColor(highlightChannels),
      start: this.formatHeatColor(startChannels),
      end: this.formatHeatColor(endChannels)
    };
  },

  exportPng(range) {
    const canvas = document.createElement('canvas');
    const dimensions = this.getDimensions();
    canvas.width = dimensions.width;
    canvas.height = dimensions.height;
    const context = canvas.getContext('2d');
    if (!context) {
      this.setStatus('PNG 导出失败: canvas 不可用');
      return;
    }

    this.drawKeyboardExport(context);

    const link = document.createElement('a');
    link.href = canvas.toDataURL('image/png');
    link.download = 'keyrecord_keyboard_' + this.exportRangeToken(range) + '_' + this.mode + '.png';
    document.body.appendChild(link);
    link.click();
    link.remove();
    this.setStatus('PNG 已导出');
  },

  drawKeyboardExport(context) {
    const countByCode = new Map(this.data.map(item => [String(item.vk_code), Number(item.count) || 0]));
    const max = this.data.length > 0 ? Math.max(...this.data.map(item => Number(item.count) || 0), 1) : 1;

    const dimensions = this.getDimensions();
    const background = context.createLinearGradient(0, 0, 0, dimensions.height);
    background.addColorStop(0, '#f8fafc');
    background.addColorStop(1, '#edf3f8');
    context.fillStyle = background;
    context.fillRect(0, 0, dimensions.width, dimensions.height);

    this.getActiveLayout().forEach((key) => {
      const count = countByCode.get(String(key.code)) || 0;
      const heat = count > 0 ? Math.max(count / max, 0.12) : 0;

      this.drawRoundedRect(context, key.x, key.y, key.width, key.height, 6);
      if (count > 0) {
        const colors = this.getHeatGradientColors(heat);
        const keyGradient = context.createLinearGradient(key.x, key.y, key.x + key.width, key.y + key.height);
        keyGradient.addColorStop(0, colors.highlight);
        keyGradient.addColorStop(0.42, colors.start);
        keyGradient.addColorStop(1, colors.end);
        context.fillStyle = keyGradient;
      } else {
        context.fillStyle = '#f4f6fb';
      }
      context.fill();
      context.strokeStyle = count > 0 ? '#9ca6ce' : '#cbd2e1';
      context.lineWidth = 1;
      context.stroke();

      context.fillStyle = '#26384d';
      context.font = '600 13px Consolas, monospace';
      context.textAlign = 'center';
      context.textBaseline = 'middle';
      context.fillText(key.label, key.x + key.width / 2, key.y + key.height / 2);
    });
  },

  drawRoundedRect(context, x, y, width, height, radius) {
    context.beginPath();
    context.moveTo(x + radius, y);
    context.lineTo(x + width - radius, y);
    context.quadraticCurveTo(x + width, y, x + width, y + radius);
    context.lineTo(x + width, y + height - radius);
    context.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
    context.lineTo(x + radius, y + height);
    context.quadraticCurveTo(x, y + height, x, y + height - radius);
    context.lineTo(x, y + radius);
    context.quadraticCurveTo(x, y, x + radius, y);
    context.closePath();
  },

  exportRangeToken(range) {
    if (range && range.start && range.end) {
      return range.start === range.end ? range.start : range.start + '_' + range.end;
    }
    return 'all';
  },

  updateKeyStates(max) {
    const countByCode = new Map(this.data.map(item => [String(item.vk_code), item.count]));

    this.container.querySelectorAll('.keyboard-key').forEach((keyElement) => {
      const count = countByCode.get(keyElement.dataset.vkCode) || 0;
      keyElement.dataset.count = String(count);
      keyElement.classList.toggle('has-heat', count > 0);
      if (count > 0) {
        const heat = Math.max(Number(count) / max, 0.12);
        const colors = this.getHeatGradientColors(heat);
        keyElement.style.setProperty('--key-heat-start', colors.start);
        keyElement.style.setProperty('--key-heat-end', colors.end);
      } else {
        keyElement.style.removeProperty('--key-heat-start');
        keyElement.style.removeProperty('--key-heat-end');
      }
    });
  },

  showKeyTooltip(event, key) {
    if (!this.tooltipElement) return;

    const dataItem = this.data.find(item => String(item.vk_code) === String(key.code));
    const count = dataItem ? dataItem.count : 0;
    const name = dataItem && dataItem.key_name ? dataItem.key_name : key.label;

    const ratio = this.rangeTotal > 0 ? count / this.rangeTotal * 100 : 0;
    this.tooltipElement.innerHTML = '<strong>' + name + '</strong><br>VK Code：' + key.code + '<br>次数：' + DateUtils.formatNumber(count) + '<br>范围占比：' + ratio.toFixed(2) + '%';
    this.tooltipElement.style.display = 'block';
    this.tooltipElement.style.left = (event.pageX + 10) + 'px';
    this.tooltipElement.style.top = (event.pageY - 10) + 'px';
  },

  setRangeTotal(total) {
    this.rangeTotal = Math.max(0, Number(total) || 0);
  },

  hideKeyTooltip() {
    if (this.tooltipElement) {
      this.tooltipElement.style.display = 'none';
    }
  },

  // 供时序回放高亮单个按键。
  flashKey(vkCode, durationMs) {
    if (!this.container) return;
    const keyElement = this.container.querySelector('.keyboard-key[data-vk-code="' + String(vkCode) + '"]');
    if (!keyElement) return;

    keyElement.classList.add('is-playing');
    if (keyElement._flashTimer) {
      clearTimeout(keyElement._flashTimer);
    }
    keyElement._flashTimer = setTimeout(() => {
      keyElement.classList.remove('is-playing');
      keyElement._flashTimer = null;
    }, Math.max(80, durationMs || 160));
  },

  clearPlaybackHighlights() {
    if (!this.container) return;
    this.container.querySelectorAll('.keyboard-key.is-playing').forEach((keyElement) => {
      if (keyElement._flashTimer) {
        clearTimeout(keyElement._flashTimer);
        keyElement._flashTimer = null;
      }
      keyElement.classList.remove('is-playing');
    });
  },

  setStatus(message) {
    if (this.statusElement) {
      this.statusElement.textContent = message;
    }
  },

  setRangeLabel(startDate, endDate) {
    if (!this.rangeElement) return;

    if (startDate && endDate) {
      this.rangeElement.textContent = startDate === endDate ? startDate : startDate + ' ~ ' + endDate;
    } else if (startDate) {
      this.rangeElement.textContent = startDate;
    } else {
      this.rangeElement.textContent = '全部';
    }
  }
};

window.KeyboardHeatmap = KeyboardHeatmap;
