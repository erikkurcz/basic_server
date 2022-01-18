## Basic Server

This is a simple server for reading off the command line and writing that to whoever is connected using `AF_INET`, which is IPv4.

### Compiling

```shell
make clean all

# OR

make c a
```

### Usage 

Server:

```bash
myserver PORT MAX_CONNECTIONS DEBUG

Where
	PORT is port number to use
	MAX_CONNECTIONS is the max number of connections this server will accept.
	DEBUG is any character to indicate you'd like extra debug logging
```

Client(s):

```bash
Usage: ./myclient PORT

Where
	PORT is the port number to connect on
```

### Running / Example

In this case, I'll demonstrate with a server and 2 clients, also demonstrating how the server handles disconnected clients.

First, open 3 terminal windows, or a `tmux` session split 3 ways.

Now, run the server with a port number of your choosing, and in this case just 1 connection max:

```bash
$ # SERVER SIDE
$ ./myserver 12227 1
Ready for input:
```

Over on the first client, go ahead and start a client with the same port:

```bash
$ # FIRST CLIENT
$ ./myclient 12227
```

Back to the server, enter the first message: 

```bash
$ # SERVER SIDE
$ ./myserver 12227 1
Ready for input:
hello there
```

You'll see this message on the first client!

```bash
$ # FIRST CLIENT
$ ./myclient 12227
hello there
```

Subsequent messages will make it over as well:

```bash
$ # SERVER SIDE
$ ./myserver 12227 1
Ready for input:
hello there
hello from the other siiiiiiiideeeeee
```

```bash
$ # FIRST CLIENT
$ ./myclient 12227
hello there
hello from the other siiiiiiiideeeeee
```

Now let's start up another client:

```bash
$ # SECOND CLIENT
$ ./myclient 12227
```

We don't see any of the previous messages - this client wasn't connected when they were sent. But let's send another from the server:

```bash
$ # SERVER SIDE
$ ./myserver 12227 1
Ready for input:
hello there
hello from the other siiiiiiiideeeeee
oh we have someone else here too? 
```

```bash
$ # FIRST CLIENT
$ ./myclient 12227
hello there
hello from the other siiiiiiiideeeeee
oh we have someone else here too? 
```

```bash
$ # SECOND CLIENT
$ ./myclient 12227
Server at max connections, retry later
$ echo $?
1
```

The first client above received the message, but the second didn't! The server has a user-imposed limit of just 1 connection, so the second client is given a message behind-the-scenes that it can recognize and then exit gracefully.

Let's suppose we really want that second client to connect, and kill the first and restart the second (in either order!):


```bash
$ # FIRST CLIENT
$ ./myclient 12227
hello there
hello from the other siiiiiiiideeeeee
oh we have someone else here too? 
^C
$ echo $?
130
$
```

```bash
$ # SECOND CLIENT
$ ./myclient 12227
Server at max connections, retry later
$ echo $?
1
$ ./myclient 12227
```

Then we send another message on our sever:

```bash
$ # SERVER SIDE
$ ./myserver 12227 1
Ready for input:
hello there
hello from the other siiiiiiiideeeeee
oh we have someone else here too? 
well anyway, let's carry on
```

Our first client is disconnected, we know this, but the second now displays that latest message:

```bash
$ # SECOND CLIENT
$ ./myclient 12227
Server at max connections, retry later
$ echo $?
1
$ ./myclient 12227
well anyway, let's carry on
```

This has been a demo of how to start the server, clients, end clients, and have the server continue on sending to any new connection.

Of course, you can make things easier just by using a higher MAX\_CONNECTIONS limit for the server in the first place. Let's start a server with MAX_CONNECTIONS of 15:

```bash
$ # SERVER SIDE
$ ./myserver 12227 15
Ready for input:
```
Then let's start 10 clients, each logging to it's own file:

```bash
$ # CLIENTS SIDE
$ for x in {1..10}
do
    ./myclient 12227 2>&1 > myclient.${x}.log & 
done
$
```

And put a message through expressing our excitement for using many clients: 

```bash
$ # SERVER SIDE
$ ./myserver 12227 15
Ready for input:
many clients!!!
^C
```

And finally, let's check the logs - we have 10 log files, each with the expected log message on the first line:

```bash
$ # CLIENTS SIDE
$ for x in {1..10}
do
    ./myclient 12227 2>&1 > myclient.${x}.log & 
done
$ grep -Hn clients *log
myclient.10.log:1:many clients!!!
myclient.1.log:1:many clients!!!
myclient.2.log:1:many clients!!!
myclient.3.log:1:many clients!!!
myclient.4.log:1:many clients!!!
myclient.5.log:1:many clients!!!
myclient.6.log:1:many clients!!!
myclient.7.log:1:many clients!!!
myclient.8.log:1:many clients!!!
myclient.9.log:1:many clients!!!
$ 
```

