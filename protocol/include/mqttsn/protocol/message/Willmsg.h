//
// Copyright 2016 - 2017 (C). Alex Robenko. All rights reserved.
//

// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include "comms/comms.h"
#include "mqttsn/protocol/MsgTypeId.h"
#include "mqttsn/protocol/field.h"
#include "mqttsn/protocol/ParsedOptions.h"

namespace mqttsn
{

namespace protocol
{

namespace message
{


namespace details
{

template <bool TClientOnly, bool TGatewayOnly>
struct ExtraWillmsgOptions
{
    typedef std::tuple<> Type;
};

template <>
struct ExtraWillmsgOptions<true, false>
{
    typedef comms::option::NoReadImpl Type;
};

template <>
struct ExtraWillmsgOptions<false, true>
{
    typedef comms::option::NoWriteImpl Type;
};

template <typename TOpts>
using ExtraWillmsgOptionsT =
    typename ExtraWillmsgOptions<TOpts::ClientOnlyVariant, TOpts::GatewayOnlyVariant>::Type;

}  // namespace details


template <typename TFieldBase, typename TOptions>
using WillmsgFields =
    std::tuple<
        field::WillMsg<TFieldBase, TOptions>
    >;

template <typename TMsgBase, typename TOptions, template<class, class> class TActual>
using WillmsgBase =
    comms::MessageBase<
        TMsgBase,
        comms::option::StaticNumIdImpl<MsgTypeId_WILLMSG>,
        comms::option::FieldsImpl<WillmsgFields<typename TMsgBase::Field, TOptions> >,
        comms::option::MsgType<TActual<TMsgBase, TOptions> >,
        details::ExtraWillmsgOptionsT<TOptions>
    >;

template <typename TMsgBase, typename TOptions = ParsedOptions<> >
class Willmsg : public WillmsgBase<TMsgBase, TOptions, Willmsg>
{
//    typedef WillmsgBase<TMsgBase, TOptions, mqttsn::protocol::message::Willmsg> Base;

public:
    COMMS_MSG_FIELDS_ACCESS(willMsg);
};

}  // namespace message

}  // namespace protocol

}  // namespace mqttsn


