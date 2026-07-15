const DateUtils = {
  parse(value) {
    if (!value || !/^\d{4}-\d{2}-\d{2}$/.test(value)) return null;
    const [year, month, day] = value.split('-').map(Number);
    const date = new Date(Date.UTC(year, month - 1, day));
    if (date.getUTCFullYear() !== year || date.getUTCMonth() !== month - 1 || date.getUTCDate() !== day) {
      return null;
    }
    return date;
  },

  format(date) {
    return date instanceof Date && !Number.isNaN(date.getTime()) ? date.toISOString().slice(0, 10) : '';
  },

  addDays(date, days) {
    const next = new Date(date.getTime());
    next.setUTCDate(next.getUTCDate() + days);
    return next;
  },

  todayLocal() {
    const now = new Date();
    return new Date(Date.UTC(now.getFullYear(), now.getMonth(), now.getDate()));
  },

  previousYearSameDay(date) {
    const year = date.getUTCFullYear() - 1;
    const month = date.getUTCMonth();
    const day = date.getUTCDate();
    const candidate = new Date(Date.UTC(year, month, day));
    if (candidate.getUTCMonth() === month) return candidate;
    return new Date(Date.UTC(year, month + 1, 0));
  },

  rangeDays(start, end) {
    const startDate = this.parse(start);
    const endDate = this.parse(end);
    if (!startDate || !endDate || startDate > endDate) return 0;
    return Math.floor((endDate - startDate) / 86400000) + 1;
  },

  eachDay(start, end) {
    const startDate = this.parse(start);
    const endDate = this.parse(end);
    if (!startDate || !endDate || startDate > endDate) return [];
    const days = [];
    for (let date = startDate; date <= endDate; date = this.addDays(date, 1)) {
      days.push(new Date(date.getTime()));
    }
    return days;
  },

  clamp(date, minDate, maxDate) {
    if (date < minDate) return new Date(minDate.getTime());
    if (date > maxDate) return new Date(maxDate.getTime());
    return new Date(date.getTime());
  },

  formatNumber(value) {
    return Math.max(0, Number(value) || 0).toLocaleString();
  },

  weekday(date) {
    return ['周日', '周一', '周二', '周三', '周四', '周五', '周六'][date.getUTCDay()];
  }
};

window.DateUtils = DateUtils;
