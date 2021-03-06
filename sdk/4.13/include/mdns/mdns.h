/*
 * Copyright (C) 2007-2011 cozybit Inc.
 * Copyright 2008-2015, Marvell International Ltd.
 * All Rights Reserved.
 *
 *
 * This file is part of libmdns.
 *
 * libmdns is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libmdns is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libmdns. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MDNS_H
#define MDNS_H

#define MDNS_LOG
#define MDNS_DBG


#include "hsf.h"
#include "mdns_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UINT32_MAX
#define UINT32_MAX	(4294967295U)
#endif

/* mdns control socket ports
 *
 * mdns uses two control sockets to communicate between the mdns threads and
 * any API calls.  This control socket is actually a UDP socket on the loopback
 * interface.  Developers who wish to specify certain ports for this control
 * socket can do so by changing MDNS_CTRL_RESPONDER and MDNS_CTRL_QUERIER.
 */
#ifndef MDNS_CTRL_RESPONDER
#define MDNS_CTRL_RESPONDER 12345
#endif

#ifndef MDNS_CTRL_QUERIER
#define MDNS_CTRL_QUERIER  (MDNS_CTRL_RESPONDER + 1)
#endif

/** Maximum length of labels
 *
 * A label is one segment of a DNS name.  For example, "foo" is a label in the
 * name "foo.local.".  RFC 1035 requires that labels do not exceed 63 bytes.
 */
#define MDNS_MAX_LABEL_LEN	63	/* defined by the standard */

/** Maximum length of names
 *
 * A name is a list of labels such as "My Webserver.foo.local" or
 * mydevice.local.  RFC 1035 requires that names do not exceed 255 bytes.
 */
#define MDNS_MAX_NAME_LEN	255	/* defined by the standard : 255*/

/** Maximum length of key/value pair
 *
 * TXT records associated with a service are populated with key/value pairs.
 * These key/value pairs must not exceed this length.
 */
#define MDNS_MAX_KEYVAL_LEN	255	/* defined by the standard : 255*/

/** mDNS Interface State
 * mDNS interface state can be changed by using mdns_iface_state_change()
 * function. For details about when to use the enum please refer to
 * documentation for mdns_iface_state_change(). */
enum iface_state {
	/** UP the interface and announce services
	 * mDNS will probe and announce all services announced via
	 * mdns_announce_service() and/or mdns_announce_service_arr().
	 * mDNS will go through entire probing sequence explained in above
	 * functions. Interface state can be changed to UP, if its DOWN.
	 */
	UP = 0,
	/** DOWN the interface and de-announce services
	 * mDNS sends good bye packet with ttl=0 so that mDNS clients can remove
	 * the services from their mDNS cache table.
	 */
	DOWN,
	/** Forcefully re-announce services
	 * This state should be used after services are already
	 * announced and force announcement is needed due to some reason.
	 * mDNS will not perform probing sequence, as it does in case of UP, and
	 * will directly re-announce services.
	 */
	REANNOUNCE
};

#define MDNS_SUCCESS 0

