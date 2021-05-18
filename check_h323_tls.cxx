/*
 * check_h323_tls.cxx - A H.323 over TLS monitoring plugin
 *
 * License: GPL
 *
 * (c) Relaxed Communications GmbH, 2021
 *     jan@willamowius.de, https://www.willamowius.com
 *
 */

#include <ptlib.h>
#include "check_h323_tls.h"

#ifndef H323_H46017
#error("H.460.17 support required");
#endif
#ifndef H323_TLS
#error("TLS support required");
#endif

PCREATE_PROCESS(CheckH323TLSProcess);

CheckH323TLSProcess::CheckH323TLSProcess()
    : PProcess("CheckH323TLS", "check_h323_tls", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
    endpoint = NULL;
}

CheckH323TLSProcess::~CheckH323TLSProcess()
{
    delete endpoint;
}

void CheckH323TLSProcess::Main()
{
    PArgList & args = GetArguments();
    args.Parse(
        "h-help."
        "i-interface:"
        "-tls-cafile:"
        "-tls-cert:"
        "-tls-privkey:"
        "-tls-passphrase:"
        "-tls-listenport:"
        "t-trace."
        "o-output:"
        "p-password:"
        "x-listenport:"
        "u-user:",
        FALSE);

    if (args.GetCount() != 1 || args.HasOption('h'))
    {
        cout << "Usage : " << GetName() << " [options] host\n"
                                           "Options:\n"
                                           "  -i --interface ipnum    : Select interface to bind to.\n"
                                           "     --tls-cafile         : TLS Certificate Authority File.\n"
                                           "     --tls-cert           : TLS Certificate File.\n"
                                           "     --tls-privkey        : TLS Private Key File.\n"
                                           "     --tls-passphrase     : TLS Private Key PassPhrase.\n"
                                           "     --tls-listenport     : TLS listen port (default: 0).\n"
                                           "  -t --trace              : Enable trace, use multiple times for more detail.\n"
                                           "  -o --output             : File for trace output.\n"
                                           "  -p --password pw        : Gatekeeper Password.\n"
                                           "  -x --listenport         : Listening port (default 61720).\n"
                                           "  -h --help               : This help message.\n"
             << endl;
        return;
    }

    if (args.GetOptionCount('t') > 0)
    {
        PTrace::Initialise(args.GetOptionCount('t'),
                           args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
                           PTrace::DateAndTime | PTrace::TraceLevel | PTrace::FileAndLine);
    }

    endpoint = new CheckH323TLSEndpoint;
    if (!endpoint->Initialise(args))
        return;
}

CheckH323TLSEndpoint::CheckH323TLSEndpoint()
{
}

CheckH323TLSEndpoint::~CheckH323TLSEndpoint()
{
}

