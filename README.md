# Reverse Shell

This project is a reverse shell implementation created for a university network programming course. It allows a client to connect to a server and execute commands on the client's system from the server.

## Features

*   Reverse shell functionality: Executes commands on the client machine from the server.
*   Simple client-server architecture.
*   Written in C.
*   Builds with `make`.

## Usage

1.  **Clone the repository:**

    ```bash
    git clone https://github.com/Mehregan-Gogona/Network-Project.git
    cd Network-Project
    ```

2.  **Build the project:**

    ```bash
    make
    ```

3.  **Run the server:**

    ```bash
    ./server
    ```
    
4.  **Run the client:**

    ```bash
    ./client 3000
    ```


## Project Structure

*   `client.c`:  Client-side code for the reverse shell.
*   `server.c`:  Server-side code for handling client connections and command execution.
*   `Makefile`:  Build configuration for compiling the project.

## Contributing

Feel free to contribute to this project by submitting pull requests.

## License

MIT License

Copyright (c) 2025 Mehregan-Gogona

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
