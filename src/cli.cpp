#include "cli.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>

#include "fifo.hpp"
#include "opt.hpp"
#include "simulator.hpp"
#include "trace.hpp"

namespace {

const char* const USAGE = "uso: sim <trace> fifo|opt --frames <n>[,<n>...]\n";

const char* const CSV_HEADER =
    "trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks";

/// A command line that does not follow USAGE.
class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// What the command line asked for.
struct Options {
    std::string trace_path;
    std::string policy;
    std::vector<std::size_t> frame_counts;
};

/// Parses a positive decimal number made only of digits — no sign, no spaces.
std::size_t parse_positive(const std::string& text, const std::string& what) {
    if (text.empty()) {
        throw UsageError(what + " vazio");
    }
    std::size_t value = 0;
    for (const char digit : text) {
        if (digit < '0' || digit > '9') {
            throw UsageError(what + " inválido: '" + text + "'");
        }
        const std::size_t units = static_cast<std::size_t>(digit - '0');
        if (value > (std::numeric_limits<std::size_t>::max() - units) / 10) {
            throw UsageError(what + " grande demais: '" + text + "'");
        }
        value = value * 10 + units;
    }
    if (value == 0) {
        throw UsageError(what + " deve ser pelo menos 1");
    }
    return value;
}

/// Parses a comma-separated list such as `4,8,16`.
std::vector<std::size_t> parse_frame_counts(const std::string& list) {
    std::vector<std::size_t> frame_counts;
    std::size_t start = 0;
    while (true) {
        const std::size_t comma = list.find(',', start);
        frame_counts.push_back(
            parse_positive(list.substr(start, comma - start), "número de frames"));
        if (comma == std::string::npos) {
            return frame_counts;
        }
        start = comma + 1;
    }
}

Options parse_arguments(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        throw UsageError("faltam o trace e a política");
    }
    Options options;
    options.trace_path = args[0];
    options.policy = args[1];
    if (options.policy != "fifo" && options.policy != "opt") {
        throw UsageError("política desconhecida: '" + options.policy + "'");
    }
    for (std::size_t i = 2; i < args.size(); i += 2) {
        if (args[i] != "--frames") {
            throw UsageError("opção desconhecida: '" + args[i] + "'");
        }
        if (i + 1 == args.size()) {
            throw UsageError("falta o valor de " + args[i]);
        }
        if (!options.frame_counts.empty()) {
            throw UsageError("--frames repetido");
        }
        options.frame_counts = parse_frame_counts(args[i + 1]);
    }
    if (options.frame_counts.empty()) {
        throw UsageError("falta --frames");
    }
    return options;
}

/// Trace name in the CSV: the file name without directory and extension.
std::string trace_name(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    const std::size_t dot = name.find_last_of('.');
    if (dot != std::string::npos && dot != 0) {
        name.erase(dot);
    }
    return name;
}

/// A fresh policy, frames empty, for one simulation of `trace`.
/// @param policy A name already validated by parse_arguments.
std::unique_ptr<Policy> make_policy(const std::string& policy, const Trace& trace,
                                    std::size_t frame_count) {
    if (policy == "opt") {
        return std::make_unique<Opt>(trace, frame_count);
    }
    return std::make_unique<Fifo>(frame_count);
}

}  // namespace

int run_cli(const std::vector<std::string>& args, std::ostream& out, std::ostream& err) {
    Options options;
    try {
        options = parse_arguments(args);
    } catch (const UsageError& error) {
        err << "erro: " << error.what() << '\n' << USAGE;
        return 2;
    }

    Trace trace;
    try {
        trace = read_trace(options.trace_path);
    } catch (const std::exception& error) {
        err << "erro: " << error.what() << '\n';
        return 1;
    }

    const std::string name = trace_name(options.trace_path);
    out << CSV_HEADER << '\n';
    for (const std::size_t frame_count : options.frame_counts) {
        const std::unique_ptr<Policy> policy = make_policy(options.policy, trace, frame_count);
        const SimulationResult result = run(trace, *policy);
        out << name << ',' << options.policy << ',' << frame_count << ",,," << result.accesses
            << ',' << result.page_faults << ',' << result.writebacks << '\n';
    }
    return 0;
}
