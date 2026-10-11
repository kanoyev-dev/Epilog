#include "sscc_validator.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace sscc {

namespace {

// Layout of a scanned code: AI (2) | extension digit (1) | company ID + serial number (16) | check digit (1)
constexpr std::string_view kApplicationIdentifier = "00";
constexpr std::size_t kCodeSize = 20;
constexpr std::size_t kExtensionDigitSize = 1;
constexpr std::size_t kCheckDigitSize = 1;
constexpr std::size_t kCompanyIdOffset = kApplicationIdentifier.size() + kExtensionDigitSize;

constexpr std::size_t kMinCompanyIdSize = 7;
constexpr std::size_t kMaxCompanyIdSize = 10;

// Safe wrapper around std::isdigit, which has two pitfalls:
// - Passing a negative value is undefined behaviour, and plain char is usually signed,
//   so bytes above 0x7F (e.g. 0x80) must be converted to unsigned char first.
// - It returns an int that is non-zero (not necessarily 1) for a digit, so the result
//   is compared with 0 to get a proper bool.
bool isDigit(char c) noexcept {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

} // namespace

std::string_view toString(ValidationErrorCode code) noexcept {
    switch (code) {
        case ValidationErrorCode::None:
            return "NONE";
        case ValidationErrorCode::InvalidLength:
            return "INVALID_LENGTH";
        case ValidationErrorCode::InvalidCharacter:
            return "INVALID_CHARACTER";
        case ValidationErrorCode::InvalidApplicationIdentifier:
            return "INVALID_APPLICATION_IDENTIFIER";
        case ValidationErrorCode::InvalidCompanyId:
            return "INVALID_COMPANY_ID";
        case ValidationErrorCode::InvalidCheckDigit:
            return "INVALID_CHECK_DIGIT";
    }
    return "UNKNOWN";
}

SSCCValidator::SSCCValidator(std::string company_id)
    : company_id_(std::move(company_id)) {
    if (company_id_.size() < kMinCompanyIdSize || company_id_.size() > kMaxCompanyIdSize) {
        throw std::invalid_argument("ID podjetja ima napačno dolžino - '" + std::to_string(company_id_.size()) + "'. Pričakovana dolžina je od '" +
            std::to_string(kMinCompanyIdSize) + "' do '" + std::to_string(kMaxCompanyIdSize) + "' mest.");
    }

    if (!std::all_of(company_id_.begin(), company_id_.end(), isDigit)) {
        throw std::invalid_argument("ID podjetja '" + company_id_ + "' vsebuje neveljaven znak. ID podjetja mora vsebovati samo številke.");
    }
}

ValidateResult SSCCValidator::validate(std::string_view code) const {
    if (code.size() != kCodeSize) {
        return {ValidationErrorCode::InvalidLength,
            "Koda ima napačno dolžino - '" + std::to_string(code.size()) + "'. Pričakovana dolžina je '" + std::to_string(kCodeSize) + "'."};
    }

    if (const auto it = std::find_if_not(code.begin(), code.end(), isDigit); it != code.end()) {
        return {ValidationErrorCode::InvalidCharacter,
            "Koda vsebuje neveljaven znak - '" + std::string(1, *it) + "'. SSCC koda mora vsebovati samo številke."};
    }

    if (const auto ai = code.substr(0, kApplicationIdentifier.size()); ai != kApplicationIdentifier) {
        return {ValidationErrorCode::InvalidApplicationIdentifier,
            "Koda vsebuje napačen Application Identifier (AI) za SSCC - '" + std::string(ai) + "'. Pričakovan AI je '" + std::string(kApplicationIdentifier) + "'."};
    }

    if (const auto company_id = code.substr(kCompanyIdOffset, company_id_.size()); company_id != company_id_) {
        return {ValidationErrorCode::InvalidCompanyId,
            "SSCC koda vsebuje napačen ID podjetja '" + std::string(company_id) + "'. Pravilen ID podjetja je '" + company_id_ + "'."};
    }

    const auto payload = code.substr(kApplicationIdentifier.size(), kCodeSize - kApplicationIdentifier.size() - kCheckDigitSize);
    const int check_digit = calculateCheckDigit(payload);
    if (check_digit != code.back() - '0') {
        return {ValidationErrorCode::InvalidCheckDigit,
            "SSCC koda ima napačno kontrolno cifro '" + std::string(1, code.back()) + "'. Pričakovana kontrolna cifra je '" + std::to_string(check_digit) + "'."};
    }

    return {};
}

// To calculate a Serial Shipping Container Code (SSCC) check digit,
// we use a standard Modulo 10 algorithm with alternating 3 and 1 weights.
int SSCCValidator::calculateCheckDigit(std::string_view digits) noexcept {
    int sum = 0;
    int weight = 3;

    // Iterate over the digits in reverse order to calculate the weighted sum.
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        sum += (*it - '0') * weight;
        weight = (weight == 3) ? 1 : 3;
    }

    return (10 - (sum % 10)) % 10; // The next highest multiple of 10 minus sum.
}

} // namespace sscc