/** MDNS Error Codes */
enum mdns_errno {
	E_MDNS_ERRNO_START,
	/** invalid argument*/
	E_MDNS_INVAL,
	/** bad service descriptor*/
	E_MDNS_BADSRC,
	/** not enough room for everything*/
	E_MDNS_TOOBIG,
	/** unimplemented feature*/
	E_MDNS_NOIMPL,
	/** insufficient memory*/
	E_MDNS_NOMEM,
	/** requested resource is in use*/
	E_MDNS_INUSE,
	/** requested resource is in use*/
	E_MDNS_NORESP,
	/** failed to create socket for mdns*/
	E_MDNS_FSOC,
	/** failed to reuse multicast socket*/
	E_MDNS_FREUSE,
	/** failed to bind mdns socket to device*/
	E_MDNS_FBINDTODEVICE,
	/** failed to bind mdns socket*/
	E_MDNS_FBIND,
	/** failed to join multicast socket*/
	E_MDNS_FMCAST_JOIN,
	/** failed to set multicast socket*/
	E_MDNS_FMCAST_SET,
	/** failed to create query socket*/
	E_MDNS_FQUERY_SOC,
	/** failed to create mdns thread*/
	E_MDNS_FQUERY_THREAD,
};
/** service descriptor
 *
 * Central to mdns is the notion of a service.  Hosts advertise service types
 * such as a website, a printer, some custom service, etc.  Network users can
 * use an mdns browser to discover services on the network.  Internally, this
 * mdns implementation uses the following struct to describe a service.  These
 * structs can be created by a user, populated, and passed to mdns announce
 * functions to specify services that are to be advertised. When a user starts a
 * query for services, the discovered services are passed back to the user in
 * this struct.
 *
 * The members include:
 *
 * servname: string that is the service instance name that will be advertised.
 * It could be something like "Brian's Website" or "Special Service on Device
 * #123".  This is the name that is typically presented to users browsing for
 * your service.  The servname must not exceed MDNS_MAX_LABEL_LEN bytes.  The
 * MDNS specification allows servname to be a UTF8 string.  However, only the
 * ascii subset of UTF-8 has been tested.
 *
 * servtype: string that represents the service type.  This should be a type
 * registered at http://dns-sd.org/ServiceTypes.html.  For example, "http" is
 * the service type for a web server and "ssh" is for an ssh server.  You may
 * use an unregisterd service type during development, but not in released
 * products.  Consider registering any new service types at the aforementioned
 * webpage.  servtype must be non-NULL.
 *
 * domain: string that represents the domain. If this value is NULL, domain
 * name ".local" will be used. The domain must not exceed
 * \ref MDNS_MAX_LABEL_LEN bytes.
 *
 * port: the tcp or udp port on which the service named servname is available
 * in network byte order.
 *
 * proto: Either MDNS_PROTO_TCP or MDNS_PROTO_UDP depending on what protocol
 * clients should use to connect to the service servtype.
 *
 * keyvals: NULL-terminated string of colon-separated key=value pairs.  These
 * are the key/value pairs for the TXT record associated with a service type.
 * For example, the servtype "http" defines the TXT keys "u", "p", and "path"
 * for the username, password, and path to a document.  If you supplied all of
 * these, the keyvals string would be:
 *
 * "u=myusername:p=mypassword:path=/index.html"
 *
 * If keyvals is NULL, no TXT record will be advertised.  If keyvals is ":", a
 * TXT record will appear, but it will not contain any key/value pairs.  The
 * key must be present (i.e., two contiguous ':' characters should never appear
 * in the keyvals string.)  A key may appear with no value.  The interpretation
 * of this depends on the nature of the service.  The length of a single
 * key/value pair cannot exceed MDNS_MAX_KEYVAL_LEN bytes.
 * It is the responsibility of the application to verify that the keyval string
 * is a valid string. The keyval string buffer is used by the mDNS module
 * internally and it can modify it. Hence, during subsequent calls to the mDNS
 * module, it is possible that the original string has been messed up and needs
 * to be recreated.
 *
 * ipaddr, fqsn, ptrname, kvlen and flags are for internal use only and should
 * not be dereferenced by the user.
 */
struct mdns_service
{
	/** Name of MDNS service  */
	/*const */char *servname;
	/** Type of MDNS service */
	/*const */char *servtype;
	/** Domain for MDNS service */
	/*const */char *domain;
	/** Port number  */
	uint16_t port;
	/** Protocol used */
	int proto;
	/** Key value pairs for TXT records*/
	char *keyvals;
	/** IP Address of device */
	uint32_t ipaddr;

	/** The following members are for internal use only and should not be
	 * dereferenced by the user.
	 */
	uint8_t fqsn[MDNS_MAX_NAME_LEN];
	/** PTR record name */
	uint8_t *ptrname;
	/** Length of keyvals string*/
	uint16_t kvlen;
	/**  MDNS flags */
	uint32_t flags;
};

/** Maximum no. of services allowed to be announced on a single interface. */
#define MAX_MDNS_LST 5
/** Maximum no. of different interfaces supported by mDNS. */
#define MAX_INTERFACE 2

/* Total number of interface config supported by mdns */
#define MDNS_MAX_SERVICE_CONFIG 2

/** protocol values for the proto member of the mdns_service descriptor */
/** TCP Protocol */
#define MDNS_PROTO_TCP 0
/** UDP Protocol */
#define MDNS_PROTO_UDP 1

