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

#include "TestMsgHandler.h"

#include <type_traits>
#include <iostream>
#include <cassert>
#include <iterator>

template <typename TStack>
void TestMsgHandler::processOutputInternal(TStack& stack, const DataBuf& data)
{
    typedef typename std::decay<decltype(stack)>::type StackType;
    typedef typename StackType::ReadIterator ReadIter;
    typedef typename StackType::MsgPtr MsgPtr;

    ReadIter iter = &data[0];
    MsgPtr msg;

    auto es = stack.read(msg, iter, data.size());
    static_cast<void>(es);
    if (es != comms::ErrorStatus::Success) {
        std::cout << "es=" << (unsigned)es << ": Output buffer: " << std::hex;
        std::copy(data.begin(), data.end(), std::ostream_iterator<unsigned>(std::cout, " "));
        std::cout << std::dec << std::endl;
    }
    assert(es == comms::ErrorStatus::Success);
    assert(msg);
    assert((unsigned)std::distance(ReadIter(&data[0]), iter) == data.size());
    msg->dispatch(*this);
}


template <typename TStack, typename TMsg>
TestMsgHandler::DataBuf TestMsgHandler::prepareInputInternal(TStack& stack, const TMsg& msg)
{
    typedef typename std::decay<decltype(stack)>::type StackType;
    typedef typename StackType::WriteIterator WriteIter;

    DataBuf buf;
    buf.resize(stack.length(msg));

    WriteIter iter = &buf[0];
    auto es = stack.write(msg, iter, buf.size());
    static_cast<void>(es);
    assert(es == comms::ErrorStatus::Success);
    assert(buf.size() == (unsigned)(std::distance(WriteIter(&buf[0]), iter)));
    return buf;
}

TestMsgHandler::TestMsgHandler() = default;
TestMsgHandler::~TestMsgHandler() = default;

TestMsgHandler::GwinfoMsgHandlerFunc
TestMsgHandler::setGwinfoMsgHandler(GwinfoMsgHandlerFunc&& func)
{
    GwinfoMsgHandlerFunc old(std::move(m_gwInfoMsgHandler));
    m_gwInfoMsgHandler = std::move(func);
    return old;
}

TestMsgHandler::ConnectMsgHandlerFunc
TestMsgHandler::setConnectMsgHandler(ConnectMsgHandlerFunc&& func)
{
    ConnectMsgHandlerFunc old(std::move(m_connectMsgHandler));
    m_connectMsgHandler = std::move(func);
    return old;
}

TestMsgHandler::ConnackMsgHandlerFunc TestMsgHandler::setConnackMsgHandler(
    ConnackMsgHandlerFunc&& func)
{
    ConnackMsgHandlerFunc old(std::move(m_connackMsgHandler));
    m_connackMsgHandler = std::move(func);
    return old;
}

void TestMsgHandler::handle(GwinfoMsg_SN& msg)
{
    if (m_gwInfoMsgHandler) {
        m_gwInfoMsgHandler(msg);
    }
}

void TestMsgHandler::handle(ConnackMsg_SN& msg)
{
    if (m_connackMsgHandler) {
        m_connackMsgHandler(msg);
    }
}

void TestMsgHandler::handle(TestMqttsnMessage& msg)
{
    std::cout << "Unhandled message sent to client: " << (unsigned)msg.getId() << std::endl;
    assert(!"Unhandled message");
}

void TestMsgHandler::handle(ConnectMsg& msg)
{
    if (m_connectMsgHandler) {
        m_connectMsgHandler(msg);
    }
}

void TestMsgHandler::handle(TestMqttMessage& msg)
{
    std::cout << "Unhandled message sent to broker: " << (unsigned)msg.getId() << std::endl;
    assert(!"Unhandled message");
}

void TestMsgHandler::processDataForClient(const DataBuf& data)
{
    processOutputInternal(m_mqttsnStack, data);
}

void TestMsgHandler::processDataForBroker(const DataBuf& data)
{
    processOutputInternal(m_mqttStack, data);
}

TestMsgHandler::DataBuf TestMsgHandler::prepareInput(const TestMqttsnMessage& msg)
{
    return prepareInputInternal(m_mqttsnStack, msg);
}

TestMsgHandler::DataBuf TestMsgHandler::prepareInput(const TestMqttMessage& msg)
{
    return prepareInputInternal(m_mqttStack, msg);
}

TestMsgHandler::DataBuf TestMsgHandler::prepareSearchgw(std::uint8_t radius)
{
    SearchgwMsg_SN msg;
    auto& fields = msg.fields();
    auto& radiusField = std::get<decltype(msg)::FieldIdx_radius>(fields);
    radiusField.value() = radius;
    return prepareInput(msg);
}

TestMsgHandler::DataBuf TestMsgHandler::prepareClientConnect(
    const std::string& id,
    std::uint16_t keepAlive,
    bool hasWill,
    bool clean)
{
    ConnectMsg_SN msg;
    auto& fields = msg.fields();
    auto& flagsField = std::get<decltype(msg)::FieldIdx_flags>(fields);
    auto& flagsMembers = flagsField.value();
    auto& midFlagsField = std::get<mqttsn::protocol::field::FlagsMemberIdx_midFlags>(flagsMembers);
    auto& durationField = std::get<decltype(msg)::FieldIdx_duration>(fields);
    auto& clientIdField = std::get<decltype(msg)::FieldIdx_clientId>(fields);

    clientIdField.value() = id;
    durationField.value() = keepAlive;
    midFlagsField.setBitValue(mqttsn::protocol::field::MidFlagsBits_will, hasWill);
    midFlagsField.setBitValue(mqttsn::protocol::field::MidFlagsBits_cleanSession, clean);
    return prepareInput(msg);
}

TestMsgHandler::DataBuf TestMsgHandler::prepareBrokerConnack(
    mqtt::message::ConnackResponseCode rc,
    bool sessionPresent)
{
    ConnackMsg msg;
    auto& fields = msg.fields();
    auto& flagsField = std::get<decltype(msg)::FieldIdx_Flags>(fields);
    auto& responseField = std::get<decltype(msg)::FieldIdx_Response>(fields);

    flagsField.setBitValue(0, sessionPresent);
    responseField.value() = rc;
    return prepareInput(msg);
}
