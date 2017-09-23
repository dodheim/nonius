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

// Pretty printing routines

#ifndef NONIUS_DETAIL_PRETTY_PRINT_HPP
#define NONIUS_DETAIL_PRETTY_PRINT_HPP

#include <nonius/reporter.h++>
#include <nonius/configuration.h++>
#include <nonius/sample_analysis.h++>
#include <nonius/execution_plan.h++>
#include <nonius/environment.h++>
#include <nonius/clock.h++>

#include <nonius/detail/compiler.h++>

#include <ios>
#include <iomanip>
#include <sstream>
#include <string>

namespace nonius {
    namespace detail {
        inline double get_magnitude(fp_seconds secs) {
            if(secs.count() >= 2.e0) {
                return 1.e0;
            } else if(secs.count() >= 2.e-3) {
                return 1.e3;
            } else if(secs.count() >= 2.e-6) {
                return 1.e6;
            } else {
                return 1.e9;
            }
        }
        inline std::string units_for_magnitude(double magnitude) {
            if(magnitude <= 1.e0) return "s";
            else if(magnitude <= 1.e3) return "ms";
            else if(magnitude <= 1.e6) return
#if defined(NONIUS_MSVC) || (defined(NONIUS_CLANG) && defined(_MSC_VER))
                    "us"
#else
                    "μs"
#endif
                ;
            else return "ns";
        }
        inline std::string pretty_duration(fp_seconds secs) {
            auto magnitude = get_magnitude(secs);
            auto units = units_for_magnitude(magnitude);
            std::ostringstream ss;
            ss << std::setprecision(ss.precision());
            ss << (secs.count() * magnitude) << ' ' << units;
            return ss.str();
        }
        inline std::string percentage(double d) {
            std::ostringstream ss;
            ss << std::setprecision(3);
            if(d != 0 && d < 1e-5) {
                ss << std::fixed;
                ss << 0.0001 << "%";
            } else {
                ss.unsetf(std::ios::floatfield);
                ss << (100. * d) << "%";
            }
            return ss.str();
        }
        inline std::string percentage_ratio(double part, double whole) {
            return percentage(part / whole);
        }
    } // namespace detail
} // namespace nonius

#endif // NONIUS_DETAIL_PRETTY_PRINT_HPP
