/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

static char netplayhost[256]={0};
static int netplayport=0xFCE;

static int netplayon=0;
static int netplaytype=0;

static HWND hwndns=0;

static SOCKET Socket=INVALID_SOCKET;
static int wsainit=0;

static volatile int abortnetplay=0;
static volatile int concommand=0;

static void WSE(char *ahh)
{
 char tmp[256];
 sprintf(tmp,"Winsock: %s",ahh);
 FCEUD_PrintError(tmp);
}

int SetBlockingSock(SOCKET Socko)
{
   unsigned long t;
   t=1;
   if(ioctlsocket(Socko,FIONBIO,&t))
   {
    WSE("Error setting socket to non-blocking mode!\n");
    return 0;
   }
  return 1;
}

BOOL CALLBACK BoogaDooga(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
   case WM_USER+1:
                if(WSAGETASYNCERROR(lParam))
                 concommand=1;
                else
                 concommand=2;
                break;
   case WM_USER:
                if(WSAGETSELECTEVENT(lParam)==FD_CONNECT)
                 {
                  if(WSAGETSELECTERROR(lParam))
                   concommand=1;
                  else
                   concommand=2;
                 }                
                break;
   case WM_INITDIALOG:
                if(!netplaytype) SetDlgItemText(hwndDlg,100,(LPTSTR)"Waiting for a connection...");
                else SetDlgItemText(hwndDlg,100,(LPTSTR)"Attempting to establish a connection...");                                 
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                  case 1:
                        gornk:
                        abortnetplay=1;
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;

}

static void CloseNSDialog(void)
{
 if(hwndns)
  {
   SendMessage(hwndns,WM_COMMAND,1,0);
   hwndns=0;
  }
}

void CreateStatusDialog(void)
{
 hwndns=CreateDialog(fceu_hInstance,"NETSTAT",hAppWnd,BoogaDooga);
}

void FCEUD_NetworkClose(void)
{
 CloseNSDialog();
 if(Socket!=INVALID_SOCKET)
 {
  closesocket(Socket);
  Socket=INVALID_SOCKET;
 }
 if(wsainit)
 {
  WSACleanup();
  wsainit=0;
 }
 /* Make sure blocking is returned to normal once network play is stopped. */
 NoWaiting&=~2;
}

