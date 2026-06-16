const App = {
  async init() {
    console.log('🚀 KeyRecord 可视化应用启动中...');
    CalendarHeatmap.init();
    await this.loadDatabaseInfo();
    await CalendarHeatmap.load();
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
    } catch (error) {
      console.error('加载数据库信息失败:', error);
      document.getElementById('total-keys').textContent = '错误';
      document.getElementById('data-range').textContent = '错误';
    }
  }
};

document.addEventListener('DOMContentLoaded', () => {
  App.init();
});

window.App = App;
