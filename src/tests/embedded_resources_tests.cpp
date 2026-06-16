#include "../embedded_resources.h"

#include <iostream>
#include <string>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

bool expectContains(const std::string& text, const char* expected, const char* message) {
    if (text.find(expected) == std::string::npos) {
        std::cerr << message << "\n缺少: " << expected << "\n";
        return false;
    }
    return true;
}

bool expectNotContains(const std::string& text, const char* unexpected, const char* message) {
    if (text.find(unexpected) != std::string::npos) {
        std::cerr << message << "\n命中: " << unexpected << "\n";
        return false;
    }
    return true;
}

std::string resourceText(const keyrecord::EmbeddedResource& resource) {
    return std::string(reinterpret_cast<const char*>(resource.data.data()), resource.data.size());
}

} // namespace

int main() {
    bool ok = true;

    ok = expect(keyrecord::embeddedResourceIndexIsSorted(), "内嵌资源索引必须按路径排序") && ok;

    auto index = keyrecord::findEmbeddedResource("/");
    ok = expect(index.has_value(), "根路径未命中 index.html") && ok;
    if (index) {
        ok = expect(index->path == "/index.html", "根路径没有规范化到 /index.html") && ok;
        ok = expect(index->contentType == "text/html; charset=utf-8", "index.html MIME 类型错误") && ok;
        ok = expect(resourceText(*index).find("KeyRecord") != std::string::npos, "index.html 内容缺少 KeyRecord") && ok;
        const auto indexText = resourceText(*index);
        ok = expectContains(indexText, "vendor/d3.v7.min.js", "index.html 必须引用本地 d3 vendor 资源") && ok;
        ok = expectContains(indexText, "vendor/heatmap.min.js", "index.html 必须引用本地 heatmap vendor 资源") && ok;
        ok = expectNotContains(indexText, "https://d3js.org", "index.html 不应再引用外部 d3 CDN") && ok;
        ok = expectNotContains(indexText, "https://cdn.jsdelivr.net", "index.html 不应再引用外部 jsdelivr CDN") && ok;
    }

    auto script = keyrecord::findEmbeddedResource("/js/main.js?v=1");
    ok = expect(script.has_value(), "带 query 的 JS 路径未命中") && ok;
    if (script) {
        ok = expect(script->path == "/js/main.js", "JS 路径没有去掉 query") && ok;
        ok = expect(script->contentType == "application/javascript; charset=utf-8", "main.js MIME 类型错误") && ok;
    }

    auto stylesheet = keyrecord::findEmbeddedResource("css/style.css");
    ok = expect(stylesheet.has_value(), "无前导斜杠的 CSS 路径未命中") && ok;
    if (stylesheet) {
        ok = expect(stylesheet->path == "/css/style.css", "CSS 路径没有补齐前导斜杠") && ok;
        ok = expect(stylesheet->contentType == "text/css; charset=utf-8", "style.css MIME 类型错误") && ok;
    }

    auto d3Vendor = keyrecord::findEmbeddedResource("/vendor/d3.v7.min.js");
    ok = expect(d3Vendor.has_value(), "本地 d3 vendor 资源未命中") && ok;
    if (d3Vendor) {
        ok = expect(d3Vendor->contentType == "application/javascript; charset=utf-8", "d3 vendor MIME 类型错误") && ok;
    }

    auto heatmapVendor = keyrecord::findEmbeddedResource("/vendor/heatmap.min.js");
    ok = expect(heatmapVendor.has_value(), "本地 heatmap vendor 资源未命中") && ok;
    if (heatmapVendor) {
        ok = expect(
                 heatmapVendor->contentType == "application/javascript; charset=utf-8",
                 "heatmap vendor MIME 类型错误") &&
             ok;
    }

    ok = expect(!keyrecord::findEmbeddedResource("/../server.js").has_value(), "路径穿越请求不应命中资源") && ok;
    ok = expect(keyrecord::contentTypeForPath("/asset.unknown") == "application/octet-stream", "未知扩展名 MIME 类型错误") && ok;

    return ok ? 0 : 1;
}
