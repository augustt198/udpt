# Introduction #
Sometimes, you want to send or receive a big file - and you can't attach it by email since it takes too much space. Or if you want to share a file as a publisher on a blog, and you don't have enough bandwidth. You should use BitTorrent.

The BitTorrent technology allows users to download from users. So when you upload a file just once or twice to other people; It would be enough for anyone who wants that file. The only bandwidth the publisher needs, is for a tiny file called a torrent.

# How It Works #
  1. You go to a website, and download a torrent or click on a magnet link.
  1. Your BitTorrent client connects to the Trackers listed by the torrent, and requests the IP addresses of others who are downloading the same thing (your IP will be published too of-course).
  1. The BitTorrent client then connects to some of the "peers" it found on the trackers it connected to, and starts downloading & uploading the requested material.

# Why use a UDP tracker? #
It doesn't really matter for the client's side. But it can make a big difference for Trackers with low bandwidth or with a high usage volume.
The UDP Tracker protocol takes less than half the memory than HTTP, you can read more about this here: [The UDP Tracker Protocol](UDPTrackerProtocol.md).