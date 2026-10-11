/*

The program checks if the given examples of scanned SSCC barcodes comply with the GS1-128 specification.

More about GS1-128 specification can be found at the following links:
https://en.wikipedia.org/wiki/GS1-128
https://www.gs1-128.info/sscc-18/

Fields of the code explained by the following example.

Code structure of the given code: 00034260311130776594

--------------------------------------------------
Part                                 | Digits | Value
--------------------------------------------------
Application Identifier (AI)          | 2      | 00
Extension digit                      | 1      | 0
Company ID                           | 8      | 34260311
Serial Number                        | 8      | 13077659
Check Digit                          | 1      | 4
--------------------------------------------------
*/

#include "sscc_validator.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void printResult(const sscc::ValidateResult& result, std::string_view valid_text) {
    if (result.isValid()) {
        std::cout << valid_text << '\n';
    } else {
        std::cout << sscc::toString(result.error_code) << " - " << result.error_message << '\n';
    }
}

} // namespace

int main() {
    try {
        const sscc::SSCCValidator validator;

        const std::vector<std::string> predefined_codes = {
            "00034260311130776594", // valid SSCC code
            "02044260311130776512", // wrong Application Identifier (AI): expected "00", not "02"
            "00034260311130776144", // wrong check digit: expected 3, not 4
            "00034260311130776570", // valid SSCC code
            "0003426031113077646",  // wrong length: 19 digits, expected 20
            "00034260321130774636"  // wrong company ID: 34260321 instead of 34260311
        };

        std::cout << "Predefinirane SSCC kode:\n";
        for (const auto& code : predefined_codes) {
            std::cout << code << ": ";
            printResult(validator.validate(code), "veljavna");
        }

        std::cout << "\nVnesite SSCC kode za katere želite preveriti veljavnost oz. Ctrl+C ali Ctrl+D za izhod iz programa:\n";
        std::string input;
        while (std::cin >> input) {
            printResult(validator.validate(input), "SSCC koda je veljavna");
        }

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Napaka: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
