const CalendarHeatmap = {
  container: null,
  brushContainer: null,
  data: [],
  cellSize: 15,
  cellPadding: 3,
  colors: ['#ebedf0', '#9be9a8', '#40c463', '#30a14e', '#216e39'],
  onDateSelected: null,
  onRangeSelected: null,
  brush: null,
  brushGroup: null,
  brushXScale: null,

  init() {
    this.container = document.getElementById('calendar-heatmap');
    this.brushContainer = document.getElementById('date-brush');
    if (!this.container) {
      console.error('日历图容器未找到');
      return;
    }
    console.log('✓ 日历图初始化完成');
  },

  async load() {
    try {
      this.data = await ApiCache.fetchJson('/api/daily-stats');
      this.render();
      this.renderBrush();
      console.log('✓ 日历图数据加载完成: ' + this.data.length + ' 天');
    } catch (error) {
      console.error('加载日历图数据失败:', error);
      this.showError(error.message);
    }
  },

  render() {
    if (this.data.length === 0) {
      this.container.innerHTML = '<p style="text-align:center;color:#7f8c8d;">暂无数据</p>';
      return;
    }

    this.container.innerHTML = '';
    d3.selectAll('.calendar-tooltip').remove();

    const dataMap = new Map();
    this.data.forEach(d => dataMap.set(d.date, d.count));

    const dates = this.data.map(d => this.parseDateValue(d.date)).filter(Boolean);
    const minDate = new Date(Math.min(...dates));
    const maxDate = new Date(Math.max(...dates));

    const startDate = new Date(minDate);
    startDate.setDate(startDate.getDate() - startDate.getDay());

    const endDate = new Date(maxDate);
    endDate.setDate(endDate.getDate() + (6 - endDate.getDay()));

    const weeks = Math.ceil((endDate - startDate) / (7 * 24 * 60 * 60 * 1000));
    const width = weeks * (this.cellSize + this.cellPadding);
    const height = 7 * (this.cellSize + this.cellPadding) + 40;
    const style = getComputedStyle(document.body);
    const panelColor = style.getPropertyValue('--panel').trim() || '#fff';
    const textColor = style.getPropertyValue('--text').trim() || '#2c3e50';

    const svg = d3.select(this.container)
      .append('svg')
      .attr('width', width + 100)
      .attr('height', height);

    const tooltip = d3.select('body').append('div')
      .attr('class', 'tooltip calendar-tooltip')
      .style('display', 'none');

    const counts = this.data.map(d => d.count);
    const maxCount = Math.max(...counts);
    const thresholds = [0, maxCount * 0.2, maxCount * 0.4, maxCount * 0.6, maxCount * 0.8];

    const getColor = (count) => {
      if (!count || count === 0) return this.colors[0];
      for (let i = thresholds.length - 1; i >= 0; i--) {
        if (count >= thresholds[i]) return this.colors[i];
      }
      return this.colors[0];
    };

    const allDates = [];
    let currentDate = new Date(startDate);
    while (currentDate <= endDate) {
      allDates.push(new Date(currentDate));
      currentDate.setDate(currentDate.getDate() + 1);
    }

    svg.selectAll('.day')
      .data(allDates)
      .enter()
      .append('rect')
      .attr('class', 'day')
      .attr('width', this.cellSize)
      .attr('height', this.cellSize)
      .attr('x', (d) => {
        const week = Math.floor((d - startDate) / (7 * 24 * 60 * 60 * 1000));
        return week * (this.cellSize + this.cellPadding) + 50;
      })
      .attr('y', (d) => {
        const day = d.getDay();
        return day * (this.cellSize + this.cellPadding) + 30;
      })
      .attr('fill', (d) => {
        const dateStr = this.formatDateValue(d);
        const count = dataMap.get(dateStr) || 0;
        return getColor(count);
      })
      .attr('data-date', (d) => this.formatDateValue(d))
      .attr('stroke', panelColor)
      .attr('stroke-width', 1)
      .attr('rx', 2)
      .on('mouseover', (event, d) => {
        const dateStr = this.formatDateValue(d);
        const count = dataMap.get(dateStr) || 0;
        tooltip.html('<strong>' + dateStr + '</strong><br>按键数: ' + count.toLocaleString())
          .style('display', 'block')
          .style('left', (event.pageX + 10) + 'px')
          .style('top', (event.pageY - 10) + 'px');
        d3.select(event.currentTarget).attr('stroke', textColor).attr('stroke-width', 2);
      })
      .on('mouseout', (event) => {
        tooltip.style('display', 'none');
        d3.select(event.currentTarget).attr('stroke', panelColor).attr('stroke-width', 1);
      })
      .on('click', (event, d) => {
        if (!this.onDateSelected) return;

        const dateStr = this.formatDateValue(d);
        this.onDateSelected(dateStr);
      });

    const weekDays = ['日', '一', '二', '三', '四', '五', '六'];
    svg.selectAll('.day-label')
      .data(weekDays)
      .enter()
      .append('text')
      .attr('class', 'day-label')
      .attr('x', 35)
      .attr('y', (d, i) => i * (this.cellSize + this.cellPadding) + 30 + this.cellSize / 2)
      .attr('dy', '0.35em')
      .attr('text-anchor', 'end')
      .text(d => d);

    let lastMonth = -1;
    allDates.forEach((d) => {
      const month = d.getMonth();
      if (month !== lastMonth && d.getDate() <= 7) {
        const week = Math.floor((d - startDate) / (7 * 24 * 60 * 60 * 1000));
        const x = week * (this.cellSize + this.cellPadding) + 50;
        svg.append('text')
          .attr('class', 'month-label')
          .attr('x', x)
          .attr('y', 15)
          .attr('text-anchor', 'start')
          .text(d.getFullYear() + '-' + String(month + 1).padStart(2, '0'));
        lastMonth = month;
      }
    });
  },

  renderBrush() {
    if (!this.brushContainer) return;

    if (this.data.length === 0) {
      this.brushContainer.innerHTML = '<p style="text-align:center;color:#7f8c8d;">暂无可筛选日期</p>';
      return;
    }

    this.brushContainer.innerHTML = '';

    const containerWidth = this.brushContainer.clientWidth || 760;
    const width = Math.max(containerWidth, 480);
    const height = 92;
    const margin = { top: 10, right: 18, bottom: 28, left: 38 };
    const dates = this.data.map(d => this.parseDateValue(d.date)).filter(Boolean);
    const minDate = d3.utcDay.floor(new Date(Math.min(...dates)));
    const maxDate = d3.utcDay.floor(new Date(Math.max(...dates)));
    const domainEnd = this.addDays(maxDate, 1);
    const maxCount = Math.max(...this.data.map(d => Number(d.count) || 0), 1);

    this.brushXScale = d3.scaleUtc()
      .domain([minDate, domainEnd])
      .range([margin.left, width - margin.right]);

    const yScale = d3.scaleLinear()
      .domain([0, maxCount])
      .range([height - margin.bottom, margin.top]);

    const svg = d3.select(this.brushContainer)
      .append('svg')
      .attr('width', width)
      .attr('height', height)
      .attr('viewBox', '0 0 ' + width + ' ' + height);

    svg.append('rect')
      .attr('class', 'brush-track')
      .attr('x', margin.left)
      .attr('y', margin.top)
      .attr('width', width - margin.left - margin.right)
      .attr('height', height - margin.top - margin.bottom);

    svg.selectAll('.brush-bar')
      .data(this.data)
      .enter()
      .append('rect')
      .attr('class', 'brush-bar')
      .attr('x', d => this.brushXScale(this.parseDateValue(d.date)))
      .attr('y', d => yScale(Number(d.count) || 0))
      .attr('width', d => Math.max(2, this.brushXScale(this.addDays(this.parseDateValue(d.date), 1)) - this.brushXScale(this.parseDateValue(d.date)) - 1))
      .attr('height', d => Math.max(1, height - margin.bottom - yScale(Number(d.count) || 0)));

    svg.append('g')
      .attr('class', 'axis')
      .attr('transform', 'translate(0,' + (height - margin.bottom) + ')')
      .call(d3.axisBottom(this.brushXScale).ticks(6).tickFormat(d3.utcFormat('%m-%d')));

    this.brush = d3.brushX()
      .extent([[margin.left, margin.top], [width - margin.right, height - margin.bottom]])
      .on('end', (event) => {
        if (!event.sourceEvent || !event.selection || !this.onRangeSelected) {
          return;
        }

        const range = this.selectionToRange(event.selection);
        if (!range) return;
        this.onRangeSelected(range);
        this.setBrushRange(range.start, range.end);
      });

    this.brushGroup = svg.append('g')
      .attr('class', 'brush')
      .call(this.brush);
  },

  selectionToRange(selection) {
    if (!this.brushXScale) return null;

    const domain = this.brushXScale.domain();
    const minDate = d3.utcDay.floor(domain[0]);
    const maxDate = this.addDays(d3.utcDay.floor(domain[1]), -1);
    const startDate = this.clampDate(d3.utcDay.floor(this.brushXScale.invert(selection[0])), minDate, maxDate);
    const endDate = this.clampDate(d3.utcDay.floor(this.brushXScale.invert(selection[1] - 1)), minDate, maxDate);

    if (startDate > endDate) {
      return {
        start: this.formatDateValue(startDate),
        end: this.formatDateValue(startDate)
      };
    }

    return {
      start: this.formatDateValue(startDate),
      end: this.formatDateValue(endDate)
    };
  },

  setBrushRange(startDate, endDate) {
    if (!this.brush || !this.brushGroup || !this.brushXScale || !startDate || !endDate) {
      return;
    }

    const start = this.parseDateValue(startDate);
    const end = this.parseDateValue(endDate);
    if (!start || !end) {
      return;
    }

    const domain = this.brushXScale.domain();
    const minDate = d3.utcDay.floor(domain[0]);
    const maxDate = this.addDays(d3.utcDay.floor(domain[1]), -1);
    const clampedStart = this.clampDate(d3.utcDay.floor(start), minDate, maxDate);
    const clampedEnd = this.clampDate(d3.utcDay.floor(end), minDate, maxDate);
    const selection = [
      this.brushXScale(clampedStart),
      this.brushXScale(this.addDays(clampedEnd, 1))
    ];

    this.brushGroup.call(this.brush.move, selection);
  },

  clampDate(date, minDate, maxDate) {
    if (date < minDate) return new Date(minDate.getTime());
    if (date > maxDate) return new Date(maxDate.getTime());
    return new Date(date.getTime());
  },

  showError(message) {
    this.container.innerHTML = '<p style="text-align:center;color:#ff6b6b;">加载失败: ' + message + '</p>';
    if (this.brushContainer) {
      this.brushContainer.innerHTML = '<p style="text-align:center;color:#ff6b6b;">日期滑块加载失败: ' + message + '</p>';
    }
  },

  setDateSelectHandler(handler) {
    this.onDateSelected = handler;
  },

  setRangeSelectHandler(handler) {
    this.onRangeSelected = handler;
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
  }
};

window.CalendarHeatmap = CalendarHeatmap;
