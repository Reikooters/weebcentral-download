//
// Created by reikooters on 1/11/25.
//

#ifndef WEEBCENTRAL_DOWNLOAD_UTILS_H
#define WEEBCENTRAL_DOWNLOAD_UTILS_H


#include <vector>  // Add this at the very top, before other includes
#include "lexbor/html/interface.h"
#include "models/Chapter.h"

namespace Utils {
    /**
     * Sanitizes a folder name by removing or replacing invalid characters, trimming whitespace,
     * and handling other platform-specific constraints (e.g., reserved names on Windows).
     * Ensures the folder name complies with general cross-platform filesystem conventions.
     *
     * @param name The original folder name to be sanitized.
     * @return A sanitized version of the folder name. Returns an empty string if the input name is invalid
     *         or cannot be sanitized into a valid folder name.
     */
    std::string sanitizeFolderName(const std::string &name);

    /**
     * Extracts the series ID from a given URL string. The method searches for a
     * specific pattern in URLs (e.g., "weebcentral.com/series/<series_id>") and
     * returns the series ID if it matches. If no valid series ID is found, an
     * empty string is returned.
     *
     * @param url The URL string from which to extract the series ID.
     * @return The extracted series ID as a string, or an empty string if no valid
     *         series ID is found in the input URL.
     */
    std::string extractSeriesId(const std::string &url);

    std::string parseMangaTitle(lxb_html_document_t *document);

    std::vector<Chapter> parseChapterList(lxb_html_document_t *document);

    std::vector<std::string> parseChapterImageURIs(lxb_html_document_t *document);
}


#endif //WEEBCENTRAL_DOWNLOAD_UTILS_H
