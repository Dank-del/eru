#include "common.h"
#include <sstream>
#include <iomanip>
#include <vector>

std::string format_size(size_t size)
{
    double size_mb = static_cast<double>(size) / (1024 * 1024);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size_mb;
    return ss.str();
}

indicators::ProgressBar make_progress_bar()
{
    return indicators::ProgressBar{
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
}
