//
// Created by reikooters on 1/11/25.
//

#ifndef WEEBCENTRAL_DOWNLOAD_HTTPCLIENT_H
#define WEEBCENTRAL_DOWNLOAD_HTTPCLIENT_H

#include <string>

class HttpClient {
public:
    HttpClient();

    ~HttpClient();

    /**
     * Checks if the given URI is a valid HTTP URI.
     *
     * This method verifies whether the provided URI follows the standard format
     * for HTTP URIs. It ensures the URI starts with "http" or "https" and meets
     * other necessary conditions for a valid HTTP URI.
     *
     * @param uri The URI to validate.
     * @return Returns true if the URI is valid and corresponds to an HTTP/HTTPS URI; otherwise, false.
     */
    bool is_valid_http_uri(const std::string &uri);

    /**
     * Downloads the HTML content from the specified URL.
     *
     * This method retrieves the HTML content from the specified URL and stores it
     * in the provided output string. It performs a network request and ensures the
     * content is fetched correctly if the given URL is valid and accessible.
     *
     * @param url The URL from which to download the HTML content.
     * @param out_html A reference to a string where the downloaded HTML content will be stored.
     * @return Returns true if the HTML content is successfully downloaded; otherwise, false.
     */
    bool download_html(const std::string &url, std::string &out_html);

    /**
     * Downloads an image from the specified URL and saves it to the specified output path.
     *
     * This method performs a network request to fetch an image from the given URL
     * and saves the downloaded image data to the provided file path. It ensures
     * the operation is successful if the URL is valid and accessible, and the output path
     * is writable.
     *
     * @param url The URL from which to download the image.
     * @param output_path The file path where the downloaded image will be saved.
     * @return Returns true if the image is successfully downloaded and saved; otherwise, false.
     */
    bool download_image(const std::string &url, const std::string &output_path);

private:
    // Callback for writing HTML to string
    static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
};

/*
// Usage example
int main() {
    HttpClient client;

    std::string url = "https://example.com";

    // 1. Validate URI
    if (!client.is_valid_http_uri(url)) {
        std::cerr << "Invalid URI" << std::endl;
        return 1;
    }

    // 2. Download HTML
    std::string html_content;
    if (!client.download_html(url, html_content)) {
        std::cerr << "Failed to download HTML" << std::endl;
        return 1;
    }

    std::cout << "Downloaded " << html_content.size() << " bytes" << std::endl;

    // 3. Parse with lexbor (you'll do this)
    // ... parse html_content with lexbor to extract image URLs ...

    // 4. Download images
    std::string image_url = "https://example.com/image.jpg";
    if (client.download_image(image_url, "downloaded_image.jpg")) {
        std::cout << "Image saved!" << std::endl;
    }

    return 0;
}
*/

#endif //WEEBCENTRAL_DOWNLOAD_HTTPCLIENT_H
