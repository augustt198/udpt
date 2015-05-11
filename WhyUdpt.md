# Introduction #
If you would like to install a tracker on your server, you should take certain details into consideration. This page was created in order to compare different types of trackers.

# Details #
## Language comparison ##
Lots of trackers out there are programmed in Higher-Level languages such as Java, C#, etc... These languages are compiled but are considered slow when comparing to Assembler, C, C++. Some trackers are written in scripting languages such as Python or PHP - Scripting languages are even slower that High-Level languages since they aren't event compiled. UDPT is written in C++ and is compiled to machine code, so it is faster and more efficient than any tracker written in a High-Level or scripting language. (_A high-level language means that the language manages memory, and some work is done for the programmer - High-Level languages always run on virtual-machines and are therefore slower, and less efficient._)

## Networking Comparison ##
Most trackers are using the HTTP or HTTPS protocol, which runs on top of the TCP/IP protocol. UDPT runs on UDP/IP, which saves about half the request size - If your server needs to handle lots of requests, this can allow your server to handle double the requests with less bandwidth in the same amount of time.

## Security ##
UDPT is designed with security in mind - All requests are validated before they are handled. _In the meantime no security issues where reported._


---

The more the project is used - the better it will get. When I see people using the project, I develop it more and more. When bugs or suggestions are sent - I fix/add them to the project.