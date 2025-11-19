//
// Created by reikooters on 1/11/25.
//

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <chrono>
#include <ranges>
#include <thread>

#include "HttpClient.h"
#include "Utils.h"
#include "models/Chapter.h"
#include "lexbor/html/interfaces/document.h"

std::string getMangaTitle(HttpClient &http_client, const std::string &manga_uri);

bool createMangaDirectory(const std::string &manga_title, std::filesystem::path &manga_folder);

std::vector<Chapter> getChapters(HttpClient &http_client, const std::string &series_id);

std::vector<std::string> getChapterImageURIs(HttpClient &http_client, const std::string &chapter_uri);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <manga_uri>" << std::endl;
        std::cerr << "Example: " << argv[0] << " https://weebcentral.com/series/01J76XYFCDK6Y8GY447DTTTZ2F" <<
                std::endl;
        return 1;
    }

    std::string arg = argv[1];

    // Convert to lowercase for case-insensitive comparison
    std::string arg_lower = arg;
    std::ranges::transform(arg_lower, arg_lower.begin(),
                          [](unsigned char c) { return std::tolower(c); });

    // Check for version flag
    if (arg_lower == "-v" || arg_lower == "--version") {
        std::cout << "weebcentral-download version 0.1" << std::endl;
        return 0;
    }

    const std::string& manga_uri = arg;

    HttpClient http_client;

    // Validate URI
    if (!http_client.is_valid_http_uri(manga_uri)) {
        std::cerr << "Invalid Manga URI: " << manga_uri << std::endl;
        return 1;
    }

    std::cout << "Manga URI: " << manga_uri << std::endl;

    // Extract Series ID from URI
    std::string series_id = Utils::extractSeriesId(manga_uri);

    if (series_id.empty()) {
        std::cerr << "Error: Could not extract series ID from URI" << std::endl;
        return 1;
    }

    std::cout << "Series ID: " << series_id << std::endl;

    // Look up manga title
    std::cout << "Looking up manga title..." << std::endl;
    std::string manga_title = getMangaTitle(http_client, manga_uri);

    if (manga_title.empty()) {
        std::cerr << "Error: Could not look up manga title" << std::endl;
        return 1;
    }

    std::cout << "Manga title: " << manga_title << std::endl;

    // Create directory using the manga's title
    std::filesystem::path manga_folder;
    bool folderSuccess = createMangaDirectory(manga_title, manga_folder);

    if (!folderSuccess) {
        return 1;
    }

    std::cout << "Created manga folder: " << manga_folder << std::endl;

    // Get chapters
    std::vector<Chapter> chapters = getChapters(http_client, series_id);

    if (chapters.empty()) {
        std::cerr << "Error: Could not get chapters" << std::endl;
        return 1;
    }

    const std::size_t chapters_count = chapters.size();

    std::cout << "\nFound " << chapters_count << " chapters:" << std::endl;

    for (size_t i = 0; i < chapters_count; ++i) {
        const auto& chapter = chapters[i];
        std::cout << "  [" << (i + 1) << "/" << chapters_count << "] " << chapter.name << " -> " << chapter.url << std::endl;

        std::filesystem::path chapter_folder = manga_folder / Utils::sanitizeFolderName(chapter.name);

        try {
            if (!std::filesystem::create_directories(chapter_folder)) {
                std::cout << "    Chapter folder " << chapter_folder << " exists, skipping." << std::endl;
                continue;
            }
        } catch (const std::filesystem::filesystem_error &e) {
            std::cerr << "Error: Could not create folder: " << chapter_folder << std::endl << e.what() << std::endl;
            return 1;
        }

        std::cout << "    Created chapter folder: " << chapter_folder << std::endl;

        std::vector<std::string> image_uris = getChapterImageURIs(http_client, chapter.url);

        if (image_uris.empty()) {
            std::cerr << "    Error: Could not get image URIs for chapter: " << chapter.name << std::endl;
            return 1;
        }

        const std::size_t image_uris_count = image_uris.size();

        for (size_t j = 0; j < image_uris_count; ++j) {
            const std::string &image_uri = image_uris[j];
            std::string image_filename = std::filesystem::path(image_uri).filename().string();

            size_t queryPos = image_filename.find('?');
            size_t fragmentPos = image_filename.find('#');
            size_t endPos = std::min(queryPos, fragmentPos);

            image_filename = image_filename.substr(0, endPos);

            std::filesystem::path image_path = chapter_folder / Utils::sanitizeFolderName(image_filename);
            std::string image_path_string = image_path.string();

            http_client.download_image(image_uri, image_path_string);

            std::cout << "    [" << (j + 1) << "/" << image_uris_count << "] " << image_uri << " -> " << image_path_string << std::endl;
        }

        if (i < chapters_count - 1) {
            // Sleep for 1500 milliseconds
            std::cout << "    Sleeping for 4 seconds before downloading the next chapter" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(4000));
        }
    }

    std::cout << "\nDownload completed." << std::endl;

    return 0;
}

