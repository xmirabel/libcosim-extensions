#define BOOST_TEST_MODULE cse::fmi::v1::fmu unittest
#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <filesystem>

#include <cse/fmi/importer.hpp>
#include <cse/fmi/v1/fmu.hpp>
#include <cse/utility/filesystem.hpp>
#include <cse/utility/zip.hpp>


using namespace cse;

BOOST_TEST_DONT_PRINT_LOG_VALUE(fmi::fmi_version)

namespace
{
void run_tests(std::shared_ptr<fmi::fmu> fmu)
{
    BOOST_TEST(fmu->fmi_version() == fmi::fmi_version::v1_0);
    const auto d = fmu->model_description();
    BOOST_TEST(d->name == "no.viproma.demo.identity");
    BOOST_TEST(d->uuid.size() == 36U);
    BOOST_TEST(d->description ==
        "Has one input and one output of each type, and outputs are always set equal to inputs");
    BOOST_TEST(d->author == "Lars Tandle Kyllingstad");
    BOOST_TEST(d->version == "0.3");
    BOOST_TEST(std::static_pointer_cast<fmi::v1::fmu>(fmu)->fmilib_handle() != nullptr);

    cse::variable_index
        realIn = 0,
        integerIn = 0, booleanIn = 0, stringIn = 0,
        realOut = 0, integerOut = 0, booleanOut = 0, stringOut = 0;
    for (const auto& v : d->variables) {
        if (v.name == "realIn") {
            realIn = v.index;
        } else if (v.name == "integerIn") {
            integerIn = v.index;
        } else if (v.name == "booleanIn") {
            booleanIn = v.index;
        } else if (v.name == "stringIn") {
            stringIn = v.index;
        } else if (v.name == "realOut") {
            realOut = v.index;
        } else if (v.name == "integerOut") {
            integerOut = v.index;
        } else if (v.name == "booleanOut") {
            booleanOut = v.index;
        } else if (v.name == "stringOut") {
            stringOut = v.index;
        }

        if (v.name == "realIn") {
            BOOST_TEST(v.type == variable_type::real);
            BOOST_TEST(v.variability == variable_variability::discrete);
            BOOST_TEST(v.causality == variable_causality::input);
        } else if (v.name == "stringOut") {
            BOOST_TEST(v.type == variable_type::string);
            BOOST_TEST(v.variability == variable_variability::discrete);
            BOOST_TEST(v.causality == variable_causality::output);
        }
    }

    const double tMax = 1.0;
    const double dt = 0.1;
    double realVal = 0.0;
    int integerVal = 0;
    bool booleanVal = false;
    std::string stringVal;

    auto instance = fmu->instantiate_slave();
    instance->setup("testSlave", "testExecution", 0.0, tMax, false, 0.0);
    instance->start_simulation();

    for (double t = 0; t < tMax; t += dt) {
        double getRealVal = -1.0;
        int getIntegerVal = -1;
        bool getBooleanVal = true;
        std::string getStringVal = "unexpected value";

        instance->get_real_variables(gsl::make_span(&realOut, 1), gsl::make_span(&getRealVal, 1));
        instance->get_integer_variables(gsl::make_span(&integerOut, 1), gsl::make_span(&getIntegerVal, 1));
        instance->get_boolean_variables(gsl::make_span(&booleanOut, 1), gsl::make_span(&getBooleanVal, 1));
        instance->get_string_variables(gsl::make_span(&stringOut, 1), gsl::make_span(&getStringVal, 1));

        BOOST_TEST(getRealVal == realVal);
        BOOST_TEST(getIntegerVal == integerVal);
        BOOST_TEST(getBooleanVal == booleanVal);
        BOOST_TEST(getStringVal == stringVal);

        realVal += 1.0;
        integerVal += 1;
        booleanVal = !booleanVal;
        stringVal += 'a';

        instance->set_real_variables(gsl::make_span(&realIn, 1), gsl::make_span(&realVal, 1));
        instance->set_integer_variables(gsl::make_span(&integerIn, 1), gsl::make_span(&integerVal, 1));
        instance->set_boolean_variables(gsl::make_span(&booleanIn, 1), gsl::make_span(&booleanVal, 1));
        instance->set_string_variables(gsl::make_span(&stringIn, 1), gsl::make_span(&stringVal, 1));

        BOOST_TEST(instance->do_step(t, dt));
    }

    instance->end_simulation();
}
} // namespace


BOOST_AUTO_TEST_CASE(v1_fmu)
{
    const auto testDataDir = std::getenv("TEST_DATA_DIR");
    BOOST_TEST_REQUIRE(!!testDataDir);
    auto importer = fmi::importer::create();
    auto fmu = importer->import(
        std::filesystem::path(testDataDir) / "fmi1" / "identity.fmu");
    run_tests(fmu);
}


BOOST_AUTO_TEST_CASE(v1_fmu_unpacked)
{
    const auto testDataDir = std::getenv("TEST_DATA_DIR");
    BOOST_TEST_REQUIRE(!!testDataDir);
    utility::temp_dir unpackDir;
    utility::zip::archive(
        std::filesystem::path(testDataDir) / "fmi1" / "identity.fmu")
        .extract_all(unpackDir.path());

    auto importer = fmi::importer::create();
    auto fmu = importer->import_unpacked(unpackDir.path());
    run_tests(fmu);

    importer->clean_cache();
    BOOST_TEST(std::filesystem::exists(unpackDir.path()));
}
