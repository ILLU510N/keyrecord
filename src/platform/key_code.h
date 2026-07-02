#pragma once

#include <cstdint>

namespace keyrecord {

// 平台无关的按键码类型。历史上采集端直接使用 Windows 的 DWORD 虚拟键码，
// 为支持跨平台采集后端，这里统一为固定宽度别名；各平台后端负责把原生键码
// 归一化为 Windows 虚拟键码数值（数据库与前端仍以该数值为契约）。
using KeyCode = std::uint32_t;

} // namespace keyrecord
