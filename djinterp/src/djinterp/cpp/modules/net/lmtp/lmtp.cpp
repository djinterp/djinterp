/*******************************************************************************
* djinterp [cpp][net]                                                   lmtp.cpp
*
* djinterp LMTP - C++ facade implementation.
*   Each opening copies its options with LMTP's dialect set, whatever the
* caller left in them, so the SMTP face beneath only ever opens LMTP
* sessions; the rest is the SMTP face's own. Empty where the header is:
* without a 64-bit integer type.
*
*
* path:      /src/djinterp/cpp/modules/net/lmtp/lmtp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "../../../../../../inc/djinterp/cpp/modules/net/lmtp/lmtp.hpp"  // corresponding header


// the header's 64-bit floor
#if (D_INTERNAL_NET_HAS_INT64 == 1)


NS_DJINTERP
NS_NET
D_NAMESPACE(lmtp)

NS_INTERNAL

    // held
    //   function: a copy of SMTP's options of any kind, in LMTP's dialect.
    template<typename Options>
    Options held(
        const Options& _options
    )
    {
        Options copy(_options);

        copy.dialect = smtp::dialect::lmtp;

        return copy;
    }

NS_END  // internal

client::client() D_NOEXCEPT
    : smtp::client()
{}

#if (D_MOVE_ENABLED == 1)
client::client(
    client&& _other
) D_NOEXCEPT
    : smtp::client(static_cast<smtp::client&&>(_other))
{}

client&
client::operator=(
    client&& _other
) D_NOEXCEPT
{
    smtp::client::operator=(static_cast<smtp::client&&>(_other));

    return *this;
}
#endif  // D_MOVE_ENABLED

void_result client::open(
    const client_options& _options
)
{
    return smtp::client::open(
               internal::held<smtp::client_options>(_options));
}

void_result client::open(
    net::connection*      _connection,
    const attach_options& _options
)
{
    return smtp::client::open(
               _connection,
               internal::held<smtp::attach_options>(_options));
}

#if (D_MOVE_ENABLED == 1)
result<client> client::connect(
    const client_options& _options
)
{
    client            opened;
    const void_result outcome = opened.open(_options);

    // the failure, or the open client
    if (!outcome.has_value())
    {
        return ::re_std::unexpected<failure>(outcome.error());
    }

    return result<client>(static_cast<client&&>(opened));
}

result<client> client::attach(
    std::unique_ptr<net::connection> _connection,
    const attach_options&            _options
)
{
    client            opened;
    const void_result outcome = opened.open(_connection.release(),
                                            _options);

    // the failure, or the open client
    if (!outcome.has_value())
    {
        return ::re_std::unexpected<failure>(outcome.error());
    }

    return result<client>(static_cast<client&&>(opened));
}
#endif  // D_MOVE_ENABLED

report_result send(
    const client_options& _options,
    const credentials&    _credentials,
    const envelope&       _envelope,
    re_std::string_view   _message
)
{
    return smtp::send(internal::held<smtp::client_options>(_options),
                      _credentials,
                      _envelope,
                      _message);
}

server::server(
    const server_options& _options,
    const server_handler& _handler
)
    : smtp::server(internal::held<smtp::server_options>(_options),
                   _handler)
{}


NS_END  // lmtp
NS_END  // net
NS_END  // djinterp

#endif  // D_INTERNAL_NET_HAS_INT64
