# Limiting the Number of Concurrent Connections per User

This is an example of how to limit the total number of connections per user to multiple services using HAProxy. In this example, the connections are limited to iRODS and a WebDAV service that sits in front of iRODS.

---
_**NOTE**_

_CyVerse does not currently limit the number of concurrent connections per user, but they plans to in the future. Currently, they limit the number of concurrent connections per IP address and only for connections to iRODS._

---

## Example Overview

This example HAProxy configuration will proxy for both iRODS and WebDAV. It will limit the total number of client user connections to no more than `5`. The local iRODS zone is `tempZone`, but it may be federated with some remote zones.

To keep the example simple, there is a single HAPRoxy server that is configured to act as a proxy for a single iRODS catalog service provider and a single WebDAV server. The proxy's hostname is `data.example.org`.

The iRODS catalog provider does not host any storage resources. They are hosted by separate catalog service consumers. The provider supports a maximum of `200` concurrent connections. It doesn't have a hostname, but its IP address is `10.0.0.1`. Finally, it uses `1247/tcp` as its zone port and the port range `20000-20199/tcp` for reconnections.

The WebDAV service is implemented by apache using the davrods module. It is configured to listen only HTTP requests on port `80/tcp`, no HTTPS requests. TLS termination is handled by the proxy. WebDAV doesn't have a hostname either, but its IP address is `10.0.1.1`. The TLS PEM file used by the proxy for TLS termination is stored on the proxy at `/etc/ssl/private/example.org.pem`.

1. Configure proxy for iRODS with no concurrency limits

   To keep this example relatively simple, we'll configure a combined frontend and backend in a `listen` block for iRODS. The proxy forwards connections on the zone port and reconnection ports. To budget for connections from the WebDAV service to iRODS, the proxy allows a maximum of `100` connections through to iRODS.

   ```haproxy
   listen irods
      mode    tcp
      bind    :1247,:20000-20199
      server  csp 10.0.0.1 maxconn 100
   ```

1. Configure proxy for WebDAV with no concurrency limits

   To keep with the theme of relative simplicity, we'll also configure a combined frontend and backend for the WebDAV service as well. The proxy is configured to handle TLS termination on port `443/tcp`. Since WebDAV connects to iRODS, the proxy allows a maximum of `100` connections through to WebDAV. This will leave at least 100 connections open for direct iRODS connections.

   ```haproxy
   listen webdav
      mode    http
      bind    :443 ssl crt /etc/ssl/private/example.org.pem
      server  dav 10.0.1.1 port 80 maxconn 100
   ```

1. Create stick table to track the iRODS identities of the the clients currently connected.

   A client iRODS identity is an iRODS user account name along with the account's authentication zone. If a client's account name is `client` and the authentication zone is `zone`, using a common convention, the client identity is the account concatenated with a `#` followed by the zone name, e.g., `client#zone`. From inspecting the ICAT DB schema, the maximum length for both an account and zone name is 250 characters. Including the `#` separator, the maximum client identity length is 501 characters.

   The proxy is configured to have a stick table to store the `conn_cur` measure. This measure tracks the number of active connections that have the same key. In this case, the key is client identity. Since the client identity is a string of at most 501 characters, the table is configured strings of length `600` as its keys. Since at most 100 direct iRODS connections and 100 WebDAV connections can happen at a time, the table is configured to allocate space for at most `300` entries.

   Concurrency tracking needs to be shared by the iRODS and WebDAV services. To do this, the stick table is placed in its own backend.

   ```haproxy
   backend concurrency_st
      stick-table  type string len 600 size 300 store conn_cur
   ```

   HAProxy uses an Access Control List (ACL) to make decisions based on some measure. An ACL named `too-many-conn` is defined that will evaluate to `true` when the number of connections for client being tested is greater than `5`. The `sc1` counter is used based on recommended practice.

   ```haproxy
   acl  too-many-conn sc1_conn_cur gt 5
   ```

