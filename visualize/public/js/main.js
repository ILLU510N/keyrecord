const App = {
  elements: {},
  currentRange: { start: null, end: null },
  defaultRange: { start: null, end: null },
  availableRange: { first: null, last: null },
  topKeys: [],
  pendingLoads: 0,
  refreshVersion: 0,
  currentTotal: 0,

  async init() {
    this.initElements();
    this.initNavigation();
    this.initTheme();
    CalendarHeatmap.init();
    KeyboardHeatmap.init();

    if (window.Analysis) {
      Analysis.init();
      Analysis.configureLive(
        () => ({ ...this.currentRange }),
        async () => {
          ApiCache.invalidateApi();
          await CalendarHeatmap.load();
          await this.refreshRangeData({ forceAnalysis: true });
        }
      );
    }

    CalendarHeatmap.setDateSelectHandler((date) => this.commitRange(date, date, 'day'));
    CalendarHeatmap.setRangeSelectHandler((range) => this.commitRange(range.start, range.end, 'brush'));

    const [info] = await this.withLoading(() => Promise.all([
      this.loadDatabaseInfo(),
      CalendarHeatmap.load()
    ]));
    this.configureDateRange(info);

    if (!this.availableRange.first || !this.availableRange.last) {
      this.setEmptyState();
      return;
    }

    this.setDateRange(this.defaultRange.start, this.defaultRange.end);
    await this.refreshRangeData();
  },

  initElements() {
    const ids = [
      'loading-bar', 'filter-start', 'filter-end', 'filter-apply', 'filter-today',
      'filter-last7', 'filter-last30', 'filter-month', 'filter-all', 'filter-status',
      'top-keys-body-left', 'top-keys-body-right', 'top-keys-status', 'top-keys-range',
      'theme-toggle', 'print-page', 'export-csv', 'export-json', 'export-png', 'export-top-csv'
    ];
    ids.forEach((id) => { this.elements[id] = document.getElementById(id); });

    this.bindClick('filter-apply', () => this.applyDateFilter());
    this.bindClick('filter-today', () => this.applyPresetRange('today'));
    this.bindClick('filter-last7', () => this.applyPresetRange('last7'));
    this.bindClick('filter-last30', () => this.applyPresetRange('last30'));
    this.bindClick('filter-month', () => this.applyPresetRange('month'));
    this.bindClick('filter-all', () => this.applyPresetRange('all'));
    this.bindClick('theme-toggle', () => this.toggleTheme());
    this.bindClick('print-page', () => window.print());
    this.bindClick('export-csv', () => this.exportData('csv'));
    this.bindClick('export-json', () => this.exportData('json'));
    this.bindClick('export-png', () => KeyboardHeatmap.exportPng(this.currentRange));
    this.bindClick('export-top-csv', () => this.exportTopKeysCsv());

    ['filter-start', 'filter-end'].forEach((id) => {
      const element = this.elements[id];
      if (element) element.addEventListener('keydown', (event) => {
        if (event.key === 'Enter') this.applyDateFilter();
      });
    });
  },

  bindClick(id, handler) {
    const element = this.elements[id];
    if (element) element.addEventListener('click', handler);
  },

  initNavigation() {
    const links = Array.from(document.querySelectorAll('.nav-item[data-section]'));
    const ratios = new Map();
    let navigationTarget = null;
    let navigationReleaseTimer = null;
    const setActiveSection = (sectionId) => {
      links.forEach((link) => link.classList.toggle('is-active', link.dataset.section === sectionId));
    };
    const updateActiveSection = () => {
      if (navigationTarget) {
        setActiveSection(navigationTarget);
        window.clearTimeout(navigationReleaseTimer);
        navigationReleaseTimer = window.setTimeout(() => {
          navigationTarget = null;
          updateActiveSection();
        }, 180);
        return;
      }

      const documentElement = document.documentElement;
      const atPageBottom = window.scrollY + window.innerHeight >= documentElement.scrollHeight - 2;
      if (atPageBottom && links.length > 0) {
        setActiveSection(links[links.length - 1].dataset.section);
        return;
      }

      const active = Array.from(ratios.entries()).sort((a, b) => b[1] - a[1])[0];
      if (active && active[1] > 0) setActiveSection(active[0]);
    };

    links.forEach((link) => link.addEventListener('click', (event) => {
      const section = document.getElementById(link.dataset.section);
      if (!section) return;
      event.preventDefault();
      navigationTarget = link.dataset.section;
      setActiveSection(link.dataset.section);
      updateActiveSection();
      section.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }));

    if (!window.IntersectionObserver) return;
    const observer = new IntersectionObserver((entries) => {
      entries.forEach((entry) => ratios.set(entry.target.id, entry.isIntersecting ? entry.intersectionRatio : 0));
      updateActiveSection();
    }, { rootMargin: '-8% 0px -54% 0px', threshold: [0, 0.15, 0.35, 0.6, 0.9] });
    links.forEach((link) => {
      const section = document.getElementById(link.dataset.section);
      if (section) observer.observe(section);
    });
    window.addEventListener('scroll', updateActiveSection, { passive: true });
  },

  async withLoading(work) {
    this.setLoading(true);
    try { return await work(); } finally { this.setLoading(false); }
  },

  setLoading(isLoading) {
    this.pendingLoads = Math.max(0, this.pendingLoads + (isLoading ? 1 : -1));
    if (this.elements['loading-bar']) this.elements['loading-bar'].classList.toggle('is-active', this.pendingLoads > 0);
  },

  initTheme() {
    let storedTheme = null;
    try { storedTheme = localStorage.getItem('keyrecord-theme'); } catch (error) { console.warn('读取主题配置失败:', error); }
    this.setTheme(storedTheme === 'dark' ? 'dark' : 'light');
  },

  setTheme(theme) {
    document.body.dataset.theme = theme === 'dark' ? 'dark' : 'light';
    const button = this.elements['theme-toggle'];
    if (button) {
      const label = theme === 'dark' ? '切换浅色主题' : '切换深色主题';
      button.setAttribute('aria-label', label);
      button.title = label;
    }
  },

  toggleTheme() {
    const theme = document.body.dataset.theme === 'dark' ? 'light' : 'dark';
    this.setTheme(theme);
    try { localStorage.setItem('keyrecord-theme', theme); } catch (error) { console.warn('保存主题配置失败:', error); }
    CalendarHeatmap.renderCalendar();
  },

  async loadDatabaseInfo() {
    try {
      return await ApiCache.fetchJson('/api/info');
    } catch (error) {
      console.error('加载数据库信息失败:', error);
      this.setFilterStatus('数据库信息加载失败: ' + error.message, true);
      return null;
    }
  },

  configureDateRange(info) {
    this.availableRange = {
      first: info && info.first_date ? info.first_date : null,
      last: info && info.last_date ? info.last_date : null
    };
    this.defaultRange = this.getDefaultDateRange();
    if (!this.availableRange.first || !this.availableRange.last) return;
    ['filter-start', 'filter-end'].forEach((id) => {
      const input = this.elements[id];
      if (!input) return;
      input.min = this.availableRange.first;
      input.max = this.availableRange.last;
    });
  },

  getDefaultDateRange() {
    const end = DateUtils.parse(this.availableRange.last);
    const first = DateUtils.parse(this.availableRange.first);
    if (!end || !first) return { start: null, end: null };
    const candidate = DateUtils.addDays(end, -29);
    return { start: DateUtils.format(first > candidate ? first : candidate), end: DateUtils.format(end) };
  },

  setEmptyState() {
    document.querySelectorAll('.filter-panel button, .filter-panel input').forEach((element) => { element.disabled = true; });
    this.setFilterStatus('数据库中暂无按键记录', false);
    CalendarHeatmap.setTrendRange(null, null);
    CalendarHeatmap.renderCalendar();
    this.renderOverview([]);
    KeyboardHeatmap.setStatus('暂无数据');
    this.renderTopKeys([]);
  },

  async applyDateFilter() {
    const start = this.elements['filter-start'] ? this.elements['filter-start'].value : '';
    const end = this.elements['filter-end'] ? this.elements['filter-end'].value : '';
    await this.commitRange(start, end, 'manual');
  },

  async applyPresetRange(preset) {
    const range = this.getPresetRange(preset);
    if (!range) return;
    await this.commitRange(range.start, range.end, preset);
  },

  getPresetRange(preset) {
    const first = DateUtils.parse(this.availableRange.first);
    const last = DateUtils.parse(this.availableRange.last);
    if (!first || !last) {
      this.setFilterStatus('当前没有可用日期范围', true);
      return null;
    }
    if (preset === 'all') return { start: DateUtils.format(first), end: DateUtils.format(last) };
    if (preset === 'today') {
      const today = new Date();
      const value = DateUtils.format(new Date(Date.UTC(today.getFullYear(), today.getMonth(), today.getDate())));
      if (value < this.availableRange.first || value > this.availableRange.last) {
        this.setFilterStatus('今天不在数据库可用范围内', true);
        return null;
      }
      return { start: value, end: value };
    }
    if (preset === 'month') {
      const startOfMonth = new Date(Date.UTC(last.getUTCFullYear(), last.getUTCMonth(), 1));
      return { start: DateUtils.format(first > startOfMonth ? first : startOfMonth), end: DateUtils.format(last) };
    }
    const days = preset === 'last30' ? 30 : 7;
    const candidate = DateUtils.addDays(last, -(days - 1));
    return { start: DateUtils.format(first > candidate ? first : candidate), end: DateUtils.format(last) };
  },

  async commitRange(start, end, source) {
    if (!this.validateDateRange(start, end)) return;
    this.setDateRange(start, end);
    this.updatePresetState(source);
    await this.refreshRangeData();
  },

  setDateRange(start, end) {
    this.currentRange = { start, end };
    if (this.elements['filter-start']) this.elements['filter-start'].value = start || '';
    if (this.elements['filter-end']) this.elements['filter-end'].value = end || '';
    CalendarHeatmap.setTrendRange(start, end);
  },

  updatePresetState(source) {
    ['today', 'last7', 'last30', 'month', 'all'].forEach((preset) => {
      const button = this.elements['filter-' + preset];
      if (button) button.classList.toggle('is-selected', source === preset);
    });
  },

  validateDateRange(start, end) {
    const startDate = DateUtils.parse(start);
    const endDate = DateUtils.parse(end);
    if (!startDate || !endDate) {
      this.setFilterStatus('请选择合法且完整的日期范围', true);
      return false;
    }
    if (startDate > endDate) {
      this.setFilterStatus('开始日期不能晚于结束日期', true);
      return false;
    }
    if (this.availableRange.first && (start < this.availableRange.first || end > this.availableRange.last)) {
      this.setFilterStatus('日期必须位于 ' + this.availableRange.first + ' 至 ' + this.availableRange.last + ' 之间', true);
      return false;
    }
    return true;
  },

  async refreshRangeData(options) {
    const { start, end } = this.currentRange;
    if (!this.validateDateRange(start, end)) return;
    const version = ++this.refreshVersion;
    const rows = CalendarHeatmap.getRangeRows(start, end);
    this.renderOverview(rows);
    CalendarHeatmap.setTrendRange(start, end);
    KeyboardHeatmap.setRangeTotal(this.currentTotal);
    if (window.Analysis) Analysis.renderActivityOverview(rows, start, end);
    this.setFilterStatus('正在刷新当前范围…');

    await this.withLoading(() => Promise.all([
      KeyboardHeatmap.loadKeyboardHeatmap(start, end),
      this.loadTopKeys(start, end, version)
    ]));
    if (version !== this.refreshVersion) return;
    if (window.Analysis && Analysis.isRangeActive()) {
      await Analysis.update(start, end, { force: options && options.forceAnalysis });
    }
    if (version !== this.refreshVersion) return;
    this.setFilterStatus('当前范围：' + this.formatRangeLabel(start, end));
  },

  renderOverview(rows) {
    const total = rows.reduce((sum, row) => sum + row.count, 0);
    const activeDays = rows.filter((row) => row.count > 0).length;
    const average = activeDays > 0 ? Math.round(total / activeDays) : 0;
    const days = DateUtils.rangeDays(this.currentRange.start, this.currentRange.end);
    this.currentTotal = total;
    this.setText('metric-total', DateUtils.formatNumber(total));
    this.setText('metric-active-days', DateUtils.formatNumber(activeDays));
    this.setText('metric-average', DateUtils.formatNumber(average));
    this.setText('metric-period', days + ' 天');
    this.setText('metric-period-range', this.currentRange.start && this.currentRange.end ? this.currentRange.start + ' → ' + this.currentRange.end : '暂无可用范围');
    this.setText('metric-period-badge', days > 0 ? '最近 ' + days + ' 天' : '等待数据');
    this.setText('overview-range-label', this.currentRange.start ? '当前统计范围：' + this.formatRangeLabel(this.currentRange.start, this.currentRange.end) : '本地键盘数据尚未建立统计范围');
  },

  async loadTopKeys(start, end, version) {
    this.setText('top-keys-status', '加载中…');
    this.setText('top-keys-range', this.formatRangeLabel(start, end));
    try {
      const rows = await ApiCache.fetchJson('/api/keys?limit=20&start=' + encodeURIComponent(start) + '&end=' + encodeURIComponent(end));
      if (version !== this.refreshVersion) return;
      this.topKeys = Array.isArray(rows) ? rows : [];
      this.renderTopKeys(this.topKeys);
      this.setText('top-keys-status', this.topKeys.length ? '已加载 ' + this.topKeys.length + ' 项' : '当前范围暂无数据');
    } catch (error) {
      if (version !== this.refreshVersion) return;
      console.error('加载 Top 20 按键排行失败:', error);
      this.topKeys = [];
      this.renderTopKeys([]);
      this.setText('top-keys-status', '加载失败: ' + error.message);
    }
  },

  renderTopKeys(keys) {
    const left = this.elements['top-keys-body-left'];
    const right = this.elements['top-keys-body-right'];
    if (!left || !right) return;
    left.innerHTML = '';
    right.innerHTML = '';
    const grid = left.closest('.ranking-grid');
    if (grid) grid.classList.toggle('is-single', keys.length <= 10);
    if (!keys.length) {
      left.innerHTML = '<tr><td colspan="5" class="empty-cell">当前范围暂无按键数据</td></tr>';
      return;
    }
    keys.forEach((item, index) => this.appendTopKeyRow(index < 10 ? left : right, item, index));
  },

  appendTopKeyRow(body, item, index) {
    const count = Math.max(0, Number(item.count) || 0);
    const ratio = this.currentTotal > 0 ? count / this.currentTotal : 0;
    const row = document.createElement('tr');
    this.appendTableCell(row, String(index + 1), 'rank-cell');
    this.appendTableCell(row, item.key_name || '(未知)', 'key-name-cell');
    this.appendTableCell(row, String(item.vk_code), 'code-cell');
    this.appendTableCell(row, DateUtils.formatNumber(count), 'count-cell');
    const ratioCell = document.createElement('td');
    ratioCell.className = 'ratio-cell';
    const wrapper = document.createElement('div');
    wrapper.className = 'ratio-wrap';
    wrapper.innerHTML = '<span class="ratio-track"><span class="ratio-bar" style="width:' + Math.min(100, ratio * 100) + '%"></span></span><span class="ratio-text">' + (ratio * 100).toFixed(2) + '%</span>';
    ratioCell.appendChild(wrapper);
    row.appendChild(ratioCell);
    body.appendChild(row);
  },

  appendTableCell(row, text, className) {
    const cell = document.createElement('td');
    cell.className = className;
    cell.textContent = text;
    row.appendChild(cell);
  },

  formatRangeLabel(start, end) {
    return start && end ? (start === end ? start : start + ' 至 ' + end) : '全部';
  },

  exportTopKeysCsv() {
    const rows = [['rank', 'key_name', 'vk_code', 'count', 'range_ratio']];
    this.topKeys.forEach((item, index) => {
      const count = Math.max(0, Number(item.count) || 0);
      rows.push([index + 1, item.key_name || '', item.vk_code, count, this.currentTotal > 0 ? (count / this.currentTotal).toFixed(6) : '0']);
    });
    this.downloadBlob('\ufeff' + this.csvRows(rows), 'keyrecord_top20_' + this.exportRangeToken() + '.csv', 'text/csv;charset=utf-8');
  },

  exportData(format) {
    try {
      const payload = this.buildExportPayload();
      const base = 'keyrecord_' + this.exportRangeToken();
      if (format === 'json') {
        this.downloadBlob(JSON.stringify(payload, null, 2), base + '.json', 'application/json;charset=utf-8');
        return;
      }
      const rows = [['section', 'range_start', 'range_end', 'date', 'key_name', 'vk_code', 'count', 'x', 'y']];
      payload.top_keys.forEach((item) => rows.push(['top_keys', payload.range.start, payload.range.end, '', item.key_name || '', item.vk_code, item.count, '', '']));
      payload.heatmap.forEach((item) => rows.push(['heatmap', payload.range.start, payload.range.end, '', item.key_name || '', item.vk_code, item.count, item.x, item.y]));
      payload.daily_stats.forEach((item) => rows.push(['daily_stats', payload.range.start, payload.range.end, item.date, '', '', item.count, '', '']));
      this.downloadBlob('\ufeff' + this.csvRows(rows), base + '.csv', 'text/csv;charset=utf-8');
    } catch (error) {
      console.error('导出范围数据失败:', error);
      this.setFilterStatus('导出失败: ' + error.message, true);
    }
  },

  buildExportPayload() {
    return {
      exported_at: new Date().toISOString(),
      range: { ...this.currentRange },
      top_keys: this.topKeys,
      heatmap: KeyboardHeatmap.data,
      daily_stats: CalendarHeatmap.getRangeRows(this.currentRange.start, this.currentRange.end)
    };
  },

  csvRows(rows) {
    return rows.map((row) => row.map((value) => this.escapeCsvValue(value)).join(',')).join('\r\n') + '\r\n';
  },

  escapeCsvValue(value) {
    if (value === null || value === undefined) return '';
    const text = String(value);
    return /[",\r\n]/.test(text) ? '"' + text.replace(/"/g, '""') + '"' : text;
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
    return start && end ? (start === end ? start : start + '_' + end) : 'all';
  },

  setFilterStatus(message, isError) {
    const element = this.elements['filter-status'];
    if (!element) return;
    element.textContent = message;
    element.classList.toggle('is-error', Boolean(isError));
  },

  setText(id, value) {
    const element = document.getElementById(id);
    if (element) element.textContent = value;
  }
};

document.addEventListener('DOMContentLoaded', () => App.init());
window.App = App;
