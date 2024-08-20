#include "downloader.h"
#include <iostream>
#include <CLI/CLI.hpp>

#define ERU_VERSION "1.0.0"

void print_about()
{
    std::cout << "Eru - A Multi-threaded File Downloader" << std::endl;
    std::cout << "Version: " << ERU_VERSION << std::endl;
    std::cout << "Author: Sayan Biswas" << std::endl;
    std::cout << "Email: me@sayanbiswas.in" << std::endl;
    std::cout << std::endl;
    std::cout << "Eru is a simple and efficient multi-threaded file downloader" << std::endl;
    std::cout << "designed to accelerate your download speeds." << std::endl;
}

int main(int argc, char *argv[])
{
    std::string url;
    std::optional<std::string> output_file;
    int num_threads = 4;
    bool show_about = false;

    CLI::App app{"Eru - Multi-threaded file downloader"};
    app.add_option("-u,--url", url, "URL of the file to download");
    app.add_option("-o,--output", output_file, "Output file name (optional)");
    app.add_option("-t,--threads", num_threads, "Number of threads to use");
    app.add_flag("-a,--about", show_about, "Show information about Eru");

    CLI11_PARSE(app, argc, argv);

    if (show_about)
    {
        print_about();
        return 0;
    }

    if (url.empty())
    {
        std::cerr << "Error: URL is required. Use -u or --url to specify the download URL." << std::endl;
        std::cout << "Use --help for more information." << std::endl;
        return 1;
    }

    std::cout << "Eru v" << ERU_VERSION << std::endl;
    std::cout << "URL: " << url << std::endl;
    if (output_file)
    {
        std::cout << "Output file: " << *output_file << std::endl;
    }
    std::cout << "Threads: " << num_threads << std::endl;

    std::cout << "Testing connection..." << std::endl;
    if (!Downloader::test_connection(url))
    {
        std::cerr << "Failed to connect to the URL. Please check your internet connection and the URL." << std::endl;
        return 1;
    }
    std::cout << "Connection test successful." << std::endl;

    try
    {
        Downloader::download_file(url, output_file, num_threads);
    }
    catch (const std::exception &e)
    {
        std::cerr << "An error occurred during download: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
