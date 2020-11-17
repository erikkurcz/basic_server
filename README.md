## Basic Server

This is a simple server for reading off the command line and writing that to whoever is connected using `AF_LOCAL`. 

### TODO
* Use `AF_INET6` instead for a more realistic example
* Figure out why the address is always in use even after restart (maybe a soft shutdown?)
    * Theory: personal dir might not work because not open permissions, but `/tmp` dir base works. Unclear why personal dir _doesn't_ work even when `chmod 777` but ok, `/tmp` dir is fine I guess

### Compiling

```shell
g++ server.cpp -o myserver && g++ client.cpp -o myclient && chmod +x myserver && chmod +x myclient
```

### Running

Either open 2 terminal windows or a split `tmux` session. 

*Server side:*
```
$ ./myserver
Waiting for connection before accepting data to send...
Master has given Dobby a sock!
Connected! Begin writing.
hello this is a test!
and it looks like it is working!
```

*Client side:*
```
$ ./myserver
hello this is a test!
and it looks like it is working!
```

