#ifndef DNS_PROTOCOL_H
#define DNS_PROTOCOL_H
/* dnsmasq is Copyright (c) 2000-2012 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991, or
   (at your option) version 3 dated 29 June, 2007.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
     
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * DNS Protocol Header
 * 
 * file from dnsmasq/src/dns-protocol.h with slight modification.
 */

#define NAMESERVER_PORT 53
#define TFTP_PORT       69

#define IN6ADDRSZ       16
#define INADDRSZ        4

#define PACKETSZ	512		/* maximum packet size */
#define MAXDNAME	1025		/* maximum presentation domain name */
#define RRFIXEDSZ	10		/* #/bytes of fixed data in r record */
#define MAXLABEL        63              /* maximum length of domain label */

#define NOERROR		0		/* no error */
#define FORMERR		1		/* format error */
#define SERVFAIL	2		/* server failure */
#define NXDOMAIN	3		/* non existent domain */
#define NOTIMP		4		/* not implemented */
#define REFUSED		5		/* query refused */

#define QUERY           0               /* opcode */

#define C_IN            1               /* the arpa internet */
#define C_CHAOS         3               /* for chaos net (MIT) */
#define C_ANY           255             /* wildcard match */

#define T_A		1
#define T_NS            2               
#define T_CNAME		5
#define T_SOA		6
#define T_PTR		12
#define T_MX		15
#define T_TXT		16
#define T_SIG		24
#define T_AAAA		28
#define T_SRV		33
#define T_NAPTR		35
#define T_OPT		41
#define	T