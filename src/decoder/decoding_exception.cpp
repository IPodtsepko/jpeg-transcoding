#include "decoder/decoding_exception.hpp"

DecodingException::DecodingException(std::string message, const Reason reason)
    : m_message(std::move(message))
    , m_reason(reason)
{
}

DecodingException::Reason DecodingException::get_reason() const noexcept
{
    return m_reason;
}

const char * DecodingException::what() const noexcept
{
    return m_message.c_str();
}
