const App = {
  elements: {},
  currentRange: { start: null, end: null },
  defaultRange: { start: null, end: null },

  async init() {
    console.log('🚀 KeyRecord 可视化应用启动中...');
    this.initFilterControls();
    CalendarHeatmap.init();
    KeyboardHeatmap.init();

    const info = await this.loadDatabaseInfo();
    this.configureDateRange(info);
    this.setDateRange(this.defaultRange.start, this.defaultRange.end);

    await Promise.all([
      this.refreshRangeData(),
      CalendarHeatmap.load()
    ]);

    CalendarHeatmap.setDateSelectHandler((date) => {
      this.setDateRange(date, date);
      this.refreshRangeData();
    });

    console.log('✓ 应用初始化完成');
  },

  initFilterControls() {
    this.elements.filterStart = document.getElementById('filter-start');
    this.elements.filterEnd = document.getElementById('filter-end');
    this.elements.filterApply = document.getElementById('filter-apply');
    this.elements.filterLast7 = document.getElementById('filter-last7');
    this.elements.filterStatus = document.getElementById('filter-status');
    this.elements.topKeysBody = document.getElementById('top-keys-body');
    this.elements.topKeysStatus = document.getElementById('top-keys-status');
    this.elements.topKeysRange = document.getElementById('top-keys-range');

    if (this.elements.filterApply) {
      this.elements.filterApply.addEventListener('click', () => {
        this.applyDateFilter();
      });
    }

    if (this.elements.filterLast7) {
      this.elements.filterLast7.addEventListener('click', () => {
        this.setDateRange(this.defaultRange.start, this.defaultRange.end);
        this.refreshRangeData();
      });
    }
  },

  async loadDatabaseInfo() {
    try {
      const response = await fetch('/api/info');
      if (!response.ok) throw new Error('获取数据库信息失败');
      const info = await response.json();
      document.getElementById('total-keys').textContent = info.total_keys.toLocaleString();
      document.getElementById('data-range').textContent = info.first_date && info.last_date
        ? info.first_date + ' ~ ' + info.last_date
        : '暂无数据';
      console.log('✓ 数据库信息加载完成');
      return info;
    } catch (error) {
      console.error('加载数据库信息失败:', error);
      document.getElementById('total-keys').textContent = '错误';
      document.getElementById('data-range').textContent = '错误';
      return null;
    }
  },

  configureDateRange(info) {
    this.defaultRange = this.getDefaultDateRange(info);

    if (this.elements.filterStart && this.elements.filterEnd && info && info.first_date && info.last_date) {
      this.elements.filterStart.min = info.first_date;
      this.elements.filterStart.max = info.last_date;
      this.elements.filterEnd.min = info.first_date;
      this.elements.filterEnd.max = info.last_date;
    }
  },

  getDefaultDateRange(info) {
    const fallbackEnd = this.todayUtcDate();
    const endDate = info && info.last_date ? this.parseDateValue(info.last_date) : fallbackEnd;
    const firstDate = info && info.first_date ? this.parseDateValue(info.first_date) : null;

    if (!endDate) {
      return { start: null, end: null };
    }

    // 默认最近 7 天包含结束日当天，因此从 end 向前推 6 天。
    const startCandidate = this.addDays(endDate, -6);
    const startDate = firstDate && firstDate > startCandidate ? firstDate : startCandidate;

    return {
      start: this.formatDateValue(startDate),
      end: this.formatDateValue(endDate)
    };
  },

  async applyDateFilter() {
    const startDate = this.elements.filterStart ? this.elements.filterStart.value : null;
    const endDate = this.elements.filterEnd ? this.elements.filterEnd.value : null;

    if (!this.validateDateRange(startDate, endDate)) {
      return;
    }

    this.setDateRange(startDate, endDate);
    await this.refreshRangeData();
  },

  async updateKeyboardHeatmap(startDate, endDate) {
    if (!this.validateDateRange(startDate, endDate)) {
      return;
    }

    this.setDateRange(startDate, endDate);
    await this.refreshRangeData();
  },

  async refreshRangeData() {
    const { start, end } = this.currentRange;

    if (!this.validateDateRange(start, end)) {
      return;
    }

    this.setFilterStatus('加载中...');
    await Promise.all([
      KeyboardHeatmap.loadKeyboardHeatmap(start, end),
      this.loadTopKeys(start, end)
    ]);
    this.setFilterStatus('当前范围: ' + this.formatRangeLabel(start, end));
  },

  async loadTopKeys(startDate, endDate) {
    this.setTopKeysStatus('加载中...');
    this.setTopKeysRangeLabel(startDate, endDate);

    try {
      const response = await fetch(this.buildTopKeysUrl(startDate, endDate));
      if (!response.ok) throw new Error('API 请求失败: ' + response.status);

      const keys = await response.json();
      const keyRows = Array.isArray(keys) ? keys : [];
      this.renderTopKeys(keyRows);
      this.setTopKeysStatus(keyRows.length > 0 ? '已加载' : '暂无数据');
      console.log('✓ Top 20 按键排行加载完成: ' + keyRows.length + ' 个按键');
    } catch (error) {
      console.error('加载 Top 20 按键排行失败:', error);
      this.renderTopKeys([]);
      this.setTopKeysStatus('加载失败: ' + error.message);
    }
  },

  buildTopKeysUrl(startDate, endDate) {
    const params = new URLSearchParams();
    params.set('limit', '20');

    if (startDate && endDate) {
      params.set('start', startDate);
      params.set('end', endDate);
    }

    return '/api/keys?' + params.toString();
  },

  renderTopKeys(keys) {
    if (!this.elements.topKeysBody) return;

    this.elements.topKeysBody.innerHTML = '';

    if (keys.length === 0) {
      const row = document.createElement('tr');
      const cell = document.createElement('td');
      cell.className = 'empty-cell';
      cell.colSpan = 5;
      cell.textContent = '当前范围暂无按键数据';
      row.appendChild(cell);
      this.elements.topKeysBody.appendChild(row);
      return;
    }

    const total = keys.reduce((sum, item) => sum + (Number(item.count) || 0), 0);

    keys.forEach((item, index) => {
      const count = Number(item.count) || 0;
      const ratio = total > 0 ? count / total : 0;
      const row = document.createElement('tr');

      this.appendTableCell(row, String(index + 1), 'rank-cell');
      this.appendTableCell(row, item.key_name || '(未知)', 'key-name-cell');
      this.appendTableCell(row, String(item.vk_code), 'code-cell');
      this.appendTableCell(row, count.toLocaleString(), 'count-cell');

      const ratioCell = document.createElement('td');
      ratioCell.className = 'ratio-cell';

      const ratioBar = document.createElement('span');
      ratioBar.className = 'ratio-bar';
      ratioBar.style.width = Math.max(ratio * 100, 2) + '%';

      const ratioText = document.createElement('span');
      ratioText.className = 'ratio-text';
      ratioText.textContent = (ratio * 100).toFixed(ratio >= 0.1 ? 0 : 1) + '%';

      ratioCell.appendChild(ratioBar);
      ratioCell.appendChild(ratioText);
      row.appendChild(ratioCell);
      this.elements.topKeysBody.appendChild(row);
    });
  },

  appendTableCell(row, text, className) {
    const cell = document.createElement('td');
    cell.className = className;
    cell.textContent = text;
    row.appendChild(cell);
  },

  setDateRange(startDate, endDate) {
    this.currentRange = { start: startDate, end: endDate };

    if (this.elements.filterStart) {
      this.elements.filterStart.value = startDate || '';
    }

    if (this.elements.filterEnd) {
      this.elements.filterEnd.value = endDate || '';
    }
  },

  validateDateRange(startDate, endDate) {
    const start = this.parseDateValue(startDate);
    const end = this.parseDateValue(endDate);

    if (!start || !end) {
      this.setFilterStatus('请选择完整日期范围', true);
      return false;
    }

    if (start > end) {
      this.setFilterStatus('开始日期不能晚于结束日期', true);
      return false;
    }

    return true;
  },

  formatRangeLabel(startDate, endDate) {
    if (startDate && endDate) {
      return startDate === endDate ? startDate : startDate + ' ~ ' + endDate;
    }

    return '全部';
  },

  setFilterStatus(message, isError = false) {
    if (!this.elements.filterStatus) return;

    this.elements.filterStatus.textContent = message;
    this.elements.filterStatus.classList.toggle('is-error', isError);
  },

  setTopKeysStatus(message) {
    if (this.elements.topKeysStatus) {
      this.elements.topKeysStatus.textContent = message;
    }
  },

  setTopKeysRangeLabel(startDate, endDate) {
    if (this.elements.topKeysRange) {
      this.elements.topKeysRange.textContent = this.formatRangeLabel(startDate, endDate);
    }
  },

  parseDateValue(value) {
    if (!value || !/^\d{4}-\d{2}-\d{2}$/.test(value)) {
      return null;
    }

    const [year, month, day] = value.split('-').map(Number);
    const date = new Date(Date.UTC(year, month - 1, day));

    if (
      date.getUTCFullYear() !== year ||
      date.getUTCMonth() !== month - 1 ||
      date.getUTCDate() !== day
    ) {
      return null;
    }

    return date;
  },

  formatDateValue(date) {
    return date.toISOString().slice(0, 10);
  },

  addDays(date, days) {
    const next = new Date(date.getTime());
    next.setUTCDate(next.getUTCDate() + days);
    return next;
  },

  todayUtcDate() {
    const now = new Date();
    return new Date(Date.UTC(now.getFullYear(), now.getMonth(), now.getDate()));
  }
};

document.addEventListener('DOMContentLoaded', () => {
  App.init();
});

window.App = App;
