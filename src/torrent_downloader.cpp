#include "torrent_downloader.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#endif

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
            "(e.g. 'apt install aria2', 'brew install aria2', or 'choco install aria2').");
    }
    if (code != 0)
    {
        throw std::runtime_error("aria2c exited with code " + std::to_string(code) + ".");
    }

    std::cout << "Download complete. Files saved under: " << save_dir << std::endl;
}
