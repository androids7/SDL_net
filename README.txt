Summary ->
works on windows operating system.

The server is started up and waits listening for tcp connections. When a client connects via tcp,various info is sent out 
to the other players.When one of the clients moves around, they send udp messages to the server about their position. 
When the server receives this message, it sends it out to the other players.When a player hits esc to quit, the server 
knows the tcp connection is closed and tells other players that they have disconnected.

To run this example, you need to run the SERVER first. Then the Client. This currently only works over your local network, ie on the same machine. 
If you want to connect over a proper network just change the "localhost" in client to an actual ip address. 

The code still needs alot of cleaning.

Things that can be added:
-> showing player names over their head
-> On server, list players , names and other useful data
-> character selection screen

I have used this example ->

CHATD:  A chat server using the SDL example network library
Copyright (C) 1997-2004 Sam Lantinga

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Sam Lantinga
slouken@libsdl.org