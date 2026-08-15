#include <catch2/catch_test_macros.hpp>

#include <varerr/varset.hpp>
#include <varerr/status.hpp>
#include <varerr/result.hpp>

TEST_CASE("stub", "[stub]") {

    CHECK(varerr::kVarSetStub);
    CHECK(varerr::kStatusStub);
    CHECK(varerr::kResultStub);

}