/** mdns_start
 *
 * Start the responder thread (and the querier thread if querying is enabled).
 *
 *
 * Note that the mdns_start() function must be called only after the network
 * stack is up.
 *
 * The responder thread wait for application's request to announce services.
 * Using mdns_announce_service() and mdns_announce_service_arr() call, services
 * are added to mDNS service list and are announced on a given interface.
 *
 * The querier thread just opens a socket and then waits until a request for
 * monitoring any given service type or subtype is received from the
 * application. When received, the same is queried and the responses are
 * processed, cached and monitored.
 * 
 * \param domain domain name string.  If this value is NULL, the domain ".local"
 * will be used. The domain must not exceed \ref MDNS_MAX_LABEL_LEN bytes.
 *
 * \param hostname string that is the hostname to resolve. This would be the
 * "foo" in "foo.local", for example.  The hostname must not exceed
 * \ref MDNS_MAX_LABEL_LEN bytes.  If hostname is NULL, the responder capability
 * will not be launched, only query support will be enabled.  This is useful if
 * only the query functionality is desired.
 *
 * \return SUCCESS for success or mdns error code
 *
 * NOTES:
 *
 * The domain and hostname must persist and remain unchanged between calls to
 * mdns_start and mdns_stop. Hence define these variables in global memory.
 *
 * While mdns_start returns immediately, the hostname and any servnames may
 * not be unique on the network.  In the event of a conflict, the names will
 * be appended with an integer.  For example, if the hostname "foo.local" is
 * taken, mdns will attempt to claim "foo-2.local", then foo-3.local, and so on
 * until the conflicts cease.  If mdns gets all the way to foo-9.local and
 * still fail, it waits for 5 seconds (per the mDNS specification) and then
 * starts back at foo.local.  If a developer anticipates a network to have many
 * of her devices, she should devise a sensible scheme outside of mdns to
 * ensure that the names are unique.
 */
int mdns_start(const char *domain, char *hostname);

/** 
 * mdns_stop
 *
 * Halt the mDNS responder thread (and querier thread if querying is enabled),
 * delete the threads and close the sockets
 *
 * Any services being monitored will be unmonitored.
 */
void mdns_stop(void);

/** Note Configuring CONFIG_MDNS_SERVICE_CACHE_SIZE in mDNS Querier
 *
 * This is a menuconfig option and can be configured from "make menuconfig" ->
 * Modules -> MDNS -> mDNS Full-Fledged Query(monitoring) API -> Maximum number
 * of service instances that can be monitored.
 *
 * Suppose CONFIG_MDNS_SERVICE_CACHE_SIZE is 16 and that a user has invoked
 * mdns_query_monitor to monitor services of type _http._tcp.local. Assume
 * the query callback handler returns SUCCESS for all the instances
 * discovered.
 *
 * Further,suppose that this particular domain has 17 instances of this type.
 * The first 16 instances to be discovered will result in 16 callbacks with the
 * status MDNS_DISCOVERED.  These instances will be cached and monitored for
 * updates, disappearance, etc.  When the 17th instance is discovered, the
 * callback will be called as usual, but the status will be MDNS_CACHE_FULL,
 * and the service will not be monitored.  While a best effort is made to
 * deliver all of the service information, the mdns_service may be incomplete.
 * Specifically, the ipaddr may be 0 and the service name may be "".  Further,
 * the callback may be called again if the 17th instance of the service
 * announces itself on the network again.  If one of the other services
 * disappears, the next announcement from the 17th instance will result in a
 * callback with status MDNS_DISCOVERED, and from that point forward it will be
 * monitored.
 *
 * So what's the "best" value for CONFIG_MDNS_SERVICE_CACHE_SIZE?  This depends
 * on the application and on the field in which the application is deployed.  If
 * a particular application knows that it will never see more than 6 instances
 * of a service, then 6 is a fine value for CONFIG_MDNS_SERVICE_CACHE_SIZE.  In
 * this case, callbacks with a status of MDNS_CACHE_FULL would represent a
 * warning or error condition.  Similarly, if an application cannot handle any
 * more than 10 instances of a service, then CONFIG_MDNS_SERVICE_CACHE_SIZE
 * should be 10 and callbacks with a status of MDNS_CACHE_FULL can be ignored.
 * If the maximum number of service instances is not known, and the application
 * retains its own state for each instance of a service, it may be able to use
 * that state to do the right thing when the callback status is MDNS_CACHE_FULL.
 *
 * For applications with constrained memory ,a point to note is that each
 * service instance requires little above 1K bytes. This should be considered
 * while deciding the CONFIG_MDNS_SERVICE_CACHE_SIZE.
 *
 * The default value of CONFIG_MDNS_SERVICE_CACHE_SIZE is set to 2.
 */
