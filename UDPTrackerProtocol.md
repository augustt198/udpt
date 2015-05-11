# Introduction #
In order for a BitTorrent client to discover other clients, it requires a BitTorrent Tracker to announce itself. The request that the Clients request from the tracker are short, and so is the response. For HTTP connections, TCP is used and that takes extra un-required overheads.

# Overheads #
  * The Ethernet layer takes 14 bytes per packet.
  * The IP layer takes 20 bytes per packet.
  * The TCP layer takes 20 bytes per packet.
  * The HTTP layer takes a nice overhead (variable size).
The TCP+HTTP layers can be reduced when using UDP by about 50%.
This isn't really a problem for Clients since every request about 1kB, but it can save half the bandwidth on heavily loaded servers.

# UDP connections, spoofing addresses #
A source address can be spoofed with UDP, to ensure that this wont happen the server will send a client a connection ID, and all future request will be done with that connection-ID. (If the client spoofs the source address, it wont receive the connection-id).
connection-IDs can live for 2 minutes and the be changed.

# Timeouts #
UDP is unreliable, we don't know if the message got to it's destination. And therefore, if a client didn't get a reply from the server, it should try the request again up to 8 times, every 15 `*` 2 ^ _n_ seconds where _n_ is the number of the request attempt.

## Examples ##
Normal Announce:
  * t = 0: connection request
  * t = 1: connection response
  * t = 2: announce request
  * t = 3: announce response
Connection Timeouts:
  * t = 0: connection request
  * t = 15: connection request
  * t = 45: connection request
  * t = 105: connection request
  * etc...

# UDP Tracker Protocol #
**All values are sent in Network byte order**. packets do not have a set size since future extensions may extend this protocol.

Before announcing or scraping, you need a connection-id (must be valid for at least two minutes). Before the client sends the connection request, it must create a random transaction id.

| **Offset** | **Size** | **Name** | **Value** |
|:-----------|:---------|:---------|:----------|
| 0 | 8 (64 bit integer) | connection id | for this request, the initial value 0x41727101980|
| 8 | 4 (32-bit integer) | action | 0 for connection request |
| 12 | 4 (32-bit integer) | transaction id | a random number created by client |

When the tracker receives the connection request (action=0), it responds with a connection id that must be valid for at least 2 minutes. The tracker must verify the packet of the connection request is 16 bytes.

The tracker responds with 16 bytes, the client must verify that the transaction-id matches the one that the request was made with.

| **Offset** | **Size** | **Name** | **Value** |
|:-----------|:---------|:---------|:----------|
| 0 | 4 (32-bit integer) | action | 0 for connect response |
| 4 | 4 (32-bit integer) | transaction id | same like request's transaction id. |
| 8 | 8 (64 bit integer) | connection id | a connection id that must be acceptable for at least 2 minutes from source |

After the client has got a connection id, it can announce itself like this:

| **Offset** | **Size** | **Name** | **Value** |
|:-----------|:---------|:---------|:----------|
| 0 | 8 (64 bit integer) | connection id | connection id from server |
| 8 | 4 (32-bit integer) | action | 1; for announce request |
| 12 | 4 (32-bit integer) | transaction id | client can make up another transaction id... |
| 16 | 20 | info\_hash | the info\_hash of the torrent that is being announced |
| 36 | 20 | peer id | the peer ID of the client announcing itself |
| 56 | 8 (64 bit integer) | downloaded | bytes downloaded by client this session |
| 64 | 8 (64 bit integer) | left | bytes left to complete the download |
| 72 | 8 (64 bit integer) | uploaded | bytes uploaded this session |
| 80 | 4 (32 bit integer) | event | 0=None; 1=Download completed; 2=Download started; 3=Download stopped. |
| 84 | 4 (32 bit integer) | IPv4 | IP address, default set to 0 (use source address)|
| 88 | 4 (32 bit integer) | key | _?_ |
| 92 | 4 (32 bit integer) | num want | -1 by default. number of clients to return |
| 96 | 2 (16 bit integer) | port | the client's TCP port |

The server then responds with the clients (announce response, action=1):

| **Offset** | **Size** | **Name** | **Value** |
|:-----------|:---------|:---------|:----------|
| 0 | 4 | action | 1 |
| 4 | 4 | transaction id | same like the transaction id sent be the announce request |
| 8 | 4 | interval | seconds to wait till next announce |
| 12| 4 | leechers | amount of leechers in swarm |
| 16| 4 | seeders | amount of seeders in swarm |
| 20 + 6 `*` _n_ | 4 | IPv4 | IP of peer |
| 24 + 6 `*` _n_ | 2 | port | TCP port of client |



---

Source: [http://www.bittorrent.org/beps/bep_0015.html]