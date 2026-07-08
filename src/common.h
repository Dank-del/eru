#pragma once

#include <cstdint>
#include <string>
#include <indicators/progress_bar.hpp>

std::string format_size(size_t size);

indicators::ProgressBar make_progress_bar();
