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
struct ExtraAdvertiseOptions
{
    typedef std::tuple<> Type;
};

template <>
struct ExtraAdvertiseOptions<true, false>
{
    typedef comms::option::NoWriteImpl Type;
};

template <>
struct ExtraAdvertiseOptions<false, true>
{
    typedef comms::option::NoReadImpl Type;
};

template <typename TOpts>
using ExtraAdvertiseOptionsT =
    typename ExtraAdvertiseOptions<TOpts::ClientOnlyVariant, TOpts::GatewayOnlyVariant>::Type;

}  // namespace details

template <typename TFieldBase>
using AdvertiseFields =
    std::tuple<
        field::GwId<TFieldBase>,
        field::Duration<TFieldBase>
    >;

template <typename TMsgBase, typename TOptions, template<class, class> class TActual>
using AdvertiseBase =
    comms::MessageBase<
        TMsgBase,
        comms::option::StaticNumIdImpl<MsgTypeId_ADVERTISE>,
        comms::option::FieldsImpl<AdvertiseFields<typename TMsgBase::Field> >,
        comms::option::MsgType<TActual<TMsgBase, TOptions> >,
        details::ExtraAdvertiseOptionsT<TOptions>
    >;

template <typename TMsgBase, typename TOptions = protocol::ParsedOptions<> >
class Advertise : public AdvertiseBase<TMsgBase, TOptions, Advertise>
{
//    typedef AdvertiseBase<TMsgBase, TOptions, mqttsn::protocol::message::Advertise> Base;

public:
    COMMS_MSG_FIELDS_ACCESS(gwId, duration);
};

}  // namespace message

}  // namespace protocol

}  // namespace mqttsn


