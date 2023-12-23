#pragma once

#include <exception>
#include <string>

/**
 * @brief Exception class for errors that may occur during JPEG decoding.
 */
class DecodingException : public std::exception
{
public:
    /**
     * @brief All possible causes of errors that the JPEG decoder may encounter.
     */
    enum class Reason
    {
        /** Not a JPEG file. */
        NO_JPEG,

        /** Unsupported format. */
        UNSUPPORTED,

        /** Internal application error. */
        INTERNAL_ERROR,

        /** Syntax error in JPEG file. */
        SYNTAX_ERROR,
    };

public:
    /**
     * @brief Constructs a DecodingException object.
     *
     * @param message A descriptive message of the error.
     * @param reason The reason of the decoding error.
     */
    DecodingException(std::string message, const Reason reason);

    /**
     * @brief Gets the reason for the decoding error.
     *
     * @return The reason for the decoding error.
     */
    Reason get_reason() const noexcept;

    /**
     * @brief Returns a message describing the exception.
     *
     * @return describing the exception.
     */
    const char * what() const noexcept override;

private:
    const std::string m_message;
    const Reason m_reason;
};
