/**
 * KeyRecord - 人文关怀与健康防护模块 (HealthCare)
 * 提供防腱鞘炎拉伸指南、饮水与作息提醒、上下文感知推荐及状态持久化。
 */
const HealthCare = {
  tips: [
    {
      id: 'tendon-posture',
      category: '手部健康',
      icon: '🖐️',
      message: '长时间打字时请尽量保持手腕平直，避免悬空或过度向下压迫腕管神经，远离腱鞘炎。',
      priority: 1
    },
    {
      id: 'tendon-high-freq',
      category: '手腕防护',
      icon: '⚡',
      message: '今日按键频率较高，手指小肌肉群处于持续拉紧状态，不妨暂停片刻做一组手部伸展。',
      condition: (ctx) => ((ctx.todayKeys !== undefined ? ctx.todayKeys : ctx.totalKeys) || 0) > 10000,
      priority: 3
    },
    {
      id: 'tendon-unilateral',
      category: '姿态关怀',
      icon: '⚖️',
      message: '若单手击键频率显著偏高，打字时可适当调整坐姿与键盘角度，减轻单侧腕关节代偿。',
      priority: 1
    },
    {
      id: 'tendon-shortcut',
      category: '按键习惯',
      icon: '⌨️',
      message: '频繁使用小指按压 Ctrl/Shift 易引发手掌尺侧疲劳，可尝试用掌根辅助或适当更换击键习惯。',
      priority: 1
    },
    {
      id: 'water-routine',
      category: '补水提醒',
      icon: '💧',
      message: '敲代码虽投入，也别忘了喝杯温水。起身接水也是一次天然的身体放松机会。',
      priority: 1
    },
    {
      id: 'water-metabolism',
      category: '代谢激活',
      icon: '🍵',
      message: '每隔 1 小时补充 150~200ml 水分，能有效维持大脑清醒度，并给久坐的椎间盘减压。',
      priority: 1
    },
    {
      id: 'water-rhythm',
      category: '饮水习惯',
      icon: '🚰',
      message: '保持小口多次饮水节奏，不仅滋润咽喉，更促进血液循环，带走腕指乳酸堆积。',
      priority: 1
    },
    {
      id: 'rest-202020',
      category: '适度休息',
      icon: '👀',
      message: '试试 20-20-20 护眼法则：每隔 20 分钟，眺望 20 英尺（约 6 米）外的远方 20 秒。',
      priority: 1
    },
    {
      id: 'rest-breathing',
      category: '呼吸舒展',
      icon: '🫁',
      message: '放下双手，做两次深长的腹式呼吸，向后转动双肩，释放肩颈紧张与敲击疲劳。',
      priority: 1
    },
    {
      id: 'rest-standing',
      category: '久坐舒缓',
      icon: '🚶',
      message: '久坐已超 45 分钟，不妨站起身轻微踱步、活动髋部与膝盖，唤醒下肢血流动力。',
      priority: 1
    },
    {
      id: 'night-sleep',
      category: '晚间关怀',
      icon: '🌙',
      message: '夜深了，今日的灵感已记录完备。早点休息，优质睡眠能让神经突触得到最好的修复。',
      condition: (ctx) => {
        const hour = ctx.hour !== undefined ? ctx.hour : new Date().getHours();
        return hour >= 23 || hour < 5;
      },
      priority: 4
    },
    {
      id: 'warmth-motto',
      category: '温暖寄语',
      icon: '✨',
      message: '每一记敲击都在创造价值，但手部与身心健康才是一切长久创造的基石。',
      priority: 1
    }
  ],

  storageKeys: {
    COLLAPSED: 'keyrecord_care_collapsed',
    INDEX: 'keyrecord_care_last_index'
  },

  elements: {},
  currentIndex: 0,
  isCollapsed: false,
  isGuideOpen: false,
  isAnimating: false,
  hasUserInteracted: false,
  currentContext: { totalKeys: 0, hour: new Date().getHours() },

  init() {
    this.initElements();
    if (!this.elements.banner) return;

    this.restoreState();
    this.bindEvents();

    // 评估初次上下文（时间维度）
    this.evaluateContext({ hour: new Date().getHours() }, false);
    this.renderCurrentTip(false);
  },

  initElements() {
    const ids = [
      'health-care-banner',
      'care-icon',
      'care-category',
      'care-message',
      'care-refresh',
      'care-guide-toggle',
      'care-guide-panel',
      'care-collapse-toggle'
    ];
    ids.forEach((id) => {
      this.elements[id] = document.getElementById(id);
    });
    this.elements.banner = this.elements['health-care-banner'];
    this.elements.icon = this.elements['care-icon'];
    this.elements.category = this.elements['care-category'];
    this.elements.message = this.elements['care-message'];
    this.elements.refresh = this.elements['care-refresh'];
    this.elements.guideToggle = this.elements['care-guide-toggle'];
    this.elements.guidePanel = this.elements['care-guide-panel'];
    this.elements.collapseToggle = this.elements['care-collapse-toggle'];
  },

  restoreState() {
    try {
      const storedCollapsed = localStorage.getItem(this.storageKeys.COLLAPSED);
      if (storedCollapsed !== null) {
        this.isCollapsed = storedCollapsed === 'true';
      }

      const storedIndex = localStorage.getItem(this.storageKeys.INDEX);
      if (storedIndex !== null) {
        const parsed = parseInt(storedIndex, 10);
        if (!isNaN(parsed) && parsed >= 0 && parsed < this.tips.length) {
          this.currentIndex = parsed;
        }
      }
    } catch (err) {
      console.warn('读取健康关怀本地配置失败:', err);
    }

    this.applyCollapsedState(false);
  },

  bindEvents() {
    const refresh = this.elements.refresh || this.elements['care-refresh'];
    const guideToggle = this.elements.guideToggle || this.elements['care-guide-toggle'];
    const collapseToggle = this.elements.collapseToggle || this.elements['care-collapse-toggle'];
    const banner = this.elements.banner || this.elements['health-care-banner'];

    if (refresh) {
      refresh.addEventListener('click', (e) => {
        e.stopPropagation();
        this.hasUserInteracted = true;
        refresh.classList.remove('care-btn--spinning');
        void refresh.offsetWidth;
        refresh.classList.add('care-btn--spinning');
        this.nextTip(true);
      });
      refresh.addEventListener('animationend', () => {
        refresh.classList.remove('care-btn--spinning');
      });
    }

    if (guideToggle) {
      guideToggle.addEventListener('click', (e) => {
        e.stopPropagation();
        this.toggleGuide();
      });
    }

    if (collapseToggle) {
      collapseToggle.addEventListener('click', (e) => {
        e.stopPropagation();
        this.toggleCollapse();
      });
    }

    if (banner) {
      banner.addEventListener('click', (e) => {
        // 如果处于折叠状态，点击横幅任何空白处均可快速展开
        if (this.isCollapsed && !e.target.closest('button')) {
          this.toggleCollapse(false);
        }
      });
    }
  },

  renderCurrentTip(animate = true) {
    const tip = this.tips[this.currentIndex] || this.tips[0];
    const icon = this.elements.icon || this.elements['care-icon'];
    const category = this.elements.category || this.elements['care-category'];
    const message = this.elements.message || this.elements['care-message'];

    if (!message) return;

    if (!animate) {
      if (icon) icon.textContent = tip.icon;
      if (category) category.textContent = tip.category;
      message.textContent = tip.message;
      return;
    }

    if (this.fadeTimer) {
      window.clearTimeout(this.fadeTimer);
      this.fadeTimer = null;
    }

    this.isAnimating = true;
    message.classList.add('is-fading');
    if (category) category.classList.add('is-fading');
    if (icon) icon.classList.add('is-fading');

    this.fadeTimer = window.setTimeout(() => {
      if (icon) icon.textContent = tip.icon;
      if (category) category.textContent = tip.category;
      message.textContent = tip.message;

      message.classList.remove('is-fading');
      if (category) category.classList.remove('is-fading');
      if (icon) icon.classList.remove('is-fading');
      this.isAnimating = false;
      this.fadeTimer = null;
    }, 200);

    try {
      localStorage.setItem(this.storageKeys.INDEX, String(this.currentIndex));
    } catch (err) {
      console.warn('保存健康关怀文案索引失败:', err);
    }
  },

  nextTip(animate = true) {
    this.currentIndex = (this.currentIndex + 1) % this.tips.length;
    this.renderCurrentTip(animate);
  },

  toggleGuide(force) {
    this.isGuideOpen = force !== undefined ? Boolean(force) : !this.isGuideOpen;
    const panel = this.elements.guidePanel || this.elements['care-guide-panel'];
    const toggleBtn = this.elements.guideToggle || this.elements['care-guide-toggle'];

    if (panel) {
      panel.classList.toggle('is-open', this.isGuideOpen);
      panel.setAttribute('aria-hidden', String(!this.isGuideOpen));
    }

    if (toggleBtn) {
      toggleBtn.classList.toggle('is-active', this.isGuideOpen);
      toggleBtn.setAttribute('aria-expanded', String(this.isGuideOpen));
    }
  },

  toggleCollapse(force) {
    this.isCollapsed = force !== undefined ? Boolean(force) : !this.isCollapsed;
    this.applyCollapsedState(true);

    try {
      localStorage.setItem(this.storageKeys.COLLAPSED, String(this.isCollapsed));
    } catch (err) {
      console.warn('保存健康关怀折叠状态失败:', err);
    }
  },

  applyCollapsedState(closeGuideIfCollapsing = true) {
    const banner = this.elements.banner || this.elements['health-care-banner'];
    const collapseToggle = this.elements.collapseToggle || this.elements['care-collapse-toggle'];

    if (banner) {
      banner.classList.toggle('is-collapsed', this.isCollapsed);
    }

    if (collapseToggle) {
      collapseToggle.setAttribute('aria-expanded', String(!this.isCollapsed));
      const label = this.isCollapsed ? '展开关怀提示' : '收起关怀提示';
      collapseToggle.setAttribute('aria-label', label);
      collapseToggle.title = label;
    }

    if (this.isCollapsed && closeGuideIfCollapsing) {
      this.toggleGuide(false);
    }
  },

  evaluateContext(data = {}, updateView = true) {
    this.currentContext = {
      ...this.currentContext,
      ...data,
      hour: data.hour !== undefined ? data.hour : new Date().getHours()
    };

    // 如果用户本次会话中刚刚主动点击过“换一条”，则不强行覆盖用户正在看的文案
    if (this.hasUserInteracted) return;

    // 寻找满足条件且优先级最高的文案
    let matchedTipIndex = -1;
    let highestPriority = 1;

    for (let i = 0; i < this.tips.length; i++) {
      const tip = this.tips[i];
      if (typeof tip.condition === 'function' && tip.condition(this.currentContext)) {
        if (tip.priority > highestPriority) {
          highestPriority = tip.priority;
          matchedTipIndex = i;
        }
      }
    }

    if (matchedTipIndex !== -1 && matchedTipIndex !== this.currentIndex) {
      this.currentIndex = matchedTipIndex;
      if (updateView) {
        this.renderCurrentTip(true);
      }
    }
  }
};

window.HealthCare = HealthCare;
