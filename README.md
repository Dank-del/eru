# Eru: Multi-threaded File Downloader
<img width="957" alt="image" src="https://github.com/user-attachments/assets/98f77b00-3c82-43f9-bfe9-1dd2ba7d6ebf">

Eru is a simple and efficient multi-threaded file downloader designed to accelerate your download speeds. Written in C++, Eru leverages multiple threads to download file chunks concurrently, potentially increasing download speeds significantly. I made this as I had a simple requirement and did not want to buy IDM or pirate it.

## Features

- Multi-threaded downloading
- Progress bar with real-time updates
- Automatic filename detection from URL
- Customizable number of download threads
- Supports both HTTP and HTTPS
- Displays download speed and estimated time remaining
- Automatic merging of downloaded chunks

## Requirements

- C++17 compatible compiler
- CMake 3.14 or higher
- libcurl
- CLI11
- indicators

## Building from Source

1. Clone the repository:
   ```
   git clone https://github.com/Dank-del/eru.git
   cd eru
   ```

2. Create a build directory and navigate to it:
   ```
   mkdir build && cd build
   ```

3. Configure the project with CMake:
   ```
   cmake ..
   ```

4. Build the project:
   ```
   cmake --build .
   ```

## Usage

After building, you can run Eru using the following command:

```
./src/eru --url <download_url> [options]
```

### Options

- `-u, --url <url>`: Specify the URL of the file to download (required)
- `-o, --output <filename>`: Set the output filename (optional, auto-detected if not specified)
- `-t, --threads <number>`: Set the number of download threads (default: 4)
- `-a, --about`: Display information about Eru
- `-h, --help`: Show help message

### Examples

1. Download a file using default settings:
   ```
   ./src/eru --url https://example.com/large_file.zip
   ```

2. Download a file with a custom output name and 8 threads:
   ```
   ./src/eru --url https://example.com/large_file.zip --output my_file.zip --threads 8
   ```

3. Display information about Eru:
   ```
   ./src/eru --about
   ```

## Contributing

Contributions to Eru are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

Sayan Biswas
- Email: me@sayanbiswas.in

## Acknowledgments

- [CPR](https://github.com/libcpr/cpr) for HTTP requests
- [CLI11](https://github.com/CLIUtils/CLI11) for command-line parsing
- [indicators](https://github.com/p-ranav/indicators) for progress bars
