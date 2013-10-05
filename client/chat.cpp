#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>


#include "fcntl.h"

#include "SDL_net.h"
#include "SDL/SDL.h"

#include "CSprite.h"
#include "CSpriteBase.h"

#include "chat.h"

using namespace std;

/* Global variables */
Uint8 yourPlayer;
bool activePlayer = false;
SDL_Surface *screen,*back;
bool finished = false;
int runSpeed = 4;

//time
float sdlgt=0;           // Stores the time that has passed
float dt=0;              // The time that has elapsed since
                         // the previous frame
int gameover=0;          // Is the game over?
int td=0,td2=0;          // Used when checking the time difference
                         // (how much time has passed since last frame)
float accum = 0;        //when this adds up to a certain number, then do player updates

//network stuff
char *server = "localhost";//"220.233.28.6";
IPaddress serverIP;
static TCPsocket tcpsock = NULL;
static UDPsocket udpsock = NULL;
static SDLNet_SocketSet socketset = NULL;
static UDPpacket **packets = NULL;
static struct {
	int active;
	int id;
	IPaddress ip;
	Uint8 name[256+1];
	CSpriteBase playerBase;     // Stores the images of the player
    CSprite player;             // Stores the player
} people[GAME_MAXPEOPLE];

//early declaration
void sendServerUDP(string Smsg);

//*****Drawing stuff****************//
// Load in an image and convert it to the display format
// for faster blitting.
SDL_Surface * ImageLoad(char *file)
{
  SDL_Surface *temp1, *temp2;
  temp1 = SDL_LoadBMP(file);
  temp2 = SDL_DisplayFormat(temp1);
  SDL_FreeSurface(temp1);
  return temp2;
}
// Blit an image on the screen surface
void DrawIMG(SDL_Surface *img, int x, int y)
{
  SDL_Rect dest;
  dest.x = x;
  dest.y = y;
  SDL_BlitSurface(img, NULL, screen, &dest);
}
//********************************//

//make yourself known to othe server
void SendHello(char *name)
{
	IPaddress *myip;
	char hello[1+1+256];
	int i, n;

	/* No people are active at first */
	for ( i=0; i<GAME_MAXPEOPLE; i++ ) {
		people[i].player.set(-1,-1);
        people[i].active = 0;
        people[i].id = i;
	}
	if ( tcpsock != NULL ) {
		/* Get our chat handle */
		if ( (name == NULL) &&
		     ((name=getenv("GAME_USER")) == NULL) &&
		     ((name=getenv("USER")) == NULL ) ) {
			name="Unknown";
		}
		cout<<"Using name "<< name<<endl;;

		/* Construct the packet */
		hello[0] = GAME_HELLO;
		myip = SDLNet_UDP_GetPeerAddress(udpsock, -1);
		memcpy(&hello[GAME_HELLO_PORT], &myip->port, 2);
		if ( strlen(name) > 255 ) {
			n = 255;
		} else {
			n = strlen(name);
		}
		hello[GAME_HELLO_NLEN] = n;
		strncpy(&hello[GAME_HELLO_NAME], name, n);
		hello[GAME_HELLO_NAME+n++] = 0;

		/* Send it to the server */
		SDLNet_TCP_Send(tcpsock, hello, GAME_HELLO_NAME+n);
	}
}

//In this example, we send all of our data to the server first, then its the
//servers job to send this data to everyone else
void sendServerUDP(string Smsg){
    UDPpacket *p;

    //string to char
    char msg[128];
    strcpy(msg,Smsg.c_str());

    /* Allocate memory for the packet */
    if (!(p = SDLNet_AllocPacket(512)))
    {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }

    //put our message into the packet
    memcpy(p->data, msg, p->maxlen);
	p->len = strlen((char *)p->data) + 1;

    p->address.host = serverIP.host;    /* Set the destination host */
    p->address.port = serverIP.port;    /* And destination port SDL_SwapBE16(7777);*/

    /* to see if we set our packet up correctly
	cout<<"Port to connect to "<<p->address.port<<" "<<SDL_SwapBE16(p->address.port)<<endl;
    cout<<"Host "<<((p->address.host>>24)&0xFF)<<" "<<
            ((p->address.host>>16)&0xFF)<<" "<<((p->address.host>>8)&0xFF)<<
            " "<< (p->address.host&0xFF)<<endl; */


    int sent = SDLNet_UDP_Send(udpsock, -1, p); /* send udp message to server*/
    //printf("Message sent %d\n",sent); 1 is pass, 0 fail, -1 error?

    //free the packet!
    SDLNet_FreePacket(p);
}

