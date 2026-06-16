const express = require('express');
const Database = require('better-sqlite3');
const cors = require('cors');
const path = require('path');
const { getKeyPosition } = require('./keyboard-layout');

const app = express();
const PORT = 3000;
const DB_PATH = path.join(__dirname, '..', 'keyrecord.db');

let db;
try {
  db = new Database(DB_PATH, { readonly: true });
  console.log('✓ 数据库连接成功:', DB_PATH);
} catch (err) {
  console.error('✗ 数据库连接失败:', err.message);
  process.exit(1);
}

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

app.get('/api/daily-stats', (req, res) => {
  try {
    const stmt = db.prepare('SELECT date, COUNT(*) as count FROM keys GROUP BY date ORDER BY date');
    const results = stmt.all();
    res.json(results);
  } catch (err) {
    console.error('Error in /api/daily-stats:', err);
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/heatmap', (req, res) => {
  try {
    const { date, start, end } = req.query;
    let stmt, params;

    if (start && end) {
      stmt = db.prepare('SELECT vk_code, key_name, COUNT(*) as count FROM keys WHERE date BETWEEN ? AND ? GROUP BY vk_code, key_name ORDER BY count DESC');
      params = [start, end];
    } else if (date) {
      stmt = db.prepare('SELECT vk_code, key_name, COUNT(*) as count FROM keys WHERE date = ? GROUP BY vk_code, key_name ORDER BY count DESC');
      params = [date];
    } else {
      stmt = db.prepare('SELECT vk_code, key_name, COUNT(*) as count FROM keys GROUP BY vk_code, key_name ORDER BY count DESC');
      params = [];
    }

    const results = stmt.all(...params);
    const heatmapData = results.map(row => {
      const position = getKeyPosition(row.vk_code);
      return position ? { vk_code: row.vk_code, key_name: row.key_name, count: row.count, x: position.x, y: position.y } : null;
    }).filter(item => item !== null);

    res.json(heatmapData);
  } catch (err) {
    console.error('Error in /api/heatmap:', err);
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/keys', (req, res) => {
  try {
    const { start, end, limit = 20 } = req.query;
    let stmt, params;

    if (start && end) {
      stmt = db.prepare('SELECT key_name, vk_code, COUNT(*) as count FROM keys WHERE date BETWEEN ? AND ? GROUP BY key_name, vk_code ORDER BY count DESC LIMIT ?');
      params = [start, end, parseInt(limit)];
    } else {
      stmt = db.prepare('SELECT key_name, vk_code, COUNT(*) as count FROM keys GROUP BY key_name, vk_code ORDER BY count DESC LIMIT ?');
      params = [parseInt(limit)];
    }

    res.json(stmt.all(...params));
  } catch (err) {
    console.error('Error in /api/keys:', err);
    res.status(500).json({ error: err.message });
  }
});

app.get('/api/info', (req, res) => {
  try {
    const totalKeys = db.prepare('SELECT COUNT(*) as count FROM keys').get();
    const dateRange = db.prepare('SELECT MIN(date) as first_date, MAX(date) as last_date FROM keys').get();
    const uniqueKeys = db.prepare('SELECT COUNT(DISTINCT vk_code) as count FROM keys').get();

    res.json({
      total_keys: totalKeys.count,
      first_date: dateRange.first_date,
      last_date: dateRange.last_date,
      unique_keys: uniqueKeys.count
    });
  } catch (err) {
    console.error('Error in /api/info:', err);
    res.status(500).json({ error: err.message });
  }
});

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(PORT, () => {
  console.log('\n🚀 KeyRecord 可视化服务已启动');
  console.log('📍 访问地址: http://localhost:' + PORT);
  console.log('📊 API 端点:');
  console.log('   - GET /api/daily-stats');
  console.log('   - GET /api/heatmap?date=YYYY-MM-DD');
  console.log('   - GET /api/keys?start=...&end=...&limit=N');
  console.log('   - GET /api/info\n');
});

process.on('SIGINT', () => {
  console.log('\n正在关闭服务器...');
  db.close();
  process.exit(0);
});
