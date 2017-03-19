/*compilar comanda gcc client.c -ansi -pedantic -Wall*/
#include <stdio.h>/*printf i altres*/
#include <string.h>
#include <stdlib.h>/*Funcions fitxers*/
#include <fcntl.h> 	/*Modes apertura.*/
#include<unistd.h>
#include<signal.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include<time.h>
int dispositiu=0;
int debug=0;
int estat;
int sock;
struct hostent  * server;
struct sockaddr_in addr_server; /* per a la informacio de la dirreccio del servidor */
/*Fase de Registre*/
#define SUBS_REQ 0x00/*Peticio de subscripcio*/
#define SUBS_ACK 0x01/*Acceptacio del paquet*/
#define SUBS_INFO 0x02/*Peticio addicional de subscripcio*/
#define INFO_ACK 0X03/*acceptacio del paquet addicional de subscripcio*/
#define SUBS_NACK 0x04/*error de subscripcio*/
#define SUBS_REJ 0x05/*Rebuig de subscripcio*/
/*Estats client*/
#define DISCONNECTED 0xa0 /*Controlador desconectat*/
#define NOT_SUBSCRIBED 0xa1/* Controlador no subscrit*/
#define WAIT_ACK_SUBS 0xa2 /* Espera confirmacio primer paquet subscripcio*/
#define WAIT_ACK_INFO 0xa3 /* Espera confirmacio primer paquet subscripcio*/
#define SUBSCRIBED 0xa4/*Controlador subscrit, sense enviar hello*/
#define SEND_HELLO 0xa5/*Controlador enviant paquets de hello*/
#define WAIT_INFO 0xa6/*Servidor esperant paquet SUBS_INFO*/
/*Llindars procediment de subscripcio*/
#define t 1
#define u 3
#define n 8
#define o 3
#define p 3
#define q 4
/*Manteniment de la connexio*/
#define HELLO 0x10 /*Enviament de HELLO*/
#define HELLO_REJ 0x11/*Rebuig de recepcio de HELLO*/
#define v 2
#define r 2/*numero de vegades de l'interval d'enviament de hello*/
#define s 4 /*numero de HELLO consecutius*/
/*Enviament de dades cap al servidor*/
#define SEND_DATA 0x20/*Enviament de dades des de el controlador*/
#define SET_DATA 0x21/*Enviament de dades des de el servidor*/
#define GET_DATA 0x22/*Petició de dades des de el servidor*/
#define DATA_ACK 0x23/*Acceptació d’un paquet de dades*/
#define DATA_NACK 0x24/*Error en un paquet de dades*/
#define DATA_REJ 0x25/*Rebuig d’un paquet de dades*/
#define w 3 /*Temps que espera el client la resposta del servidor*/
/*Estructura del controlador*/
struct configuracio{
  char name[9];
  char situation[13];
  char elements [10][8];
  char mac[13];
  char tcp[5];
  char server[10];
  char srv[5];
};
/*Format PDU UDP*/
struct pdu_udp{
  unsigned char tipus_paquet;
  char mac[13];
  char numero_aleatori[9];
  char dades[80];
};
/*Format PDU TCP*/
struct pdu_tcp{
  unsigned char tipus_paquet;
  char mac[13];/*Adreça  MAC  de  la  interfície  de  xarxa  del  controlador  (de  l’arxiu
configuració)*/
  char numero_aleatori[9];/*valor enviat en el primer paquet del servidor (dades identificació
servidor)*/
  char dispositiu[8];/*nom del dispositiu especificat en la comanda
send*/
  char valor[7];/*valor associat al dispositiu*/
  char info[80];/*Valor buit ("")*/
};
void llegir_fitxer(char * f,struct configuracio * controlador);
void init_pdu(struct pdu_udp * paquet,struct configuracio * client,unsigned char tipus_paquet,char numero_aleatori[9]);
void subscripcio(struct configuracio client,struct sockaddr_in addr_server);
char * get_time();
int main(int argc,char * argv[]){
  char * fitxer;
  int i;
  struct configuracio client;
  fitxer="client.cfg";
  estat=NOT_SUBSCRIBED;

  for(i=1;i<argc;i++){
    if(strcmp(argv[i],"-c")==0){
      fitxer=argv[i+1];
      continue;
    }
    if (strcmp(argv[i],"-d")==0) {
      debug=1;
    }

  }
  /*Llegir fitxer de configuracio*/
  llegir_fitxer(fitxer ,&client);
  printf("%i\n",dispositiu );
  printf("Name:%s\n",client.name);
  printf("Situation:%s\n",client.situation);
  printf("Element0 :%s\n",client.elements[0]);
  printf("Element1 :%s\n",client.elements[1]);
  printf("Element2 :%s\n",client.elements[2]);
  printf("MAC:%s\n",client.mac);
  printf("TCP:%s\n",client.tcp);
  printf("server:%s\n",client.server);
  printf("srv:%s\n",client.srv);
  printf("debug=%i\n",debug);
  /*Obrim socket UDP*/
  sock=socket(AF_INET,SOCK_DGRAM,0);
  if(sock==-1){
    printf("No puc obrir el socket");
    exit(-1);
  }
  server=gethostbyname(client.server);
  if(!server){
    exit(-1);
  }
  memset(&addr_server,0,sizeof (struct sockaddr_in));
	addr_server.sin_addr.s_addr=inet_addr(inet_ntoa(*(struct in_addr *)*(server->h_addr_list)++));
  addr_server.sin_family=AF_INET;
  addr_server.sin_port=htons(atoi(client.srv));
  subscripcio(client,addr_server);






  /*Procediment de subscripcio*/
  /*Mantenir comunicacio periodica amb el Servidor*/
  /*Enviar dades cap al Servidor*/
  /*Rebre dades del Servidor*/
  /*Esperar comandes per consola*/
  return 0;

}
/*Funcio que llegeix el fitxer de configuracio del client*/
void llegir_fitxer(char * f,struct configuracio * controlador){
  int fd;
  char c;
  int line=0;
  /*int dispositiu=0;*/
  fd=open(f, O_RDONLY);
  if(fd ==-1){
    printf("Error en obrir el fitxer\n");
    exit(-1);
  }
  printf("fitxer a imprimir: %s\n",f);
  while(read(fd,&c,1)!=0){
    int i =0;
    if(c == '='){
      while(read(fd,&c,1)!=0 && c !='\n'){
        if(c != ' '){
          if(line==0){
            controlador->name[i]=c;
            if(i+1==8){
              controlador->name[i+1]='\0';
            }

          }
          else if(line==1){
            controlador->situation[i]=c;
            if(i+1==12){
              controlador->situation[i+1]='\0';
            }
          }
          else if (line==2){
            if(c!=';'){
              controlador->elements[dispositiu][i]=c;
              if(i+1==7){
                controlador->elements[dispositiu][i+1]='\0';
              }
            }else{
              i=-1;
              dispositiu++;
            }
          }
          else if (line==3){
            controlador->mac[i]=c;
            if(i+1==12){
              controlador->mac[i+1]='\0';
            }

          }
          else if(line==4){
            controlador->tcp[i]=c;
            if(i+1==4){
              controlador->tcp[i+1]='\0';

            }
          }
          else if(line==5){
            controlador->server[i]=c;
            if(i+1==9){
              controlador->server[i+1]='\0';

            }
          }
          else if(line==6){
            controlador->srv[i]=c;
            if(i+1==4){
              controlador->srv[i+1]='\0';

            }
          }
          i++;
        }
      }
      line++;

    }

  }


  close(fd);
}
void init_pdu(struct pdu_udp * paquet,struct configuracio * client,unsigned char tipus_paquet,char numero_aleatori[9]){
  if(tipus_paquet==SUBS_REQ){
    paquet->tipus_paquet=SUBS_REQ;
    strcpy(paquet->dades,client->name);
    paquet->dades[8]=',';
    paquet->dades[9]='\0';
    strcat(paquet->dades,client->situation);
  }else if(tipus_paquet==SUBS_INFO){
    int i=0;
    int pos=4;
    paquet->tipus_paquet=SUBS_INFO;
    strcpy(paquet->dades,client->tcp);
    paquet->dades[4]=',';
    paquet->dades[5]='\0';

    while(i<=dispositiu){
      printf("%i dispositiu\n",dispositiu);
      printf("%s\n",client->elements[i]);
      strcat(paquet->dades,client->elements[i]);
      pos=pos+8;
      if(i!=dispositiu){
        paquet->dades[pos]=';';
        paquet->dades[pos+1]='\0';
      }


      i++;
    }



  }
  strcpy(paquet->numero_aleatori,numero_aleatori);
  strcpy(paquet->mac,client->mac);

}
void subscripcio(struct configuracio client,struct sockaddr_in addr_server){
  fd_set rfds;
  int retval;
  struct pdu_udp paquet;
  struct timeval timeout;
  int nopaquets=0;
  long temps=t;
  timeout.tv_sec=temps;
  timeout.tv_usec = 0;
  FD_ZERO(&rfds);
  FD_SET(sock, &rfds);
  estat=NOT_SUBSCRIBED;
  if(estat==NOT_SUBSCRIBED){
    printf("%s  MSG => Client passa a l'estat NOT_SUBSCRIBED\n",get_time());
  }
  init_pdu(&paquet,&client,SUBS_REQ,"00000000");
  printf("Paquet SUBSREQ %s\n",paquet.dades);
  sendto(sock,&paquet,sizeof(paquet),0,(const struct sockaddr *)&addr_server,sizeof(addr_server));
  nopaquets++;
  if(nopaquets==1){
    estat=WAIT_ACK_SUBS;
  }
  retval=select(sock+1,&rfds,NULL,NULL,&timeout);
  if(retval==-1){
    perror("select()");

  }else if(retval){
    printf("Data disponible\n");
  }else{
    printf("No dades en 1s\n");
  }

}
char * get_time(){
  char aux[80];
  char * now = malloc(sizeof(char) * 80);
  time_t hora=time(0);
  struct tm * tlocal=localtime(&hora);
  strftime(aux,80,"%H:%M:%S",tlocal);
  strcpy(now,aux);
  return now;
}
