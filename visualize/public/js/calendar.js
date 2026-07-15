const CalendarHeatmap = {
  container: null,
  brushContainer: null,
  summaryElement: null,
  data: [],
  trendRange: { start: null, end: null },
  cellSize: 14,
  cellPadding: 3,
  colors: ['#edf1f4', '#bce8b6', '#73cf76', '#36a952', '#167b38'],
  onDateSelected: null,
  onRangeSelected: null,
  brush: null,
  brushGroup: null,
  brushXScale: null,

  init() {
    this.container = document.getElementById('calendar-heatmap');
    this.brushContainer = document.getElementById('date-brush');
    this.summaryElement = document.getElementById('calendar-summary');
    if (!this.container || !this.brushContainer) {
      console.error('日期可视化容器未找到');
    }
  },

  async load() {
    try {
      const rows = await ApiCache.fetchJson('/api/daily-stats');
      this.data = Array.isArray(rows) ? rows.map((row) => ({
        date: row.date,
        count: Math.max(0, Number(row.count) || 0)
      })) : [];
      this.renderCalendar();
      if (this.trendRange.start && this.trendRange.end) this.renderTrend();
      return this.data;
    } catch (error) {
      console.error('加载每日统计失败:', error);
      this.data = [];
      this.showError('每日统计加载失败: ' + error.message);
      return this.data;
    }
  },

  setTrendRange(start, end) {
    this.trendRange = { start, end };
    this.renderTrend();
  },

  getRangeRows(start, end) {
    if (!start || !end) return [];
    const counts = new Map(this.data.map((row) => [row.date, row.count]));
    return DateUtils.eachDay(start, end).map((date) => {
      const value = DateUtils.format(date);
      return { date: value, count: counts.get(value) || 0 };
    });
  },

  getAnnualRange() {
    const endDate = DateUtils.todayLocal();
    const startDate = DateUtils.previousYearSameDay(endDate);
    return { start: DateUtils.format(startDate), end: DateUtils.format(endDate) };
  },

  renderCalendar() {
    if (!this.container) return;
    const { start, end } = this.getAnnualRange();

    const rows = this.getRangeRows(start, end);
    const dataMap = new Map(rows.map((row) => [row.date, row.count]));
    const rangeStart = DateUtils.parse(start);
    const rangeEnd = DateUtils.parse(end);
    const startDate = DateUtils.addDays(rangeStart, -rangeStart.getUTCDay());
    const endDate = DateUtils.addDays(rangeEnd, 6 - rangeEnd.getUTCDay());
    const allDates = [];
    for (let date = startDate; date <= endDate; date = DateUtils.addDays(date, 1)) {
      allDates.push(new Date(date.getTime()));
    }

    this.container.innerHTML = '';
    d3.selectAll('.calendar-tooltip').remove();
    const weeks = Math.floor((endDate - startDate) / 604800000) + 1;
    const width = Math.max(weeks * (this.cellSize + this.cellPadding) + 68, 260);
    const height = 7 * (this.cellSize + this.cellPadding) + 38;
    const maxCount = Math.max(...rows.map((row) => row.count), 1);
    const style = getComputedStyle(document.body);
    const panelColor = style.getPropertyValue('--panel').trim() || '#fff';
    const textColor = style.getPropertyValue('--text').trim() || '#172033';
    const getColor = (count) => count <= 0 ? this.colors[0] : this.colors[Math.min(4, Math.max(1, Math.ceil(count / maxCount * 4)))];

    const svg = d3.select(this.container).append('svg')
      .attr('width', width)
      .attr('height', height)
      .attr('data-window-start', start)
      .attr('data-window-end', end);
    const tooltip = d3.select('body').append('div').attr('class', 'tooltip calendar-tooltip').style('display', 'none');

    svg.selectAll('.day')
      .data(allDates)
      .enter()
      .append('rect')
      .attr('class', 'day')
      .attr('width', this.cellSize)
      .attr('height', this.cellSize)
      .attr('x', (date) => Math.floor((date - startDate) / 604800000) * (this.cellSize + this.cellPadding) + 46)
      .attr('y', (date) => date.getUTCDay() * (this.cellSize + this.cellPadding) + 25)
      .attr('fill', (date) => {
        const value = DateUtils.format(date);
        return value < start || value > end ? 'transparent' : getColor(dataMap.get(value) || 0);
      })
      .attr('data-date', (date) => DateUtils.format(date))
      .attr('stroke', panelColor)
      .attr('stroke-width', 1)
      .attr('rx', 2)
      .style('pointer-events', (date) => {
        const value = DateUtils.format(date);
        return value < start || value > end ? 'none' : 'auto';
      })
      .on('mouseover', (event, date) => {
        const value = DateUtils.format(date);
        const count = dataMap.get(value) || 0;
        tooltip.html('<strong>' + value + ' · ' + DateUtils.weekday(date) + '</strong><br>按键数：' + DateUtils.formatNumber(count))
          .style('display', 'block').style('left', (event.pageX + 10) + 'px').style('top', (event.pageY - 10) + 'px');
        d3.select(event.currentTarget).attr('stroke', textColor).attr('stroke-width', 2);
      })
      .on('mouseout', (event) => {
        tooltip.style('display', 'none');
        d3.select(event.currentTarget).attr('stroke', panelColor).attr('stroke-width', 1);
      })
      .on('click', (event, date) => {
        if (this.onDateSelected) this.onDateSelected(DateUtils.format(date));
      });

    svg.selectAll('.day-label').data(['日', '一', '二', '三', '四', '五', '六']).enter().append('text')
      .attr('class', 'day-label').attr('x', 33)
      .attr('y', (label, index) => index * (this.cellSize + this.cellPadding) + 25 + this.cellSize / 2)
      .attr('dy', '.35em').attr('text-anchor', 'end').text((label) => label);

    let lastMonth = '';
    allDates.forEach((date) => {
      const monthKey = date.getUTCFullYear() + '-' + String(date.getUTCMonth() + 1).padStart(2, '0');
      if (monthKey === lastMonth || date.getUTCDate() > 7) return;
      lastMonth = monthKey;
      const week = Math.floor((date - startDate) / 604800000);
      svg.append('text').attr('class', 'month-label')
        .attr('x', week * (this.cellSize + this.cellPadding) + 46).attr('y', 12).text(monthKey);
    });

    const total = rows.reduce((sum, row) => sum + row.count, 0);
    const active = rows.filter((row) => row.count > 0).length;
    this.setSummary('近一年 · ' + start + ' 至 ' + end + ' · ' + active + ' 个活跃日 · ' + DateUtils.formatNumber(total) + ' 次按键');
  },

  renderTrend() {
    if (!this.brushContainer) return;
    const { start, end } = this.trendRange;
    if (!start || !end) {
      this.brushContainer.innerHTML = '<p class="empty-cell">暂无可用日期范围</p>';
      this.setTrendSummary([]);
      return;
    }

    const rows = this.getRangeRows(start, end);
    this.brushContainer.innerHTML = '';
    d3.selectAll('.trend-tooltip').remove();
    const width = Math.max(this.brushContainer.clientWidth || 780, 560);
    const height = 180;
    const margin = { top: 13, right: 12, bottom: 30, left: 42 };
    const minDate = DateUtils.parse(start);
    const domainEnd = DateUtils.addDays(DateUtils.parse(end), 1);
    const maxCount = Math.max(...rows.map((row) => row.count), 1);
    this.brushXScale = d3.scaleUtc().domain([minDate, domainEnd]).range([margin.left, width - margin.right]);
    const yScale = d3.scaleLinear().domain([0, maxCount]).nice().range([height - margin.bottom, margin.top]);
    const svg = d3.select(this.brushContainer).append('svg').attr('width', width).attr('height', height).attr('viewBox', '0 0 ' + width + ' ' + height);

    svg.append('g').attr('class', 'trend-grid').attr('transform', 'translate(' + margin.left + ',0)')
      .call(d3.axisLeft(yScale).ticks(3).tickSize(-(width - margin.left - margin.right)).tickFormat(d3.format('~s')));
    svg.append('g').attr('class', 'axis').attr('transform', 'translate(0,' + (height - margin.bottom) + ')')
      .call(d3.axisBottom(this.brushXScale).ticks(Math.min(7, rows.length)).tickFormat(d3.utcFormat('%m-%d')));

    const tooltip = d3.select('body').append('div').attr('class', 'tooltip trend-tooltip').style('display', 'none');
    svg.selectAll('.trend-bar').data(rows).enter().append('rect').attr('class', 'trend-bar')
      .attr('x', (row) => this.brushXScale(DateUtils.parse(row.date)) + 1)
      .attr('y', (row) => yScale(row.count))
      .attr('width', (row) => Math.max(2, this.brushXScale(DateUtils.addDays(DateUtils.parse(row.date), 1)) - this.brushXScale(DateUtils.parse(row.date)) - 2))
      .attr('height', (row) => Math.max(row.count > 0 ? 2 : 0, height - margin.bottom - yScale(row.count)))
      .attr('rx', 2)
      .on('mouseover', (event, row) => tooltip.html('<strong>' + row.date + '</strong><br>按键数：' + DateUtils.formatNumber(row.count)).style('display', 'block').style('left', (event.pageX + 10) + 'px').style('top', (event.pageY - 10) + 'px'))
      .on('mouseout', () => tooltip.style('display', 'none'))
      .on('click', (event, row) => { if (this.onDateSelected) this.onDateSelected(row.date); });

    this.brush = d3.brushX().extent([[margin.left, margin.top], [width - margin.right, height - margin.bottom]])
      .on('end', (event) => {
        if (!event.sourceEvent || !event.selection || !this.onRangeSelected) return;
        const range = this.selectionToRange(event.selection);
        if (range) this.onRangeSelected(range);
      });
    this.brushGroup = svg.append('g').attr('class', 'brush').call(this.brush);
    this.setBrushRange(start, end);
    this.setTrendSummary(rows);
  },

  selectionToRange(selection) {
    if (!this.brushXScale) return null;
    const domain = this.brushXScale.domain();
    const minDate = d3.utcDay.floor(domain[0]);
    const maxDate = DateUtils.addDays(d3.utcDay.floor(domain[1]), -1);
    const startDate = DateUtils.clamp(d3.utcDay.floor(this.brushXScale.invert(selection[0])), minDate, maxDate);
    const endDate = DateUtils.clamp(d3.utcDay.floor(this.brushXScale.invert(selection[1] - 1)), minDate, maxDate);
    return { start: DateUtils.format(startDate), end: DateUtils.format(endDate < startDate ? startDate : endDate) };
  },

  setBrushRange(start, end) {
    if (!this.brush || !this.brushGroup || !this.brushXScale || !start || !end) return;
    const domain = this.brushXScale.domain();
    const minDate = d3.utcDay.floor(domain[0]);
    const maxDate = DateUtils.addDays(d3.utcDay.floor(domain[1]), -1);
    const clampedStart = DateUtils.clamp(d3.utcDay.floor(DateUtils.parse(start)), minDate, maxDate);
    const clampedEnd = DateUtils.clamp(d3.utcDay.floor(DateUtils.parse(end)), minDate, maxDate);
    this.brushGroup.call(this.brush.move, [this.brushXScale(clampedStart), this.brushXScale(DateUtils.addDays(clampedEnd, 1))]);
  },

  setTrendSummary(rows) {
    const total = rows.reduce((sum, row) => sum + row.count, 0);
    const peak = rows.reduce((max, row) => Math.max(max, row.count), 0);
    const totalElement = document.getElementById('trend-total');
    const peakElement = document.getElementById('trend-peak');
    if (totalElement) totalElement.textContent = DateUtils.formatNumber(total);
    if (peakElement) peakElement.textContent = DateUtils.formatNumber(peak);
  },

  setSummary(message) {
    if (this.summaryElement) this.summaryElement.textContent = message;
  },

  showError(message) {
    if (this.container) this.container.innerHTML = '<p class="empty-cell">' + message + '</p>';
    if (this.brushContainer) this.brushContainer.innerHTML = '<p class="empty-cell">' + message + '</p>';
  },

  setDateSelectHandler(handler) { this.onDateSelected = handler; },
  setRangeSelectHandler(handler) { this.onRangeSelected = handler; }
};

window.CalendarHeatmap = CalendarHeatmap;
