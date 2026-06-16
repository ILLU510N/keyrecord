const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '..');
const repoRoot = path.join(root, '..');

function read(relativePath) {
  return fs.readFileSync(path.join(root, relativePath), 'utf8');
}

function exists(relativePath) {
  return fs.existsSync(path.join(root, relativePath));
}

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

const html = read('public/index.html');
const css = read('public/css/style.css');
const todo = fs.readFileSync(path.join(repoRoot, 'todo.md'), 'utf8');

assert(html.includes('id="heatmap-section"'), 'index.html 缺少键盘热力图 section');
assert(html.includes('id="keyboard-heatmap"'), 'index.html 缺少 keyboard-heatmap 容器');
assert(html.includes('id="keyboard-status"'), 'index.html 缺少键盘热力图状态区域');
assert(html.includes('js/keyboard.js'), 'index.html 未引入 js/keyboard.js');
assert(
  html.indexOf('js/keyboard.js') < html.indexOf('js/main.js'),
  'keyboard.js 必须在 main.js 之前引入'
);

assert(exists('public/js/keyboard.js'), 'public/js/keyboard.js 不存在');
const keyboardJs = read('public/js/keyboard.js');
assert(keyboardJs.includes('h337.create'), 'keyboard.js 未初始化 heatmap.js');
assert(keyboardJs.includes('loadKeyboardHeatmap'), 'keyboard.js 缺少 loadKeyboardHeatmap 数据加载函数');
assert(keyboardJs.includes('/api/heatmap'), 'keyboard.js 未调用 /api/heatmap 接口');
assert(keyboardJs.includes('keyboard-key'), 'keyboard.js 未渲染键盘按键布局');
assert(keyboardJs.includes('keyboard-tooltip'), 'keyboard.js 缺少按键悬停提示');

assert(css.includes('#keyboard-heatmap'), 'style.css 缺少键盘热力图容器样式');
assert(css.includes('.keyboard-key'), 'style.css 缺少键盘按键样式');
assert(css.includes('.heatmap-legend'), 'style.css 缺少热力图图例样式');

assert(todo.includes('- [x] 初始化 heatmap.js 实例'), 'todo.md 未标记 heatmap.js 初始化完成');
assert(todo.includes('- [x] 实现数据加载和渲染'), 'todo.md 未标记键盘热力图数据加载完成');
assert(todo.includes('- [x] 添加鼠标悬停提示'), 'todo.md 未标记鼠标悬停提示完成');
assert(todo.includes('- [x] **M2**: 基础键盘热力图显示'), 'todo.md 未标记 M2 完成');

console.log('键盘热力图模块静态验证通过');
