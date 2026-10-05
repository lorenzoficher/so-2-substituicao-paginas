#include "cli.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>

#include "fifo.hpp"
#include "lru_approx.hpp"
#include "opt.hpp"
#include "simulator.hpp"
#include "trace.hpp"

namespace {

const char* const USAGE =
    "uso: sim <trace> fifo|opt --frames <n>[,<n>...]\n"
    "     sim <trace> lru-approx --bits <N> --interval <I> --frames <n>[,<n>...]\n"
    "     (N de 1 a 32; I pelo menos 1)\n";

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
    unsigned history_bits = 0;       ///< N; 0 outside the LRU aproximado.
    std::size_t aging_interval = 0;  ///< I; 0 outside the LRU aproximado.
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
    const bool lru_approx = options.policy == "lru-approx";
    if (options.policy != "fifo" && options.policy != "opt" && !lru_approx) {
        throw UsageError("política desconhecida: '" + options.policy + "'");
    }
    for (std::size_t i = 2; i < args.size(); i += 2) {
        const std::string& option = args[i];
        const bool lru_option = option == "--bits" || option == "--interval";
        if (option != "--frames" && !lru_option) {
            throw UsageError("opção desconhecida: '" + option + "'");
        }
        if (lru_option && !lru_approx) {
            throw UsageError(option + " só vale para lru-approx");
        }
        if (i + 1 == args.size()) {
            throw UsageError("falta o valor de " + option);
        }
        const std::string& value = args[i + 1];
        if (option == "--frames") {
            if (!options.frame_counts.empty()) {
                throw UsageError("--frames repetido");
            }
            options.frame_counts = parse_frame_counts(value);
        } else if (option == "--bits") {
            if (options.history_bits != 0) {
                throw UsageError("--bits repetido");
            }
            const std::size_t bits = parse_positive(value, "número de bits de histórico");
            if (bits > LruApprox::MAX_HISTORY_BITS) {
                throw UsageError("número de bits de histórico deve ser no máximo 32");
            }
            options.history_bits = static_cast<unsigned>(bits);
        } else {
            if (options.aging_interval != 0) {
                throw UsageError("--interval repetido");
            }
            options.aging_interval = parse_positive(value, "intervalo de envelhecimento");
        }
    }
    if (options.frame_counts.empty()) {
        throw UsageError("falta --frames");
    }
    if (lru_approx && options.history_bits == 0) {
        throw UsageError("falta --bits");
    }
    if (lru_approx && options.aging_interval == 0) {
        throw UsageError("falta --interval");
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
/// @param options Already validated by parse_arguments.
std::unique_ptr<Policy> make_policy(const Options& options, const Trace& trace,
                                    std::size_t frame_count) {
    if (options.policy == "opt") {
        return std::make_unique<Opt>(trace, frame_count);
    }
    if (options.policy == "lru-approx") {
        return std::make_unique<LruApprox>(frame_count, options.history_bits,
                                           options.aging_interval);
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
    // history_bits and aging_interval stay empty outside the LRU aproximado.
    const std::string lru_columns =
        options.policy == "lru-approx"
            ? std::to_string(options.history_bits) + ',' + std::to_string(options.aging_interval)
            : ",";
    out << CSV_HEADER << '\n';
    for (const std::size_t frame_count : options.frame_counts) {
        const std::unique_ptr<Policy> policy = make_policy(options, trace, frame_count);
        const SimulationResult result = run(trace, *policy);
        out << name << ',' << options.policy << ',' << frame_count << ',' << lru_columns << ','
            << result.accesses << ',' << result.page_faults << ',' << result.writebacks << '\n';
    }
    return 0;
}