PBoolean CheckH323TLSEndpoint::Initialise(PArgList & args)
{
    host = args.GetParameter(0);

    if (args.HasOption('u'))
    {
        PStringArray aliases = args.GetOptionString('u').Tokenise(" ,;\n");
        SetLocalUserName(aliases[0]);
        for (PINDEX i = 1; i < aliases.GetSize(); i++)
            AddAliasName(aliases[i]);
    }
    else {
        SetLocalUserName("check_h323_tls");
    }

    // Load the base featureSet
    LoadBaseFeatureSet();

#ifdef H323_H46018
    H46018Enable(PFalse);
#endif
#ifdef H323_H46023
    H46023Enable(false);
#endif

    PString iface = args.GetOptionString('i');
    PString listenPort = args.GetOptionString('x');

    if (listenPort.IsEmpty())
        listenPort = "61720"; // we never use the listen port, try not to conflict with other H.323 software

#if PTLIB_VER >= 2110
    if (iface.IsEmpty())
    {
        PIPSocket::InterfaceTable interfaceTable;
        if (PIPSocket::GetInterfaceTable(interfaceTable))
        {
            for (PINDEX j = 0; j < interfaceTable.GetSize(); j++)
            {
                if (interfaceTable[j].GetAddress().IsLoopback())
                {
                    iface = interfaceTable[j].GetAddress().AsString();
                    break;
                }
            }
        }
    }
#endif

    PIPSocket::Address interfaceAddress(iface);
    WORD interfacePort = (WORD)listenPort.AsInteger();

    H323ListenerTCP *listener = new H323ListenerTCP(*this, interfaceAddress, interfacePort);

    if (iface.IsEmpty())
        iface = "*";
    if (!StartListener(listener))
    {
        cout << "CRITICAL - Can not open listener" << endl;
        _exit(1);
        return FALSE;
    }

    DisableH245Tunneling(false);                                        // Tunneling must be used with TLS
    TLS_SetCipherList("ALL:!LOW:!EXP:!MD5:!RC4:@STRENGTH:@SECLEVEL=1"); // set to level 1 so OpenSSL 1.1.0 will allow SHA1
    if (args.HasOption("tls-cafile") && !TLS_SetCAFile(args.GetOptionString("tls-cafile")))
    {
        cout << "CRITICAL - Can not load TLS CA file" << endl;
        _exit(1);
    }
    if (args.HasOption("tls-cert") && !TLS_SetCertificate(args.GetOptionString("tls-cert")))
    {
        cout << "CRITICAL - Can not load TLS certifiate" << endl;
        _exit(1);
    }
    if (args.HasOption("tls-privkey"))
    {
        PString passphrase = PString();
        if (args.HasOption("tls-passphrase"))
            passphrase = args.GetOptionString("tls-passphrase");
        if (!TLS_SetPrivateKey(args.GetOptionString("tls-privkey"), passphrase))
        {
            cout << "CRITICAL - Can not load TLS private key" << endl;
            _exit(1);
        }
    }
    WORD tlsListenPort = (WORD)args.GetOptionString("tls-listenport", "0").AsUnsigned(); // we will never use this listen port

    if (!TLS_Initialise(interfaceAddress, tlsListenPort))
    {
        cout << "CRITICAL - Can not enable TLS" << endl;
        _exit(1);
    }

    if (args.HasOption('p'))
    {
        SetGatekeeperPassword(args.GetOptionString('p'));
    }

    startTime = PTime();

    if (H46017CreateConnection(host, false))
    {
        double responseTime = PTime().GetMicrosecond() - startTime.GetMicrosecond();
        cout << "OK - Registered with " << host << " | responseTime=" << abs(responseTime / (1000 * 1000)) << "s"<< endl;
        _exit(0);
    }
    cout << "CRITICAL - Could not register with " << host << endl;
    _exit(1);
}

void CheckH323TLSEndpoint::OnSecureSignallingChannel(bool isSecured)
{
    if (!isSecured)
    {
        cout << "CRITICAL - Can not enable TLS" << endl;
        _exit(1);
    }
}

void CheckH323TLSEndpoint::OnGatekeeperConfirm()
{
    double responseTime = PTime().GetMicrosecond() - startTime.GetMicrosecond();
    cout << "OK - GatekeeperConfirm from " << host << " | responseTime=" << abs(responseTime / (1000 * 1000)) << "s"<< endl;
    _exit(0);
}

void CheckH323TLSEndpoint::OnGatekeeperReject()
{
    double responseTime = PTime().GetMicrosecond() - startTime.GetMicrosecond();
    cout << "OK - GatekeeperReject from " << host << " | responseTime=" << abs(responseTime / (1000 * 1000)) << "s"<< endl;
    _exit(0);
}

void CheckH323TLSEndpoint::OnRegistrationReject()
{
    double responseTime = PTime().GetMicrosecond() - startTime.GetMicrosecond();
    cout << "OK - RegistrationReject from " << host << " | responseTime=" << abs(responseTime / (1000 * 1000)) << "s"<< endl;
    _exit(0);
}
