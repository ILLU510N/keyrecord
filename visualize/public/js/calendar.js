const CalendarHeatmap = {
  container: null,
  data: [],
  cellSize: 15,
  cellPadding: 3,
  colors: ['#ebedf0', '#9be9a8', '#40c463', '#30a14e', '#216e39'],
  onDateSelected: null,

  init() {
    this.container = document.getElementById('calendar-heatmap');
    if (!this.container) {
      console.error('日历图容器未找到');
      return;
    }
    console.log('✓ 日历图初始化完成');
  },

  async load() {
    try {
      const response = await fetch('/api/daily-stats');
      if (!response.ok) throw new Error('API 请求失败: ' + response.status);
      this.data = await response.json();
      this.render();
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
    const dataMap = new Map();
    this.data.forEach(d => dataMap.set(d.date, d.count));

    const dates = this.data.map(d => new Date(d.date));
    const minDate = new Date(Math.min(...dates));
    const maxDate = new Date(Math.max(...dates));

    const startDate = new Date(minDate);
    startDate.setDate(startDate.getDate() - startDate.getDay());

    const endDate = new Date(maxDate);
    endDate.setDate(endDate.getDate() + (6 - endDate.getDay()));

    const weeks = Math.ceil((endDate - startDate) / (7 * 24 * 60 * 60 * 1000));
    const width = weeks * (this.cellSize + this.cellPadding);
    const height = 7 * (this.cellSize + this.cellPadding) + 40;

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
        const dateStr = d.toISOString().split('T')[0];
        const count = dataMap.get(dateStr) || 0;
        return getColor(count);
      })
      .attr('data-date', (d) => d.toISOString().split('T')[0])
      .attr('stroke', '#fff')
      .attr('stroke-width', 1)
      .attr('rx', 2)
      .on('mouseover', function(event, d) {
        const dateStr = d.toISOString().split('T')[0];
        const count = dataMap.get(dateStr) || 0;
        tooltip.html('<strong>' + dateStr + '</strong><br>按键数: ' + count.toLocaleString())
          .style('display', 'block')
          .style('left', (event.pageX + 10) + 'px')
          .style('top', (event.pageY - 10) + 'px');
        d3.select(this).attr('stroke', '#333').attr('stroke-width', 2);
      })
      .on('mouseout', function() {
        tooltip.style('display', 'none');
        d3.select(this).attr('stroke', '#fff').attr('stroke-width', 1);
      })
      .on('click', (event, d) => {
        if (!this.onDateSelected) return;

        const dateStr = d.toISOString().split('T')[0];
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

  showError(message) {
    this.container.innerHTML = '<p style="text-align:center;color:#ff6b6b;">加载失败: ' + message + '</p>';
  },

  setDateSelectHandler(handler) {
    this.onDateSelected = handler;
  }
};

window.CalendarHeatmap = CalendarHeatmap;
