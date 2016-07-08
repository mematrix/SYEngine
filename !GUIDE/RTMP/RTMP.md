# Play RTMP Stream 

## Client

### Winsocks
You must init winsocks before playing RTMP stream.
```
WSADATA wsa;
WSAStartup(MAKEWORD(2,2), &wsa);
```

It can't be use in WinRT environment.

## Server

### Timestamp
Your media server must fix timestamp for every client start with 0.

So you can't use nginx-rtmp-module , try simple-rtmp-server or others.