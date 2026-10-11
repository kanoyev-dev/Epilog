#pragma once

#include <string>
#include <string_view>

namespace sscc {

inline constexpr std::string_view kDefaultCompanyId = "34260311";

enum class ValidationErrorCode {
    None,
    InvalidLength,
    InvalidCharacter,
    InvalidApplicationIdentifier,
    InvalidCompanyId,
    InvalidCheckDigit
};

[[nodiscard]] std::string_view toString(ValidationErrorCode code) noexcept;

struct ValidateResult {
    ValidationErrorCode error_code{ValidationErrorCode::None};
    std::string error_message{}; // empty when the code is valid

    [[nodiscard]] bool isValid() const noexcept { return error_code == ValidationErrorCode::None; }
};

class SSCCValidator {
public:
    // Throws std::invalid_argument if company_id is not a 7 to 10 digit GS1 company prefix.
    explicit SSCCValidator(std::string company_id = std::string(kDefaultCompanyId));

    // Validates a scanned code: the AI "00" followed by the 18-digit SSCC.
    [[nodiscard]] ValidateResult validate(std::string_view code) const;

    // Calculates the GS1 check digit for a string of digits (the SSCC without its check digit).
    [[nodiscard]] static int calculateCheckDigit(std::string_view digits) noexcept;

private:
    std::string company_id_;
};

} // namespace sscc