#define CONFIG_MDNS_SERVICE_CACHE_SIZE 2
#define CONFIG_MDNS_MAX_SERVICE_MONITORS 2

/** mdns query callback
 *
 * A user initiates a query for services by calling the mdns_query_monitor
 * function with a fully-qualified service type, an mdns_query_cb, and an
 * opaque argument.  When a service instance is discovered, the query callback
 * will be invoked with following arguments:
 *
 * \param data a void * that was passed to mdns_query_monitor().  This can be
 * anything that the user wants, such as pointer to a custom internal data
 * structure.
 *
 * \param s A pointer to the struct mdns_service that was discovered.  The struct
 * mdns_service is only valid until the callback returns.  So if attributes of
 * the service (such as IP address and port) are required by the user for later
 * use, they must be copied and preserved elsewhere.
 *
 * \param status A code that reports the status of the query.  It takes one of the
 * following values:
 *
 * MDNS_DISCOVERED: The mdns_service s has just been discovered on the network
 * and will be monitored by the mdns stack.
 *
 * MDNS_UPDATED: The mdns_service s, which is being monitored, has been updated
 * in some way (e.g., it's IP address has changed, it's key/value pairs have
 * changed.)
 *
 * MDNS_DISAPPEARED: The mdns_service has left the network.  This usually
 * happens when a service has shut down, or when it has stopped responding
 * queries.  Applications may also detect this condition by some means outside
 * of mdns, such as a closed TCP connection.
 *
 * MDNS_CACHE_FULL: The mdns_service has been discovered.  However, the number
 * of monitored service instances has exceeded CONFIG_MDNS_SERVICE_CACHE_SIZE.
 * So the returned mdns_service may not be complete.  See NOTES below on other
 * implications of an MDNS_CACHE_FULL status.
 *
 * NOTES:
 *
 * The query callback should return SUCCESS in the case where it has
 * discovered service of interest (MDNS_DISCOVERED, MDNS_UPDATED status). If
 * the callback return non-zero value for MDNS_DISCOVERED and MDNS_UPDATED
 * status codes, that particular service instance is not cached by the mDNS
 * querier. This is required as each cached service instance takes little above
 * 1KB memory and the device can't monitor large number of service instances.
 *
 * Callback implementers must take care to not make any blocking calls, nor to
 * call any mdns API functions from within callbacks.
 *
 */
typedef int (* mdns_query_cb)(void *data, const struct mdns_service *s,
			int status);

#define MDNS_DISCOVERED		1
#define MDNS_UPDATED		2
#define MDNS_DISAPPEARED	3
#define MDNS_CACHE_FULL		4

/** mdns_query_monitor
 *
 * Query for and monitor instances of a service
 *
 * When instances of the specified service are discovered, the specified
 * query callback is called as described above.
 *
 * \param fqst Pointer to a null-terminated string specifying the fully-qualified
 * service type.  For example, "_http._tcp.local" would query for all http
 * servers in the ".local" domain.
 *
 * \param cb an mdns_query_cb to be called when services matching the specified fqst
 * are discovered, are updated, or disappear.  cb will be passed the opaque
 * data argument described below, a struct mdns_service that represents the
 * discovered service, and a status code.
 *
 * \param data a void * that will passed to cb when services are discovered, are
 * updated, or disappear.  This can be anything that the user wants, such as
 * pointer to a custom internal data structure.
 *
 * \param iface Interface handle on which services are to be queried. Interface
 * handle can be obtained from net_get_sta_handle() or net_get_uap_handle()
 * function calls
 *
 * \return SUCCESS: the query was successfully launched.  The caller should expect
 * the mdns_query_cb to be invoked as instances of the specified service are
 * discovered.
 *
 * \return E_MDNS_INVAL: cb was NULL or fqst was not valid.
 *
 * \return -E_MDNS_NOMEM: CONFIG_MDNS_MAX_SERVICE_MONITORS is already being
 * monitored.  Either this value must be increased, or a service must be
 * unmonitored by calling mdns_query_unmonitor.
 *
 * \return -E_MDNS_INUSE: The specified service type is already being monitored by another
 * callback, and multiple callbacks per service are not supported.
 *
 * \return -E_MDNS_NORESP: No response from the querier.  Perhaps it was not launched or
 * it has crashed.
 *
 * Note: multiple calls to mdns_query_service_start are allowed.  This enables
 * the caller to query for more than just one service type.
 */
