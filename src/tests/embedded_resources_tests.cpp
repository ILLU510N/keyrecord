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
        std::cerr << message << "\nMissing: " << expected << "\n";
        return false;
    }
    return true;
}

bool expectNotContains(const std::string& text, const char* unexpected, const char* message) {
    if (text.find(unexpected) != std::string::npos) {
        std::cerr << message << "\nUnexpected match: " << unexpected << "\n";
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

    ok = expect(keyrecord::embeddedResourceIndexIsSorted(), "Embedded resource index must be sorted by path") && ok;

    auto index = keyrecord::findEmbeddedResource("/");
    ok = expect(index.has_value(), "Root path should resolve to index.html") && ok;
    if (index) {
        ok = expect(index->path == "/index.html", "Root path should normalize to /index.html") && ok;
        ok = expect(index->contentType == "text/html; charset=utf-8", "index.html MIME type mismatch") && ok;
        ok = expect(resourceText(*index).find("KeyRecord") != std::string::npos, "index.html should contain KeyRecord") && ok;
        const auto indexText = resourceText(*index);
        ok = expectContains(indexText, "vendor/d3.v7.min.js", "index.html must reference the local d3 vendor asset") && ok;
        ok = expectContains(indexText, "vendor/heatmap.min.js", "index.html must reference the local heatmap vendor asset") && ok;
        ok = expectContains(indexText, "filter-start", "index.html must provide the start date control") && ok;
        ok = expectContains(indexText, "filter-end", "index.html must provide the end date control") && ok;
        ok = expectContains(indexText, "date-brush", "index.html must provide the D3 date brush container") && ok;
        ok = expectContains(indexText, "filter-last30", "index.html must provide the last-30-days shortcut") && ok;
        ok = expectContains(indexText, "theme-toggle", "index.html must provide the theme toggle control") && ok;
        ok = expectContains(indexText, "export-csv", "index.html must provide the CSV export control") && ok;
        ok = expectContains(indexText, "export-json", "index.html must provide the JSON export control") && ok;
        ok = expectContains(indexText, "export-png", "index.html must provide the PNG export control") && ok;
        ok = expectNotContains(indexText, "keyboard-zoom-in", "index.html must not provide keyboard zoom controls") && ok;
        ok = expectContains(
                 indexText,
                 "https://github.com/ILLU510N/keyrecord/issues",
                 "index.html must link help and feedback to GitHub Issues") &&
             ok;
        ok = expectContains(indexText, "target=\"_blank\"", "GitHub Issues must open in a new tab") && ok;
        ok = expectContains(indexText, "top-keys-body", "index.html must provide the Top 20 keys table") && ok;
        ok = expectNotContains(indexText, "https://d3js.org", "index.html must not reference the external d3 CDN") && ok;
        ok = expectNotContains(indexText, "https://cdn.jsdelivr.net", "index.html must not reference the external jsdelivr CDN") && ok;
    }

    auto script = keyrecord::findEmbeddedResource("/js/main.js?v=1");
    ok = expect(script.has_value(), "JS path with query string should resolve") && ok;
    if (script) {
        ok = expect(script->path == "/js/main.js", "JS path should strip the query string") && ok;
        ok = expect(script->contentType == "application/javascript; charset=utf-8", "main.js MIME type mismatch") && ok;
        const auto mainText = resourceText(*script);
        ok = expectContains(mainText, "loadTopKeys", "main.js must load the Top 20 keys ranking") && ok;
        ok = expectContains(mainText, "getDefaultDateRange", "main.js must compute the default last-30-days range") && ok;
        ok = expectContains(mainText, "applyPresetRange", "main.js must wire shortcut date ranges") && ok;
        ok = expectContains(mainText, "exportData", "main.js must export CSV and JSON data") && ok;
    }

    auto keyboardScript = keyrecord::findEmbeddedResource("/js/keyboard.js");
    ok = expect(keyboardScript.has_value(), "keyboard.js resource should resolve") && ok;
    if (keyboardScript) {
        const auto keyboardText = resourceText(*keyboardScript);
        ok = expectContains(keyboardText, "code: 116, label: 'F5'", "Keyboard layout should include F5") && ok;
        ok = expectContains(keyboardText, "code: 36, label: 'Home'", "Keyboard layout should include Home") && ok;
        ok = expectContains(keyboardText, "code: 39, label: 'Right'", "Keyboard layout should include Right") && ok;
        ok = expectContains(keyboardText, "code: 98, label: '2'", "Keyboard layout should include Numpad 2") && ok;
        ok = expectContains(keyboardText, "initFixedLayout", "Keyboard heatmap should use a fixed responsive layout") && ok;
        ok = expectNotContains(keyboardText, "d3.zoom", "Keyboard heatmap must not register interactive zoom") && ok;
        ok = expectNotContains(keyboardText, "h337.create", "Keyboard heatmap must not render circular canvas overlays") && ok;
        ok = expectContains(keyboardText, "--key-heat-start", "Keyboard heatmap should define a gradient start color") && ok;
        ok = expectContains(keyboardText, "--key-heat-end", "Keyboard heatmap should define a gradient end color") && ok;
        ok = expectContains(keyboardText, "createLinearGradient(key.x", "PNG export should use per-key linear gradients") && ok;
        ok = expectContains(keyboardText, "numpadGap: 30", "Numpad should be separated from navigation keys by 30 pixels") && ok;
        ok = expectContains(keyboardText, "width: 1430", "Standard keyboard width should include the numpad gap") && ok;
        ok = expectContains(keyboardText, "exportPng", "Keyboard heatmap should export PNG") && ok;
    }

    auto calendarScript = keyrecord::findEmbeddedResource("/js/calendar.js");
    ok = expect(calendarScript.has_value(), "calendar.js resource should resolve") && ok;
    if (calendarScript) {
        const auto calendarText = resourceText(*calendarScript);
        ok = expectContains(calendarText, "setTrendRange", "Calendar trend must have an independent range setter") && ok;
        ok = expectContains(calendarText, "getAnnualRange", "Daily calendar must compute a fixed annual window") && ok;
        ok = expectContains(calendarText, "renderCalendar", "Daily calendar must render independently from the trend") && ok;
    }

    auto stylesheet = keyrecord::findEmbeddedResource("css/style.css");
    ok = expect(stylesheet.has_value(), "CSS path without leading slash should resolve") && ok;
    if (stylesheet) {
        ok = expect(stylesheet->path == "/css/style.css", "CSS path should gain a leading slash") && ok;
        ok = expect(stylesheet->contentType == "text/css; charset=utf-8", "style.css MIME type mismatch") && ok;
        const auto stylesheetText = resourceText(*stylesheet);
        ok = expectContains(stylesheetText, "@media print", "style.css must include print optimization") && ok;
        ok = expectContains(stylesheetText, "body[data-theme=\"dark\"]", "style.css must include dark theme styles") && ok;
        ok = expectContains(stylesheetText, "linear-gradient(135deg", "Heated keycaps should use a 135-degree gradient") && ok;
        ok = expectContains(stylesheetText, "width: 1430px", "Keyboard stylesheet should include the expanded standard width") && ok;
    }

    auto d3Vendor = keyrecord::findEmbeddedResource("/vendor/d3.v7.min.js");
    ok = expect(d3Vendor.has_value(), "Local d3 vendor resource should resolve") && ok;
    if (d3Vendor) {
        ok = expect(d3Vendor->contentType == "application/javascript; charset=utf-8", "d3 vendor MIME type mismatch") && ok;
    }

    auto heatmapVendor = keyrecord::findEmbeddedResource("/vendor/heatmap.min.js");
    ok = expect(heatmapVendor.has_value(), "Local heatmap vendor resource should resolve") && ok;
    if (heatmapVendor) {
        ok = expect(
                 heatmapVendor->contentType == "application/javascript; charset=utf-8",
                 "heatmap vendor MIME type mismatch") &&
             ok;
    }

    ok = expect(!keyrecord::findEmbeddedResource("/../server.js").has_value(), "Path traversal request should not resolve a resource") && ok;
    ok = expect(keyrecord::normalizeResourcePath("/js/./main.js") == "/js/main.js", "Dot path segments should normalize") && ok;
    ok = expect(keyrecord::normalizeResourcePath("//js//main.js") == "/js/main.js", "Duplicate separators should normalize") && ok;
    ok = expect(keyrecord::normalizeResourcePath("/asset..name.js") == "/asset..name.js", "Double dots inside a filename should be allowed") && ok;
    ok = expect(keyrecord::contentTypeForPath("/asset.unknown") == "application/octet-stream", "Unknown extension MIME type mismatch") && ok;

    return ok ? 0 : 1;
}
