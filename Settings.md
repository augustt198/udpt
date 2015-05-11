# Introduction #
Settings have a default file name "udpt.conf", but you can store settings in any file and put the configuration's file name as the first argument when starting the program - `$> ./udpt <configuration-file>`.

If no configuration file is present, a default configuration file will be created by the program (if a parameter was specified, it will be created there).

# Details #
The configuration file is build like a `*`.ini file in Windows. You have groups of settings, keys, and values.

**Example for a configuration file:**
```
; Lines that start with a semicolon are ignored.

[database]
; Here you can specify were you want the database to be.
file=tracker.db

[tracker]
; Here you can specify were the tracker will listen to (UDP port).
port = 6969
; Here you can specify how many threads will be working on the specified UDP port.
threads = 5
; Allow clients to send a remote IP address?
allow_remotes = Yes
```

# All Settings #
This is a table that will provide all the setting properties and their descriptions.

| **Class** | **Key** | **Data Type** | **Description** |
|:----------|:--------|:--------------|:----------------|
| database | file | string | The file where the SQLite database will be stored |
| database | driver | string | Which Database driver to use (only sqlite3 for now...) |
| tracker | port | int | The UDP port of the tracker |
| tracker | threads | int | amount of listener threads |
| tracker | allow\_remotes | bool | Allow clients to send Remote IP addresses? |
| tracker | allow\_iana\_ips | bool | Allow IANA reserved addresses? (such as 127.0.0.1, **disable in production!**) |
| tracker | announce\_interval | int | Seconds between announce requests |
| tracker | cleanup\_interval | int | Seconds between cleanup procedures |
| tracker | bind | IP[:port] | Interface to find to. Optional. If specified, overrides port |
| apiserver | enable | bool | '1' to enable the API server, otherwise HTTP API will be disabled |
| apiserver | port | int | the TCP port to listen to, defaults to 6969 |
| apiserver | threads | int | threads that the HTTP server should use, defaults to 1. |
| apiserver| bind | IP[:port] | Interface to find to. Optional. If specified, overrides port |

## API Keys ##
The HTTP API server requires a key to do any actions. The key is limited to specified IP addresses.
All keys are under the class `api.keys`, and the key should be the app's key - the value is a list of IPs that are allowed to use the key.

Example:
```
# Other Configuration stuff...

[api.keys]
#format: <key>=<list of IPs>
indexsite1=127.0.0.1; 10.0.0.1
proxytracker=127.0.0.1; 10.0.0.2
```