1. Modify iRODS proxy configuration to enforce the client concurrency limit.

   The proxy needs to be configured to extract the client identity from an iRODS connection message. When a client initiates a connection with iRODS a connection message is sent. This message has the form _\[`####`\]\[Header\]\[Startup Packet\]_, where _`####`_ is a four byte binary integer holding the length of _Header_, _Header_ is a `MsgHeader_PI` XML element, and _Startup Packet_ is a `StartupPack_PI` XML element. `StartupPack_PI` has string elements `clientUser` and `clientRcatZone` that hold the client's account name and its authentication zone, respectively. It should look something like the following.

   ```xml
   ####<MsgHeader_PI>
      ...
   </MsgHeader_PI><StartupPack_PI>
      ...
      <clientUser>user</clientUser>
      ...
      <clientRcatZone>zone</clientRcatZone>
      ...
   </StartupPack_PI>
   ```

   `tcp-request content  <action> [if <condition>]` operations are used to work with the contents of a TCP request. Multiple of these operations can be defined in sequence to define a sequence of oprations to perform on a request's contents.

   When working with request contents, the proxy needs to be configured to give up waiting for the contents to arrive to avoid waiting indefinitely for lost data. The `tcp-request inspect-delay` operation is used to set a timeout. This example chooses to wait `5` seconds.

   The `tcp-request content` action `capture <sample> len <length>` is used to to extract information from a TCP request body. The expression `<sample>` is used to extract the sample, converting to a string of at most length `<length>`. `<sample>` has the form `<fetch>[,<converter>]*`, where `<fetch>` is a operation that retrieves a data from the buffer contents, and `<converter>` transforms the output of the `<fetch>` or `<converter>` immediately preceding it. Converters form a filter chain for a fetch. The `capture` stores the sample in the variable `capture.req.hdr(<n>)` where `<n>` is the (n+1)<sup>th</sup> `capture` performed.

   A regular expression can be used to extract the client user name from the TCP request after discarding the initial, binary four bytes. The fetch `req.payload(4,0)` retrieves the request content starting after the first `4` bytes. The converter `regsub([\s\S]*<StartupPack_PI\s*>[\s\S]*<clientUser\s*>,)` removes the text before the client user name, and the converter `regsub(</clientUser\s*>[\s\S]*,)` removes the text after the user name. Since, a user name is at most 250 characters long, we limit the sample to `250` in length. Here's the full `tcp-request content` `capture`.

   ```haproxy
   tcp-request content  capture req.payload(4,0),regsub([\s\S]*<StartupPack_PI\s*>[\s\S]*<clientUser\s*>,),regsub(</clientUser\s*>[\s\S]*,) len 250
   ```

   A very similar `capture` can be performed to extract the client's authentication zone.

   ```haproxy
   tcp-request content  capture req.payload(4,0),regsub([\s\S]*<StartupPack_PI\s*>[\s\S]*<clientRcatZone\s*>,),regsub(</clientRcatZone\s*>[\s\S]*,) len 250
   ```

   The client identity is created from the captured name and zone in `capture.req.hdr(0)` and `capture.req.hdr(1)`, respectively, using the `concat` converter. For clarity, the captured name and zone are stored in the session variables `sess.user` and `sess.zone` before concatenating them using the expression `var(sess.user),concat(\#,sess.zone,)`. The resulting client identity is then stored in the session variable `sess.id`. All of this is done in the following logic.

   ```haproxy
   tcp-request content  set-var(sess.user) capture.req.hdr(0)
   tcp-request content  set-var(sess.zone) capture.req.hdr(1)
   tcp-request content  set-var(sess.id) var(sess.user),concat(\#,sess.zone,)
   ```

   The client identity is added to the `concurrency_st` stick table for tracking using the `sc1` counter by the following operation.

   ```haproxy
   tcp-request content  track-sc1 var(sess.id) table concurrency_st
   ```

   Finally, the following operation uses the `too-many-conn` ACL to reject the connection if this client has reached their concurrent connection limit.

   ```haproxy
   tcp-request content  reject if too-many-conn
   ```

   Here's the complete definition of the concurrency limit enforcing iRODS proxy.

   ```haproxy
   listen irods
      mode                       tcp
      bind                       :1247,:20000-20199
      server                     irods 10.0.0.1 maxconn 100
      acl                        too-many-conn sc1_conn_cur gt 5
      tcp-request inspect-delay  5s
      tcp-request content        capture req.payload(4,0),regsub([\s\S]*<StartupPack_PI\s*>[\s\S]*<clientUser\s*>,),regsub(</clientUser\s*>[\s\S]*,) len 250
      tcp-request content        capture req.payload(4,0),regsub([\s\S]*<StartupPack_PI\s*>[\s\S]*<clientRcatZone\s*>,),regsub(</clientRcatZone\s*>[\s\S]*,) len 250
      tcp-request content        set-var(sess.user) capture.req.hdr(0)
      tcp-request content        set-var(sess.zone) capture.req.hdr(1)
      tcp-request content        set-var(sess.id) var(sess.user),concat(\#,sess.zone,)
      tcp-request content        track-sc1 var(sess.id) table concurrency_st
      tcp-request content        reject if too-many-conn
   ```