//Send your players position
void sendPos()
{
    if(activePlayer){
        int x = people[yourPlayer].player.getX();
        int y =  people[yourPlayer].player.getY();
        char msg[packets[0]->maxlen];

        //building message, eg 0 pos 25 300
        sprintf(msg, "%d pos %d %d", people[yourPlayer].id, x, y);
        sendServerUDP(msg);

    }
}

int HandleServerData(Uint8 *data)
{
	int used;

	switch (data[0]) {
		case GAME_ADD: {
			Uint8 which;
			IPaddress newip;

			/* Figure out which channel we got */
			which = data[GAME_ADD_SLOT];
			if ((which >= GAME_MAXPEOPLE) || people[which].active) {
				/* Invalid channel?? */
				break;
			}
			/* Get the client IP address */
			newip.host=SDLNet_Read32(&data[GAME_ADD_HOST]);
			newip.port=SDLNet_Read16(&data[GAME_ADD_PORT]);

			people[which].ip.host=newip.host;
			people[which].ip.port=newip.port;

			/* Copy name into channel */
			memcpy(people[which].name, &data[GAME_ADD_NAME], 256);
			people[which].name[256] = 0;
			people[which].active = 1;
			cout<<"Player "<<people[which].id<<" is now active"<<endl;

			/* Let the user know what happened */
			cout<<"* New client on "<< which<<" from "<<
		((newip.host>>24)&0xFF)<<" "<<((newip.host>>16)&0xFF)<<" "<<
			((newip.host>>8)&0xFF)<<" "<< (newip.host&0xFF)<<" "<<
					(newip.port)<<" "<<( people[which].name)<<endl;

            //If the player could chose an appearance, this is where you would
            //load it up. But since we are not, just load the default
            people[which].playerBase.init("data/pixman");
            people[which].player.init(&people[which].playerBase,screen);
            people[which].player.set(300,220);
            people[which].player.setSpeed(1);

			/* Put the address back in network form */
			newip.host = SDL_SwapBE32(newip.host);
			newip.port = SDL_SwapBE16(newip.port);

			/* Bind the address to the UDP socket */
			//SDLNet_UDP_Bind(udpsock, which, &newip);//prob doesnt work, as newip isnt copied
		}
		used = GAME_ADD_NAME+data[GAME_ADD_NLEN];
		break;
		case GAME_ID: {
            Uint8 which;
            /* Figure out which channel we got */
            cout << "GAME ID"<<data[1]<<endl;
			which = data[1];
			/*if ((which >= GAME_MAXPEOPLE) || people[which].active) {
				break;
			}*/

			yourPlayer = which;
			activePlayer = true;
			cout<< "You are player "<<people[yourPlayer].id<<" "<<people[yourPlayer].name<<endl;
        }
        used = GAME_ID_LEN;
        break;
		case GAME_DEL: {
			Uint8 which;

			/* Figure out which channel we lost */
			which = data[GAME_DEL_SLOT];
			if ( (which >= GAME_MAXPEOPLE) ||
						! people[which].active ) {
				/* Invalid channel?? */
				break;
			}
			people[which].active = 0;

			/* Let the user know what happened */
			cout<<"* Lost client on "<< which<<" "<< people[which].name<<endl;

			/* Unbind the address on the UDP socket */
			SDLNet_UDP_Unbind(udpsock, which);
		}
		used = GAME_DEL_LEN;
		break;
		case GAME_BYE: {
			cout<<"* Chat server full\n";
		}
		used = GAME_BYE_LEN;
		break;
		default: {
			/* Unknown packet type?? */;
		}
		used = 0;
		break;
	}
	return(used);
}

