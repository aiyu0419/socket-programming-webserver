Author: Aiyu Wu
UNI: aw3537

Project Overview
This project is a simple HTTP server capable of handling static file serving and database lookups. It was developed as part of a networking lab and implements essential web server functionalities such as handling HTTP requests, responding with appropriate status codes, and communicating with a backend lookup server.

The key functionalities include:

Serving static files from a directory.
Handling HTTP requests with correct status codes.
Communicating with a backend lookup server to search for records.
Ensuring robust error handling and resource management.

Implementation Details:

Part 1: Static Webpage Setup
Successfully configured the webpage for static file hosting.
Part 2: HTTP Server Implementation
1)General Server Behavior:
Built using socket programming with socket(), bind(), listen(), and accept().
Reference materials: Lab 6 solutions, tcp-receiver.c, and tcp-sender.c.
Handles persistent connections and efficiently manages resources.
2)Request Handling Logic:
The server correctly interprets HTTP requests by parsing the status line.
The request validation order follows: method → HTTP version → request URI.
Ensures correct behavior for both web browsers and command-line clients (netcat).
3)Static File & Directory Handling:
Implements stat() to check file validity before serving.
Supports directory navigation and appends index.html when needed.
Implements proper error handling for missing files (404) and bad requests (400).

Part 2b: Database Lookup Feature
The server connects to a lookup host to retrieve database search results.
Only allows valid request URIs:
/mdb-lookup (returns the search form).
/mdb-lookup?key=<query> (returns search results).
Uses strncmp() to verify request prefixes efficiently.
Maintains resource safety by ensuring buffers are freed and sockets closed properly.

Challenges & Solutions:
Ensuring request parsing robustness → Implemented proper status line validation to mimic reference server behavior.
Handling early client disconnections → Ensured proper cleanup in case of partial request transmissions.
Maintaining consistency with lookup-server → Adjusted newline handling (\n vs \r\n) for correct query formatting.

Key Takeaways:
Gained hands-on experience in socket programming for web servers.
Improved understanding of HTTP request/response mechanics and error handling.
Learned about efficient buffer management and memory safety in C

Future Improvements:
Implementing multithreading for concurrent request handling.
Adding TLS encryption for secure communication.
Expanding functionality to support dynamic content processing.
