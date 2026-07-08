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
#include <memory>
#include <vector>
#include <cstdint>

#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/alert_types.hpp>

namespace lt = libtorrent;

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

    const char *state_name(lt::torrent_status::state_t state)
    {
        switch (state)
        {
        case lt::torrent_status::checking_files:
        case lt::torrent_status::checking_resume_data:
            return "checking";
        case lt::torrent_status::downloading_metadata:
            return "metadata";
        case lt::torrent_status::downloading:
            return "downloading";
        case lt::torrent_status::finished:
        case lt::torrent_status::seeding:
            return "seeding";
        default:
            return "queued";
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

    lt::settings_pack settings;
    settings.set_int(lt::settings_pack::alert_mask,
                     lt::alert_category::error | lt::alert_category::status);
    settings.set_str(lt::settings_pack::user_agent, "Eru/1.0 libtorrent");
    lt::session session(settings);

    lt::add_torrent_params params;
    if (is_magnet(source))
    {
        params = lt::parse_magnet_uri(source);
    }
    else
    {
        params.ti = std::make_shared<lt::torrent_info>(source);
    }
    params.save_path = save_dir;

    lt::torrent_handle handle = session.add_torrent(std::move(params));

    std::signal(SIGINT, handle_sigint);

    std::cout << "Saving to: " << save_dir << std::endl;
    if (is_magnet(source))
    {
        std::cout << "Fetching metadata from peers..." << std::endl;
    }

    std::optional<indicators::ProgressBar> progress_bar;
    const auto started_at = std::chrono::steady_clock::now();
    auto last_progress_at = started_at;
    std::int64_t last_done = 0;
    bool have_metadata = false;
    bool completed = false;

    while (!g_interrupted.load())
    {
        std::vector<lt::alert *> alerts;
        session.pop_alerts(&alerts);
        for (lt::alert *a : alerts)
        {
            if (auto *e = lt::alert_cast<lt::torrent_error_alert>(a))
            {
                std::cout << std::endl;
                std::cerr << "Torrent error: " << e->message() << std::endl;
            }
        }

        const lt::torrent_status st = handle.status();

        if (!st.has_metadata)
        {
            std::cout << "\rFetching metadata... peers: " << st.num_peers << "   " << std::flush;
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
            std::cout << "Name: " << st.name << std::endl;
            std::cout << "Size: " << format_size(st.total_wanted) << " MB" << std::endl;
        }

        const double percent = st.total_wanted > 0
                                   ? 100.0 * static_cast<double>(st.total_wanted_done) / static_cast<double>(st.total_wanted)
                                   : 0.0;
        progress_bar->set_progress(percent);
        std::cout << "\r" << state_name(st.state) << "  "
                  << format_size(st.total_wanted_done) << " / " << format_size(st.total_wanted) << " MB  "
                  << std::fixed << std::setprecision(2) << (st.download_rate / (1024.0 * 1024.0)) << " MB/s  "
                  << "peers: " << st.num_peers << "   " << std::flush;

        if (st.is_finished || st.state == lt::torrent_status::seeding || st.state == lt::torrent_status::finished)
        {
            completed = true;
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (st.total_wanted_done > last_done)
        {
            last_done = st.total_wanted_done;
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

    session.pause();
    lt::session_proxy proxy = session.abort();

    if (g_interrupted.load())
    {
        std::cout << "\nInterrupted. Partial data left in: " << save_dir << std::endl;
    }
    else if (completed)
    {
        std::cout << "\nDownload complete. Files saved under: " << save_dir << std::endl;
    }
}
