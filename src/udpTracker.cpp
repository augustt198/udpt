/*
 *	Copyright © 2012,2013 Naim A.
 *
 *	This file is part of UDPT.
 *
 *		UDPT is free software: you can redistribute it and/or modify
 *		it under the terms of the GNU General Public License as published by
 *		the Free Software Foundation, either version 3 of the License, or
 *		(at your option) any later version.
 *
 *		UDPT is distributed in the hope that it will be useful,
 *		but WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *		GNU General Public License for more details.
 *
 *		You should have received a copy of the GNU General Public License
 *		along with UDPT.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "udpTracker.hpp"
#include "tools.h"
#include <cstdlib> // atoi
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include "multiplatform.h"
#include "logging.h"

extern UDPT::Logger *logger;

using namespace std;
using namespace UDPT::Data;

#define UDP_BUFFER_SIZE		2048

namespace UDPT
{
	UDPTracker::UDPTracker (Settings *settings)
	{
		Settings::SettingClass *sc_tracker;

		sc_tracker = settings->getClass("tracker");

		this->allowRemotes = sc_tracker->getBool("allow_remotes", true);
		this->allowIANA_IPs = sc_tracker->getBool("allow_iana_ips", false);
		this->isDynamic = sc_tracker->getBool("is_dynamic", true);

		this->announce_interval = sc_tracker->getInt("announce_interval", 1800);
		this->cleanup_interval = sc_tracker->getInt("cleanup_interval", 120);
		this->port = sc_tracker->getInt("port", 6969);
		this->thread_count = abs (sc_tracker->getInt("threads", 5)) + 1;

		list<SOCKADDR_IN> addrs;
		sc_tracker->getIPs("bind", addrs);

		if (addrs.empty())
		{
			SOCKADDR_IN sa;
			sa.sin_port = m_hton16(port);
			sa.sin_addr.s_addr = 0L;
			addrs.push_back(sa);
		}

		this->localEndpoint = addrs.front();


		this->threads = new HANDLE[this->thread_count];

		this->isRunning = false;
		this->conn = NULL;
		this->o_settings = settings;
	}

	UDPTracker::~UDPTracker ()
	{
		int i; // loop index

		this->isRunning = false;

		// drop listener connection to continue thread loops.
		// wait for request to finish (1 second max; allot of time for a computer!).

	#ifdef linux
		close (this->sock);

		sleep (1);
	#elif defined (WIN32)
		closesocket (this->sock);

		Sleep (1000);
	#endif

		for (i = 0;i < this->thread_count;i++)
		{
	#ifdef WIN32
			TerminateThread (this->threads[i], 0x00);
	#elif defined (linux)
			pthread_detach (this->threads[i]);
			pthread_cancel (this->threads[i]);
	#endif
			stringstream str;
			str << "Thread (" << (i + 1) << "/" << ((int)this->thread_count) << ") terminated.";
			logger->log(Logger::LL_INFO, str.str());
		}
		if (this->conn != NULL)
			delete this->conn;
		delete[] this->threads;
	}

	void UDPTracker::wait()
	{
#ifdef WIN32
		WaitForMultipleObjects(this->thread_count, this->threads, TRUE, INFINITE);
#else
		int i;
		for (i = 0;i < this->thread_count; i++)
		{
			pthread_join (this->threads[i], NULL);
		}
#endif
	}

	enum UDPTracker::StartStatus UDPTracker::start ()
	{
		stringstream ss;
		SOCKET sock;
		int r,		// saves results
			i,		// loop index
			yup;	// just to set TRUE
		string dbname;// saves the Database name.

		sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET)
			return START_ESOCKET_FAILED;

		yup = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yup, 1);

		this->localEndpoint.sin_family = AF_INET;
		r = bind (sock, (SOCKADDR*)&this->localEndpoint, sizeof(SOCKADDR_IN));

		if (r == SOCKET_ERROR)
		{
	#ifdef WIN32
			closesocket (sock);
	#elif defined (linux)
			close (sock);
	#endif
			return START_EBIND_FAILED;
		}

		this->sock = sock;

		this->conn = new Data::SQLite3Driver (this->o_settings->getClass("database"),
				this->isDynamic);

		this->isRunning = true;

		ss.str("");
		ss << "Starting maintenance thread (1/" << ((int)this->thread_count) << ")";
		logger->log(Logger::LL_INFO, ss.str());

		// create maintainer thread.
	#ifdef WIN32
		this->threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_maintainance_start, (LPVOID)this, 0, NULL);
	#elif defined (linux)
		pthread_create (&this->threads[0], NULL, _maintainance_start, (void*)this);
	#endif

		for (i = 1;i < this->thread_count; i++)
		{
			ss.str("");
			ss << "Starting thread (" << (i + 1) << "/" << ((int)this->thread_count) << ")";
			logger->log(Logger::LL_INFO, ss.str());

			#ifdef WIN32
			this->threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_thread_start, (LPVOID)this, 0, NULL);
	#elif defined (linux)
			pthread_create (&(this->threads[i]), NULL, _thread_start, (void*)this);
	#endif
		}

		return START_OK;
	}

	int UDPTracker::sendError (UDPTracker *usi, SOCKADDR_IN *remote, uint32_t transactionID, const string &msg)
	{
		struct udp_error_response error;
		int msg_sz,	// message size to send.
			i;		// copy loop
		char buff [1024];	// more than reasonable message size...

		error.action = m_hton32 (3);
		error.transaction_id = transactionID;
		error.message = (char*)msg.c_str();

		msg_sz = 4 + 4 + 1 + msg.length();

		// test against overflow message. resolves issue 4.
		if (msg_sz > 1024)
			return -1;

		memcpy(buff, &error, 8);
		for (i = 8;i <= msg_sz;i++)
		{
			buff[i] = msg[i - 8];
		}

		sendto(usi->sock, buff, msg_sz, 0, (SOCKADDR*)remote, sizeof(*remote));

		return 0;
	}

	int UDPTracker::handleConnection (UDPTracker *usi, SOCKADDR_IN *remote, char *data)
	{
		ConnectionRequest *req;
		ConnectionResponse resp;

		req = (ConnectionRequest*)data;

		resp.action = m_hton32(0);
		resp.transaction_id = req->transaction_id;

		if (!usi->conn->genConnectionId(&resp.connection_id,
				m_hton32(remote->sin_addr.s_addr),
				m_hton16(remote->sin_port)))
		{
			return 1;
		}

		sendto(usi->sock, (char*)&resp, sizeof(ConnectionResponse), 0, (SOCKADDR*)remote, sizeof(SOCKADDR_IN));

		return 0;
	}

	int UDPTracker::handleAnnounce (UDPTracker *usi, SOCKADDR_IN *remote, char *data)
	{
		AnnounceRequest *req;
		AnnounceResponse *resp;
		int q,		// peer counts
			bSize,	// message size
			i;		// loop index
		DatabaseDriver::PeerEntry *peers;
		DatabaseDriver::TorrentEntry tE;

		uint8_t buff [1028];	// Reasonable buffer size. (header+168 peers)

		req = (AnnounceRequest*)data;

		if (!usi->conn->verifyConnectionId(req->connection_id,
				m_hton32(remote->sin_addr.s_addr),
				m_hton16(remote->sin_port)))
		{
			return 1;
		}

		// change byte order:
		req->port = m_hton16 (req->port);
		req->ip_address = m_hton32 (req->ip_address);
		req->downloaded = m_hton64 (req->downloaded);
		req->event = m_hton32 (req->event);	// doesn't really matter for this tracker
		req->uploaded = m_hton64 (req->uploaded);
		req->num_want = m_hton32 (req->num_want);
		req->left = m_hton64 (req->left);

		if (!usi->allowRemotes && req->ip_address != 0)
		{
			UDPTracker::sendError (usi, remote, req->transaction_id, "Tracker doesn't allow remote IP's; Request ignored.");
			return 0;
		}

		if (!usi->conn->isTorrentAllowed(req->info_hash))
		{
			UDPTracker::sendError(usi, remote, req->transaction_id, "info_hash not registered.");
			return 0;
		}

		// load peers
		q = 30;
		if (req->num_want >= 1)
			q = min (q, req->num_want);

		peers = new DatabaseDriver::PeerEntry [q];


		DatabaseDriver::TrackerEvents event;
		switch (req->event)
		{
		case 1:
			event = DatabaseDriver::EVENT_COMPLETE;
			break;
		case 2:
			event = DatabaseDriver::EVENT_START;
			break;
		case 3:
			event = DatabaseDriver::EVENT_STOP;
			break;
		default:
			event = DatabaseDriver::EVENT_UNSPEC;
			break;
		}

		if (event == DatabaseDriver::EVENT_STOP)
			q = 0;	// no need for peers when stopping.

		if (q > 0)
			usi->conn->getPeers(req->info_hash, &q, peers);

		bSize = 20; // header is 20 bytes
		bSize += (6 * q); // + 6 bytes per peer.

		tE.info_hash = req->info_hash;
		usi->conn->getTorrentInfo(&tE);

		resp = (AnnounceResponse*)buff;
		resp->action = m_hton32(1);
		resp->interval = m_hton32 ( usi->announce_interval );
		resp->leechers = m_hton32(tE.leechers);
		resp->seeders = m_hton32 (tE.seeders);
		resp->transaction_id = req->transaction_id;

		for (i = 0;i < q;i++)
		{
			int x = i * 6;
			// network byte order!!!

			// IP
			buff[20 + x] = ((peers[i].ip & (0xff << 24)) >> 24);
			buff[21 + x] = ((peers[i].ip & (0xff << 16)) >> 16);
			buff[22 + x] = ((peers[i].ip & (0xff << 8)) >> 8);
			buff[23 + x] = (peers[i].ip & 0xff);

			// port
			buff[24 + x] = ((peers[i].port & (0xff << 8)) >> 8);
			buff[25 + x] = (peers[i].port & 0xff);

		}
		delete[] peers;
		sendto(usi->sock, (char*)buff, bSize, 0, (SOCKADDR*)remote, sizeof(SOCKADDR_IN));

		// update DB.
		uint32_t ip;
		if (req->ip_address == 0) // default
			ip = m_hton32 (remote->sin_addr.s_addr);
		else
			ip = req->ip_address;
		usi->conn->updatePeer(req->peer_id, req->info_hash, ip, req->port,
				req->downloaded, req->left, req->uploaded, event);

		return 0;
	}

	int UDPTracker::handleScrape (UDPTracker *usi, SOCKADDR_IN *remote, char *data, int len)
	{
		ScrapeRequest *sR;
		int v,	// validation helper
			c,	// torrent counter
			i,	// loop counter
			j;	// loop counter
		uint8_t hash [20];
		ScrapeResponse *resp;
		uint8_t buffer [1024];	// up to 74 torrents can be scraped at once (17*74+8) < 1024


		sR = (ScrapeRequest*)data;

		// validate request length:
		v = len - 16;
		if (v < 0 || v % 20 != 0)
		{
			UDPTracker::sendError (usi, remote, sR->transaction_id, "Bad scrape request.");
			return 0;
		}

		if (!usi->conn->verifyConnectionId(sR->connection_id,
				m_hton32(remote->sin_addr.s_addr),
				m_hton16(remote->sin_port)))
		{
			return 1;
		}

		// get torrent count.
		c = v / 20;

		resp = (ScrapeResponse*)buffer;
		resp->action = m_hton32 (2);
		resp->transaction_id = sR->transaction_id;

		for (i = 0;i < c;i++)
		{
			int32_t *seeders,
				*completed,
				*leechers;

			for (j = 0; j < 20;j++)
				hash[j] = data[j + (i*20)+16];

			seeders = (int32_t*)&buffer[i*12+8];
			completed = (int32_t*)&buffer[i*12+12];
			leechers = (int32_t*)&buffer[i*12+16];

			DatabaseDriver::TorrentEntry tE;
			tE.info_hash = hash;
			if (!usi->conn->getTorrentInfo(&tE))
			{
				sendError(usi, remote, sR->transaction_id, "Scrape Failed: couldn't retrieve torrent data");
				return 0;
			}

			*seeders = m_hton32 (tE.seeders);
			*completed = m_hton32 (tE.completed);
			*leechers = m_hton32 (tE.leechers);
		}

		sendto (usi->sock, (const char*)buffer, sizeof(buffer), 0, (SOCKADDR*)remote, sizeof(SOCKADDR_IN));

		return 0;
	}

static int _isIANA_IP (uint32_t ip)
{
	uint8_t x = (ip % 256);
	if (x == 0 || x == 10 || x == 127 || x >= 224)
		return 1;
	return 0;
}

	int UDPTracker::resolveRequest (UDPTracker *usi, SOCKADDR_IN *remote, char *data, int r)
	{
		ConnectionRequest *cR;
		uint32_t action;

		cR = (ConnectionRequest*)data;

		action = m_hton32(cR->action);

		if (!usi->allowIANA_IPs)
		{
			if (_isIANA_IP (remote->sin_addr.s_addr))
			{
				return 0;	// Access Denied: IANA reserved IP.
			}
		}

//		cout << ":: " << (void*)m_hton32(remote->sin_addr.s_addr) << ": " << m_hton16(remote->sin_port) << " ACTION=" << action << endl;

		if (action == 0 && r >= 16)
			return UDPTracker::handleConnection (usi, remote, data);
		else if (action == 1 && r >= 98)
			return UDPTracker::handleAnnounce (usi, remote, data);
		else if (action == 2)
			return UDPTracker::handleScrape (usi, remote, data, r);
		else
		{
//			cout << "E: action=" << action << ", r=" << r << endl;
			UDPTracker::sendError (usi, remote, cR->transaction_id, "Tracker couldn't understand Client's request.");
			return -1;
		}

		return 0;
	}

#ifdef WIN32
	DWORD UDPTracker::_thread_start (LPVOID arg)
#elif defined (linux)
	void* UDPTracker::_thread_start (void *arg)
#endif
	{
		UDPTracker *usi;
		SOCKADDR_IN remoteAddr;

#ifdef linux
		socklen_t addrSz;
#else
		int addrSz;
#endif

		int r;
		char tmpBuff [UDP_BUFFER_SIZE];

		usi = (UDPTracker*)arg;

		addrSz = sizeof (SOCKADDR_IN);


		while (usi->isRunning)
		{
			cout.flush();
			// peek into the first 12 bytes of data; determine if connection request or announce request.
			r = recvfrom(usi->sock, (char*)tmpBuff, UDP_BUFFER_SIZE, 0, (SOCKADDR*)&remoteAddr, &addrSz);
			if (r <= 0)
				continue;	// bad request...
			r = UDPTracker::resolveRequest (usi, &remoteAddr, tmpBuff, r);
		}

	#ifdef linux
		pthread_exit (NULL);
	#endif
		return 0;
	}

#ifdef WIN32
	DWORD UDPTracker::_maintainance_start (LPVOID arg)
#elif defined (linux)
	void* UDPTracker::_maintainance_start (void *arg)
#endif
	{
		UDPTracker *usi;

		usi = (UDPTracker *)arg;

		while (usi->isRunning)
		{
			usi->conn->cleanup();

#ifdef WIN32
			Sleep (usi->cleanup_interval * 1000);
#elif defined (linux)
			sleep (usi->cleanup_interval);
#else
#error Unsupported OS.
#endif
		}

		return 0;
	}

};