int FCEUD_NetworkConnect(void)
{
 WSADATA WSAData;
 SOCKADDR_IN sockin;    /* I want to play with fighting robots. */
 SOCKET TSocket;

 if(WSAStartup(MAKEWORD(1,1),&WSAData))
 {
  FCEUD_PrintError("Error initializing Windows Sockets.");
  return(0);
 }
 wsainit=1;
 concommand=abortnetplay=0;

 if( (TSocket=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)
 {
  WSE("Error creating socket.");
  FCEUD_NetworkClose();
  return(0);
 }

 memset(&sockin,0,sizeof(sockin));
 sockin.sin_family=AF_INET;
 sockin.sin_port=htons(netplayport);

 if(!netplaytype)       /* Act as a server. */
 {

  int sockin_len;
  sockin.sin_addr.s_addr=INADDR_ANY;

  sockin_len=sizeof(sockin);

  if(bind(TSocket,(struct sockaddr *)&sockin,sizeof(sockin))==SOCKET_ERROR)
  {
   WSE("Error binding to socket.");
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }

  if(listen(TSocket,1)==SOCKET_ERROR)
  {
   WSE("Error listening on socket.");
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }

  CreateStatusDialog();
  if(!SetBlockingSock(TSocket))
  {
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }

  while( (Socket=accept(TSocket,(struct sockaddr *) &sockin,(int *)&sockin_len)) ==
                INVALID_SOCKET)
  {
   if(abortnetplay || WSAGetLastError()!=WSAEWOULDBLOCK)
   {
    if(!abortnetplay)
     WSE("Error accepting connection.");
    closesocket(TSocket);
    FCEUD_NetworkClose();
    return(0);
   }
   else
    BlockingCheck();
  }

  if(!SetBlockingSock(Socket))
  {
   FCEUD_NetworkClose();
   return(0);
  }

 }    
 else   /* We're a client... */
 {
  char phostentb[MAXGETHOSTSTRUCT];
  unsigned long hadr;

  hadr=inet_addr(netplayhost);

  CreateStatusDialog();

  if(hadr!=INADDR_NONE)
   sockin.sin_addr.s_addr=hadr;
  else
  {
   if(!WSAAsyncGetHostByName(hwndns,WM_USER+1,(const char *)netplayhost,phostentb,MAXGETHOSTSTRUCT))
   {
    ghosterr:
    WSE("Error getting host network information.");
    closesocket(TSocket);
    FCEUD_NetworkClose();
    return(0);
   }
   while(concommand!=2)
   {
    BlockingCheck();
    if(concommand==1 || abortnetplay)
     goto ghosterr;
   }
   memcpy((char *)&sockin.sin_addr,((PHOSTENT)phostentb)->h_addr,((PHOSTENT)phostentb)->h_length);
  }
  concommand=0;

  if(WSAAsyncSelect(TSocket,hwndns,WM_USER,FD_CONNECT|FD_CLOSE)==SOCKET_ERROR)
  {
   eventnoterr:
   WSE("Error setting event notification on socket.");
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }

  if(!SetBlockingSock(TSocket))
  {
   closesocket(TSocket);
   FCEUD_NetworkClose();
   return(0);
  }

  if(connect(TSocket,(PSOCKADDR)&sockin,sizeof(sockin))==SOCKET_ERROR)
  {
   if(WSAGetLastError()!=WSAEWOULDBLOCK)
   {
    cerrav:
    WSE("Error connecting to remote host.");

    cerra:
    closesocket(TSocket);
    FCEUD_NetworkClose();
    return(0);
   }

   while(concommand!=2)
   {
    BlockingCheck();        
    if(abortnetplay) goto cerra;
    if(concommand==1) goto cerrav;
   }
  }
  if(WSAAsyncSelect(TSocket,hAppWnd,WM_USER,0)==SOCKET_ERROR)
   goto eventnoterr;
  Socket=TSocket;

 }
 CloseNSDialog();
 return(1);
}


int FCEUD_NetworkSendData(uint8 *data, uint32 len)
{
        int erc;

        while((erc=send(Socket,data,len,0)))
        {
         if(erc!=SOCKET_ERROR)
         {
          len-=erc;
          data+=erc;
          if(!len)
           return(1);   /* All data sent. */

          if(!BlockingCheck()) return(0);
         }
         else
         {
          if(WSAGetLastError()==WSAEWOULDBLOCK)
          {
           if(!BlockingCheck()) return(0);
           continue;
          }
          return(0);
         }
        }
        return(0);
}

int FCEUD_NetworkRecvData(uint8 *data, uint32 len, int block)
{
 int erc;

 if(block)      // TODO: Add code elsewhere to handle sound buffer underruns.
 {
  while((erc=recv(Socket,data,len,0))!=len)
  {         
         if(!erc)
          return(0);
         if(WSAGetLastError()==WSAEWOULDBLOCK)
         {
          if(!BlockingCheck()) return(0);
          continue;
         }
         return(0);
  }

  {
   char buf[24];
   if(recv(Socket,buf,24,MSG_PEEK)==SOCKET_ERROR)
   {
    if(WSAGetLastError()==WSAEWOULDBLOCK)
     NoWaiting&=~2;
    else
     return(0);
   }
   else
    NoWaiting|=2;       /* We're the client and we're lagging behind.
                           disable blocking(particularly sound...) to *try*
                           to catch up.
                        */                          
  }

  return 1;
 }

 else   /* We're the server.  See if there's any new data
           from player 2.  If not, then return(-1).
        */
 {
  erc=recv(Socket,data,len,0);
  if(!erc)
   return(0);
  if(erc==SOCKET_ERROR)
  {
   if(WSAGetLastError()==WSAEWOULDBLOCK)
    return(-1);
   return(0);	// Some other(bad) error occurred.
  }
  return(1);
 } // end else to if(block)
}

BOOL CALLBACK NetConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
   case WM_INITDIALOG:                

                CheckDlgButton(hwndDlg,100,netplayon?BST_CHECKED:BST_UNCHECKED);
                CheckRadioButton(hwndDlg,101,102,101+netplaytype);
                SetDlgItemInt(hwndDlg,107,netplayport,0);

                if(netplayhost[0])
                 SetDlgItemText(hwndDlg,104,netplayhost);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:

                        netplayport=GetDlgItemInt(hwndDlg,107,0,0);

                        if(IsDlgButtonChecked(hwndDlg,100)==BST_CHECKED)
                         netplayon=1;
                        else
                         netplayon=0;

                        if(IsDlgButtonChecked(hwndDlg,101)==BST_CHECKED)
                         netplaytype=0;
                        else
                         netplaytype=1;

                        GetDlgItemText(hwndDlg,104,netplayhost,255);
                        netplayhost[255]=0;

                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}


static void ConfigNetplay(void)
{
 DialogBox(fceu_hInstance,"NETPLAYCONFIG",hAppWnd,NetConCallB);

 if(netplayon)
  FCEUI_SetNetworkPlay(netplaytype+1);
 else
  FCEUI_SetNetworkPlay(0);
}

