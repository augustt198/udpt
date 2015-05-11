# Introduction #
The UDPT project now supports a HTTP API. The HTTP API can help websites manage torrents (When working in non-[dynamic](TrackerTypes#Dynamic_Tracker.md) mode).

# Details #
Well, the API runs on a small HTTP server that has the following:

| **Path** | **Description** |
|:---------|:----------------|
| / | Root Path of the server; Displays information about UDPT |
| /announce | A path that will notify an error to anyone trying to use the tracker as a HTTP Tracker |
| /api | The API server |

The API path requires parameters:
  * _key_: The application key that is accessing the API. (Must be specified in the [settings](Settings#API_Keys.md) file with the accessing IP).
  * _action_: The action that the application wants to do.
    * _add_: Adds a torrent to the allowed torrents. Requires another parameter _hash_ which must be 40 characters long.
    * _remove_: Removes a torrent from allowed torrents. Requires another parameter _hash_ which must be 40 characters long.

## Examples ##
If you would like you program to add a torrent to the database:
```
GET /api?key=<app_key>&action=add&hash=1234567890123456789012345678901234567890 HTTP/1.1
```
The Server expects lowercase requests & hashes. action should be 'add' not 'Add'. All characters of the hash must be lowercase or the request will result in a error.

The response should be a status 200 response; otherwise there is a problem with the key you used (doesn't exist or isn't authorized).

If the action was done successfully: you will get a JSON reply `{success:1}` otherwise you will get a JSON reply with the key `error` and a human readable reason as the value.