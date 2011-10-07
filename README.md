Requirements:

1. libspotify 0.9.x (available from the Spotify developer site)

2. libevent 2.0.x (available from http://libevent.org)

3. A developer API key from the Spotify developer site

4. A premium account with Spotify

To build using libspotify:

1. Update your Makefile.local.mk to uncomment the line LIBSPOTIFY = 1

2. Copy your key.h file to the base of this repo

3. $ make

