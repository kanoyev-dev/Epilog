#include "sscc_validator.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

using sscc::SSCCValidator;
using sscc::ValidationErrorCode;
using namespace std::string_literals;

namespace {

bool testValidate() {
    const SSCCValidator validator;

    const struct TestCase {
        std::string code;
        bool expected_valid;
        ValidationErrorCode expected_error;
        const char* description;
    } cases[] = {
    // ---- Valid ------------------------------------------------------------
    {"00034260311130776594", true, ValidationErrorCode::None, "original valid code (check digit 4)"},
    {"00134260311234567897", true, ValidationErrorCode::None, "another valid code, extension digit 1, check digit 7"},
    {"00934260311130776597", true, ValidationErrorCode::None, "extension digit 9, correct company prefix"},
    {"00034260311130776570", true, ValidationErrorCode::None, "check digit 0 (sum % 10 == 0)"},

    // ---- Company prefix ----------------------------------
    {"00094260311130776574", false, ValidationErrorCode::InvalidCompanyId, "check digit 4 is correct, but company prefix 94260311 is not the expected 34260311"},
    {"00034260321130776593", false, ValidationErrorCode::InvalidCompanyId, "check digit 3 is correct, but company prefix 34260321 is not the expected 34260311"},
    {"00000000000000000000", false, ValidationErrorCode::InvalidCompanyId, "minimum: all zeros, check digit 0, but wrong company prefix"},
    {"00999999999999999995", false, ValidationErrorCode::InvalidCompanyId, "maximum: all nines, correct check digit, but wrong company prefix"},
 
    // ---- Length -------------------------------------------
    {"",                        false, ValidationErrorCode::InvalidLength, "empty string"},
    {"0",                       false, ValidationErrorCode::InvalidLength, "1 character"},
    {"0003426031113077659",     false, ValidationErrorCode::InvalidLength, "19 characters (one short)"},
    {"000342603111307765944",   false, ValidationErrorCode::InvalidLength, "21 characters (one too many)"},
    {"034260311130776594",      false, ValidationErrorCode::InvalidLength, "bare 18-digit SSCC without AI"},
    {"00034260311130776594\n",  false, ValidationErrorCode::InvalidLength, "trailing newline (e.g. from input)"},
    {" 00034260311130776594",   false, ValidationErrorCode::InvalidLength, "leading space"},
    {"]C100034260311130776594", false, ValidationErrorCode::InvalidLength, "scanner symbology prefix ]C1"},
    {"0O03426031113077659",     false, ValidationErrorCode::InvalidLength, "bad AI AND bad length -> length wins"},
 
    // ---- AI ------------------
    {"10034260311130776594", false, ValidationErrorCode::InvalidApplicationIdentifier, "first AI digit wrong"},
    {"01034260311130776594", false, ValidationErrorCode::InvalidApplicationIdentifier, "second AI digit wrong (AI 01 = GTIN)"},
    {"0O034260311130776594", false, ValidationErrorCode::InvalidCharacter, "letter O instead of zero -> invalid char before AI check"},
    {"/0034260311130776594", false, ValidationErrorCode::InvalidCharacter, "'/' (0x2F, just below '0') in AI -> invalid char before AI check"},
 
    // ---- Non-digit characters ----------------------------
    {"00A34260311130776594",        false, ValidationErrorCode::InvalidCharacter, "letter at first payload position (index 2)"},
    {"000342603111307765/4",        false, ValidationErrorCode::InvalidCharacter, "'/' (0x2F, just below '0') at index 18"},
    {"0003426031113077659:",        false, ValidationErrorCode::InvalidCharacter, "':' (0x3A, just above '9') as check digit"},
    {"0003426031 130776594",        false, ValidationErrorCode::InvalidCharacter, "space in the middle"},
    {"0003426031\0"s + "130776594", false, ValidationErrorCode::InvalidCharacter, "embedded NUL byte (20 characters)"},
    {"0003426031113077659\x80",     false, ValidationErrorCode::InvalidCharacter, "byte 0x80 (CP1252 euro, negative char): UB with ::isdigit"},
    {"00\xFF" "34260311130776594",  false, ValidationErrorCode::InvalidCharacter, "byte 0xFF (-1 as signed char == EOF)"},
 
    // ---- Checksum ------------------------------------------
    {"00034260311130776144", false, ValidationErrorCode::InvalidCheckDigit, "original mismatch example (expected 3)"},
    {"00034260311130776595", false, ValidationErrorCode::InvalidCheckDigit, "check digit +1"},
    {"00034260311130776593", false, ValidationErrorCode::InvalidCheckDigit, "check digit -1"},
    {"00034260311130776579", false, ValidationErrorCode::InvalidCheckDigit, "wrap: correct 0, given 9"},
    {"00034260311130776540", false, ValidationErrorCode::InvalidCheckDigit, "wrap: correct 9, given 0"}
    };

    bool all_passed = true;
    for (const auto& tc : cases) {
        const auto result = validator.validate(tc.code);
        if (result.isValid() != tc.expected_valid || result.error_code != tc.expected_error) {
            std::cerr << "FAIL: " << tc.description
                      << " | code=" << tc.code
                      << " | expected_valid=" << (tc.expected_valid ? "true" : "false")
                      << " | expected_error=" << toString(tc.expected_error)
                      << " | got_valid=" << (result.isValid() ? "true" : "false")
                      << " | got_error=" << toString(result.error_code)
                      << " | message=" << result.error_message << '\n';
            all_passed = false;
        }
    }


    return all_passed;
}

bool testCheckDigit() {
    const struct TestCase {
        std::string digits;
        int expected;
    } cases[] = {
        {"03426031113077659", 4},
        {"03426031113077657", 0},
        {"03426031113077654", 9},
        {"00000000000000000", 0},
        {"99999999999999999", 5}
    };

    bool all_passed = true;
    for (const auto& tc : cases) {
        const int got = SSCCValidator::calculateCheckDigit(tc.digits);
        if (got != tc.expected) {
            std::cerr << "FAIL: check digit of " << tc.digits << " | expected=" << tc.expected << " | got=" << got << '\n';
            all_passed = false;
        }
    }
    return all_passed;
}

bool testCustomCompanyId() {
    const SSCCValidator validator{"94260311"};

    bool all_passed = true;
    if (!validator.validate("00094260311130776574").isValid()) {
        std::cerr << "FAIL: code with company ID 94260311 should be valid for a validator configured with it\n";
        all_passed = false;
    }
    if (validator.validate("00034260311130776594").error_code != ValidationErrorCode::InvalidCompanyId) {
        std::cerr << "FAIL: code with the default company ID should be rejected by a validator configured with 94260311\n";
        all_passed = false;
    }
    return all_passed;
}

bool testInvalidCompanyIdThrows() {
    bool all_passed = true;
    const struct TestCase {
        std::string company_id;
        bool expected_throw;
        const char* description;
    } cases[] = {
        {"",            true,  "empty"},
        {"123456",      true,  "6 digits (one short)"},
        {"12345678901", true,  "11 digits (one too many)"},
        {"3426031A",    true,  "contains a letter"},
        {"1234567",     false, "7 digits (minimum length)"},
        {"1234567890",  false, "10 digits (maximum length)"}
    };

    for (const auto& tc : cases) {
        // Any other exception type is not caught here and fails the run in main().
        bool threw = false;
        try {
            const SSCCValidator validator{tc.company_id};
        } catch (const std::invalid_argument&) {
            threw = true;
        }

        if (threw != tc.expected_throw) {
            std::cerr << "FAIL: company ID '" << tc.company_id << "' (" << tc.description << ")"
                      << " | expected_throw=" << (tc.expected_throw ? "true" : "false")
                      << " | got_throw=" << (threw ? "true" : "false") << '\n';
            all_passed = false;
        }
    }
    return all_passed;
}

} // namespace

int main() {
    try {
        bool all_passed = testValidate();
        all_passed = testCheckDigit() && all_passed;
        all_passed = testCustomCompanyId() && all_passed;
        all_passed = testInvalidCompanyIdThrows() && all_passed;

        if (all_passed) {
            std::cout << "All SSCC unit tests passed.\n";
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: unexpected exception: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
