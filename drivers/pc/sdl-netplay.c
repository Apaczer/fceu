/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2001 LULU
 *  Copyright (C) 2002 Ben Parnell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sdl.h"
#include <SDL/SDL_net.h>
#include "sdl-netplay.h"

char *netplayhost=0;

static int tonowait;

int Port=0xFCE;
int FDnetplay=0;


static SDLNet_SocketSet socketset = NULL;
static TCPsocket tcpsock = NULL, servsock = NULL;

void cleanup(void)
{
    if (tcpsock != NULL) {
        SDLNet_TCP_DelSocket(socketset, tcpsock);
        SDLNet_TCP_Close(tcpsock);
        tcpsock = NULL;
    }
    if (servsock != NULL) {
        SDLNet_TCP_DelSocket(socketset, servsock);
        SDLNet_TCP_Close(servsock);
        servsock = NULL;
    }
    if (socketset != NULL) {
        SDLNet_FreeSocketSet(socketset);
        socketset = NULL;
    }
}

int FCEUD_NetworkConnect(void)
{
    IPaddress serverIP;

    tonowait=0;

    if (netplay == 2) {
        /* client */
        printf("connecting to %s\n", netplayhost);

        SDLNet_ResolveHost(&serverIP, netplayhost, Port);
        if (serverIP.host == INADDR_NONE) {
            fprintf(stderr, "Couldn't connected to %s\n", netplayhost);
            return -1;
        } else {
            tcpsock = SDLNet_TCP_Open(&serverIP);
            if (tcpsock == NULL) {
                fprintf(stderr, "Couldn't connected to %s\n", netplayhost);
                return -1;
            }
        }
        printf("connected to %s\n", netplayhost);

        socketset = SDLNet_AllocSocketSet(1);
        if (socketset == NULL) {
            fprintf(stderr, "Couldn't create socket set: %s\n",
                    SDLNet_GetError());
            return -1;
        }
        SDLNet_TCP_AddSocket(socketset, tcpsock);

        return 1;
    } else {
        /* server */

        SDLNet_ResolveHost(&serverIP, NULL, Port);
        printf("Server IP: %x, %d\n", serverIP.host, serverIP.port);
        servsock = SDLNet_TCP_Open(&serverIP);
        if (servsock == NULL) {
            cleanup();
            fprintf(stderr, "Couldn't create server socket: %s\n",
                    SDLNet_GetError());
            return -1;
        }

        socketset = SDLNet_AllocSocketSet(2);
        if (socketset == NULL) {
            fprintf(stderr, "Couldn't create socket set: %s\n",
                    SDLNet_GetError());
            return -1;
        }
        SDLNet_TCP_AddSocket(socketset, servsock);

        if (SDLNet_CheckSockets(socketset, ~0)) {
            tcpsock = SDLNet_TCP_Accept(servsock);
            if (tcpsock == NULL) {
                return -1;
            }
            SDLNet_TCP_AddSocket(socketset, tcpsock);

            printf("OK, connected\n");
            return 1;
        }
    }

    return -1;
}

void FCEUD_NetworkClose(void)
{
    cleanup();
}

int FCEUD_NetworkRecvData(uint8 *data, uint32 len, int block)
{
  if(block)
  {
   if(SDLNet_TCP_Recv(tcpsock, (void *) data, len)!=len)
   {
    cleanup();
    return(0);
   }
   switch(SDLNet_CheckSockets(socketset,0))
   {
    case -1:return(0);
    case 0:NoWaiting&=~2;tonowait=0;break; 
    default:if(tonowait>=3)
	     NoWaiting|=2;
	    else tonowait++;
	    break;
   }
   return(1);
  }
  else
  {
   int t=SDLNet_CheckSockets(socketset,0);
   if(t<0) return(0);
   if(!t) return(-1);
   return(SDLNet_TCP_Recv(tcpsock, (void *) data, len)==len);
  }
}

/* 0 on failure, 1 on success.  This function should always block. */
int FCEUD_NetworkSendData(uint8 *Value, uint32 len)
{
    if (tcpsock)
        return(SDLNet_TCP_Send(tcpsock, (void *) Value, len)==len);
    return 0;
}
