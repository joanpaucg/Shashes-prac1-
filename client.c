/*compilar comanda gcc client.c -ansi -pedantic -Wall*/
#include <stdio.h>/*printf i altres*/
#include <stdlib.h>/*Funcions fitxers*/
#include <fcntl.h> 	/*Modes apertura.*/
#include<unistd.h>
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
  char mac[13];
  char numero_aleatori[9];
  char dispositiu[8];
  char valor[7];
  char info[80];
};
void llegir_fitxer(char * f,char name[9],char situation[13],char elements [10][8],char mac[13],char tcp[5],char server[10],char srv[5]);
int main(int argc,char * argv[]){
  int estat= NOT_SUBSCRIBED;
  char * fitxer;
  char name[9];
  char situation[13];
  char elements [10][8];
  char mac[13];
  char tcp[5];
  char server[10];
  char srv[5];

  /*Llegir fitxer de configuracio*/
  if (argc == 1){
    fitxer="client.cfg";
  }
  llegir_fitxer(fitxer ,name,situation,elements,mac,tcp,server,srv);
  printf("Name:%s\n",name);
  printf("Situation:%s\n",situation);
  printf("Element0 :%s\n",elements[0]);
  printf("Element1 :%s\n",elements[1]);
  printf("Element2 :%s\n",elements[2]);
  printf("MAC:%s\n",mac);
  printf("TCP:%s\n",tcp);
  printf("server:%s\n",server);
  printf("srv:%s\n",srv);
  /*Procediment de subscripcio*/
  /*Mantenir comunicacio periodica amb el Servidor*/
  /*Enviar dades cap al Servidor*/
  /*Rebre dades del Servidor*/
  /*Esperar comandes per consola*/
  printf("%i\n",estat);
  return 0;

}
/*Funcio que llegeix el fitxer de configuracio del client*/
void llegir_fitxer(char * f,char name[9],char situation[13],char elements [10][8],char mac[13],char tcp[5],char server[10],char srv[5]){
  int fd;
  char c;
  int line=0;
  int dispositiu=0;
  fd=open(f, O_RDONLY);
  if(fd ==-1){
    printf("Error en obrir el fitxer");
    exit(-1);
  }
  printf("fitxer a imprimir: %s\n",f);
  while(read(fd,&c,1)!=0){
    int i =0;
    if(c == '='){
      while(read(fd,&c,1)!=0 && c !='\n'){
        if(c != ' '){
          if(line==0){
            name[i]=c;
            if(i+1==8){
              name[i+1]='\0';
            }

          }
          else if(line==1){
            situation[i]=c;
            if(i+1==12){
              situation[i+1]='\0';
            }
          }
          else if (line==2){
            if(c!=';'){
              elements[dispositiu][i]=c;
              if(i+1==7){
                elements[dispositiu][i+1]='\0';
              }
            }else{
              i=-1;
              dispositiu++;
            }
          }
          else if (line==3){
            mac[i]=c;
            if(i+1==12){
              mac[i+1]='\0';
            }

          }
          else if(line==4){
            tcp[i]=c;
            if(i+1==4){
              tcp[i+1]='\0';

            }
          }
          else if(line==5){
            server[i]=c;
            if(i+1==9){
              server[i+1]='\0';

            }
          }
          else if(line==6){
            srv[i]=c;
            if(i+1==4){
              srv[i+1]='\0';

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
