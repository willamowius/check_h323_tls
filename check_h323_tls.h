/*
 * check_h323_tls.h - A H.323 over TLS monitoring plugin
 *
 * License: GPL
 *
 * (c) Relaxed Communications GmbH, 2021
 *     jan@willamowius.de, https://www.willamowius.com
 *
 */

#ifndef CHECK_H323_TLS_H
#define CHECK_H323_TLS_H

#include <ptlib.h>
#include <ptlib/sockets.h>
#include <h323.h>

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define BUILD_TYPE ReleaseCode
#define BUILD_NUMBER 0

class CheckH323TLSEndpoint : public H323EndPoint
{
public:
    CheckH323TLSEndpoint();
    ~CheckH323TLSEndpoint();

#ifdef H323_TLS
    virtual void OnSecureSignallingChannel(bool isSecured);
#endif
    virtual void OnGatekeeperConfirm();
    virtual void OnGatekeeperReject();
    virtual void OnRegistrationReject();

    PBoolean Initialise(PArgList &);

protected:
    PString host;
    PTime startTime;
};

class CheckH323TLSProcess : public PProcess
{
public:
    CheckH323TLSProcess();
    ~CheckH323TLSProcess();

    void Main();

protected:
    CheckH323TLSEndpoint *endpoint;
};

#endif
