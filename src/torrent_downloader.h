#pragma once

#include <string>
#include <optional>

class TorrentDownloader
{
public:
    static void download(const std::string &source, const std::optional<std::string> &save_path);

    static bool is_magnet(const std::string &s);
    static bool is_torrent_file(const std::string &s);
};
