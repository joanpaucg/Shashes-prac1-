#!/usr/bin/env python2.7
import argparse
import socket
import struct
import select
import re
from random import randint
import threading


#Constants
SUBS_REQ=0x00
SUBS_ACK=0x01
SUBS_INFO=0x02
INFO_ACK=0x03
SUBS_NACK=0X04
SUBS_REJ=0X05
s=2.0
t=1.0





class PaquetUDP(object):
    def __init__(self,tipus_paquet=None,mac=None,numero_aleatori=None,dades=None):
        self.tipus_paquet=tipus_paquet#1
        self.mac=mac#13
        self.numero_aleatori=numero_aleatori#9
        self.dades=dades#80



class Server(object):
    def __init__(self,fitxer_configuracio='server.cfg',fitxer_autoritzats='controlers.dat'):
        self.fitxer_configuracio=fitxer_configuracio
        self.fitxer_autoritzats=fitxer_autoritzats
        self.configuracio={}#diccionari que emmagatzema les dades de configuracio del servidor
        self.controladors={}#Controladors autoritzats en el sistema
        self.socket_udp=None
        self.socket_tcp=None
        self.socket_udp_client=None
        self.num=None



    def llegir_fitxer_configuracio(self):
        reader=open(self.fitxer_configuracio,'r')
        lines=[l.strip() for l in reader]
        for l in lines:
            if l !='':
                value=l.split()
                self.configuracio[value[0]]=value[2]
        reader.close()
    def llegir_fitxer_autoritzats(self):
        reader=open(self.fitxer_autoritzats,'r')
        lines=[l.strip() for l in reader]
        for l in lines:
            value=l.split(',')
            self.controladors[value[0]]={'mac':value[1],'estat':'DISCONNECTED'}
    def setup(self):
        self.socket_udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.socket_tcp=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        self.socket_udp.bind(('localhost',int(self.configuracio['UDP-port'])))
        self.socket_tcp.bind(('localhost',int(self.configuracio['TCP-port'])))
    def atendre_peticions_de_subscripcio(self,paquet_rebut,ip,port):
        nom,situacio = str(paquet_rebut.dades).split(',')
        if self.controlador_autoritzat(nom,paquet_rebut.mac)==False or paquet_rebut.numero_aleatori!="00000000":
            print "Dades Incorrectes"
            #print "Controlador "+nom + " passa a l'estat UNKNOWN"
            pdu=PaquetUDP(SUBS_REJ,self.configuracio['MAC'],"00000000","Discrepancia amb les dades del Controlador")
            paquet=convert_pdu_to_struct(pdu)
            self.socket_udp.sendto(paquet,(ip,port))
            return -1
        else:#Dades correctes
            if self.controladors[nom]["estat"]=='DISCONNECTED':
                num=str(randint(0,99999999))
                if(len(num)<8):
                    zeros=8-len(num)
                    for x in range(zeros):
                        num='0'+num
                self.num=num
                self.socket_udp_client=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)#crea el nou socket pel client
                self.socket_udp_client.bind(('',0))
                port_udp=str(self.socket_udp_client.getsockname()[1])
                print "Nou port "+str(port)
                print len(nom)
                self.controladors[nom]['estat']='WAIT_INFO'
                pdu=PaquetUDP(SUBS_ACK,self.configuracio['MAC'],num,port_udp)
                paquet=convert_pdu_to_struct(pdu)
                mida=self.socket_udp.sendto(paquet, (ip, port))
                i, o, e = select.select([self.socket_udp_client], [], [], s*t)
                if(len(i)!=0):
                    print "Paquet SUBS_INFO rebut"
                    (data, (ip, port)) = self.socket_udp_client.recvfrom(1 + 13 + 9 + 80)
                    pdu = convert_to_Paquet_UDP(data)
                    if self.controlador_autoritzat(nom,pdu.mac)==True and self.num==pdu.numero_aleatori:
                        print "Tot correcte"




                    return 0
                else:
                    print "Paquet SUBS_INFO no rebut"
                    self.controladors[nom]['estat']='DISCONNECTED'
                    return -1










    def controlador_autoritzat(self,nom,mac):# Funcio que comprova si un controlador esta en el fitxer d'autoritzats

        if self.controladors.has_key(nom):
            return self.controladors[nom]["mac"]==mac
        else:
            return False











def convert_pdu_to_struct(pdu):
    myStruct = struct.Struct("B13s9s80s")
    paquet=myStruct.pack(pdu.tipus_paquet,pdu.mac,pdu.numero_aleatori,pdu.dades)
    return paquet

def convert_to_Paquet_UDP(data):
    pdu = PaquetUDP()
    myStruct = struct.Struct("B13s9s80s")
    paquet = myStruct.unpack(data)
    pdu.tipus_paquet = str(paquet[0]).strip()
    pdu.mac = str(paquet[1])
    pdu.mac=re.sub(r'[^ -~].*', '', pdu.mac)
    pdu.numero_aleatori = str(paquet[2])
    pdu.numero_aleatori=re.sub(r'[^ -~].*', '',pdu.numero_aleatori)
    pdu.dades = str(paquet[3])
    pdu.dades=re.sub(r'[^ -~].*', '',pdu.dades)
    print (pdu.tipus_paquet)
    print (pdu.mac)
    print (pdu.numero_aleatori)
    print (pdu.dades)
    return pdu
def main():
    finish=False
    threads=list()
    server=Server()
    server.llegir_fitxer_configuracio()
    print server.configuracio
    server.llegir_fitxer_autoritzats()
    print server.controladors
    server.setup()
    while finish==False:
        i,o,e=select.select([server.socket_udp],[],[],0.0)
        if len(i)!=0:
            print "Connexio UDP"
            (data,(ip,port))=server.socket_udp.recvfrom(1+13+9+80)
            pdu=convert_to_Paquet_UDP(data)
            if int(pdu.tipus_paquet)==SUBS_REQ:
                print "Paquet SUBS_REQ"

                t=threading.Thread(target=server.atendre_peticions_de_subscripcio,args=(pdu,ip,port))
                t.start()
                #s.atendre_peticions_de_subscripcio(pdu,ip,port)








if __name__=='__main__':
    main()

