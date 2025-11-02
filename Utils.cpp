//
// Created by reikooters on 1/11/25.
//

#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <algorithm>

#include "Utils.h"

#include "lexbor/dom/collection.h"
#include "lexbor/dom/interfaces/element.h"
#include "lexbor/html/interfaces/document.h"
#include "models/Chapter.h"

// Helper function to sanitize folder names for cross-platform compatibility
std::string Utils::sanitizeFolderName(const std::string &name) {
    if (name.empty()) {
        return {};
    }

    std::string sanitized;
    sanitized.reserve(name.size());

    // Remove characters invalid on Windows/Linux/macOS
    // Including / and \ since we're sanitizing folder *names* (not paths)
    const std::string invalidChars = "<>:\"/\\|?*";

    // Process byte by byte, but preserve UTF-8 sequences
    for (size_t i = 0; i < name.size();) {
        unsigned char c = static_cast<unsigned char>(name[i]);

        // Check if this is the start of a UTF-8 multibyte sequence
        if (c >= 0x80) {
            // UTF-8 multibyte character - copy the entire sequence
            int bytes = 0;
            if ((c & 0xE0) == 0xC0) bytes = 2; // 110xxxxx
            else if ((c & 0xF0) == 0xE0) bytes = 3; // 1110xxxx
            else if ((c & 0xF8) == 0xF0) bytes = 4; // 11110xxx
            else {
                // Invalid UTF-8 start byte, skip it
                i++;
                continue;
            }

            // Verify we have enough bytes and they're valid continuation bytes
            bool valid = true;
            if (i + bytes > name.size()) {
                valid = false;
            } else {
                for (int j = 1; j < bytes; j++) {
                    if ((static_cast<unsigned char>(name[i + j]) & 0xC0) != 0x80) {
                        valid = false;
                        break;
                    }
                }
            }

            if (valid) {
                // Copy the entire multibyte sequence
                sanitized.append(name, i, bytes);
                i += bytes;
            } else {
                // Invalid sequence, skip the byte
                i++;
            }
        }
        // ASCII character
        else {
            char asciiChar = static_cast<char>(c);

            // Skip control characters (0-31) and DEL (127)
            if (c < 32 || c == 127) {
                i++;
                continue;
            }

            // Skip invalid characters
            if (invalidChars.find(asciiChar) != std::string::npos) {
                i++;
                continue;
            }

            sanitized += asciiChar;
            i++;
        }
    }

    // Trim whitespace from both ends
    auto start = sanitized.find_first_not_of(' ');
    if (start == std::string::npos) {
        return {}; // All whitespace
    }

    auto end = sanitized.find_last_not_of(' ');
    sanitized = sanitized.substr(start, end - start + 1);

    // Windows: Remove trailing dots and spaces (invalid on Windows)
    while (!sanitized.empty() && (sanitized.back() == '.' || sanitized.back() == ' ')) {
        sanitized.pop_back();
    }

    // Check if empty after sanitization
    if (sanitized.empty()) {
        return {};
    }

    // Windows: Check for reserved names (case-insensitive, ASCII only)
    static const std::unordered_set<std::string> reservedNames = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    // Extract ASCII prefix for reserved name check
    std::string asciiPrefix;
    for (char c: sanitized) {
        if (static_cast<unsigned char>(c) >= 0x80) break;
        asciiPrefix += c;
    }

    std::string upperName = asciiPrefix;
    std::ranges::transform(upperName, upperName.begin(),
                           [](unsigned char c) { return std::toupper(c); });

    auto dotPos = upperName.find('.');
    std::string baseName = (dotPos != std::string::npos) ? upperName.substr(0, dotPos) : upperName;

    if (reservedNames.contains(baseName)) {
        sanitized = "_" + sanitized;
    }

    // Limit byte length (most filesystems support 255 bytes, not characters)
    // Be careful not to cut in the middle of a UTF-8 sequence
    if (sanitized.length() > 255) {
        size_t cutPoint = 255;
        // Walk back to find a safe UTF-8 boundary
        while (cutPoint > 0 && (static_cast<unsigned char>(sanitized[cutPoint]) & 0xC0) == 0x80) {
            cutPoint--;
        }
        sanitized = sanitized.substr(0, cutPoint);

        // Re-trim trailing dots/spaces after truncation
        while (!sanitized.empty() && (sanitized.back() == '.' || sanitized.back() == ' ')) {
            sanitized.pop_back();
        }
    }

    return sanitized;
}

// Extract series ID from URL
std::string Utils::extractSeriesId(const std::string &url) {
    std::regex seriesRegex(R"(weebcentral\.com/series/([A-Z0-9]+))");
    std::smatch match;
    if (std::regex_search(url, match, seriesRegex)) {
        return match[1].str();
    }
    return {};
}

