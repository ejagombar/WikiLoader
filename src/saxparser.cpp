#include "saxparser.h"
#include <algorithm>
#include <cstdlib>
#include <ostream>
#include <pstl/glue_algorithm_defs.h>

MySaxParser::MySaxParser() : xmlpp::SaxParser() {}

MySaxParser::~MySaxParser() {}

void MySaxParser::on_start_document() {}

void MySaxParser::on_comment(const xmlpp::ustring &text) {}

void MySaxParser::on_warning(const xmlpp::ustring &text) {}

std::vector<Page> MySaxParser::getPages() { return pages; }

void MySaxParser::clear() {
    pages.clear();
    depth = 0;
    content = "";
    page = {};
    count = 0;
    nextElement = OTHER;
}

void MySaxParser::on_end_document() {}

void MySaxParser::on_start_element(const xmlpp::ustring &name, const AttributeList &attributes) {
    if (name == "title") {
        nextElement = TITLE;
    } else if (name == "text") {
        nextElement = CONTENT;
    } else if (name == "redirect") {
        page.redirect = true;
    } else {
        nextElement = OTHER;
    }

    depth++;
}

void MySaxParser::on_end_element(const xmlpp::ustring & /* name */) {
    nextElement = OTHER;
    count++;

    if (depth == 2) {
        // extractAllLinks();

        if ((page.title.size() > 0) && !RE2::PartialMatch(page.title, "^.+:")) {
            pages.push_back(page);
        }

        page = {};
        content = "";
    }

    depth--;
}

void MySaxParser::on_characters(const xmlpp::ustring &text) {
    if (nextElement == TITLE) {
        std::string title = text;
        formatLink(title);

        page.title = page.title + title;

    } else if (nextElement == CONTENT) {
        content = content + text;
    }
}

inline bool MySaxParser::stringReplace(std::string &string, const std::string &oldString,
                                       const std::string &newString) {
    if (oldString.empty()) {
        return false;
    }

    size_t startPos = string.find(oldString);
    if (startPos == std::string::npos) {
        return false;
    }

    while (startPos != std::string::npos) {
        string.replace(startPos, oldString.length(), newString);
        startPos = string.find(oldString, newString.length() + startPos);
    }
    return true;
}

inline void MySaxParser::formatLink(std::string &str) {
    // Convert to lower case
    transform(str.begin(), str.end(), str.begin(), ::tolower);

    // Replace double quotes with single quotes
    std::replace(str.begin(), str.end(), '\"', '\'');
    std::replace(str.begin(), str.end(), '_', ' ');

    stringReplace(str, "&nbsp;", " ");
    stringReplace(str, "&amp;", "&");

    str.erase(str.find_last_not_of(' ') + 1);
    str.erase(0, str.find_first_not_of(' '));

    // Remove quotes from string
    // str.erase(remove(str.begin(), str.end(), '\"'), str.end());
}

void MySaxParser::on_error(const xmlpp::ustring &text) {
    std::cout << "Most recent page title: " << page.title << "\nError: " << text << std::endl;
    std::cout << "depth: " << depth << std::endl;
    std::cout << "content: " << content << std::endl;
}

void MySaxParser::on_fatal_error(const xmlpp::ustring &text) {
    std::cout << "Most recent page title: " << page.title << "\nFatal Error: " << text << std::endl;
}

void MySaxParser::extractAllLinks() {
    re2::RE2 re("\\[\\[([^\\]]+)\\]\\]");
    re2::StringPiece input(content);

    re2::StringPiece match;
    while (RE2::FindAndConsume(&input, re, &match)) {

        // Filters out any Wikipedia Namespaces
        // https://en.wikipedia.org/wiki/Wikipedia:Namespace
        if (RE2::PartialMatch(match, "^.+:")) {
            continue;
        }

        std::string str(match);

        // Sometimes the page that is being linked to is not the same as the
        // text being shown in the link. This ensures that only the true page
        // name is shown
        std::string subStr(str.begin(), find(str.begin(), str.end(), '|'));
        std::string subStr2(subStr.begin(), find(subStr.begin(), subStr.end(), '#'));

        formatLink(subStr2);

        page.links.push_back(subStr2);
    }
}
