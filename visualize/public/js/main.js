const App = {
  elements: {},
  currentRange: { start: null, end: null },
  defaultRange: { start: null, end: null },
  availableRange: { first: null, last: null },
  topKeys: [],
  pendingLoads: 0,

  async init() {
    console.log('🚀 KeyRecord 可视化应用启动中...');
    this.initFilterControls();
    this.initTheme();
    CalendarHeatmap.init();
    KeyboardHeatmap.init();
    if (window.Analysis) {
      Analysis.init();
      Analysis.configureLive(
        () => ({ ...this.currentRange }),
        async () => {
          if (window.ApiCache) ApiCache.invalidateApi();
          await this.refreshRangeData({ forceAnalysis: true });
        }
      );
    }

    CalendarHeatmap.setDateSelectHandler((date) => {
      this.setDateRange(date, date);
      this.refreshRangeData();
    });
    CalendarHeatmap.setRangeSelectHandler((range) => {
      this.setDateRange(range.start, range.end);
      this.refreshRangeData();
    });

    const info = await this.withLoading(() => this.loadDatabaseInfo());
    this.configureDateRange(info);
    this.setDateRange(this.defaultRange.start, this.defaultRange.end);

    await Promise.all([
      this.refreshRangeData(),
      this.withLoading(() => CalendarHeatmap.load())
    ]);

    CalendarHeatmap.setBrushRange(this.currentRange.start, this.currentRange.end);

    console.log('✓ 应用初始化完成');
  },

  initFilterControls() {
    this.elements.loadingBar = document.getElementById('loading-bar');
    this.elements.filterStart = document.getElementById('filter-start');
    this.elements.filterEnd = document.getElementById('filter-end');
    this.elements.filterApply = document.getElementById('filter-apply');
    this.elements.filterToday = document.getElementById('filter-today');
    this.elements.filterLast7 = document.getElementById('filter-last7');
    this.elements.filterLast30 = document.getElementById('filter-last30');
    this.elements.filterMonth = document.getElementById('filter-month');
    this.elements.filterAll = document.getElementById('filter-all');
    this.elements.filterStatus = document.getElementById('filter-status');
    this.elements.topKeysBody = document.getElementById('top-keys-body');
    this.elements.topKeysStatus = document.getElementById('top-keys-status');
    this.elements.topKeysRange = document.getElementById('top-keys-range');
    this.elements.themeToggle = document.getElementById('theme-toggle');
    this.elements.printPage = document.getElementById('print-page');
    this.elements.exportCsv = document.getElementById('export-csv');
    this.elements.exportJson = document.getElementById('export-json');
    this.elements.exportPng = document.getElementById('export-png');

    this.bindClick(this.elements.filterApply, () => this.applyDateFilter());
    this.bindClick(this.elements.filterToday, () => this.applyPresetRange('today'));
    this.bindClick(this.elements.filterLast7, () => this.applyPresetRange('last7'));
    this.bindClick(this.elements.filterLast30, () => this.applyPresetRange('last30'));
    this.bindClick(this.elements.filterMonth, () => this.applyPresetRange('month'));
    this.bindClick(this.elements.filterAll, () => this.applyPresetRange('all'));
    this.bindClick(this.elements.themeToggle, () => this.toggleTheme());
    this.bindClick(this.elements.printPage, () => window.print());
    this.bindClick(this.elements.exportCsv, () => this.exportData('csv'));
    this.bindClick(this.elements.exportJson, () => this.exportData('json'));
    this.bindClick(this.elements.exportPng, () => KeyboardHeatmap.exportPng(this.currentRange));
  },

  bindClick(element, handler) {
    if (element) {
      element.addEventListener('click', handler);
    }
  },

  async withLoading(work) {
    this.setLoading(true);
    try {
      return await work();
    } finally {
      this.setLoading(false);
    }
  },

  setLoading(isLoading) {
    this.pendingLoads += isLoading ? 1 : -1;
    if (this.pendingLoads < 0) {
      this.pendingLoads = 0;
    }
    if (this.elements.loadingBar) {
      this.elements.loadingBar.classList.toggle('is-active', this.pendingLoads > 0);
    }
  },

  initTheme() {
    const storedTheme = this.readStoredTheme();
    const preferredTheme = storedTheme || 'light';
    this.setTheme(preferredTheme);
  },

  readStoredTheme() {
    try {
      return localStorage.getItem('keyrecord-theme');
    } catch (error) {
      console.warn('读取主题配置失败:', error);
      return null;
    }
  },

  setTheme(theme) {
    const safeTheme = theme === 'dark' ? 'dark' : 'light';
    document.body.dataset.theme = safeTheme;
    if (this.elements.themeToggle) {
      this.elements.themeToggle.textContent = safeTheme === 'dark' ? '☀' : '◐';
      this.elements.themeToggle.setAttribute('aria-label', safeTheme === 'dark' ? '切换浅色主题' : '切换深色主题');
      this.elements.themeToggle.title = safeTheme === 'dark' ? '切换浅色主题' : '切换深色主题';
    }
  },

  toggleTheme() {
    const nextTheme = document.body.dataset.theme === 'dark' ? 'light' : 'dark';
    this.setTheme(nextTheme);
    try {
      localStorage.setItem('keyrecord-theme', nextTheme);
    } catch (error) {
      console.warn('保存主题配置失败:', error);
    }
  },

  async loadDatabaseInfo() {
    try {
      const info = await ApiCache.fetchJson('/api/info');
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
    this.availableRange = {
      first: info && info.first_date ? info.first_date : null,
      last: info && info.last_date ? info.last_date : null
    };
    this.defaultRange = this.getDefaultDateRange(info);

    if (this.elements.filterStart && this.elements.filterEnd && this.availableRange.first && this.availableRange.last) {
      this.elements.filterStart.min = this.availableRange.first;
      this.elements.filterStart.max = this.availableRange.last;
      this.elements.filterEnd.min = this.availableRange.first;
      this.elements.filterEnd.max = this.availableRange.last;
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

  async applyPresetRange(preset) {
    const range = this.getPresetRange(preset);
    if (!range) {
      this.setFilterStatus('当前没有可用日期范围', true);
      return;
    }

    this.setDateRange(range.start, range.end);
    await this.refreshRangeData();
  },

  getPresetRange(preset) {
    const today = this.todayUtcDate();
    const lastDataDate = this.availableRange.last ? this.parseDateValue(this.availableRange.last) : null;
    const firstDataDate = this.availableRange.first ? this.parseDateValue(this.availableRange.first) : null;
    const referenceDate = lastDataDate || today;

    if (preset === 'today') {
      const value = this.formatDateValue(today);
      return { start: value, end: value };
    }

    if (preset === 'all') {
      if (!firstDataDate || !lastDataDate) {
        return null;
      }
      return {
        start: this.formatDateValue(firstDataDate),
        end: this.formatDateValue(lastDataDate)
      };
    }

    if (preset === 'month') {
      const start = new Date(Date.UTC(today.getUTCFullYear(), today.getUTCMonth(), 1));
      const end = new Date(Date.UTC(today.getUTCFullYear(), today.getUTCMonth() + 1, 0));
      return {
        start: this.formatDateValue(start),
        end: this.formatDateValue(end)
      };
    }

    const days = preset === 'last30' ? 30 : 7;
    const startCandidate = this.addDays(referenceDate, -(days - 1));
    const startDate = firstDataDate && firstDataDate > startCandidate ? firstDataDate : startCandidate;

    return {
      start: this.formatDateValue(startDate),
      end: this.formatDateValue(referenceDate)
    };
  },

  async updateKeyboardHeatmap(startDate, endDate) {
    if (!this.validateDateRange(startDate, endDate)) {
      return;
    }

    this.setDateRange(startDate, endDate);
    await this.refreshRangeData();
  },

  async refreshRangeData(options) {
    const { start, end } = this.currentRange;

    if (!this.validateDateRange(start, end)) {
      return;
    }

    this.setFilterStatus('加载中...');
    await this.withLoading(async () => {
      await Promise.all([
        KeyboardHeatmap.loadKeyboardHeatmap(start, end),
        this.loadTopKeys(start, end)
      ]);
    });
    if (window.Analysis && Analysis.isRangeActive()) {
      await Analysis.update(start, end, { force: options && options.forceAnalysis });
    }
    this.setFilterStatus('当前范围: ' + this.formatRangeLabel(start, end));
  },

  async loadTopKeys(startDate, endDate) {
    this.setTopKeysStatus('加载中...');
    this.setTopKeysRangeLabel(startDate, endDate);

    try {
      const keys = await ApiCache.fetchJson(this.buildTopKeysUrl(startDate, endDate));
      const keyRows = Array.isArray(keys) ? keys : [];
      this.topKeys = keyRows;
      this.renderTopKeys(keyRows);
      this.setTopKeysStatus(keyRows.length > 0 ? '已加载' : '暂无数据');
      console.log('✓ Top 20 按键排行加载完成: ' + keyRows.length + ' 个按键');
    } catch (error) {
      console.error('加载 Top 20 按键排行失败:', error);
      this.topKeys = [];
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

    CalendarHeatmap.setBrushRange(startDate, endDate);
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

  exportData(format) {
    const payload = this.buildExportPayload();
    const filenameBase = 'keyrecord_' + this.exportRangeToken();

    if (format === 'json') {
      const json = JSON.stringify(payload, null, 2);
      this.downloadBlob(json, filenameBase + '.json', 'application/json;charset=utf-8');
      return;
    }

    const csv = this.buildCsv(payload);
    this.downloadBlob(csv, filenameBase + '.csv', 'text/csv;charset=utf-8');
  },

  buildExportPayload() {
    return {
      exported_at: new Date().toISOString(),
      range: { ...this.currentRange },
      top_keys: this.topKeys,
      heatmap: KeyboardHeatmap.data,
      daily_stats: CalendarHeatmap.data
    };
  },

  buildCsv(payload) {
    const rows = [
      ['section', 'range_start', 'range_end', 'date', 'key_name', 'vk_code', 'count', 'x', 'y']
    ];

    payload.top_keys.forEach((item) => {
      rows.push(['top_keys', payload.range.start, payload.range.end, '', item.key_name || '', item.vk_code, item.count, '', '']);
    });

    payload.heatmap.forEach((item) => {
      rows.push(['heatmap', payload.range.start, payload.range.end, '', item.key_name || '', item.vk_code, item.count, item.x, item.y]);
    });

    payload.daily_stats.forEach((item) => {
      rows.push(['daily_stats', '', '', item.date, '', '', item.count, '', '']);
    });

    return rows.map(row => row.map(value => this.escapeCsvValue(value)).join(',')).join('\r\n') + '\r\n';
  },

  escapeCsvValue(value) {
    if (value === null || value === undefined) {
      return '';
    }
    const text = String(value);
    if (/[",\r\n]/.test(text)) {
      return '"' + text.replace(/"/g, '""') + '"';
    }
    return text;
  },

  downloadBlob(content, filename, contentType) {
    const blob = new Blob([content], { type: contentType });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    link.remove();
    URL.revokeObjectURL(url);
  },

  exportRangeToken() {
    const { start, end } = this.currentRange;
    if (start && end) {
      return start === end ? start : start + '_' + end;
    }
    return 'all';
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