// Parse manga title from HTML
std::string Utils::parseMangaTitle(lxb_html_document_t *document) {
    lxb_dom_collection_t *collection = lxb_dom_collection_make(&document->dom_document, 16);
    lxb_status_t status;

    // Find <title> element
    status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->head),
                                          collection,
                                          (const lxb_char_t *) "title",
                                          5);

    if (status != LXB_STATUS_OK || lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return "";
    }

    lxb_dom_element_t *title_elem = lxb_dom_collection_element(collection, 0);
    const lxb_char_t *text = lxb_dom_node_text_content(lxb_dom_interface_node(title_elem), nullptr);

    std::string title = reinterpret_cast<const char *>(text);

    // Remove " | Weeb Central" suffix
    size_t pos = title.find(" | Weeb Central");
    if (pos != std::string::npos) {
        title = title.substr(0, pos);
    }

    lxb_dom_collection_destroy(collection, true);
    return title;
}

// Parse chapters from full-chapter-list HTML
std::vector<Chapter> Utils::parseChapterList(lxb_html_document_t *document) {
    std::vector<Chapter> chapters;
    lxb_dom_collection_t *collection = lxb_dom_collection_make(&document->dom_document, 128);
    lxb_status_t status;

    // Find all <a> elements with href containing "/chapters/"
    status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->body),
                                          collection,
                                          (const lxb_char_t *) "a",
                                          1);

    if (status != LXB_STATUS_OK) {
        lxb_dom_collection_destroy(collection, true);
        return chapters;
    }

    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t *elem = lxb_dom_collection_element(collection, i);

        // Get href attribute
        const lxb_char_t *href = lxb_dom_element_get_attribute(elem,
                                                               (const lxb_char_t *) "href",
                                                               4,
                                                               nullptr);

        if (href != nullptr) {
            std::string href_str = reinterpret_cast<const char *>(href);

            // Check if this is a chapter link
            if (href_str.find("https://weebcentral.com/chapters/") != std::string::npos) {
                // Get the chapter name from the text content
                // We need to find the <span> with the chapter name
                lxb_dom_node_t *node = lxb_dom_interface_node(elem);
                lxb_dom_node_t *child = node->first_child;

                std::string chapterName;
                while (child != nullptr) {
                    if (child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
                        lxb_dom_element_t *child_elem = lxb_dom_interface_element(child);
                        const lxb_char_t *class_attr = lxb_dom_element_get_attribute(child_elem,
                            (const lxb_char_t *) "class",
                            5,
                            nullptr);

                        if (class_attr != nullptr) {
                            std::string class_str = reinterpret_cast<const char *>(class_attr);
                            // Look for span with "grow flex items-center gap-2" class
                            if (class_str.find("grow") != std::string::npos) {
                                // Get first span child which contains chapter name
                                lxb_dom_node_t *span_child = child->first_child;
                                while (span_child != nullptr) {
                                    if (span_child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
                                        const lxb_char_t *text = lxb_dom_node_text_content(span_child, nullptr);
                                        if (text != nullptr) {
                                            chapterName = reinterpret_cast<const char *>(text);
                                            break;
                                        }
                                    }
                                    span_child = span_child->next;
                                }
                                break;
                            }
                        }
                    }
                    child = child->next;
                }

                if (!chapterName.empty()) {
                    Chapter chapter;
                    chapter.name = chapterName;
                    chapter.url = href_str;
                    chapters.push_back(chapter);
                }
            }
        }
    }

    lxb_dom_collection_destroy(collection, true);

    // Reverse the order so Chapter 1 comes first
    std::reverse(chapters.begin(), chapters.end());

    return chapters;
}

// Parse image URLs from chapter images page
std::vector<std::string> Utils::parseChapterImageURIs(lxb_html_document_t *document) {
    std::vector<std::string> image_uris;
    lxb_dom_collection_t *collection = lxb_dom_collection_make(&document->dom_document, 128);
    lxb_status_t status;

    // Find all <img> elements with href containing "/chapters/" (FIX ME)
    status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->body),
                                          collection,
                                          (const lxb_char_t *) "img",
                                          3);

    if (status != LXB_STATUS_OK) {
        lxb_dom_collection_destroy(collection, true);
        return image_uris;
    }

    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t *elem = lxb_dom_collection_element(collection, i);

        // Get src attribute
        const lxb_char_t *src = lxb_dom_element_get_attribute(elem,
                                                              (const lxb_char_t *) "src",
                                                              3,
                                                              nullptr);

        if (src != nullptr) {
            std::string href_str = reinterpret_cast<const char *>(src);

            if (!href_str.empty()) {
                image_uris.push_back(href_str);
            }
        }
    }

    lxb_dom_collection_destroy(collection, true);

    return image_uris;
}