int mdns_query_monitor(char *fqst, mdns_query_cb cb, void *data, void *iface);

/** mdns_query_unmonitor
 *
 * Stop monitoring a particular service
 *
 * \param fqst The service type to stop monitoring, or NULL to unmonitor all
 * services.
 *
 * \note Suppose a service has just been discovered and is being processed
 * while the call to mdns_query_monitor is underway.  A callback may be
 * generated before the service is unmonitored.
 */
void mdns_query_unmonitor(char *fqst);

/** mdns_announce_service
 *
 * Announce single mDNS service on an interface
 *
 * This function checks validity of service and if service is invalid
 * then it will return with appropriate error code.
 * Function sends command on mDNS control socket for announcing service on
 * "iface" interface. mDNS then announces the service by following prescribed
 * set of steps mentioned below:
 *  -# Send probing packets for a given service or services 3 times, spaced 250
 *  ms apart.
 *  -# If a conflict is received for any service record, resolve them by
 *  appending a number to that particular record to make it unique. Go to step
 *  1.
 *  -# Announce the service or services.
 * In order to announce multiple services simultaneously, use
 * mdns_announce_service_arr(). This is recommended to reduce network
 * congestion. Maximum no. of services that can be announced per interface
 * is \ref MAX_MDNS_LST
 *
 * \param service Pointer to \ref mdns_service structure corresponding to
 * service to be announced. Structure must persist and remain unchanged between
 * calls to mdns_start and mdns_stop. Hence define this variable in global
 * memory.
 *
 * \param iface Pointer to interface handle on which service is to be announced.
 * Interface handle can be obtained from net_get_sta_handle or
 * net_get_uap_handle function calls.
 *
 * \return SUCCESS: success
 *
 * \return -E_MDNS_INVAL: input was invalid.  Perhaps a label exceeded
 * MDNS_MAX_LABEL_LEN, or a name composed of the supplied label exceeded
 * MDNS_MAX_NAME_LEN. Perhaps hostname was NULL and the query capability is not
 * compiled. Perhaps service pointer or iface pointer is NULL
 *
 * \return -E_MDNS_BADSRC: one of the service descriptors in the service
 * was invalid.  Perhaps key/val pair exceeded MDNS_MAX_KEYVAL_LEN
 *
 * \return -E_MDNS_TOOBIG: The combination of name information and service
 * descriptors does not fit in a single packet, which is required by this
 * implementation
 *
 * \return -FAIL: No space to add new interface or a new service in a given
 * interface.
 *
 * \return -E_MDNS_FSOC: failed to create loopback control socket
 *
 * \return -E_MDNS_FBIND: failed to bind control socket
 *
 */
int mdns_announce_service(struct mdns_service *service, void *iface);

/** mdns_deannounce_service
 *
 * Deannounce single service from an interface
 *
 * This function sends command on mDNS control socket to deannounce a given
 * service on "iface" interface. mDNS then de-announces the service by sending
 * a good bye packet with ttl=0 so that mDNS clients can remove the services
 * from their mDNS cache table. Function removes service from a list of services
 * associated with that interface.
 * In order to de-announce multiple services simultaneously, use
 * mdns_deannounce_service_arr(). This is recommended to reduce network
 * congestion.
 *
 * \param service Pointer to \ref mdns_service structure corresponding to
 * service to be de-announced
 *
 * \param iface Pointer to interface handle on which service is to be
 * de-announced. Interface handle can be obtained from net_get_sta_handle or
 * net_get_uap_handle function calls
 *
 * \return SUCCESS: success
 *
 * \return -E_MDNS_INVAL: invalid parameters. Perhaps service pointer or
 * iface pointer is NULL
 *
 * \return -E_MDNS_FSOC: failed to create loopback control socket
 *
 * \return -E_MDNS_FBIND: failed to bind control socket
 *
 */
int mdns_deannounce_service(struct mdns_service *service, void *iface);

