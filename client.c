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
#include <signal.h>
#include<pthread.h>
/*Variables globals*/
int dispositiu=0;
int debug=0;/*Variable que permet els missatges adicionals*/
char * ip;
char * localhost;
int estat;/*estat del client*/
int sock;/*socket udp*/
int sock_tcp;
int procesos;/*procesos de subscripico*/
long temps_subscripcio;/*Interval de temps en la subscripcio*/
int nopaquets;
int prebuts;
pthread_t tid [3];
struct hostent  * server;
struct sockaddr_in addr_server; /* per a la informacio de la dirreccio del servidor */
struct idServidor dadesServidor;
char port_udp[6];
char port_tcp_servidor[6];
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
  char tcp[6];
  char server[10];
  char srv[6];
};
struct idServidor{/*Estructura per a emmagatzemar les dades d'identificacio del servidor*/
  char mac[13];
  char numero_aleatori[9];
  char ip[10];
};
/*Format PDU UDP*/
struct pdu_udp{/*Estructura per enviar paquets udp*/
  unsigned char tipus_paquet;
  char mac[13];
  char numero_aleatori[9];
  char dades[80];
};
/*Format PDU TCP*/
struct pdu_tcp{/*Estructura per enviar paquets tcp*/
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
/*Capçaleres*/
void llegir_fitxer(char * f,struct configuracio * controlador);
void init_pdu(struct pdu_udp * paquet,struct configuracio * client,unsigned char tipus_paquet,char numero_aleatori[9]);
void subscripcio(struct configuracio client,struct sockaddr_in addr_server);
char * get_time();
void iniciarNouProcesDeSubscripcio();
void reinicialitzar_proces_subscripcio();
int comprovarDadesServidor(struct idServidor dadesServidor,struct pdu_udp paquet);
void mantenir_comunicacio();
void gestor_sigalarm(int sig);
void *introduir_comandes_per_consola(struct configuracio * client);
int main(int argc,char * argv[]){
  char * fitxer;
  int i;
  struct configuracio client;
  fitxer="client.cfg";
  procesos=1;

  for(i=1;i<argc;i++){
    if(strcmp(argv[i],"-c")==0){
      fitxer=argv[i+1];
      continue;
    }
    if (strcmp(argv[i],"-d")==0) {
      debug=1;
    }



  }
  if (debug == 1){
    printf("%s  DEBUG =>Llegits parametres de linia de comandes\n",get_time());
  }

  /*Llegir fitxer de configuracio*/
  llegir_fitxer(fitxer ,&client);
  if (debug == 1){
    printf("%s  DEBUG =>Llegits parametres arxiu de configuració\n",get_time());
  }

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
  ip=inet_ntoa(*(struct in_addr *)*(server->h_addr_list)++);
  localhost=ip;
  memset(&addr_server,0,sizeof (struct sockaddr_in));
  addr_server.sin_addr.s_addr=inet_addr(localhost);
  addr_server.sin_family=AF_INET;
  addr_server.sin_port=htons(atoi(client.srv));
  if (debug == 1){
    printf("%s  DEBUG =>Inici bucle de servei equip :%s\n",get_time(),client.name);
  }
  while(1){

    subscripcio(client,addr_server);
    if(estat==SUBSCRIBED){
      mantenir_comunicacio(client);
      pthread_join(tid[0],NULL);
    }

  }







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
  if(tipus_paquet==SUBS_REQ || tipus_paquet==HELLO){
    if(tipus_paquet==SUBS_REQ){
      paquet->tipus_paquet=SUBS_REQ;
    }
    else{
      paquet->tipus_paquet=HELLO;
    }
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

  int finish=0;
  /*int procesos=1;*/
  fd_set rfds;
  int retval;
  struct pdu_udp paquet;
  struct timeval timeout;
  /*int norespostos=0;*/
  prebuts=0;
  /*long temps=t;*/
  nopaquets=0;
  temps_subscripcio=t;
  estat=NOT_SUBSCRIBED;
  while(finish==0){
    if(procesos>o){
      printf("%s  MSG =>El client no s'ha pogut connectar amb el servidor\n",get_time());
      exit(-1);
    }
    timeout.tv_sec=temps_subscripcio;
    timeout.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    if(estat==NOT_SUBSCRIBED){
      printf("%s  MSG => Controlador en l'estat NOT_SUBSCRIBED,procés de subscripció o: %i\n",get_time(),procesos);
      init_pdu(&paquet,&client,SUBS_REQ,"00000000");
      addr_server.sin_port=htons(atoi(client.srv));

    }
    if (debug==1){
      if(estat==WAIT_ACK_SUBS){
        printf("%s  DEBUG =>  Enviat: bytes=103, comanda=SUBS_REQ, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet.mac,paquet.numero_aleatori,paquet.dades);
      }

    }

    sendto(sock,&paquet,sizeof(paquet),0,(const struct sockaddr *)&addr_server,sizeof(addr_server));
    nopaquets++;
    if(nopaquets==1){
      if (debug==1){
        printf("%s  DEBUG =>  Enviat: bytes=103, comanda=SUBS_REQ, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet.mac,paquet.numero_aleatori,paquet.dades);
      }
      estat=WAIT_ACK_SUBS;
      printf("%s  MSG => Controlador passa a l'estat WAIT_ACK_SUBS\n",get_time());
    }
    retval=select(sock+1,&rfds,NULL,NULL,&timeout);
    if(retval==-1){
      perror("select()");

    }else if(retval>0){
      recvfrom(sock,&paquet,sizeof(paquet),0,(struct sockaddr *)&addr_server,(socklen_t *)sizeof(addr_server));
      if(paquet.tipus_paquet==SUBS_ACK){
        prebuts++;
        if (debug==1){
          printf("%s  DEBUG =>  REBUT: bytes=103, comanda=SUBS_ACK, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet.mac,paquet.numero_aleatori,paquet.dades);
        }

        if(estat==WAIT_ACK_SUBS){
          ip=inet_ntoa(addr_server.sin_addr);
          if(prebuts==1){
            strcpy(dadesServidor.ip,inet_ntoa(addr_server.sin_addr));
            printf("Adreça IP %s\n",dadesServidor.ip);
            strcpy(dadesServidor.mac,paquet.mac);
            strcpy(dadesServidor.numero_aleatori,paquet.numero_aleatori);
            strcpy(port_udp,paquet.dades);
            addr_server.sin_port=htons(atoi(port_udp));
            if (debug==1){
              printf("%s  DEBUG =>  Dades del servidor emmagatzemades\n",get_time());
            }
            init_pdu(&paquet,&client,SUBS_INFO,dadesServidor.numero_aleatori);
            estat=WAIT_ACK_INFO;
            if (debug==1){
              printf("%s  DEBUG =>  Enviat: bytes=103, comanda=SUBS_INFO, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet.mac,paquet.numero_aleatori,paquet.dades);
            }
            printf("%s  MSG => Controlador passa a l'estat WAIT_ACK_INFO\n",get_time());

          }


        }else{
          if (debug==1){
            printf("%s  DEBUG => Rebut Paquet SUBS_ACK en estat incorrecte \n",get_time());
          }
          iniciarNouProcesDeSubscripcio();
        }

      }
      else if (paquet.tipus_paquet==SUBS_NACK) {
        /* code */
        if (debug==1){
          printf("%s  DEBUG =>  Rebut: bytes=103, comanda=SUBS_NACK, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet.mac,paquet.numero_aleatori,paquet.dades);
        }
        iniciarNouProcesDeSubscripcio();
      }
      else if (paquet.tipus_paquet==SUBS_REJ) {
        /* code */
        if (debug==1){
          printf("%s  DEBUG =>  Rebut: bytes=103, comanda=SUBS_REJ, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet.mac,paquet.numero_aleatori,paquet.dades);
          printf("%s  DEBUG => Rebutjat paquet de subscripció enviat, motiu: %s\n",get_time(),paquet.dades);
        }
        iniciarNouProcesDeSubscripcio();
      }
      else if (paquet.tipus_paquet==INFO_ACK) {
        /* code */
        if (debug==1){
          printf("%s  DEBUG =>  Rebut: bytes=103, comanda=INFO_ACK, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet.mac,paquet.numero_aleatori,paquet.dades);
        }

        if(estat==WAIT_ACK_INFO){
          strcpy(port_tcp_servidor,paquet.dades);
          printf("Port tcp per a enviar Controladors %s\n",port_tcp_servidor);
          if(comprovarDadesServidor(dadesServidor,paquet)==0){
            estat=SUBSCRIBED;
            if (debug==1){
              printf("%s  DEBUG =>  Acceptada la subscripció del controlador en el servidor\n",get_time());
            }
            printf("%s  MSG => Controlador passa a l'estat SUBSCRIBED\n",get_time());

            finish=1;
          }else{
            if (debug==1){
              printf("%s  DEBUG =>Error en les dades d'identificació del servidor (rebut ip: %s, mac: %s, rndm: %s)\n",get_time(),ip,paquet.mac,paquet.numero_aleatori);
            }
            iniciarNouProcesDeSubscripcio();

          }

        }else{
          iniciarNouProcesDeSubscripcio();
        }
      }

    }else if(retval==0){
      /*norespostos++;*/
      printf("No dades en %lu s\n",temps_subscripcio);
      if(nopaquets>p){
        if(temps_subscripcio<q*t){
          temps_subscripcio=temps_subscripcio + t;
        }
        if(nopaquets==n){
          iniciarNouProcesDeSubscripcio();

        }
      }
    }

  }

}
void mantenir_comunicacio(struct configuracio client){
  struct pdu_udp paquet_enviat;
  struct pdu_udp paquet_rebut;
  struct timeval timeout;
  fd_set rfds;
  int retval;
  int hello_rebuts=0;
  int hello_perduts=0;
  int temps=r*v-v;
  /*int port_tcp_obert=0;*/
  if (debug==1){
    printf("%s  DEBUG =>  El controlador comença el enviament periòdic de HELLO \n",get_time());
    printf("%s  DEBUG =>  Establert temporitzador per enviament de HELLO \n",get_time());
  }
  init_pdu(&paquet_enviat,&client,HELLO,dadesServidor.numero_aleatori);
  timeout.tv_sec=temps;
  timeout.tv_usec = 0;
  while(estat==SUBSCRIBED||estat==SEND_HELLO){
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    printf("Enviant HELLO\n");
    sendto(sock,&paquet_enviat,sizeof(paquet_enviat),0,(const struct sockaddr *)&addr_server,sizeof(addr_server));
    if (debug==1){
      printf("%s  DEBUG =>  Enviat: bytes=103, comanda=HELLO, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet_enviat.mac,paquet_enviat.numero_aleatori,paquet_enviat.dades);
    }
    signal(SIGALRM,gestor_sigalarm);
    alarm(v);
    pause();
    retval=select(sock+1,&rfds,NULL,NULL,&timeout);
    if(retval==-1){
      perror("Error select\n");
    }else if(retval>0){
      recvfrom(sock,&paquet_rebut,sizeof(paquet_rebut),0,(struct sockaddr *)&addr_server,(socklen_t *)sizeof(addr_server));
      ip=inet_ntoa(addr_server.sin_addr);
      if(paquet_rebut.tipus_paquet==HELLO){
        if (debug==1){
          printf("%s  DEBUG =>  Rebut: bytes=103, comanda=HELLO, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet_rebut.mac,paquet_rebut.numero_aleatori,paquet_rebut.dades);
        }
        if(hello_rebuts==0){
          estat=SEND_HELLO;
          printf("%s  MSG => Controlador passa a l'estat SEND_HELLO\n",get_time());
          timeout.tv_sec=0;
          if (debug==1){
            printf("%s  DEBUG => Obert Port TCP %s per la comunicació amb el servidor\n",get_time(),client.tcp);
          }
          sock_tcp=socket(AF_INET,SOCK_STREAM,0);/*Obrir port tcp en l'estat SEND_HELLO*/
          pthread_create(&tid[0], NULL, (void *(*) (void *)) introduir_comandes_per_consola,&client);
          if (debug==1){
            printf("%s  DEBUG => Creat procés per a introduir comandes per consola\n",get_time());
          }
        }
        hello_perduts=0;/*Reinicialitza el comptador de hello no rebuts consecutius*/
        hello_rebuts++;
        if(comprovarDadesServidor(dadesServidor,paquet_rebut)!=0 || strcmp(paquet_enviat.dades,paquet_rebut.dades)!=0){
          /*Enviar HELLO_REJ*/
          if (debug==1){
            printf("%s  DEBUG => Error en les dades d'identificació del servidor (rebut ip: %s,mac: %s,rndm: %s)\n",get_time(),ip,paquet_rebut.mac,paquet_rebut.numero_aleatori);
          }
          paquet_enviat.tipus_paquet=HELLO_REJ;
          strcpy(paquet_enviat.dades,"Dades d'identificació incorrectes");
          sendto(sock,&paquet_enviat,sizeof(paquet_enviat),0,(const struct sockaddr *)&addr_server,sizeof(addr_server));
          if (debug==1){
            printf("%s  DEBUG =>  Enviat: bytes=103, comanda=HELLO_REJ, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet_enviat.mac,paquet_enviat.numero_aleatori,paquet_enviat.dades);
          }
          /*Iniciar nou procés de subscripcio*/
          iniciarNouProcesDeSubscripcio();
          /*reinicialitzar_proces_subscripcio();*/
        }
      }else{
        if (debug==1){
          printf("%s  DEBUG =>  Rebut: bytes=103, comanda=HELLO_REJ, mac=%s, rndm=%s,dades=%s\n",get_time(),paquet_rebut.mac,paquet_rebut.numero_aleatori,paquet_rebut.dades);
        }
        /*Iniciar nou procés de subscripcio*/
        iniciarNouProcesDeSubscripcio();
        /*reinicialitzar_proces_subscripcio();*/
      }


    }else if(retval==0){
      if(hello_rebuts==0){/*No ha rebut el primer HELLO en el temps maxim*/
        /*reinicialitzar_proces_subscripcio();*/
        printf("%s  Finalitzat el temporitzador per la confirmació del primer HELLO(%i)\n",get_time(),r*v);
        iniciarNouProcesDeSubscripcio();
      }
      hello_perduts++;
      if(hello_perduts==s){
        iniciarNouProcesDeSubscripcio();
      }

    }
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
void iniciarNouProcesDeSubscripcio(){
  estat=NOT_SUBSCRIBED;
  printf("%s  MSG => Controlador passa a l'estat NOT_SUBSCRIBED\n",get_time());
  signal(SIGALRM,gestor_sigalarm);
  alarm(u);
  pause();
  procesos=procesos+1;
  nopaquets=0;
  temps_subscripcio=t;
  prebuts=0;
}
int comprovarDadesServidor(struct idServidor dadesServidor,struct pdu_udp paquet){
  if(strcmp(dadesServidor.mac,paquet.mac)==0
  && strcmp(dadesServidor.numero_aleatori,paquet.numero_aleatori)==0
  && strcmp(dadesServidor.ip,ip)==0){
    return 0;
  }else{
    return -1;
  }
}
void gestor_sigalarm(int sig){

}
void reinicialitzar_proces_subscripcio(){
  procesos=1;
  estat=NOT_SUBSCRIBED;
  nopaquets=0;
  temps_subscripcio=t;
  prebuts=0;

}
void *introduir_comandes_per_consola(struct configuracio * client){
  struct timeval timeout;
  fd_set rfds;
  int retval;
  int i;
  char  comanda[50];
  timeout.tv_sec=0;
  timeout.tv_usec=0;

  while(estat==SEND_HELLO){
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    retval=select(1,&rfds,NULL,NULL,&timeout);
    if(retval==-1){
      perror("select()");
    }else if(retval>0){
      scanf("%s",comanda);
      if(strcmp(comanda,"stat")==0){
        fprintf(stdout,"******************** DADES CONTROLADOR *********************\n");
        fprintf(stdout,"  MAC: %s, Nom: %s, Situació: %s\n",client->mac,client->name
        ,client->situation);
        fprintf(stdout,"Estat: SEND_HELLO\n");
        fprintf(stdout,"    Dispos.	 valor\n");
        fprintf(stdout,"    -------	 ------\n");
        for(i=0;i<=dispositiu;i++){
          fprintf(stdout,"%s\n",client->elements[i]);
        }
        fprintf(stdout,"************************************************************\n");

      }
      else if(strcmp(comanda,"quit")==0){
        if (debug==1){
          printf("%s  DEBUG =>  Tancat socket TCP per la comunicació amb el servidor\n",get_time());
          printf("%s  DEBUG =>  Tancat socket UDP per la comunicació amb el servidor\n",get_time());
        }
        fprintf(stdout,"Terminat\n");
        exit(0);
      }
      else{
        fprintf(stdout,"%s  MSG => Comanda Incorrecta (%s)\n",get_time(),comanda);
      }

    }

  }
  return NULL;

}
