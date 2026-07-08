# Eru: Multi-threaded File Downloader
<img width="957" alt="image" src="https://github.com/user-attachments/assets/98f77b00-3c82-43f9-bfe9-1dd2ba7d6ebf">

Eru is a small multi-threaded file downloader written in C++. It splits the target file into chunks, fetches them in parallel over HTTP/HTTPS with libcurl, and stitches the result back together. It also downloads **BitTorrent magnet links and `.torrent` files** by handing them off to [aria2c](https://aria2.github.io/). I built it because I wanted IDM-like behaviour without paying for IDM or pirating it.

## Features

- Multi-threaded HTTP/HTTPS downloading
- BitTorrent support via magnet links and `.torrent` files (handled by aria2c)
- Progress bar with real-time updates
- Automatic filename detection from URL
- Customizable number of download threads
- Displays download speed and estimated time remaining
- Automatic merging of downloaded chunks

## Requirements

- C++17 compatible compiler
- CMake 3.16 or higher
- libcurl (build dependency; CLI11 and indicators are fetched automatically by CMake)
- [aria2](https://aria2.github.io/) at runtime for torrent/magnet downloads (`apt install aria2`, `brew install aria2`, or `choco install aria2`)

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
./src/eru --url <download_url_or_magnet_or_torrent> [options]
```

Eru auto-detects the source type: `magnet:` links and paths ending in `.torrent` are downloaded over BitTorrent, everything else over HTTP/HTTPS.

### Options

- `-u, --url <url>`: URL to download, a `magnet:` link, or a path to a `.torrent` file (required)
- `-o, --output <path>`: Output filename for HTTP downloads, or the save **directory** for torrents (optional)
- `-t, --threads <number>`: Number of download threads for HTTP (default: 4; ignored for torrents)
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

3. Download a magnet link into a folder:
   ```
   ./src/eru --url "magnet:?xt=urn:btih:..." --output ./downloads
   ```

4. Download from a local `.torrent` file:
   ```
   ./src/eru --url ./ubuntu.torrent --output ./downloads
   ```

5. Display information about Eru:
   ```
   ./src/eru --about
   ```

## Contributing

Pull requests welcome.

## License

MIT. See [LICENSE](LICENSE).

## Author

Sayan Biswas
- Email: me@sayanbiswas.in

## Acknowledgments

- [CPR](https://github.com/libcpr/cpr) for HTTP requests
- [CLI11](https://github.com/CLIUtils/CLI11) for command-line parsing
- [indicators](https://github.com/p-ranav/indicators) for progress bars
- [aria2](https://aria2.github.io/) for BitTorrent downloads