/* mdns_announce_service_arr
 *
 * Announce multiple services simultaneously
 *
 * This function checks validity of all services in array and if any service is
 * invalid then it will return with appropriate error code.
 * Function sends command on mDNS control socket for announcing services in a
 * single mDNS packet on "iface" interface. mDNS follows same set of steps
 * as described in mdns_announce_service(), to announce given services.
 * Maximum no. of services that can be announced per interface is
 * \ref MAX_MDNS_LST. If no. of services in an array exceedes \ref MAX_MDNS_LST,
 * no services from given array are announced.
 *
 * \param services array of pointers to \ref mdns_service structure
 * corresponding to services to be announced. Array must persist and remain
 * unchanged between calls to mdns_start and mdns_stop. Hence define this
 * variable in global memory. Array should be NULL terminated.
 *
 * \param iface Pointer to interface handle on which service is to be announced.
 * Interface handle can be obtained from net_get_sta_handle or
 * net_get_uap_handle function calls.
 *
 * \return SUCCESS: success
 *
 * \return -E_MDNS_INVAL: input was invalid.  Perhaps a label exceeded
 * MDNS_MAX_LABEL_LEN, or a name composed of the supplied labels exceeded
 * MDNS_MAX_NAME_LEN. Perhaps hostname was NULL and the query capability is not
 * compiled. Perhaps services pointer or iface pointer is NULL
 *
 * \return -E_MDNS_BADSRC: one of the service descriptors in the services
 * list was invalid.  Perhaps one of the key/val pairs exceeded
 * MDNS_MAX_KEYVAL_LEN
 *
 * \return -E_MDNS_TOOBIG: The combination of name information and service
 * descriptors does not fit in a single packet, which is required by this
 * implementation
 *
 * \return -FAIL: No space to add new interface or services in a given
 * interface. Maximum no. of services that can be announced per interface is
 * \ref MAX_MDNS_LST
 *
 */
int mdns_announce_service_arr(struct mdns_service *services[], void *iface);

/* mdns_deannounce_service_arr
 *
 * Deannounce mulitple services simultaneously
 *
 * This function sends command on mDNS control socket to de-announce given set
 * of services in a single mDNS packet on "iface" interface.
 * mDNS then de-announce services as described in mdns_deannounce_service().
 * Function removes given set of services from a list of services associated
 * with that interface.
 *
 * \param services array of pointer to \ref mdns_service structure corresponding
 * to services to be de-announced
 *
 * \param iface Pointer to interface handle on which service is to be
 * de-announced. Interface handle can be obtained from net_get_sta_handle or
 * net_get_uap_handle function calls
 *
 * \return SUCCESS: success
 *
 * \return -E_MDNS_INVAL: invalid parameters. Perhaps services pointer or
 * iface pointer is NULL
 *
 * \return -E_MDNS_FSOC: failed to create loopback control socket
 *
 * \return -E_MDNS_FBIND: failed to bind control socket
 *
 */
int mdns_deannounce_service_arr(struct mdns_service *services[], void *iface);

/* mdns_deannounce_service_all
 *
 * Deannounce all registered services on a given interface
 *
 * This function sends command on mDNS control socket to de-announce all
 * services, in a single mDNS packet, announced on "iface" interface. mDNS then
 * de-announce services as described in mdns_deannounce_service(). Function
 * removes all services from a list of services associated with that interface.
 *
 * \param iface Pointer to interfae handle on which service is to be
 * de-announced. Interface handle can be obtained from net_get_sta_handle or
 * net_get_uap_handle function calls.
 */
int mdns_deannounce_service_all(void *iface);

/** mdns_iface_state_change
 *
 * Send interface state change event to mdns
 *
 * This will start or stop mdns state machine for given interface. Before
 * calling this function for first time, interface and services should be added
 * to mDNS using mdns_announce_service() or mdns_announce_service_arr(). For
 * \ref UP state, mdns will initially send init probes for all announced
 * services to check for name conflict in services and if
 * there are any conflicts it will resolve them by appending integer after
 * service name. After checking name conflicts, it will announce services
 * to multicast group. \ref DOWN state will send good bye packet corresponding
 * to all announced services on a given interface. Note that taking an interface
 * DOWN will not remove services from a list of services associated with that
 * interface. In order to do so, use mdns_deannounce_service() or
 * mdns_deannounce_service_arr() or mdns_deannounce_service_all(). \ref
 * REANNOUNCE will forcefully reannounce services. \ref REANNOUNCE do not send
 * init probes and directly re-announce services. Generally REANNOUNCE should be
 * used during DHCP change event or network up even after link lost event.
 *
 * \param iface interface handle
 *
 * \param state \ref UP, \ref DOWN or \ref REANNOUNCE
 *
 * \return SUCCESS: success
 *
 * \return -E_MDNS_FSOC: failed to create loopback control socket
 *
 * \return -E_MDNS_FBIND: failed to bind control socket
 *
 */