void HandleServer(void)
{
	Uint8 data[512];
	int pos, len;
	int used;

	/* Has the connection been lost with the server? */
	// A non-blocking way of using this function is to check the socket with
    // SDLNet_CheckSockets and SDLNet_SocketReady and call SDLNet_TCP_Recv only
    // if the socket is active.
	len = SDLNet_TCP_Recv(tcpsock, (char *)data, 512);
	if ( len <= 0 ) {
		SDLNet_TCP_DelSocket(socketset, tcpsock);
		SDLNet_TCP_Close(tcpsock);
		tcpsock = NULL;
		cout<<"Connection with server lost!\n";
	} else {
		pos = 0;
		while ( len > 0 ) {
			used = HandleServerData(&data[pos]);
			pos += used;
			len -= used;
			if ( used == 0 ) {
				/* We might lose data here.. oh well,
				   we got a corrupt packet from server
				 */
				len = 0;
			}
		}
	}
}
void HandleClient(void)
{
	//check to see if we've got a udp packet coming in
	if (SDLNet_UDP_Recv(udpsock, packets[0]))
        {
            /*readout of the packets info, use to see if your actually receiving
            anything

			cout<<"Check it, got a msg"<<endl;
            printf("UDP Packet incoming\n");
            printf("\tChan:    %d\n", packets[0]->channel);
            printf("\tData:    %s\n", (char *)packets[0]->data);
            printf("\tLen:     %d\n", packets[0]->len);
            printf("\tMaxlen:  %d\n", packets[0]->maxlen);
            printf("\tStatus:  %d\n", packets[0]->status);
            printf("\tAddress: %x %x\n",packets[0]->address.host, packets[0]->address.port);*/

            //we receive data from any messages sent out to us
            string data = (char *)packets[0]->data;

          	//tearing the data up into smaller strings to interpret what the message is
            istringstream iss(data, istringstream::in);
            string tmp;
            string id;

            iss >> id;
            iss >> tmp;

            //if this is a position type message
            if(tmp == "pos" && atoi(id.c_str()) != people[yourPlayer].id){
                string x;
                iss >> x;

                string y;
                iss >> y;
                cout<<"MSG: "<<x<<" "<<y<<endl;
                people[atoi(id.c_str())].player.set(atoi(x.c_str()),atoi(y.c_str()));
            }
        }
}

//return false when we over this shit
bool HandleNet(void)
{
	SDLNet_CheckSockets(socketset, 0);
	if ( SDLNet_SocketReady(tcpsock) ) {
		HandleServer();
	}
	if ( SDLNet_SocketReady(udpsock) ) {
		HandleClient();
	}



	return true;
}



extern "C"
void cleanup(int exitcode)
{
	/* Close the network connections */
	if ( tcpsock != NULL ) {
		SDLNet_TCP_Close(tcpsock);
		tcpsock = NULL;
	}
	if ( udpsock != NULL ) {
		SDLNet_UDP_Close(udpsock);
		udpsock = NULL;
	}
	if ( socketset != NULL ) {
		SDLNet_FreeSocketSet(socketset);
		socketset = NULL;
	}
	if ( packets != NULL ) {
		SDLNet_FreePacketV(packets);
		packets = NULL;
	}
	SDLNet_Quit();
	SDL_Quit();
	exit(exitcode);
}

void playerControl(){
    //time management
    td2=SDL_GetTicks();
    dt=((float)(td2-td))*0.1;
    td=td2;
    sdlgt+=dt*10;
    accum += dt;

    // Check if we have some interesting events...
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        // Has a key been pressed down?
        if(event.type == SDL_KEYDOWN)
        {
            // If it was escape then quit
            if(event.key.keysym.sym == SDLK_ESCAPE)
            {
                //we are quiting the game now
                finished = 1;
            }
        }
    }
    if(activePlayer && accum > 1){
            Uint8* keys;
            bool moved = false;
            // Get the state of the keyboard keys
            keys = SDL_GetKeyState(NULL);
            if(keys[SDLK_DOWN])
            {
                people[yourPlayer].player.yadd(runSpeed);
                moved = true;
            }
            if(keys[SDLK_UP])
            {
                people[yourPlayer].player.yadd(-runSpeed);
                moved = true;
            }
            if(keys[SDLK_RIGHT])
            {
                people[yourPlayer].player.xadd(runSpeed);
                moved = true;
            }
            if(keys[SDLK_LEFT])
            {
                people[yourPlayer].player.xadd(-runSpeed);
                moved = true;
            }
            //if our position has changed, send our new pos to the server
            if(moved)
                sendPos();

            accum = 0;
        }
}

