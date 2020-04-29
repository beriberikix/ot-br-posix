/*
 *    Copyright (c) 2019, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *    POSSIBILITY OF SUCH DAMAGE.
 */

#include "common/logging.hpp"

#include "dbus/common/dbus_message_helper.hpp"
#include "dbus/common/dbus_resources.hpp"
#include "dbus/server/error_helper.hpp"

namespace otbr {
namespace DBus {

/**
 * This class represents a incoming call for a d-bus method.
 *
 */
class DBusRequest
{
public:
    /**
     * The constructor of dbus request.
     *
     * @param[in]   aConnection   The dbus connection.
     * @param[in]   aMessage      The incoming dbus message.
     *
     */
    DBusRequest(DBusConnection *aConnection, DBusMessage *aMessage)
        : mConnection(aConnection)
        , mMessage(aMessage)
    {
        dbus_message_ref(aMessage);
        dbus_connection_ref(aConnection);
    }

    /**
     * The copy constructor of dbus request.
     *
     * @param[in]   aOther    The object to be copied from.
     *
     */
    DBusRequest(const DBusRequest &aOther)
        : mConnection(nullptr)
        , mMessage(nullptr)
    {
        CopyFrom(aOther);
    }

    /**
     * The assignment operator of dbus request.
     *
     * @param[in]   aOther    The object to be copied from.
     *
     */
    DBusRequest &operator=(const DBusRequest &aOther)
    {
        CopyFrom(aOther);
        return *this;
    }

    /**
     * This method returns the message sent to call the d-bus method.
     *
     * @returns   The dbus message.
     *
     */
    DBusMessage *GetMessage(void) { return mMessage; }

    /**
     * This method returns underlying d-bus connection.
     *
     * @returns   The dbus connection.
     *
     */
    DBusConnection *GetConnection(void) { return mConnection; }

    /**
     * This method replies to the d-bus method call.
     *
     * @param[in] aReply  The tuple to be sent.
     *
     */
    template <typename... Args> void Reply(const std::tuple<Args...> &aReply)
    {
        UniqueDBusMessage reply{dbus_message_new_method_return(mMessage)};

        VerifyOrExit(reply != nullptr);
        VerifyOrExit(otbr::DBus::TupleToDBusMessage(*reply, aReply) == OTBR_ERROR_NONE);

        dbus_connection_send(mConnection, reply.get(), nullptr);

    exit:
        return;
    }

    /**
     * This method replies an otError to the d-bus method call.
     *
     * @param[in] aError  The error to be sent.
     *
     */
    void ReplyOtResult(otError aError)
    {
        UniqueDBusMessage reply{nullptr};
        auto              logLevel = (aError == OT_ERROR_NONE) ? OTBR_LOG_INFO : OTBR_LOG_ERR;

        otbrLog(logLevel, "Replied with result %s", ConvertToDBusErrorName(aError));
        if (aError == OT_ERROR_NONE)
        {
            reply = UniqueDBusMessage(dbus_message_new_method_return(mMessage));
        }
        else
        {
            reply = UniqueDBusMessage(dbus_message_new_error(mMessage, ConvertToDBusErrorName(aError), nullptr));
        }

        VerifyOrExit(reply != nullptr);
        dbus_connection_send(mConnection, reply.get(), nullptr);

    exit:
        return;
    }

    /**
     * The destructor of DBusRequest
     *
     */
    ~DBusRequest(void)
    {
        if (mConnection)
        {
            dbus_connection_unref(mConnection);
        }
        if (mMessage)
        {
            dbus_message_unref(mMessage);
        }
    }

private:
    void CopyFrom(const DBusRequest &aOther)
    {
        if (mMessage)
        {
            dbus_message_unref(mMessage);
        }
        if (mConnection)
        {
            dbus_connection_unref(mConnection);
        }
        mConnection = aOther.mConnection;
        mMessage    = aOther.mMessage;
        dbus_message_ref(mMessage);
        dbus_connection_ref(mConnection);
    }

    DBusConnection *mConnection;
    DBusMessage *   mMessage;
};

} // namespace DBus
} // namespace otbr