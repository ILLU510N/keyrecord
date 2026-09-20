#include "../visualization_url.h"

#include <iostream>
#include <string>

namespace {

bool expectEqual(const std::string& actual, const std::string& expected, const char* message) {
    if (actual == expected) {
        return true;
    }
    std::cerr << message << "\nExpected: " << expected << "\nActual: " << actual << "\n";
    return false;
}

} // namespace

int main() {
    bool ok = true;

    ok = expectEqual(
             keyrecord::buildVisualizationPageUrl({}),
             "http://127.0.0.1:3000/",
             "Default visualization URL mismatch") &&
         ok;

    keyrecord::ConfigFileValues ipv4Values;
    ipv4Values.address = "192.168.10.20";
    ipv4Values.port = static_cast<unsigned short>(8088);
    ok = expectEqual(
             keyrecord::buildVisualizationPageUrl(ipv4Values),
             "http://192.168.10.20:8088/",
             "Configured IPv4 visualization URL mismatch") &&
         ok;

    keyrecord::ConfigFileValues wildcardIpv4Values;
    wildcardIpv4Values.address = "0.0.0.0";
    wildcardIpv4Values.port = static_cast<unsigned short>(8080);
    ok = expectEqual(
             keyrecord::buildVisualizationPageUrl(wildcardIpv4Values),
             "http://127.0.0.1:8080/",
             "Wildcard IPv4 visualization URL mismatch") &&
         ok;

    keyrecord::ConfigFileValues wildcardIpv6Values;
    wildcardIpv6Values.address = "::";
    wildcardIpv6Values.port = static_cast<unsigned short>(8081);
    ok = expectEqual(
             keyrecord::buildVisualizationPageUrl(wildcardIpv6Values),
             "http://[::1]:8081/",
             "Wildcard IPv6 visualization URL mismatch") &&
         ok;

    keyrecord::ConfigFileValues ipv6Values;
    ipv6Values.address = "2001:db8::8";
    ipv6Values.port = static_cast<unsigned short>(8082);
    ok = expectEqual(
             keyrecord::buildVisualizationPageUrl(ipv6Values),
             "http://[2001:db8::8]:8082/",
             "Configured IPv6 visualization URL mismatch") &&
         ok;

    return ok ? 0 : 1;
}
