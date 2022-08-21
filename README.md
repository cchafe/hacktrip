# hacktrip
## jacktrip hub client with rtaudio and a new API (optionally built as libraries)

hacktrip is a hub client written from scratch and hopefully a handy tutorial, too
- simplified, uses rtaudio and qt sockets in standard ways
- establishes a client API to connect local audio to a hub server
- API components in hapitrip.h and hapitrip.cpp
- hapitrip creates a shared library
- grabs latest rtaudio as git submodule from URL and builds as shared library
- works for linux and windows
- qt5 and qt6 compatible

## code examples (all are qt projects, supported by qt creator workflow)
- app called "app" 
  - minimal GUI (qt designer)
  - hapitrip and rtaudio included via shared libraries
    - open in creator project file guiDemo/guiDemo.pro
    - executable is app/app
- plugin called "ChuckTrip" is a chuck chugin accessible from chuck scripts 
  - this version is with hapitrip compiled in for ease of chucking (but shared lib tests ok, too)
    - open in creator project file ChuckTrip/ChuckTrip.pro
    - chugin is ChuckTrip/ChuckTrip.chug
    - example chuck scripts in chucktrip/ck/

## tutorials that can result from the code examples
- "app" AUDIO_ONLY = rtaudio local audio loopback, uses audio callback, sine test
- "app" SERVER_HANDSHAKE_ONLY = connect to server, keep connection alive, quit
- "app" (complete) = bidirectional audio stream, simple ring buffer for received packets
- "ChuckTrip" (complete) = a plugin that uses sample synchronous polling and has no event loop

## Done:
- integrate all of the above into new branch dev
- flush out API with choice of server, FPP, channels, etc.
- fully comment all code examples
- document API
- merge to main
- release 0.2
- prpject layouts completed
```
gVersion = "0.2.1"
```
- merge to main
- release 0.2.1
## TODO:
```
gVersion = "0.3-rc.1"
```
- substitute Regulator.h and Regulator.cpp for the initial simple ring buffer
- release 0.3
```
gVersion = "0.4-rc.1"
```
- generate all of chuckTrip.cpp automatically using chuginate standard
- release 0.4
```
gVersion = "0.5-rc.1"
```
- try things out in a concert setting


