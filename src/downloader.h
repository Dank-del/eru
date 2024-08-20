#pragma once

#include <string>
#include <optional>

class Downloader
{
public:
    static void download_file(const std::string &url, const std::optional<std::string> &output_file, int num_threads);
    static bool test_connection(const std::string &url);

private:
    static void download_chunk(const std::string &url, size_t start, size_t end, const std::string &output_file, int chunk_id);
    static void merge_chunks(const std::string &output_file, int num_chunks);
};
