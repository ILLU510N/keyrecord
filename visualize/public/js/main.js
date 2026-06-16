const App = {
  async init() {
    console.log('🚀 KeyRecord 可视化应用启动中...');
    CalendarHeatmap.init();
    KeyboardHeatmap.init();
    const info = await this.loadDatabaseInfo();
    await this.loadKeyboardHeatmap(info);
    await CalendarHeatmap.load();
    CalendarHeatmap.setDateSelectHandler((date) => {
      this.updateKeyboardHeatmap(date, date);
    });
    console.log('✓ 应用初始化完成');
  },

  async loadDatabaseInfo() {
    try {
      const response = await fetch('/api/info');
      if (!response.ok) throw new Error('获取数据库信息失败');
      const info = await response.json();
      document.getElementById('total-keys').textContent = info.total_keys.toLocaleString();
      document.getElementById('data-range').textContent = info.first_date + ' ~ ' + info.last_date;
      console.log('✓ 数据库信息加载完成');
      return info;
    } catch (error) {
      console.error('加载数据库信息失败:', error);
      document.getElementById('total-keys').textContent = '错误';
      document.getElementById('data-range').textContent = '错误';
      return null;
    }
  },

  async loadKeyboardHeatmap(info) {
    if (info && info.first_date && info.last_date) {
      await KeyboardHeatmap.loadKeyboardHeatmap(info.first_date, info.last_date);
      return;
    }

    await KeyboardHeatmap.loadKeyboardHeatmap();
  },

  async updateKeyboardHeatmap(startDate, endDate) {
    await KeyboardHeatmap.loadKeyboardHeatmap(startDate, endDate);
  }
};

document.addEventListener('DOMContentLoaded', () => {
  App.init();
});

window.App = App;