void initSDL(){
    /* Initialize SDL */
    if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
            fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
            exit(1);
	}

	/* Set a 640x480 video mode*/
	screen = SDL_SetVideoMode(640, 480, 0, SDL_SWSURFACE);
	if ( screen == NULL ) {
                fprintf(stderr, "Couldn't set video mode: %s\n",SDL_GetError());
		SDL_Quit();
                exit(1);
	}
}

void initSDLNet(){
    /* Initialize the network */
	if ( SDLNet_Init() < 0 ) {
		fprintf(stderr, "Couldn't initialize net: %s\n",
						SDLNet_GetError());
		SDL_Quit();
		exit(1);
	}
}

void initBackgroundImage(){
    // We also load in all the graphics...
    back = ImageLoad("data/shitgrass.bmp");
}

void allocatePackets(){
    /* Allocate a vector of packets for client messages */
	packets = SDLNet_AllocPacketV(4, GAME_PACKETSIZE);
	if ( packets == NULL ) {
		fprintf(stderr, "Couldn't allocate packets: Out of memory\n");
		cleanup(2);
	}
}

void initClient(){
    initSDL();
    initSDLNet();
    initBackgroundImage();
    allocatePackets();
}

void connectToHost(){
    /* Connect to remote host and create UDP endpoint */
	cout<<"Connecting to "<<server<<endl;
	SDLNet_ResolveHost(&serverIP, server, GAME_PORT);

	if ( serverIP.host == INADDR_NONE ) {
		cout<<"Couldn't resolve hostname\n";
	} else {
		/* If we fail, it's okay, the GUI shows the problem */
		tcpsock = SDLNet_TCP_Open(&serverIP);
		if ( tcpsock == NULL ) {
			cout<<"Connect failed\n";
			cout<<SDLNet_GetError()<<endl;
		} else {
			cout<<"Connected\n";
		}
	}
}

void tryPorts(){
    /* Try ports in the range {GAME_PORT - GAME_PORT+10} */
	for (int i=0; (udpsock == NULL) && i<10; ++i ) {
		udpsock = SDLNet_UDP_Open(GAME_PORT+i);
		cout<<GAME_PORT+i<<endl;
	}
	if ( udpsock == NULL ) {
		SDLNet_TCP_Close(tcpsock);
		tcpsock = NULL;
		cout<<"Couldn't create UDP endpoint\n";
	}
}

void allocateSocketSet(){
    /* Allocate the socket set for polling the network */
	socketset = SDLNet_AllocSocketSet(2);
	if ( socketset == NULL ) {
		fprintf(stderr, "Couldn't create socket set: %s\n",
						SDLNet_GetError());
		cleanup(2);
	}
	SDLNet_TCP_AddSocket(socketset, tcpsock);
	SDLNet_UDP_AddSocket(socketset, udpsock);
}

int main(int argc, char *argv[])
{

    initClient();

    connectToHost();
    tryPorts();
	allocateSocketSet();

/* main loop: handle data sent, control player, draw */
	SendHello("ANUJ");

	while(!finished){
	   HandleNet();
	   playerControl();


	   //draw background
	   DrawIMG(back, 0,0);
	   //draw active players
	   for (int i=0; i<GAME_MAXPEOPLE; i++ ) {
		if(people[i].active != 0)
        {
            //cout <<"Drawing a player!"<<endl;
            people[i].player.draw();
        }
	   }

	   // Filp the screen - if you dont, you wont see anything!
        SDL_Flip(screen);
    }
	cleanup(0);

	return(0);
}
