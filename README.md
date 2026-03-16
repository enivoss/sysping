# sysping

continuous network latency monitor for Windows. pings multiple hosts in real-time, dark terminal UI, keyboard navigation.

```
  sysping by enivoss   tick 12
  ------------------------------------------------------------------------
  HOST            ADDRESS            LAST      AVG       HISTORY
  ------------------------------------------------------------------------
  cloudflare      1.1.1.1            8ms       9ms       ___::||||||||||||
  google dns      8.8.8.8            12ms      11ms      __:::||||||||||||
  google          google.com         14ms      13ms      __:::||||||||||||
  github          github.com         18ms      17ms      _::::||||||||||||
  opendns         208.67.222.222     10ms      10ms      ___::||||||||||||
  ------------------------------------------------------------------------
  [UP/DOWN] navigate   [Q] quit
```

## what it does

pings a list of hosts every second via Win32 ICMP API. shows last RTT, rolling average, and a 20-tick history bar per host. color codes by latency: red for timeouts, yellow for high latency, white for normal.

## controls

| key | action |
|-----|--------|
| UP / DOWN | navigate |
| Q | quit |

## build

requires MSVC (Visual Studio). run:

```
build.bat
```

outputs `sysping.exe` in the same directory.

## default hosts

cloudflare (1.1.1.1), google dns (8.8.8.8), google.com, github.com, opendns (208.67.222.222). edit the hosts list in `sysping.cpp` to add your own.
