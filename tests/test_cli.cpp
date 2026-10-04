#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cli.hpp"
#include "doctest.h"

namespace {

/// Trace file shared by the tests: Belady's sequence, pages 1..5, reads only.
/// Lives under build/ because `make test` runs from the repository root.
const std::string SAMPLE_TRACE = "build/tests/belady.trace";

void write_sample_trace() {
    std::ofstream file(SAMPLE_TRACE);
    for (const int page : {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5}) {
        file << std::hex << (page << 12) << " R\n";
    }
}

struct CliRun {
    int exit_code;
    std::string out;
    std::string err;
};

CliRun run(const std::vector<std::string>& args) {
    std::ostringstream out;
    std::ostringstream err;
    const int exit_code = run_cli(args, out, err);
    return {exit_code, out.str(), err.str()};
}

}  // namespace

TEST_CASE("CLI: FIFO imprime o cabeçalho e uma linha de CSV por número de frames") {
    write_sample_trace();

    const CliRun result = run({SAMPLE_TRACE, "fifo", "--frames", "3,4"});

    CHECK(result.exit_code == 0);
    CHECK(result.err.empty());
    CHECK(result.out ==
          "trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks\n"
          "belady,fifo,3,,,12,9,0\n"
          "belady,fifo,4,,,12,10,0\n");
}

TEST_CASE("CLI: argumento inválido dá erro de uso no stderr, sem nenhuma linha de CSV") {
    write_sample_trace();
    const std::vector<std::vector<std::string>> invalid = {
        {},                                                // nothing
        {SAMPLE_TRACE},                                    // no policy
        {SAMPLE_TRACE, "fifo"},                            // no --frames
        {SAMPLE_TRACE, "fifo", "--frames"},                // --frames without a value
        {SAMPLE_TRACE, "clock", "--frames", "4"},          // unknown policy
        {SAMPLE_TRACE, "fifo", "--frames", "4", "--x"},    // unknown option
        {SAMPLE_TRACE, "fifo", "--frames", "0"},           // zero frames
        {SAMPLE_TRACE, "fifo", "--frames", "-4"},          // sign
        {SAMPLE_TRACE, "fifo", "--frames", "+4"},          // sign
        {SAMPLE_TRACE, "fifo", "--frames", " 4"},          // leading space
        {SAMPLE_TRACE, "fifo", "--frames", "4a"},          // trailing text
        {SAMPLE_TRACE, "fifo", "--frames", "4,,8"},        // empty item
        {SAMPLE_TRACE, "fifo", "--frames", "4,"},          // empty last item
        {SAMPLE_TRACE, "fifo", "--frames", "99999999999999999999999"},  // overflow
    };
    for (const std::vector<std::string>& args : invalid) {
        std::string joined;
        for (const std::string& arg : args) {
            joined += "[" + arg + "]";
        }
        CAPTURE(joined);

        const CliRun result = run(args);

        CHECK(result.exit_code != 0);
        CHECK(result.out.empty());
        CHECK(result.err.find("uso:") != std::string::npos);
    }
}

TEST_CASE("CLI: trace inexistente dá erro com o caminho e nenhuma linha de CSV") {
    const CliRun result = run({"nao/existe.trace", "fifo", "--frames", "4"});

    CHECK(result.exit_code != 0);
    CHECK(result.out.empty());
    CHECK(result.err.find("nao/existe.trace") != std::string::npos);
}

TEST_CASE("CLI: número de frames enorme roda sem reservar frames") {
    write_sample_trace();

    const CliRun result = run({SAMPLE_TRACE, "fifo", "--frames", "99999999999"});

    CHECK(result.exit_code == 0);
    CHECK(result.out.find("belady,fifo,99999999999,,,12,5,0\n") != std::string::npos);
}

TEST_CASE("CLI: FIFO preenche writebacks com as vítimas sujas") {
    const std::string trace = "build/tests/escritas.trace";
    std::ofstream(trace) << "00001000 W\n00002000 R\n00003000 R\n";

    const CliRun result = run({trace, "fifo", "--frames", "1,3"});

    CHECK(result.exit_code == 0);
    // 1 frame: page 1 leaves dirty. 3 frames: no victim, page 1 still dirty at the end.
    CHECK(result.out ==
          "trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks\n"
          "escritas,fifo,1,,,3,3,1\n"
          "escritas,fifo,3,,,3,3,0\n");
}

TEST_CASE("build/sim: o binário imprime o CSV no stdout e sai com 0") {
    write_sample_trace();
    const std::string output = "build/tests/belady.csv";

    const int status = std::system(
        ("./build/sim " + SAMPLE_TRACE + " fifo --frames 3 > " + output).c_str());

    CHECK(status == 0);
    std::ifstream csv(output);
    std::stringstream content;
    content << csv.rdbuf();
    CHECK(content.str() ==
          "trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks\n"
          "belady,fifo,3,,,12,9,0\n");
}

TEST_CASE("build/sim: argumento inválido sai com código diferente de zero") {
    const int status = std::system("./build/sim 2> /dev/null");

    CHECK(status != 0);
}
