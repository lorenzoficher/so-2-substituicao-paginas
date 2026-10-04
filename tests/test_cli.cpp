#include <cstdint>
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

TEST_CASE("CLI: OPT imprime o CSV no mesmo formato do FIFO") {
    write_sample_trace();

    const CliRun result = run({SAMPLE_TRACE, "opt", "--frames", "3,4"});

    CHECK(result.exit_code == 0);
    CHECK(result.err.empty());
    CHECK(result.out ==
          "trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks\n"
          "belady,opt,3,,,12,7,0\n"
          "belady,opt,4,,,12,6,0\n");
}

TEST_CASE("CLI: LRU aproximado preenche bits de histórico e intervalo de envelhecimento") {
    write_sample_trace();

    // With I = 1000 no envelhecimento happens in 12 accesses: every page keeps the
    // bit on and histórico zerado, so the victim is the one loaded first, as in FIFO.
    const CliRun result =
        run({SAMPLE_TRACE, "lru-approx", "--bits", "8", "--interval", "1000", "--frames", "3,4"});

    CHECK(result.exit_code == 0);
    CHECK(result.err.empty());
    CHECK(result.out ==
          "trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks\n"
          "belady,lru-approx,3,8,1000,12,9,0\n"
          "belady,lru-approx,4,8,1000,12,10,0\n");
}

TEST_CASE("CLI: opções do LRU aproximado aceitas em qualquer ordem") {
    write_sample_trace();

    const CliRun result =
        run({SAMPLE_TRACE, "lru-approx", "--frames", "3", "--interval", "1", "--bits", "32"});

    CHECK(result.exit_code == 0);
    CHECK(result.out.find("belady,lru-approx,3,32,1,12,") != std::string::npos);
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
        {SAMPLE_TRACE, "lru-approx", "--frames", "4"},                  // no N, no I
        {SAMPLE_TRACE, "lru-approx", "--bits", "8", "--frames", "4"},   // no I
        {SAMPLE_TRACE, "lru-approx", "--interval", "9", "--frames", "4"},  // no N
        {SAMPLE_TRACE, "lru-approx", "--bits", "0", "--interval", "9", "--frames", "4"},
        {SAMPLE_TRACE, "lru-approx", "--bits", "33", "--interval", "9", "--frames", "4"},
        {SAMPLE_TRACE, "lru-approx", "--bits", "x", "--interval", "9", "--frames", "4"},
        {SAMPLE_TRACE, "lru-approx", "--bits", "8", "--interval", "0", "--frames", "4"},
        {SAMPLE_TRACE, "lru-approx", "--bits", "8", "--interval", "-1", "--frames", "4"},
        {SAMPLE_TRACE, "lru-approx", "--bits", "8", "--bits", "8", "--interval", "9",
         "--frames", "4"},                                              // repeated
        {SAMPLE_TRACE, "lru-approx", "--bits", "8", "--interval"},      // no value
        {SAMPLE_TRACE, "fifo", "--bits", "8", "--frames", "4"},         // N outside LRU
        {SAMPLE_TRACE, "opt", "--interval", "9", "--frames", "4"},      // I outside LRU
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

TEST_CASE("build/sim: LRU aproximado num trace grande dá a mesma saída em duas execuções") {
    // 500 000 accesses from a fixed linear congruential generator: a hot set of 64
    // pages mixed with a cold range of 4096, about a quarter of them writes.
    const std::string trace_path = "build/tests/large.trace";
    {
        std::ofstream file(trace_path);
        uint32_t state = 12345;
        for (int i = 0; i < 500000; ++i) {
            state = state * 1664525u + 1013904223u;
            const uint32_t page = (state >> 31) != 0 ? (state >> 8) % 64 : (state >> 8) % 4096;
            const bool is_write = (state >> 4) % 4 == 0;
            file << std::hex << (page << 12 | (state & 0xfff)) << (is_write ? " W\n" : " R\n");
        }
    }
    const std::string command = "./build/sim " + trace_path +
                                " lru-approx --bits 8 --interval 1000 --frames 4,64,256 > ";

    const int first = std::system((command + "build/tests/large-1.csv").c_str());
    const int second = std::system((command + "build/tests/large-2.csv").c_str());

    REQUIRE(first == 0);
    REQUIRE(second == 0);
    std::ifstream csv_1("build/tests/large-1.csv");
    std::ifstream csv_2("build/tests/large-2.csv");
    std::stringstream content_1;
    std::stringstream content_2;
    content_1 << csv_1.rdbuf();
    content_2 << csv_2.rdbuf();
    CHECK(content_1.str().find("large,lru-approx,256,8,1000,500000,") != std::string::npos);
    CHECK(content_1.str() == content_2.str());
}

TEST_CASE("build/sim: argumento inválido sai com código diferente de zero") {
    const int status = std::system("./build/sim 2> /dev/null");

    CHECK(status != 0);
}
