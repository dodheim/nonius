// Nonius - C++ benchmarking tool
//
// Written in 2014- by the nonius contributors <nonius@rmf.io>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related
// and neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>

// Run configuration

#ifndef NONIUS_CONFIGURATION_HPP
#define NONIUS_CONFIGURATION_HPP

#include <nonius/param.h++>

#if !defined(NONIUS_USE_BOOST_OPTIONAL) && defined(__has_include)
#   if __has_include(<optional>) && (__cplusplus > 201402L || (defined(_MSVC_LANG) && _MSVC_LANG > 201402L))
#      include <optional>
#      define NONIUS_OPTIONAL_NS std
#   elif __has_include(<experimental/optional>) && __cplusplus >= 201402L
#      include <experimental/optional>
#      define NONIUS_OPTIONAL_NS std::experimental
#   elif __has_include(<boost/optional.hpp>)
#      include <boost/optional.hpp>
#      define NONIUS_OPTIONAL_NS boost
#   else
#      error "Missing <optional>"
#   endif
#else
#   include <boost/optional.hpp>
#   define NONIUS_OPTIONAL_NS boost
#endif

#include <string>
#include <cstddef>

namespace nonius {
    struct run_configuration {
        std::string name;
        std::string op;
        param init;
        param step;
        std::size_t count;
    };

    struct param_configuration {
        parameters map;
        NONIUS_OPTIONAL_NS::optional<run_configuration> run;
    };

    struct configuration {
        int samples = 100;
        double confidence_interval = 0.95;
        int resamples = 100000;
        std::string title = "benchmarks";
        std::string output_file;
        std::string reporter;
        std::string filter_pattern = ".*";
        bool list_benchmarks = false;
        bool list_params = false;
        bool list_reporters = false;
        bool no_analysis = false;
        bool verbose = false;
        bool summary = false;
        bool help = false;
        param_configuration params;
    };
} // namespace nonius

#endif // NONIUS_CONFIGURATION_HPP