1. Modify WebDAV proxy configuration to enforce the client concurrency limit.

   Since the WebDAV service is implemented using apache with the davrods module, it uses basic authentication. The client user name can be extracted from the `Authorization` HTTP header. Since the WebDAV service can only handle requests made of the local iRODS zone, it is assumed that the client's authentication zone is the local zone `tempZone`.

   For basic authentication, the `Authorization` header value has the form `Basic <encoded-credentials>`, where `<encode-crendentials>` are base64-encoded client username and password in the form `<username>:<password>`. The maximum length of an iRODS username is 250 characters. The maximum length of the encrypted password in the ICAT DB is 250 characters, so the password has to be smaller than 250 characters in length. The means the maximum credential length is less than 501 characters, which when base64-encoded with have a maximum length of less than 668 characters. Include the '`Basic `' prefix, the maximum length is 674 characters for the `Authorization` header value.

   The `capture request header <name> len <length>` operation is used to capture the `Authorization` header. `<length>` is set to `700` to ensure the entire header value is captured. The captured value is stored in `capture.req.hdr(0)`. The following is the complete operation that does this.

   ```haproxy
   capture request header  Authorization len 700
   ```

   Now `capture.req.hdr(0)` holds the `Authorization` header value. To get the username, the '`Basic `' prefix needs to be removed, the result needs to be decoded from base64, and the `:<password>` suffix needs to be removed. This can be done in the following `http request <action>` operation that stores the extracted username in the session variable `sess.name`.

   ```haproxy
   http-request  set-var(sess.name) capture.req.hdr(0),regsub(^Basic\ ,),b64dec,regsub(:.*$,)
   ```

   The client id is created, it is added to the `concurrency_st` stick table, and the request is possibly rejected using similar operations as the operations used by the `irods` proxy.

   ```haproxy
   http-request  set-var(sess.id) var(sess.name),concat(\#tempZone,,)
   http-request  track-sc1 var(sess.id) table concurrency_st
   http-request  reject if too-many-conn
   ```

   Here's the complete definition of the concurrency limit enforcing WebDAV proxy.

   ```harpoxy
   listen webdav
      mode                    http
      bind                    :443 ssl crt /etc/ssl/private/example.org.pem
      server                  dav 10.0.1.1 port 80 maxconn 100
      acl                     too-many-conn sc1_conn_cur gt 5
      capture request header  Authorization len 700
      http-request            set-var(sess.name) capture.req.hdr(0),regsub(^Basic\ ,),b64dec,regsub(:.*$,)
      http-request            set-var(sess.id) var(sess.name),concat(\#tempZone,,)
      http-request            track-sc1 var(sess.id) table concurrency_st
      http-request            reject if too-many-conn
   ```

## Complete Example

Here's the complete `haproxy.cfg` for the example above.

```haproxy
##########################
#         GLOBAL         #
##########################

global
   daemon
   chroot                     /var/lib/haproxy
   user                       haproxy
   group                      haproxy
   log                        127.0.0.1 local0 debug
   pidfile                    /var/run/haproxy.pid
   unix-bind                  mode 770 user haproxy group haproxy
   maxconn                    400
   tune.ssl.default-dh-param  2048


##########################
#        DEFAULT         #
##########################

defaults
   log            global
   maxconn        200
   option tcplog


##########################
#  CONNECTION TRACKING   #
##########################

backend concurrency_st
   stick-table  type string len 600 size 300 store conn_cur


##########################
#         IRODS          #
##########################

listen irods
   mode                       tcp
   bind                       :1247,:20000-20199
   server                     irods 10.0.0.1 maxconn 100
   acl                        too-many-conn sc1_conn_cur gt 5
   tcp-request inspect-delay  5s
   tcp-request content        capture req.payload(4,0),regsub([\s\S]*<StartupPack_PI\s*>[\s\S]*<clientUser\s*>,),regsub(</clientUser\s*>[\s\S]*,) len 250
   tcp-request content        capture req.payload(4,0),regsub([\s\S]*<StartupPack_PI\s*>[\s\S]*<clientRcatZone\s*>,),regsub(</clientRcatZone\s*>[\s\S]*,) len 250
   tcp-request content        set-var(sess.user) capture.req.hdr(0)
   tcp-request content        set-var(sess.zone) capture.req.hdr(1)
   tcp-request content        set-var(sess.id) var(sess.user),concat(\#,sess.zone,)
   tcp-request content        track-sc1 var(sess.id) table concurrency_st
   tcp-request content        reject if too-many-conn


##########################
#         WEBDAV         #
##########################

listen webdav
   mode                    http
   bind                    :443 ssl crt /etc/ssl/private/cyverse.rocks.pem
   server                  dav 10.0.1.1 port 80 maxconn 100
   acl                     too-many-conn sc1_conn_cur gt 5
   capture request header  Authorization len 700
   http-request            set-var(sess.id) capture.req.hdr(0),regsub(^Basic\ ,),b64dec,regsub(:.*$,\#tempZone)
   http-request            track-sc1 var(sess.id) table concurrency_st
   http-request            reject if too-many-conn
```
