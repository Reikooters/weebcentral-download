//
// Created by reikooters on 1/11/25.
//

#include "HttpClient.h"

#include <cstring>
#include <curl/curl.h>
#include <fstream>

HttpClient::HttpClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
}

// Validate HTTP/HTTPS URI
bool HttpClient::is_valid_http_uri(const std::string &uri) {
    CURLU *url = curl_url();
    CURLUcode rc = curl_url_set(url, CURLUPART_URL, uri.c_str(), 0);

    if (rc == CURLUE_OK) {
        char *scheme = nullptr;
        if (curl_url_get(url, CURLUPART_SCHEME, &scheme, 0) == CURLUE_OK) {
            bool is_http = (strcmp(scheme, "http") == 0 || strcmp(scheme, "https") == 0);
            curl_free(scheme);
            curl_url_cleanup(url);
            return is_http;
        }
    }

    curl_url_cleanup(url);
    return false;
}

// Download HTML content to string
bool HttpClient::download_html(const std::string &url, std::string &out_html) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_html);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0"); // Set user agent

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}

// Download image to disk
bool HttpClient::download_image(const std::string &url, const std::string &output_path) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;

    FILE *fp = fopen(output_path.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr); // Use default
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}

// Callback for writing HTML to string
size_t HttpClient::write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    std::string *str = static_cast<std::string *>(userp);
    str->append(static_cast<char *>(contents), total_size);
    return total_size;
}
