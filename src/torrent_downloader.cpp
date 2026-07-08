#include "torrent_downloader.h"

#include <string>
#include <stdexcept>
#include <filesystem>

#ifdef ERU_HAVE_LIBARIA2
#include "common.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <optional>
#include <vector>
#include <cstdint>
#include <aria2/aria2.h>
#else
#include <iostream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#endif
#endif

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

#ifdef ERU_HAVE_LIBARIA2

namespace
{
    struct TorrentState
    {
        bool completed = false;
        bool errored = false;
    };

    int on_download_event(aria2::Session *, aria2::DownloadEvent event, aria2::A2Gid, void *user_data)
    {
        auto *state = static_cast<TorrentState *>(user_data);
        if (event == aria2::EVENT_ON_DOWNLOAD_COMPLETE || event == aria2::EVENT_ON_BT_DOWNLOAD_COMPLETE)
        {
            state->completed = true;
        }
        else if (event == aria2::EVENT_ON_DOWNLOAD_ERROR)
        {
            state->errored = true;
        }
        return 0;
    }
}

void TorrentDownloader::download(const std::string &source, const std::optional<std::string> &save_path)
{
    const std::string save_dir = save_path.value_or(".");
    std::filesystem::create_directories(save_dir);

    if (aria2::libraryInit() != 0)
    {
        throw std::runtime_error("Failed to initialize the aria2 library.");
    }

    TorrentState state;
    aria2::SessionConfig config;
    config.keepRunning = false;
    config.useSignalHandler = true;
    config.downloadEventCallback = on_download_event;
    config.userData = &state;

    aria2::Session *session = aria2::sessionNew(aria2::KeyVals(), config);
    if (session == nullptr)
    {
        aria2::libraryDeinit();
        throw std::runtime_error("Failed to create an aria2 session.");
    }

    const aria2::KeyVals options = {
        {"dir", save_dir},
        {"seed-time", "0"},
        {"bt-save-metadata", "false"}};

    int rv = is_magnet(source)
                 ? aria2::addUri(session, nullptr, {source}, options)
                 : aria2::addTorrent(session, nullptr, source, options);
    if (rv < 0)
    {
        aria2::sessionFinal(session);
        aria2::libraryDeinit();
        throw std::runtime_error("Failed to add the magnet link or .torrent file to aria2.");
    }

    std::cout << "Saving to: " << save_dir << std::endl;
    if (is_magnet(source))
    {
        std::cout << "Fetching metadata from peers..." << std::endl;
    }

    indicators::ProgressBar progress_bar = make_progress_bar();
    bool have_metadata = false;
    auto last_print = std::chrono::steady_clock::now() - std::chrono::seconds(1);

    for (;;)
    {
        rv = aria2::run(session, aria2::RUN_ONCE);
        if (rv != 1)
        {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - last_print < std::chrono::milliseconds(300))
        {
            continue;
        }
        last_print = now;

        const std::vector<aria2::A2Gid> gids = aria2::getActiveDownload(session);
        if (gids.empty())
        {
            continue;
        }

        aria2::DownloadHandle *handle = aria2::getDownloadHandle(session, gids[0]);
        if (handle == nullptr)
        {
            continue;
        }

        const std::int64_t total = handle->getTotalLength();
        const std::int64_t done = handle->getCompletedLength();
        const int speed = handle->getDownloadSpeed();
        const int connections = handle->getConnections();
        aria2::deleteDownloadHandle(handle);

        if (total <= 0)
        {
            std::cout << "\rFetching metadata... peers: " << connections << "   " << std::flush;
            continue;
        }

        if (!have_metadata)
        {
            have_metadata = true;
            std::cout << "\r" << std::string(60, ' ') << "\r";
            std::cout << "Size: " << format_size(static_cast<size_t>(total)) << " MB" << std::endl;
        }

        progress_bar.set_progress(100.0 * static_cast<double>(done) / static_cast<double>(total));
        std::cout << "\rdownloading  "
                  << format_size(static_cast<size_t>(done)) << " / "
                  << format_size(static_cast<size_t>(total)) << " MB  "
                  << std::fixed << std::setprecision(2) << (speed / (1024.0 * 1024.0)) << " MB/s  "
                  << "peers: " << connections << "   " << std::flush;
    }

    aria2::sessionFinal(session);
    aria2::libraryDeinit();

    if (state.errored)
    {
        throw std::runtime_error("aria2 reported a download error.");
    }

    if (have_metadata && state.completed)
    {
        progress_bar.set_progress(100);
    }
    std::cout << "\nDownload complete. Files saved under: " << save_dir << std::endl;
}

#else

namespace
{
    constexpr int process_not_found = 127;

#ifdef _WIN32
    std::string quote_arg(const std::string &arg)
    {
        std::string out = "\"";
        std::size_t backslashes = 0;
        for (char c : arg)
        {
            if (c == '\\')
            {
                ++backslashes;
            }
            else if (c == '"')
            {
                out.append(backslashes * 2 + 1, '\\');
                out.push_back('"');
                backslashes = 0;
            }
            else
            {
                out.append(backslashes, '\\');
                out.push_back(c);
                backslashes = 0;
            }
        }
        out.append(backslashes * 2, '\\');
        out.push_back('"');
        return out;
    }

    int run_process(const std::string &exe, const std::vector<std::string> &args)
    {
        std::string command = quote_arg(exe);
        for (const std::string &a : args)
        {
            command.push_back(' ');
            command += quote_arg(a);
        }

        std::vector<char> buffer(command.begin(), command.end());
        buffer.push_back('\0');

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        if (!CreateProcessA(nullptr, buffer.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
        {
            return GetLastError() == ERROR_FILE_NOT_FOUND ? process_not_found : -1;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD code = 1;
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return static_cast<int>(code);
    }
#else
    int run_process(const std::string &exe, const std::vector<std::string> &args)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            return -1;
        }

        if (pid == 0)
        {
            std::vector<char *> argv;
            argv.push_back(const_cast<char *>(exe.c_str()));
            for (const std::string &a : args)
            {
                argv.push_back(const_cast<char *>(a.c_str()));
            }
            argv.push_back(nullptr);
            execvp(exe.c_str(), argv.data());
            _exit(process_not_found);
        }

        int status = 0;
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
        {
        }
        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }
        return -1;
    }
#endif
}

void TorrentDownloader::download(const std::string &source, const std::optional<std::string> &save_path)
{
    const std::string save_dir = save_path.value_or(".");
    std::filesystem::create_directories(save_dir);

    const std::vector<std::string> args = {
        "--dir=" + save_dir,
        "--seed-time=0",
        "--summary-interval=1",
        source};

    std::cout << "Handing off to aria2c..." << std::endl;
    const int code = run_process("aria2c", args);

    if (code == process_not_found)
    {
        throw std::runtime_error(
            "aria2c was not found. Install aria2 to download torrents "
            "(e.g. 'choco install aria2').");
    }
    if (code != 0)
    {
        throw std::runtime_error("aria2c exited with code " + std::to_string(code) + ".");
    }

    std::cout << "Download complete. Files saved under: " << save_dir << std::endl;
}

#endif
