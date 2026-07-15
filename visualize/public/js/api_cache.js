const ApiCache = {
  memory: new Map(),
  pending: new Map(),
  defaultTtlMs: 30000,
  storagePrefix: 'keyrecord-api-cache:',

  async fetchJson(url, options) {
    const ttlMs = options && Number(options.ttlMs) > 0 ? Number(options.ttlMs) : this.defaultTtlMs;
    const useStorage = !(options && options.storage === false);
    const cached = this.read(url, useStorage);
    if (cached) {
      return cached;
    }

    if (this.pending.has(url)) {
      return this.pending.get(url);
    }

    const request = fetch(url)
      .then((response) => {
        if (!response.ok) throw new Error('请求失败: ' + response.status);
        return response.json();
      })
      .then((data) => {
        this.write(url, data, ttlMs, useStorage);
        return data;
      })
      .finally(() => this.pending.delete(url));

    this.pending.set(url, request);
    return request;
  },

  read(url, useStorage) {
    const now = Date.now();
    const memoryEntry = this.memory.get(url);
    if (memoryEntry && memoryEntry.expiresAt > now) {
      return memoryEntry.data;
    }
    this.memory.delete(url);

    if (!useStorage) {
      return null;
    }

    try {
      const raw = localStorage.getItem(this.storagePrefix + url);
      if (!raw) return null;

      const entry = JSON.parse(raw);
      if (!entry || entry.expiresAt <= now) {
        localStorage.removeItem(this.storagePrefix + url);
        return null;
      }

      this.memory.set(url, entry);
      return entry.data;
    } catch (error) {
      console.warn('读取 API 缓存失败:', error);
      return null;
    }
  },

  write(url, data, ttlMs, useStorage) {
    const entry = { data, expiresAt: Date.now() + ttlMs };
    this.memory.set(url, entry);

    if (!useStorage) {
      return;
    }

    try {
      localStorage.setItem(this.storagePrefix + url, JSON.stringify(entry));
    } catch (error) {
      console.warn('保存 API 缓存失败:', error);
    }
  },

  invalidateApi() {
    this.memory.clear();

    try {
      const keys = [];
      for (let i = 0; i < localStorage.length; i += 1) {
        const key = localStorage.key(i);
        if (key && key.indexOf(this.storagePrefix) === 0) {
          keys.push(key);
        }
      }

      keys.forEach((key) => localStorage.removeItem(key));
    } catch (error) {
      console.warn('清理 API 缓存失败:', error);
    }
  },

  invalidateMatching(prefix) {
    Array.from(this.memory.keys()).forEach((key) => {
      if (key.indexOf(prefix) === 0) this.memory.delete(key);
    });

    try {
      const keys = [];
      for (let i = 0; i < localStorage.length; i += 1) {
        const key = localStorage.key(i);
        if (key && key.indexOf(this.storagePrefix + prefix) === 0) keys.push(key);
      }
      keys.forEach((key) => localStorage.removeItem(key));
    } catch (error) {
      console.warn('清理指定 API 缓存失败:', error);
    }
  }
};

window.ApiCache = ApiCache;
