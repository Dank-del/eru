#include "downloader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <cpr/cpr.h>
#include <indicators/progress_bar.hpp>

std::mutex console_mutex;
std::atomic<size_t> total_progress(0);

std::string get_filename_from_url(const std::string &url)
{
    size_t pos = url.find_last_of("/");
    if (pos != std::string::npos)
    {
        return url.substr(pos + 1);
    }
    return "downloaded_file";
}

std::string format_size(size_t size)
{
    double size_mb = static_cast<double>(size) / (1024 * 1024);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size_mb;
    return ss.str();
}

void Downloader::download_file(const std::string &url, const std::optional<std::string> &output_file, int num_threads)
{
    cpr::Response r = cpr::Head(cpr::Url{url});
    if (r.status_code != 200)
    {
        std::cerr << "Failed to get file information. Status code: " << r.status_code << std::endl;
        return;
    }

    size_t file_size = std::stoull(r.header["Content-Length"]);
    size_t chunk_size = std::ceil(static_cast<double>(file_size) / num_threads);

    std::string filename = output_file.value_or(get_filename_from_url(url));
    std::cout << "Downloading to: " << filename << std::endl;
    std::cout << "File size: " << format_size(file_size) << " MB" << std::endl;

    indicators::ProgressBar progress_bar{
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"="},
        indicators::option::Lead{">"},
        indicators::option::Remainder{" "},
        indicators::option::End{"]"},
        indicators::option::ForegroundColor{indicators::Color::green},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}};

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i)
    {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? file_size - 1 : (i + 1) * chunk_size - 1;
        threads.emplace_back(&Downloader::download_chunk, url, start, end, filename, i);
    }

    while (total_progress < file_size)
    {
        size_t current_progress = total_progress.load();
        progress_bar.set_progress(100.0 * current_progress / file_size);
        std::cout << "\rDownloaded: " << format_size(current_progress) << " MB / " << format_size(file_size) << " MB" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto &t : threads)
    {
        t.join();
    }

    progress_bar.set_progress(100);
    std::cout << "\nDownload complete. Merging chunks..." << std::endl;

    merge_chunks(filename, num_threads);

    std::cout << "File saved as: " << filename << std::endl;
}

void Downloader::download_chunk(const std::string &url, size_t start, size_t end, const std::string &output_file, int chunk_id)
{
    std::string chunk_file = output_file + ".part" + std::to_string(chunk_id);
    std::ofstream out(chunk_file, std::ios::binary);

    cpr::Response r = cpr::Get(cpr::Url{url},
                               cpr::Header{{"Range", "bytes=" + std::to_string(start) + "-" + std::to_string(end)}},
                               cpr::WriteCallback([&](const std::string &data, intptr_t)
                                                  {
                                   out.write(data.c_str(), data.length());
                                   total_progress += data.length();
                                   return true; }));

    out.close();

    if (r.status_code != 206)
    {
        std::lock_guard<std::mutex> lock(console_mutex);
        std::cerr << "Failed to download chunk " << chunk_id << ". Status code: " << r.status_code << std::endl;
    }
}

void Downloader::merge_chunks(const std::string &output_file, int num_chunks)
{
    std::ofstream out(output_file, std::ios::binary);

    for (int i = 0; i < num_chunks; ++i)
    {
        std::string chunk_file = output_file + ".part" + std::to_string(i);
        std::ifstream in(chunk_file, std::ios::binary);
        out << in.rdbuf();
        in.close();
        std::remove(chunk_file.c_str());
    }

    out.close();
}

bool Downloader::test_connection(const std::string &url)
{
    cpr::Response r = cpr::Head(cpr::Url{url});
    return r.status_code == 200;
}
