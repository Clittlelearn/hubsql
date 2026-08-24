#include "fetcher/http_client.h"

#include <curl/curl.h>

#include <stdexcept>
#include <thread>

#include "common/logger.h"

namespace hubsql {

namespace {

// libcurl 写回调：把响应内容追加到 std::string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

}  // namespace

HttpClient::HttpClient(const HttpConfig& cfg) : cfg_(cfg) {}

std::string HttpClient::Get(const std::string& url) {
    return Request(url, "");
}

std::string HttpClient::Post(const std::string& url, const std::string& body) {
    return Request(url, body);
}

std::string HttpClient::Request(const std::string& url, const std::string& body) {
    long wait_ms = 1000;
    int attempt = 0;

    while (true) {
        ++attempt;
        std::string response;
        CURL* curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("curl_easy_init 失败");
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, cfg_.timeout_ms);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        if (!body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            curl_easy_cleanup(curl);

            bool ok = (res == CURLE_OK) && (http_code == 200);
            if (ok) return response;

            if (attempt > cfg_.retry) {
                throw std::runtime_error(
                    "HTTP POST 失败 (url=" + url + ", curl=" +
                    curl_easy_strerror(res) + ", http=" + std::to_string(http_code) + ")");
            }
            LOG_WARN("HTTP POST 失败，{}ms 后重试 ({}/{}): {}", wait_ms, attempt,
                     cfg_.retry, url);
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
            wait_ms = std::min<long>(wait_ms * 2, cfg_.max_retry_wait_ms);
            continue;
        }

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);

        bool ok = (res == CURLE_OK) && (http_code == 200);
        if (ok) {
            return response;
        }

        if (attempt > cfg_.retry) {
            throw std::runtime_error(
                "HTTP 请求失败 (url=" + url + ", curl=" +
                curl_easy_strerror(res) + ", http=" + std::to_string(http_code) + ")");
        }

        LOG_WARN("HTTP 请求失败，{}ms 后重试 ({}/{}): {}",
                 wait_ms, attempt, cfg_.retry, url);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        wait_ms = std::min<long>(wait_ms * 2, cfg_.max_retry_wait_ms);
    }
}

}  // namespace hubsql
