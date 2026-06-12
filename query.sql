-- 查询每日敲击总数
SELECT date, COUNT(*) as total
FROM keys
GROUP BY date
ORDER BY date DESC;

-- 查询每小时热力分布
SELECT hour, COUNT(*) as total
FROM keys
GROUP BY hour
ORDER BY hour;

-- 查询某日每小时分布
SELECT hour, COUNT(*) as total
FROM keys
WHERE date = '2026-06-12'
GROUP BY hour
ORDER BY hour;

-- 查询最常按的键（Top 20）
SELECT key_name, COUNT(*) as count
FROM keys
GROUP BY key_name
ORDER BY count DESC
LIMIT 20;

-- 查询某个键的使用频率（按日期）
SELECT date, COUNT(*) as count
FROM keys
WHERE key_name = 'Enter'
GROUP BY date
ORDER BY date DESC;

-- 查询最近的按键记录（显示键名和时间）
SELECT datetime(timestamp, 'unixepoch', 'localtime') as time,
       key_name,
       vk_code
FROM keys
ORDER BY timestamp DESC
LIMIT 50;

-- 查询字母键使用统计
SELECT key_name, COUNT(*) as count
FROM keys
WHERE key_name GLOB '[A-Z]'
GROUP BY key_name
ORDER BY count DESC;

-- 查询功能键使用统计
SELECT key_name, COUNT(*) as count
FROM keys
WHERE key_name IN ('F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'F8', 'F9', 'F10', 'F11', 'F12')
GROUP BY key_name
ORDER BY count DESC;
