//
// Copyright 2016 (C). Alex Robenko. All rights reserved.
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

#include <tuple>

#include "ParsedOptions.h"
#include "message/Advertise.h"
#include "message/Searchgw.h"
#include "message/Gwinfo.h"
#include "message/Connect.h"
#include "message/Connack.h"
#include "message/Willtopicreq.h"
#include "message/Willtopic.h"
#include "message/Willmsgreq.h"
#include "message/Willmsg.h"
#include "message/Register.h"
#include "message/Regack.h"
#include "message/Publish.h"
#include "message/Puback.h"
#include "message/Pubcomp.h"
#include "message/Pubrec.h"
#include "message/Pubrel.h"
#include "message/Subscribe.h"
#include "message/Suback.h"
#include "message/Unsubscribe.h"
#include "message/Unsuback.h"
#include "message/Pingreq.h"
#include "message/Pingresp.h"
#include "message/Disconnect.h"
#include "message/Willtopicupd.h"
#include "message/Willtopicresp.h"
#include "message/Willmsgupd.h"
#include "message/Willmsgresp.h"

namespace mqttsn
{

namespace protocol
{

template <typename TMsgBase, typename TOptions = ParsedOptions<> >
using AllMessages =
    std::tuple<
        message::Advertise<TMsgBase, TOptions>,
        message::Searchgw<TMsgBase, TOptions>,
        message::Gwinfo<TMsgBase, TOptions>,
        message::Connect<TMsgBase, TOptions>,
        message::Connack<TMsgBase, TOptions>,
        message::Willtopicreq<TMsgBase, TOptions>,
        message::Willtopic<TMsgBase, TOptions>,
        message::Willmsgreq<TMsgBase, TOptions>,
        message::Willmsg<TMsgBase, TOptions>,
        message::Register<TMsgBase, TOptions>,
        message::Regack<TMsgBase>,
        message::Publish<TMsgBase, TOptions>,
        message::Puback<TMsgBase>,
        message::Pubcomp<TMsgBase>,
        message::Pubrec<TMsgBase>,
        message::Pubrel<TMsgBase>,
        message::Subscribe<TMsgBase, TOptions>,
        message::Suback<TMsgBase, TOptions>,
        message::Unsubscribe<TMsgBase, TOptions>,
        message::Unsuback<TMsgBase, TOptions>,
        message::Pingreq<TMsgBase, TOptions>,
        message::Pingresp<TMsgBase>,
        message::Disconnect<TMsgBase>,
        message::Willtopicupd<TMsgBase, TOptions>,
        message::Willtopicresp<TMsgBase, TOptions>,
        message::Willmsgupd<TMsgBase, TOptions>,
        message::Willmsgresp<TMsgBase, TOptions>
    >;

}  // namespace protocol

}  // namespace mqttsn


