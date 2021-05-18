check_h323_tls - A Nagios/Icinga/Docker plugin to monitor H.323 over TLS devices
================================================================================

Author: Jan Willamowius <jan@willamowius.de>
		Relaxedcommunications GmbH
		https://www.willamowius.com/

License: GPL (https://www.gnu.org/copyleft/gpl.html)

To compile use H323Plus and PTLib and say "make optnoshared".

Technically we are using "H.323 over H.460.17 over TLS" here, as supported by
Innovaphone as "H.323/TLS" or in the GNU Gatekeeper with H.460.17 and TLS enabled.

Usage: check_h323_tls [options] host

Without options, the plugin will establish a TLS connection without a TLS certificate
of it's own.  If the server requires a client certificate (like Innovaphone servers
or GnuGk with RequireRemoteCertificate=1), you need to set it with --tls-cert,
--tls-privkey and --tls-passphrase.

Example:
check_h323_tls --tls-cert /path/to/cert.pem --tls-privkey /path/to/key.pem --tls-passphrase secret 1.2.3.4

Options:
     --tls-cafile         : TLS Certificate Authority File.
     --tls-cert           : TLS Certificate File.
     --tls-privkey        : TLS Private Key File.
     --tls-passphrase     : TLS Private Key PassPhrase.
     --tls-listenport     : TLS listen port (default: 0).
  -i --interface ipnum    : Select interface to bind to.
  -x --listenport         : Listening port (default 61720).
  -t --trace              : Enable trace, use multiple times for more detail.
  -o --output             : File for trace output.
  -p --password pw        : Gatekeeper Password.
  -h --help               : This help message.


As Docker health check:

HEALTHCHECK CMD /usr/local/bin/check_h323_tls 127.0.0.1

