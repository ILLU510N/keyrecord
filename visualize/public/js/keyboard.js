const KeyboardHeatmap = {
  container: null,
  statusElement: null,
  totalElement: null,
  rangeElement: null,
  tooltipElement: null,
  heatmapInstance: null,
  data: [],
  layout: [
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

    { code: 162, label: 'Ctrl', x: 50, y: 320, width: 70, height: 50 },
    { code: 164, label: 'Alt', x: 210, y: 320, width: 70, height: 50 },
    { code: 32, label: 'Space', x: 290, y: 320, width: 300, height: 50 },
    { code: 165, label: 'Alt', x: 600, y: 320, width: 70, height: 50 },
    { code: 163, label: 'Ctrl', x: 840, y: 320, width: 100, height: 50 }
  ],

  init() {
    this.container = document.getElementById('keyboard-heatmap');
    this.statusElement = document.getElementById('keyboard-status');
    this.totalElement = document.getElementById('keyboard-total');
    this.rangeElement = document.getElementById('keyboard-range');

    if (!this.container) {
      console.error('键盘热力图容器未找到');
      return;
    }

    this.renderKeyboardLayout();
    this.initHeatmap();
    console.log('✓ 键盘热力图初始化完成');
  },

  renderKeyboardLayout() {
    const fragment = document.createDocumentFragment();

    this.layout.forEach((key) => {
      const keyElement = document.createElement('button');
      keyElement.type = 'button';
      keyElement.className = 'keyboard-key';
      keyElement.dataset.vkCode = String(key.code);
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

    this.tooltipElement = document.createElement('div');
    this.tooltipElement.className = 'tooltip keyboard-tooltip';
    this.tooltipElement.style.display = 'none';
    document.body.appendChild(this.tooltipElement);
  },

  initHeatmap() {
    if (!window.h337) {
      this.setStatus('heatmap.js 未加载，已显示键盘布局');
      return;
    }

    this.heatmapInstance = h337.create({
      container: this.container,
      radius: 32,
      maxOpacity: 0.78,
      minOpacity: 0.15,
      blur: 0.88,
      gradient: {
        0.0: '#3b82f6',
        0.45: '#22c55e',
        0.72: '#facc15',
        1.0: '#ef4444'
      }
    });
  },

  async loadKeyboardHeatmap(startDate, endDate) {
    if (!this.container) return;

    const url = this.buildUrl(startDate, endDate);
    this.setStatus('加载中...');
    this.setRangeLabel(startDate, endDate);

    try {
      const response = await fetch(url);
      if (!response.ok) throw new Error('API 请求失败: ' + response.status);

      this.data = await response.json();
      this.renderHeatmapData();
      this.setStatus(this.data.length > 0 ? '已加载' : '暂无可映射按键数据');
      console.log('✓ 键盘热力图数据加载完成: ' + this.data.length + ' 个按键');
    } catch (error) {
      console.error('加载键盘热力图失败:', error);
      this.data = [];
      this.renderHeatmapData();
      this.setStatus('加载失败: ' + error.message);
    }
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
    const max = this.data.length > 0 ? Math.max(...this.data.map(item => item.count)) : 1;
    const points = this.data.map(item => ({
      x: item.x,
      y: item.y,
      value: item.count
    }));

    if (this.heatmapInstance) {
      this.heatmapInstance.setData({ max, data: points });
    }

    this.totalElement.textContent = String(this.data.length);
    this.updateKeyStates(max);
  },

  updateKeyStates(max) {
    const countByCode = new Map(this.data.map(item => [String(item.vk_code), item.count]));

    this.container.querySelectorAll('.keyboard-key').forEach((keyElement) => {
      const count = countByCode.get(keyElement.dataset.vkCode) || 0;
      keyElement.dataset.count = String(count);
      keyElement.classList.toggle('has-heat', count > 0);
      keyElement.style.setProperty('--key-heat', count > 0 ? String(Math.max(count / max, 0.12)) : '0');
    });
  },

  showKeyTooltip(event, key) {
    if (!this.tooltipElement) return;

    const dataItem = this.data.find(item => String(item.vk_code) === String(key.code));
    const count = dataItem ? dataItem.count : 0;
    const name = dataItem && dataItem.key_name ? dataItem.key_name : key.label;

    this.tooltipElement.innerHTML = '<strong>' + name + '</strong><br>按键数: ' + count.toLocaleString();
    this.tooltipElement.style.display = 'block';
    this.tooltipElement.style.left = (event.pageX + 10) + 'px';
    this.tooltipElement.style.top = (event.pageY - 10) + 'px';
  },

  hideKeyTooltip() {
    if (this.tooltipElement) {
      this.tooltipElement.style.display = 'none';
    }
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
