#include "torrent_downloader.h"
#include "common.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <optional>

#include <libtransmission/transmission.h>
#include <libtransmission/variant.h>
#include <libtransmission/error.h>

namespace
{
    std::atomic<bool> g_interrupted{false};

    void handle_sigint(int)
    {
        g_interrupted.store(true);
    }

    constexpr auto metadata_timeout = std::chrono::seconds(120);
    constexpr auto stall_timeout = std::chrono::seconds(180);
    constexpr auto poll_interval = std::chrono::milliseconds(400);

    const char *activity_name(tr_torrent_activity activity)
    {
        switch (activity)
        {
        case TR_STATUS_CHECK_WAIT:
        case TR_STATUS_CHECK:
            return "checking";
        case TR_STATUS_DOWNLOAD_WAIT:
        case TR_STATUS_DOWNLOAD:
            return "downloading";
        case TR_STATUS_SEED_WAIT:
        case TR_STATUS_SEED:
            return "seeding";
        default:
            return "stopped";
        }
    }
}

bool TorrentDownloader::is_magnet(const std::string &s)
{
    return s.rfind("magnet:", 0) == 0;
}

bool TorrentDownloader::is_torrent_file(const std::string &s)
{
    const std::string ext = ".torrent";
    return s.size() >= ext.size() &&
           s.compare(s.size() - ext.size(), ext.size(), ext) == 0;
}

void TorrentDownloader::download(const std::string &source, const std::optional<std::string> &save_path)
{
    namespace fs = std::filesystem;

    const std::string save_dir = save_path.value_or(".");
    fs::create_directories(save_dir);

    const fs::path config_dir = fs::temp_directory_path() / "eru-session";
    std::error_code ec;
    fs::remove_all(config_dir, ec);
    fs::create_directories(config_dir);

    tr_variant settings;
    tr_variantInitDict(&settings, 0);
    tr_sessionGetDefaultSettings(&settings);
    tr_session *session = tr_sessionInit(config_dir.string().c_str(), false, &settings);
    tr_variantClear(&settings);

    if (session == nullptr)
    {
        fs::remove_all(config_dir, ec);
        throw std::runtime_error("Failed to initialize the BitTorrent session.");
    }

    tr_ctor *ctor = tr_ctorNew(session);
    tr_error *err = nullptr;
    const bool loaded = is_magnet(source)
                            ? tr_ctorSetMetainfoFromMagnetLink(ctor, source.c_str(), &err)
                            : tr_ctorSetMetainfoFromFile(ctor, source.c_str(), &err);

    if (!loaded)
    {
        std::string message = err != nullptr && err->message != nullptr
                                  ? err->message
                                  : "invalid magnet link or .torrent file";
        tr_error_free(err);
        tr_ctorFree(ctor);
        tr_sessionClose(session);
        fs::remove_all(config_dir, ec);
        throw std::runtime_error("Failed to load torrent: " + message);
    }

    tr_ctorSetDownloadDir(ctor, TR_FORCE, save_dir.c_str());

    tr_torrent *torrent = tr_torrentNew(ctor, nullptr);
    tr_ctorFree(ctor);
    if (torrent == nullptr)
    {
        tr_sessionClose(session);
        fs::remove_all(config_dir, ec);
        throw std::runtime_error("Failed to add the torrent to the session.");
    }

    std::signal(SIGINT, handle_sigint);
    tr_torrentStart(torrent);

    std::cout << "Saving to: " << save_dir << std::endl;
    if (is_magnet(source))
    {
        std::cout << "Fetching metadata from peers..." << std::endl;
    }

    std::optional<indicators::ProgressBar> progress_bar;
    const auto started_at = std::chrono::steady_clock::now();
    auto last_progress_at = started_at;
    uint64_t last_have = 0;
    bool have_metadata = false;
    bool completed = false;

    while (!g_interrupted.load())
    {
        const tr_stat *st = tr_torrentStat(torrent);

        if (st->metadataPercentComplete < 1.0f)
        {
            std::cout << "\rFetching metadata... peers: " << st->peersConnected << "   " << std::flush;
            if (std::chrono::steady_clock::now() - started_at > metadata_timeout)
            {
                std::cout << std::endl;
                std::cerr << "Timed out waiting for torrent metadata (no reachable peers?)." << std::endl;
                break;
            }
            std::this_thread::sleep_for(poll_interval);
            continue;
        }

        if (!have_metadata)
        {
            have_metadata = true;
            progress_bar.emplace(make_progress_bar());
            std::cout << "\r" << std::string(60, ' ') << "\r";
            std::cout << "Name: " << tr_torrentName(torrent) << std::endl;
            std::cout << "Size: " << format_size(st->sizeWhenDone) << " MB" << std::endl;
        }

        const double percent = 100.0 * st->percentDone;
        progress_bar->set_progress(percent);
        std::cout << "\r" << activity_name(st->activity) << "  "
                  << format_size(st->haveValid) << " / " << format_size(st->sizeWhenDone) << " MB  "
                  << std::fixed << std::setprecision(2) << (st->pieceDownloadSpeed_KBps / 1024.0) << " MB/s  "
                  << "peers: " << st->peersConnected << "   " << std::flush;

        if (st->percentDone >= 1.0f || st->activity == TR_STATUS_SEED || st->activity == TR_STATUS_SEED_WAIT)
        {
            completed = true;
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (st->haveValid > last_have)
        {
            last_have = st->haveValid;
            last_progress_at = now;
        }
        else if (now - last_progress_at > stall_timeout)
        {
            std::cout << std::endl;
            std::cerr << "Download stalled (no new data from peers)." << std::endl;
            break;
        }

        std::this_thread::sleep_for(poll_interval);
    }

    std::signal(SIGINT, SIG_DFL);

    if (progress_bar && completed)
    {
        progress_bar->set_progress(100);
    }

    tr_torrentStop(torrent);
    tr_sessionClose(session);
    fs::remove_all(config_dir, ec);

    if (g_interrupted.load())
    {
        std::cout << "\nInterrupted. Partial data left in: " << save_dir << std::endl;
    }
    else if (completed)
    {
        std::cout << "\nDownload complete. Files saved under: " << save_dir << std::endl;
    }
}