int mdns_iface_state_change(void  *iface, int state);

void mdns_set_hostname(char *hostname);
int mdns_stat(void);

#ifdef CONFIG_DNSSD_QUERY
/** dnssd_query_monitor
 *
 * Query for and monitor instances of a service published by unicast DNS server.
 *
 * When instances of the specified service are discovered, the specified
 * query callback is called as described above.
 *
 * \param fqst Pointer to a null-terminated string specifying the fully-
 * qualified service type.  For example, "_http._tcp.marvelltest" would query
 * for all http servers in the ".marvelltest" domain.
 *
 * \param cb an mdns_query_cb to be called when services matching the specified
 * fqst are discovered, are updated, or disappear.  callback will be passed the
 * opaque data argument described below, a struct mdns_service that represents
 * the discovered service, and a status code.
 *
 * \param dns_addr DNS IP address in form of struct in_addr. One can get the IP
 * from DNS string using dns_gethostbyname API.
 *
 * \param data a void * that will passed to cb when services are discovered,
 * are updated, or disappear.  This can be anything that the user wants, such
 * as pointer to a custom internal data structure.
 *
 * \return SUCCESS: the query was successfully launched.  The caller should
 * expect the mdns_query_cb to be invoked as instances of the specified service
 * are discovered.
 *
 * \return E_MDNS_INVAL: cb was NULL or fqst was not valid.
 *
 * \return -E_MDNS_NOMEM: CONFIG_MDNS_MAX_SERVICE_MONITORS is already being
 * monitored.  Either this value must be increased, or a service must be
 * unmonitored by calling mdns_query_unmonitor.
 *
 * \return -E_MDNS_INUSE: The specified service type is already being
 * monitored by another callback, and multiple callbacks per service are not
 * supported.
 *
 * \return -E_MDNS_NORESP: No response from the querier.  Perhaps it was not
 * launched or it has crashed.
 *
 * Note #1: Only single dnssd query monitor is allowed.
 * Note #2: Internally, state machines for DNS and mDNS queriers are same, hence
 * to save the footprint DNS and mDNS queriers are merged together.
 */
int dnssd_query_monitor(char *fqst, mdns_query_cb cb,
			struct in_addr dns_addr, void *data, void *iface);

/** dnssd_query_unmonitor
 *
 * Stop monitoring a particular DNS-SD service on unicast socket
 *
 * \param fqst The service type to stop monitoring, or NULL to unmonitor all
 * services.
 *
 * \note Suppose a service has just been discovered and is being processed
 * while the call to mdns_query_monitor is underway.  A callback may be
 * generated before the service is unmonitored.
 */
void dnssd_query_unmonitor(char *fqst);
#endif

/** Set mDNS TXT Records
 *
 * This function sets the TXT record field for a given service.
 * mDNS TXT record string is converted and stored in a format defined
 * by TXT resource record format.
 * This function MUST be called only before adding the service. If this
 * function is called while the mDNS service is already added, then the
 * resultant behavior can be erroneous.
 *
 * \param s Pointer to mDNS service
 *
 * \param recname The name of the character string which has been populated
 * with the keyvals as described in struct mdns_service
 *
 * \return SUCCESS on success
 *
 * \return -E_MDNS_TOOBIG if length of TXT record exceeds the permissible
 * limit as specified by \ref MDNS_MAX_KEYVAL_LEN
 *
 * \return -E_MDNS_INVAL if value of recname is invalid
 *
 */
int mdns_set_txt_rec(struct mdns_service *s, char *recname);


void *net_get_interf_handle(uint32_t index);

int mdnsd_gethostname(char *name);

#ifdef __cplusplus
}
#endif

#endif
