#include <sstream>
#include <stdexcept>
#include <string>

#include "doctest.h"
#include "trace.hpp"

TEST_CASE("acesso de escrita vira a página do endereço sem os 12 bits de deslocamento") {
    std::istringstream input("31348900 W\n");

    const Trace trace = read_trace(input);

    REQUIRE(trace.size() == 1);
    CHECK(trace[0].page == 0x31348);
    CHECK(trace[0].is_write);
}

TEST_CASE("acesso de leitura não é escrita e linhas vazias são ignoradas") {
    std::istringstream input("\n00001000 R\n\n00002fff W\n");

    const Trace trace = read_trace(input);

    REQUIRE(trace.size() == 2);
    CHECK(trace[0].page == 0x1);
    CHECK_FALSE(trace[0].is_write);
    CHECK(trace[1].page == 0x2);
    CHECK(trace[1].is_write);
}

TEST_CASE("linha malformada gera erro com o número da linha") {
    const std::string malformed[] = {
        "zz W",          // not hexadecimal
        "31348900",      // no operation
        "31348900 X",    // unknown operation
        "31348900 W x",  // trailing text
        "1ffffffff W",   // wider than 32 bits
    };
    for (const std::string& line : malformed) {
        CAPTURE(line);
        std::istringstream input(std::string("00001000 R\n") + line + "\n");
        CHECK_THROWS_WITH_AS(read_trace(input), doctest::Contains("linha 2"),
                             std::runtime_error);
    }
}

TEST_CASE("trace inexistente gera erro com o caminho") {
    CHECK_THROWS_WITH_AS(read_trace(std::string("nao/existe.trace")),
                         doctest::Contains("nao/existe.trace"), std::runtime_error);
}