std::string getMangaTitle(HttpClient &http_client, const std::string &manga_uri) {
    // Download HTML
    std::string html_content;
    if (!http_client.download_html(manga_uri, html_content)) {
        std::cerr << "Failed to download HTML" << std::endl;
        return {};
    }

    // std::cout << "Downloaded " << html_content.size() << " bytes" << std::endl;

    // Parse HTML with lexbor
    lxb_html_document_t *document = lxb_html_document_create();
    lxb_status_t status = lxb_html_document_parse(document,
                                                  (const lxb_char_t *) html_content.c_str(),
                                                  html_content.length());

    if (status != LXB_STATUS_OK) {
        std::cerr << "Error: Failed to parse HTML" << std::endl;

        // Clean up document
        lxb_html_document_destroy(document);

        return {};
    }

    // Parse manga title
    std::string manga_title = Utils::parseMangaTitle(document);

    // Clean up document
    lxb_html_document_destroy(document);

    return manga_title;
}

bool createMangaDirectory(const std::string &manga_title, std::filesystem::path &manga_folder) {
    // Build sanitized string for creating a folder with the manga's name
    std::string folder_name = Utils::sanitizeFolderName(manga_title);

    if (folder_name.empty()) {
        std::cerr << "Error: Could not create sanitize folder name from manga title" << std::endl;
        return false;
    }

    std::cout << "Sanitized folder name: " << folder_name << std::endl;

    // Create the manga name directory
    manga_folder = std::filesystem::path(folder_name);

    try {
        std::filesystem::create_directories(manga_folder);
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Error: Could not create folder: " << manga_folder << std::endl << e.what() << std::endl;
        return false;
    }

    return true;
}

std::vector<Chapter> getChapters(HttpClient &http_client, const std::string &series_id) {
    // Build full chapter list URL
    std::string chapter_list_url = "https://weebcentral.com/series/" + series_id + "/full-chapter-list";
    std::cout << "Chapter list URL: " << chapter_list_url << std::endl;

    // Download HTML
    std::string html_content;
    if (!http_client.download_html(chapter_list_url, html_content)) {
        std::cerr << "Failed to download HTML" << std::endl;
        return {};
    }

    // std::cout << "Downloaded " << html_content.size() << " bytes" << std::endl;

    // Parse chapter list HTML
    lxb_html_document_t *document = lxb_html_document_create();
    lxb_status_t status = lxb_html_document_parse(document,
                                                  (const lxb_char_t *) html_content.c_str(),
                                                  html_content.length());


    if (status != LXB_STATUS_OK) {
        std::cerr << "Error: Failed to parse chapter list HTML" << std::endl;

        // Clean up document
        lxb_html_document_destroy(document);

        return {};
    }

    // Parse chapter names and URLs
    std::vector<Chapter> chapters = Utils::parseChapterList(document);

    // Clean up document
    lxb_html_document_destroy(document);

    return chapters;
}

std::vector<std::string> getChapterImageURIs(HttpClient &http_client, const std::string &chapter_uri) {
    std::string images_uri = chapter_uri + "/images?is_prev=False&current_page=1&reading_style=long_strip";

    // Download HTML
    std::string html_content;
    if (!http_client.download_html(images_uri, html_content)) {
        std::cerr << "Failed to download HTML" << std::endl;
        return {};
    }

    // std::cout << "Downloaded " << html_content.size() << " bytes" << std::endl;

    // Parse chapter images list HTML
    lxb_html_document_t *document = lxb_html_document_create();
    lxb_status_t status = lxb_html_document_parse(document,
                                                  (const lxb_char_t *) html_content.c_str(),
                                                  html_content.length());

    if (status != LXB_STATUS_OK) {
        std::cerr << "Error: Failed to parse chapter images list HTML" << std::endl;

        // Clean up document
        lxb_html_document_destroy(document);

        return {};
    }

    // Parse chapter image URIs
    std::vector<std::string> image_uris = Utils::parseChapterImageURIs(document);

    // Clean up document
    lxb_html_document_destroy(document);

    return image_uris;
}
