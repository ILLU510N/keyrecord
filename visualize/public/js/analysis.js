// 高级分析模块：小时级热力图、速度曲线、分区/左右手统计、组合统计、时序回放、实时监控与数据对比。
const Analysis = {
  elements: {},
  focusDate: null,
  weekdayLabels: ['周日', '周一', '周二', '周三', '周四', '周五', '周六'],
  timeline: { events: [], timers: [], playing: false },
  live: { enabled: false, timer: null, getRange: null, onTick: null },

  init() {
    const ids = [
      'hourly-heatmap', 'speed-chart', 'speed-date', 'speed-status',
      'region-chart', 'hand-chart', 'combo-body',
      'timeline-date', 'timeline-status', 'timeline-play', 'timeline-stop',
      'timeline-speed', 'timeline-current', 'timeline-progress-bar',
      'live-toggle', 'live-status',
      'compare-a-start', 'compare-a-end', 'compare-b-start', 'compare-b-end',
      'compare-run', 'compare-status', 'compare-body'
    ];
    ids.forEach((id) => {
      this.elements[id] = document.getElementById(id);
    });

    this.bind('timeline-play', 'click', () => this.playTimeline());
    this.bind('timeline-stop', 'click', () => this.stopTimeline());
    this.bind('live-toggle', 'change', (event) => this.toggleLive(event.target.checked));
    this.bind('compare-run', 'click', () => this.runCompare());

    console.log('✓ 高级分析模块初始化完成');
  },

  bind(id, type, handler) {
    const element = this.elements[id];
    if (element) {
      element.addEventListener(type, handler);
    }
  },

  // 由 main.js 在范围数据刷新时调用。
  async update(start, end) {
    this.focusDate = end || start || null;
    this.setText('speed-date', this.focusDate || '全部');
    this.setText('timeline-date', this.focusDate || '全部');

    const rangeQuery = this.rangeQuery(start, end);

    await Promise.all([
      this.loadInto('hourly-heatmap', '/api/hourly-heatmap' + rangeQuery, (rows) => this.renderHourlyHeatmap(rows)),
      this.loadInto('region-chart', '/api/region-stats' + rangeQuery, (rows) => this.renderRegion(rows)),
      this.loadInto('hand-chart', '/api/hand-stats' + rangeQuery, (rows) => this.renderHand(rows)),
      this.loadCombos(rangeQuery),
      this.loadSpeed(this.focusDate),
      this.loadTimeline(this.focusDate)
    ]);
  },

  rangeQuery(start, end) {
    if (start && end) {
      return '?start=' + encodeURIComponent(start) + '&end=' + encodeURIComponent(end);
    }
    return '';
  },

  async fetchJson(url) {
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error('请求失败: ' + response.status);
    }
    return response.json();
  },

  async loadInto(elementId, url, render) {
    try {
      const rows = await this.fetchJson(url);
      render(Array.isArray(rows) ? rows : []);
    } catch (error) {
      console.error('加载失败 ' + url + ':', error);
      render([]);
    }
  },

  // 小时级热力图：7（星期）× 24（小时）网格。
  renderHourlyHeatmap(rows) {
    const container = this.elements['hourly-heatmap'];
    if (!container) return;
    container.innerHTML = '';

    const matrix = Array.from({ length: 7 }, () => new Array(24).fill(0));
    let max = 0;
    rows.forEach((row) => {
      const weekday = Number(row.weekday);
      const hour = Number(row.hour);
      const count = Number(row.count) || 0;
      if (weekday >= 0 && weekday < 7 && hour >= 0 && hour < 24) {
        matrix[weekday][hour] = count;
        if (count > max) max = count;
      }
    });

    const grid = document.createElement('div');
    grid.className = 'hh-grid';

    grid.appendChild(this.hhCell('', 'hh-corner'));
    for (let hour = 0; hour < 24; hour += 1) {
      grid.appendChild(this.hhCell(hour % 2 === 0 ? String(hour) : '', 'hh-head'));
    }

    for (let weekday = 0; weekday < 7; weekday += 1) {
      grid.appendChild(this.hhCell(this.weekdayLabels[weekday], 'hh-row-head'));
      for (let hour = 0; hour < 24; hour += 1) {
        const count = matrix[weekday][hour];
        const cell = this.hhCell('', 'hh-cell');
        const intensity = max > 0 && count > 0 ? Math.max(count / max, 0.08) : 0;
        cell.style.setProperty('--hh-heat', String(intensity));
        cell.classList.toggle('has-heat', count > 0);
        cell.title = this.weekdayLabels[weekday] + ' ' + hour + ':00 — ' + count.toLocaleString() + ' 次';
        grid.appendChild(cell);
      }
    }

    container.appendChild(grid);
  },

  hhCell(text, className) {
    const cell = document.createElement('div');
    cell.className = className;
    if (text) cell.textContent = text;
    return cell;
  },

  renderRegion(rows) {
    const labels = {
      letters: '字母', digits: '数字', numpad: '小键盘', function: '功能键',
      navigation: '导航', modifiers: '修饰键', control: '控制键', punctuation: '标点', other: '其它'
    };
    const data = rows
      .map((row) => ({ label: labels[row.region] || row.region, count: Number(row.count) || 0 }))
      .filter((item) => item.count > 0)
      .sort((a, b) => b.count - a.count);
    this.renderBars(this.elements['region-chart'], data);
  },

  renderHand(rows) {
    const labels = { left: '左手', right: '右手', both: '双手(空格)', unknown: '未分类' };
    const container = this.elements['hand-chart'];
    if (!container) return;
    container.innerHTML = '';

    const data = rows.map((row) => ({ key: row.hand, label: labels[row.hand] || row.hand, count: Number(row.count) || 0 }));
    const total = data.reduce((sum, item) => sum + item.count, 0);
    if (total === 0) {
      container.innerHTML = '<p class="empty-cell">暂无数据</p>';
      return;
    }

    const left = data.find((item) => item.key === 'left') || { count: 0 };
    const right = data.find((item) => item.key === 'right') || { count: 0 };

    const bar = document.createElement('div');
    bar.className = 'hand-bar';
    const leftPart = document.createElement('span');
    leftPart.className = 'hand-left';
    leftPart.style.width = (left.count / total * 100) + '%';
    const rightPart = document.createElement('span');
    rightPart.className = 'hand-right';
    rightPart.style.width = (right.count / total * 100) + '%';
    bar.appendChild(leftPart);
    bar.appendChild(rightPart);
    container.appendChild(bar);

    const legend = document.createElement('div');
    legend.className = 'hand-legend';
    data.filter((item) => item.count > 0).forEach((item) => {
      const span = document.createElement('span');
      const ratio = (item.count / total * 100).toFixed(1);
      span.textContent = item.label + ': ' + item.count.toLocaleString() + ' (' + ratio + '%)';
      legend.appendChild(span);
    });
    container.appendChild(legend);
  },

  // 通用横向条形图（SVG）。
  renderBars(container, data) {
    if (!container) return;
    container.innerHTML = '';
    if (!data.length) {
      container.innerHTML = '<p class="empty-cell">暂无数据</p>';
      return;
    }

    const max = Math.max(...data.map((item) => item.count), 1);
    const list = document.createElement('div');
    list.className = 'bar-list';
    data.forEach((item) => {
      const row = document.createElement('div');
      row.className = 'bar-row';
      const label = document.createElement('span');
      label.className = 'bar-label';
      label.textContent = item.label;
      const track = document.createElement('span');
      track.className = 'bar-track';
      const fill = document.createElement('span');
      fill.className = 'bar-fill';
      fill.style.width = Math.max(item.count / max * 100, 1.5) + '%';
      const value = document.createElement('span');
      value.className = 'bar-value';
      value.textContent = item.count.toLocaleString();
      track.appendChild(fill);
      row.appendChild(label);
      row.appendChild(track);
      row.appendChild(value);
      list.appendChild(row);
    });
    container.appendChild(list);
  },

  async loadCombos(rangeQuery) {
    const body = this.elements['combo-body'];
    if (!body) return;
    try {
      const rows = await this.fetchJson('/api/combos' + rangeQuery + (rangeQuery ? '&' : '?') + 'limit=20');
      this.renderCombos(Array.isArray(rows) ? rows : []);
    } catch (error) {
      console.error('加载组合统计失败:', error);
      this.renderCombos([]);
    }
  },

  renderCombos(rows) {
    const body = this.elements['combo-body'];
    if (!body) return;
    body.innerHTML = '';
    if (!rows.length) {
      body.innerHTML = '<tr><td colspan="2" class="empty-cell">当前范围暂无组合数据</td></tr>';
      return;
    }
    rows.forEach((row) => {
      const tr = document.createElement('tr');
      const combo = document.createElement('td');
      combo.textContent = row.combo;
      const count = document.createElement('td');
      count.className = 'count-cell';
      count.textContent = (Number(row.count) || 0).toLocaleString();
      tr.appendChild(combo);
      tr.appendChild(count);
      body.appendChild(tr);
    });
  },

  async loadSpeed(date) {
    this.setText('speed-status', '加载中...');
    if (!date) {
      this.renderSpeed([]);
      this.setText('speed-status', '请选择单日范围');
      return;
    }
    try {
      const rows = await this.fetchJson('/api/speed?date=' + encodeURIComponent(date) + '&bucket=5');
      this.renderSpeed(Array.isArray(rows) ? rows : []);
      this.setText('speed-status', rows.length > 0 ? '每 5 分钟聚合' : '暂无数据');
    } catch (error) {
      console.error('加载速度曲线失败:', error);
      this.renderSpeed([]);
      this.setText('speed-status', '加载失败');
    }
  },

  // 输入速度曲线（SVG 折线，x 轴为一天 24 小时）。
  renderSpeed(rows) {
    const container = this.elements['speed-chart'];
    if (!container) return;
    container.innerHTML = '';
    if (!rows.length) {
      container.innerHTML = '<p class="empty-cell">暂无数据</p>';
      return;
    }

    const width = 720;
    const height = 200;
    const pad = { top: 16, right: 16, bottom: 28, left: 44 };
    const innerW = width - pad.left - pad.right;
    const innerH = height - pad.top - pad.bottom;
    const maxCount = Math.max(...rows.map((row) => Number(row.count) || 0), 1);
    const xOf = (minute) => pad.left + (minute / 1440) * innerW;
    const yOf = (count) => pad.top + innerH - (count / maxCount) * innerH;

    const svgNs = 'http://www.w3.org/2000/svg';
    const svg = document.createElementNS(svgNs, 'svg');
    svg.setAttribute('viewBox', '0 0 ' + width + ' ' + height);
    svg.setAttribute('class', 'speed-svg');
    svg.setAttribute('preserveAspectRatio', 'xMidYMid meet');

    // 坐标轴
    const axis = document.createElementNS(svgNs, 'path');
    axis.setAttribute('d', 'M' + pad.left + ' ' + pad.top + ' V' + (pad.top + innerH) + ' H' + (pad.left + innerW));
    axis.setAttribute('class', 'speed-axis');
    svg.appendChild(axis);

    // 每 4 小时一条 x 轴刻度
    for (let hour = 0; hour <= 24; hour += 4) {
      const x = xOf(hour * 60);
      const tick = document.createElementNS(svgNs, 'text');
      tick.setAttribute('x', String(x));
      tick.setAttribute('y', String(pad.top + innerH + 18));
      tick.setAttribute('class', 'speed-tick');
      tick.setAttribute('text-anchor', 'middle');
      tick.textContent = hour + ':00';
      svg.appendChild(tick);
    }

    const maxLabel = document.createElementNS(svgNs, 'text');
    maxLabel.setAttribute('x', String(pad.left - 6));
    maxLabel.setAttribute('y', String(pad.top + 4));
    maxLabel.setAttribute('class', 'speed-tick');
    maxLabel.setAttribute('text-anchor', 'end');
    maxLabel.textContent = String(maxCount);
    svg.appendChild(maxLabel);

    const sorted = rows.slice().sort((a, b) => Number(a.minute) - Number(b.minute));
    const points = sorted.map((row) => xOf(Number(row.minute)) + ',' + yOf(Number(row.count) || 0));

    const area = document.createElementNS(svgNs, 'polyline');
    area.setAttribute('class', 'speed-line');
    area.setAttribute('points', points.join(' '));
    svg.appendChild(area);

    sorted.forEach((row) => {
      const dot = document.createElementNS(svgNs, 'circle');
      dot.setAttribute('cx', String(xOf(Number(row.minute))));
      dot.setAttribute('cy', String(yOf(Number(row.count) || 0)));
      dot.setAttribute('r', '2.2');
      dot.setAttribute('class', 'speed-dot');
      const hour = Math.floor(Number(row.minute) / 60);
      const minute = Number(row.minute) % 60;
      const title = document.createElementNS(svgNs, 'title');
      title.textContent = String(hour).padStart(2, '0') + ':' + String(minute).padStart(2, '0') +
        ' — ' + (Number(row.count) || 0) + ' 次/5分钟';
      dot.appendChild(title);
      svg.appendChild(dot);
    });

    container.appendChild(svg);
  },

  async loadTimeline(date) {
    this.stopTimeline();
    this.setText('timeline-status', '加载中...');
    if (!date) {
      this.timeline.events = [];
      this.setText('timeline-status', '请选择单日范围');
      return;
    }
    try {
      const events = await this.fetchJson('/api/timeline?date=' + encodeURIComponent(date) + '&limit=5000');
      this.timeline.events = Array.isArray(events) ? events : [];
      this.setText('timeline-status', this.timeline.events.length + ' 个事件，就绪');
      this.setProgress(0);
    } catch (error) {
      console.error('加载时序失败:', error);
      this.timeline.events = [];
      this.setText('timeline-status', '加载失败');
    }
  },

  playTimeline() {
    if (this.timeline.playing) return;
    const events = this.timeline.events;
    if (!events.length) {
      this.setText('timeline-status', '无可回放事件');
      return;
    }

    this.timeline.playing = true;
    this.setText('timeline-status', '回放中...');
    const speed = Number(this.elements['timeline-speed'] && this.elements['timeline-speed'].value) || 4;

    let accumulated = 0;
    let previousTs = Number(events[0].ts);
    events.forEach((event, index) => {
      const ts = Number(event.ts);
      const gapMs = Math.min(Math.max((ts - previousTs) * 1000, 0), 1500);
      previousTs = ts;
      accumulated += Math.max(gapMs, 45) / speed;

      const timer = setTimeout(() => {
        if (window.KeyboardHeatmap && KeyboardHeatmap.flashKey) {
          KeyboardHeatmap.flashKey(event.vk_code, 220 / speed);
        }
        this.setText('timeline-current', (event.key_name || '?') + '  (' + (index + 1) + '/' + events.length + ')');
        this.setProgress((index + 1) / events.length);
        if (index === events.length - 1) {
          this.timeline.playing = false;
          this.setText('timeline-status', '回放完成');
        }
      }, accumulated);
      this.timeline.timers.push(timer);
    });
  },

  stopTimeline() {
    this.timeline.timers.forEach((timer) => clearTimeout(timer));
    this.timeline.timers = [];
    this.timeline.playing = false;
    if (window.KeyboardHeatmap && KeyboardHeatmap.clearPlaybackHighlights) {
      KeyboardHeatmap.clearPlaybackHighlights();
    }
    this.setText('timeline-current', '—');
    if (this.timeline.events.length) {
      this.setText('timeline-status', this.timeline.events.length + ' 个事件，就绪');
    }
    this.setProgress(0);
  },

  setProgress(ratio) {
    const bar = this.elements['timeline-progress-bar'];
    if (bar) {
      bar.style.width = Math.max(0, Math.min(1, ratio)) * 100 + '%';
    }
  },

  // 实时监控：定时轮询并触发回调刷新当前范围数据。
  configureLive(getRange, onTick) {
    this.live.getRange = getRange;
    this.live.onTick = onTick;
  },

  toggleLive(enabled) {
    this.live.enabled = enabled;
    if (this.live.timer) {
      clearInterval(this.live.timer);
      this.live.timer = null;
    }
    if (!enabled) {
      this.setText('live-status', '已停止');
      return;
    }
    this.setText('live-status', '监控中（每 5 秒刷新）');
    this.live.timer = setInterval(() => {
      if (typeof this.live.onTick === 'function') {
        this.live.onTick();
      }
    }, 5000);
  },

  async runCompare() {
    const aStart = this.value('compare-a-start');
    const aEnd = this.value('compare-a-end');
    const bStart = this.value('compare-b-start');
    const bEnd = this.value('compare-b-end');

    if (!aStart || !aEnd || !bStart || !bEnd) {
      this.setText('compare-status', '请填写两个完整范围');
      return;
    }
    if (aStart > aEnd || bStart > bEnd) {
      this.setText('compare-status', '开始日期不能晚于结束日期');
      return;
    }

    this.setText('compare-status', '对比中...');
    try {
      const [regionA, regionB, handA, handB] = await Promise.all([
        this.fetchJson('/api/region-stats?start=' + aStart + '&end=' + aEnd),
        this.fetchJson('/api/region-stats?start=' + bStart + '&end=' + bEnd),
        this.fetchJson('/api/hand-stats?start=' + aStart + '&end=' + aEnd),
        this.fetchJson('/api/hand-stats?start=' + bStart + '&end=' + bEnd)
      ]);
      this.renderCompare(regionA, regionB, handA, handB);
      this.setText('compare-status', '对比完成');
    } catch (error) {
      console.error('数据对比失败:', error);
      this.setText('compare-status', '对比失败: ' + error.message);
    }
  },

  renderCompare(regionA, regionB, handA, handB) {
    const body = this.elements['compare-body'];
    if (!body) return;

    const sum = (rows) => rows.reduce((acc, row) => acc + (Number(row.count) || 0), 0);
    const pick = (rows, key, field) => {
      const found = rows.find((row) => row[field] === key);
      return found ? Number(found.count) || 0 : 0;
    };

    const metrics = [
      ['总击键', sum(regionA), sum(regionB)],
      ['字母', pick(regionA, 'letters', 'region'), pick(regionB, 'letters', 'region')],
      ['数字', pick(regionA, 'digits', 'region'), pick(regionB, 'digits', 'region')],
      ['修饰键', pick(regionA, 'modifiers', 'region'), pick(regionB, 'modifiers', 'region')],
      ['左手', pick(handA, 'left', 'hand'), pick(handB, 'left', 'hand')],
      ['右手', pick(handA, 'right', 'hand'), pick(handB, 'right', 'hand')]
    ];

    body.innerHTML = '';
    metrics.forEach(([label, a, b]) => {
      const tr = document.createElement('tr');
      const diff = b - a;
      const diffText = (diff > 0 ? '+' : '') + diff.toLocaleString();
      [label, a.toLocaleString(), b.toLocaleString(), diffText].forEach((text, index) => {
        const td = document.createElement('td');
        td.textContent = text;
        if (index === 3) {
          td.className = diff > 0 ? 'diff-up' : (diff < 0 ? 'diff-down' : '');
        }
        tr.appendChild(td);
      });
      body.appendChild(tr);
    });
  },

  value(id) {
    const element = this.elements[id];
    return element ? element.value : null;
  },

  setText(id, text) {
    const element = this.elements[id];
    if (element) {
      element.textContent = text;
    }
  }
};

window.Analysis = Analysis;
