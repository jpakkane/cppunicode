/*

Copyright (c) 2020 Jussi Pakkanen

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you
    must not claim that you wrote the original software. If you use
    this software in a product, an acknowledgment in the product
    documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and
    must not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.

 */

#include<cassert>
#include<cstdint>
#include<iostream>
#include<fstream>
#include<filesystem>
#include<unordered_map>
#include<string>
#include<vector>
#include<algorithm>
#include<future>

#include<unicode/unistr.h>
#include<unicode/regex.h>

struct WordCount {
    std::string word;
    int64_t count;
};

typedef std::unordered_map<std::string, int64_t> WordMap;

auto count_order_desc = [](const WordCount &w1, const WordCount &w2) { return w2.count < w1.count; };

WordMap count_word_files(std::filesystem::directory_entry e) {
    UErrorCode status = U_ZERO_ERROR;
    icu::RegexMatcher u_word_regex("([a-z]{2,})", UREGEX_CASE_INSENSITIVE, status);
    if(U_FAILURE(status)) {
        std::cout << "Regex creation failed.\n";
        std::abort();
    }
    WordMap file_word_counts;
    if(!e.is_regular_file()) {
      return file_word_counts;
    }
    std::ifstream input_file(e.path());
    if(input_file.fail()) {
        return file_word_counts;
    }

    for (std::string line; std::getline(input_file, line); ) {
        const auto us = icu::UnicodeString::fromUTF8(line); // Bad sequences get assigned the Unicode replacement character.
        u_word_regex.reset(us);
        while(u_word_regex.find()) {
            auto match_start = u_word_regex.start(status);
            assert(!U_FAILURE(status));
            auto match_end = u_word_regex.end(status);
            assert(!U_FAILURE(status));
            auto current_match = us.tempSubString(match_start, match_end - match_start).toLower();
            std::string matched_word;
            current_match.toUTF8String(matched_word);
            ++file_word_counts[matched_word];
        }
    }
    return file_word_counts;
}

void pop_future(WordMap &word_counts, std::vector<std::future<WordMap>> &futures) {
    assert(!futures.empty());
    auto file_counts = futures.front().get();
    futures.erase(futures.begin());
    for(const auto&[key, value]: file_counts) {
        word_counts[key] += value;
    }
}

int count_words() {
    WordMap word_counts;
    std::vector<std::future<WordMap>> futures;
    const size_t num_threads = std::thread::hardware_concurrency();
    assert(num_threads != 0); // Very rare, but possible.
    futures.reserve(num_threads);
    for(const auto &e: std::filesystem::recursive_directory_iterator(".")) {
        auto extension = e.path().extension();
        if(!(extension == ".txt" || extension == ".TXT")) {
            continue;
        }
        futures.emplace_back(std::async(std::launch::async,
                                        count_word_files,
                                        e));
        if(futures.size() > num_threads) {
            pop_future(word_counts, futures);
        }
    }
    while(!futures.empty()) {
        pop_future(word_counts, futures);
    }

    std::vector<WordCount> word_array;
    word_array.reserve(word_counts.size());
    for(const auto &[word, count]: word_counts) {
        word_array.emplace_back(WordCount{word, count});
    }

    const auto final_size = std::min(10, (int)word_array.size());
    std::partial_sort(word_array.begin(),
                      word_array.begin() + final_size,
                      word_array.end(),
                      count_order_desc);
    word_array.erase(word_array.begin() + final_size, word_array.end());
    for(const auto& word_info: word_array) {
        std::cout << word_info.count << " " << word_info.word << "\n";
    };
    return 0;
}

int main() {
    // https://unicode-org.atlassian.net/browse/ICU-21346
    UErrorCode status = U_ZERO_ERROR;
    icu::RegexMatcher u_word_regex("ICU_threading_bug_workaround", UREGEX_CASE_INSENSITIVE, status);
    if(U_FAILURE(status)) {
        std::cout << "Regex creation failed.\n";
        return 1;
    }
    int rc = count_words();
    return rc;
}
