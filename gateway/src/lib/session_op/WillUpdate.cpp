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

#include "WillUpdate.h"

#include <cassert>
#include <algorithm>

namespace mqttsn
{

namespace gateway
{

namespace session_op
{

WillUpdate::WillUpdate(SessionState& sessionState)
  : Base(sessionState)
{
}

WillUpdate::~WillUpdate() = default;

void WillUpdate::tickImpl()
{
    if (m_op == Op::None) {
        return;
    }

    sendFailureAndTerm();
}

void WillUpdate::brokerConnectionUpdatedImpl()
{
    if (m_op == Op::None) {
        return;
    }

    auto& st = state();
    if ((st.m_brokerConnected) &&
        (m_reconnectRequested) &&
        (!m_brokerConnectSent)) {
        cancelTick();
        doNextStage();
        return;
    }
}

void WillUpdate::handle(ConnectMsg_SN& msg)
{
    static_cast<void>(msg);
    if (m_op != Op::None) {
        cancelOp();
    }
}

void WillUpdate::handle(DisconnectMsg_SN& msg)
{
    static_cast<void>(msg);
    if (m_op != Op::None) {
        cancelOp();
    }
}

void WillUpdate::handle(WilltopicupdMsg_SN& msg)
{
    if (state().m_connStatus != ConnectionStatus::Connected) {
        sendTopicResp(mqttsn::protocol::field::ReturnCodeVal_NotSupported);
        return;
    }

    if (m_op == Op::MsgUpd) {
        sendTopicResp(mqttsn::protocol::field::ReturnCodeVal_Congestion);
        sendToBroker(PingreqMsg());
        return;
    }

    if (m_op == Op::TopicUpd) {
        sendToBroker(PingreqMsg());
        return;
    }

    auto& midFlagsField = msg.field_flags().field().field_midFlags();
    typedef typename std::decay<decltype(midFlagsField)>::type MidFlags;

    auto qos = translateQos(msg.field_flags().field().field_qos().value());
    bool retain = midFlagsField.getBitValue(MidFlags::BitIdx_retain);

    auto& st = state();
    auto& willTopic = msg.field_willTopic().value();
    if ((st.m_will.m_topic == willTopic) &&
        (st.m_will.m_qos == qos) &&
        (st.m_will.m_retain == retain)) {
        sendTopicResp(mqttsn::protocol::field::ReturnCodeVal_Accepted);
        sendToBroker(PingreqMsg());
        return;
    }

    m_will.m_topic.assign(willTopic.begin(), willTopic.end());
    m_will.m_msg = st.m_will.m_msg;
    m_will.m_qos = qos;
    m_will.m_retain = retain;
    startOp(Op::TopicUpd);
}

void WillUpdate::handle(WillmsgupdMsg_SN& msg)
{
    if (state().m_connStatus != ConnectionStatus::Connected) {
        sendMsgResp(mqttsn::protocol::field::ReturnCodeVal_NotSupported);
        return;
    }

    if (m_op == Op::TopicUpd) {
        sendMsgResp(mqttsn::protocol::field::ReturnCodeVal_Congestion);
        sendToBroker(PingreqMsg());
        return;
    }

    if (m_op == Op::MsgUpd) {
        sendToBroker(PingreqMsg());
        return;
    }

    auto& st = state();
    auto& willData = msg.field_willMsg().value();
    using WillDataStorage = typename std::decay<decltype(willData)>::type;
    WillDataStorage storedDataView(&(*st.m_will.m_msg.begin()), st.m_will.m_msg.size());
    if (storedDataView == willData) {
        sendMsgResp(mqttsn::protocol::field::ReturnCodeVal_Accepted);
        sendToBroker(PingreqMsg());
        return;
    }

    m_will.m_topic = st.m_will.m_topic;
    m_will.m_msg.assign(willData.begin(), willData.end());
    m_will.m_qos = st.m_will.m_qos;
    m_will.m_retain = st.m_will.m_retain;
    startOp(Op::MsgUpd);
}

void WillUpdate::handle(ConnackMsg& msg)
{
    if (m_op == Op::None) {
        return;
    }

    using ResponseFieldType = typename std::decay<decltype(msg.field_responseCode())>::type;
    if ((msg.field_responseCode().value() != ResponseFieldType::ValueType::Accepted) ||
        (!msg.field_flags().getBitValue(0))) {
        sendFailureAndTerm();
        return;
    }

    state().m_will = m_will;
    sendResp(mqttsn::protocol::field::ReturnCodeVal_Accepted);
    cancelOp();
}

void WillUpdate::startOp(Op op)
{
    cancelTick();
    m_op = op;
    m_reconnectRequested = false;
    m_brokerConnectSent = false;
    doNextStage();
}

void WillUpdate::doNextStage()
{
    if (m_reconnectRequested) {
        assert(state().m_brokerConnected);
        sendConnectMsg();
        m_brokerConnectSent = true;
        nextTickReq(state().m_retryPeriod);
        return;
    }

    sendToBroker(DisconnectMsg());
    brokerReconnectRequest();
    m_reconnectRequested = true;
    nextTickReq(state().m_retryPeriod);
}

void WillUpdate::cancelOp()
{
    cancelTick();
    m_op = Op::None;
}

void WillUpdate::sendTopicResp(mqttsn::protocol::field::ReturnCodeVal rc)
{
    WilltopicrespMsg_SN msg;
    msg.field_returnCode().value() = rc;
    sendToClient(msg);
}

void WillUpdate::sendMsgResp(mqttsn::protocol::field::ReturnCodeVal rc)
{
    WillmsgrespMsg_SN msg;
    msg.field_returnCode().value() = rc;
    sendToClient(msg);
}

void WillUpdate::sendResp(mqttsn::protocol::field::ReturnCodeVal rc)
{
    assert(m_op != Op::None);
    if (m_op == Op::TopicUpd) {
        sendTopicResp(rc);
        return;
    }

    if (m_op == Op::MsgUpd) {
        sendMsgResp(rc);
        return;
    }
}

void WillUpdate::sendConnectMsg()
{
    ConnectMsg msg;

    auto& st = state();
    msg.field_clientId().value() = st.m_clientId;
    msg.field_keepAlive().value() = st.m_keepAlive;

    auto& flagsField = msg.field_flags();

    typedef typename std::decay<decltype(flagsField.field_flagsLow())>::type FlagsLowFieldType;
    typedef typename std::decay<decltype(flagsField.field_flagsHigh())>::type FlagsHighFieldType;

    if (!m_will.m_topic.empty()) {
        flagsField.field_flagsLow().setBitValue(FlagsLowFieldType::BitIdx_willFlag, true);
        msg.field_willTopic().field().value() = m_will.m_topic;
        msg.field_willMessage().field().value() = m_will.m_msg;
        flagsField.field_willQos().value() = translateQosForBroker(m_will.m_qos);
        flagsField.field_flagsHigh().setBitValue(FlagsHighFieldType::BitIdx_willRetain, m_will.m_retain);
    }

    if (!st.m_username.empty()) {
        msg.field_userName().field().value() = state().m_username;
        flagsField.field_flagsHigh().setBitValue(FlagsHighFieldType::BitIdx_username, true);

        if (!state().m_password.empty()) {
            msg.field_password().field().value() = state().m_password;
            flagsField.field_flagsHigh().setBitValue(FlagsHighFieldType::BitIdx_password, true);
        }
    }

    msg.doRefresh();
    sendToBroker(msg);
}

void WillUpdate::sendFailureAndTerm()
{
    sendResp(mqttsn::protocol::field::ReturnCodeVal_NotSupported);
    cancelOp();
    sendDisconnectToClient();
    termRequest();
}

}  // namespace session_op

}  // namespace gateway

}  // namespace mqttsn



