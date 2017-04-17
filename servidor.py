#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-
import argparse
import socket
import struct
import select
import re
from random import randint
import threading
import sys
import time


#Constants
SUBS_REQ=0x00
SUBS_ACK=0x01
SUBS_INFO=0x02
INFO_ACK=0x03
SUBS_NACK=0x04
SUBS_REJ=0x05
HELLO=0x10
HELLO_REJ=0x11
s=2.0
t=1.0
x=4





class PaquetUDP(object):#Paquets per al protocol udp
    def __init__(self,tipus_paquet=None,mac=None,numero_aleatori=None,dades=None):
        self.tipus_paquet=tipus_paquet#1
        self.mac=mac#13
        self.numero_aleatori=numero_aleatori#9
        self.dades=dades#80



class Server(object):
    def __init__(self,fitxer_configuracio='server.cfg',fitxer_autoritzats='controlers.dat'):
        self.fitxer_configuracio=fitxer_configuracio #fitxer configuracio servidor
        self.fitxer_autoritzats=fitxer_autoritzats#fitxer controladors autoritzats
        self.configuracio={}#diccionari que emmagatzema les dades de configuracio del servidor
        self.controladors={}#Controladors autoritzats en el sistema
        self.socket_udp=None#socket udp de la configuracio
        self.socket_tcp=None#socket tcp de la configuraico
        self.debug=False



    #funcio que emmagatzema les dades del fitxer de configuracio del servidor
    def llegir_fitxer_configuracio(self):
        reader=open(self.fitxer_configuracio,'r')
        lines=[l.strip() for l in reader]
        for l in lines:
            if l !='':
                value=l.split()
                self.configuracio[value[0]]=value[2]
        reader.close()
        if self.debug==True:
            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Controlador llegeix l'arxiu de configuracio"

    #fitxer que emmagatema els controladors autoritzats amb la seva respectiva mac
    def llegir_fitxer_autoritzats(self):
        reader=open(self.fitxer_autoritzats,'r')
        lines=[l.strip() for l in reader]
        for l in lines:
            value=l.split(',')
            #clau nom del controlador i s'inicialitza els seus parametres
            self.controladors[value[0]]={'mac':value[1],'estat':'DISCONNECTED','rndm':"",'udp':"",'tcp':"",\
                                         'elements':"",'ip':"",'situacio':""}
        reader.close()
        if self.debug==True:
            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Controlador llegeix l'arxiu de controladors autoritzats: "\
            +str(len(self.controladors))

    def esperar_comandes(self):
        if self.debug==True:
            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Creat fill per a gestionar les comandes"
        while(True):#Mentre el programa estigui en funcionament
            i, o, e = select.select([0], [], [], 0.0)#Mira si es pot fer lectura del fd 0 sense bloqueig
            if len(i)!=0:
                line=raw_input()#Introduir comanda
                if line == 'quit':
                    print "Terminat"
                    sys.exit(0)#Finalitza el programa
                elif(line=='list'):
                    print "--NOM--- ------IP------- -----MAC---- --RNDM-- ----ESTAT--- --SITUACIÓ-- --ELEMENTS-------------------------------------------"
                    autoritzats=self.controladors#controladors autoritzats
                    for client in autoritzats:#Mirem les claus(noms del diccionari)
                        dades_client=autoritzats[client]
                        #s'escriu les dades del client quan ja s'ha subscrit
                        if dades_client['estat']=='SUBSCRIBED' or dades_client['estat']=='SEND_HELLO':
                            print client +"   "+dades_client['ip']+"     "+dades_client['mac']+"  "\
                              +dades_client['rndm']+" "+dades_client['estat']+"  "+dades_client['situacio']\
                            +"   "+dades_client['elements']
                        else:# nom del client i estat
                            print client +"                                        "+dades_client['estat']



                else:
                    print "Comanda incorrecta ("+line+")"




    def setup(self):
        self.socket_udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)#socket udp
        self.socket_tcp=socket.socket(socket.AF_INET,socket.SOCK_STREAM)#socket tcp
        #Es vincula al port_udp del fitxer de configuracio del servidor
        self.socket_udp.bind(('localhost',int(self.configuracio['UDP-port'])))
        #Es vincula al port_tcp del fitxer de configuracio del servidor
        self.socket_tcp.bind(('localhost',int(self.configuracio['TCP-port'])))
        if self.debug==True:
            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Socket UDP actiu"
            print time.strftime("%H:%M:%S", time.localtime()) + ' ' + "DEBUG.  --> Socket TCP actiu"

    def atendre_peticions_de_subscripcio(self,paquet_rebut,ip,port):

        situacio=[]
        # Si el paquet SUBS_REQ conté a les dades el nom i la situacio del controlador
        if len(str(paquet_rebut.dades))==21:
            nom,situacio = str(paquet_rebut.dades).split(',')
        else:#El paquet conte el nom del controlador
            nom=str(paquet_rebut.dades)

        autoritzat=self.controlador_autoritzat(nom,paquet_rebut.mac)#Controlador autoritzat?
        situacio_correcta=self.comprovar_situacio(situacio)#Situacio enviada i amb el format correcte
        #Si les dades son incorrectes
        if autoritzat==False or paquet_rebut.numero_aleatori!="00000000" or situacio_correcta==False:
            print "Dades Incorrectes"
            if autoritzat==False:# El controlador no esta autoritzat perque no esta en el fitxer o la mac no es la correcta
                pdu=PaquetUDP(SUBS_REJ,self.configuracio['MAC'],"00000000","Controlador no autoritzat en el sistema")
            elif(paquet_rebut.numero_aleatori!="00000000" or situacio_correcta==False ):# Si les dades son incorrectes
                pdu=PaquetUDP(SUBS_REJ,self.configuracio['MAC'],"00000000","El Controlador ha enviat dades incorrectes")
                self.controladors[nom]['estat'] = 'DISCONNECTED'
                print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "MSG.  --> Controlador " + nom + " passa a l'estat DISCONNECTED"

            paquet=convert_pdu_to_struct(pdu)#Converteix el paquet en struct
            self.socket_udp.sendto(paquet,(ip,port))#Envia el paquet pel socket udp del servidor
            if self.debug==True:
                print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Enviat:103 bytes, tipus=SUBS_REJ, " \
                      + " mac = %s, rndm = %s , dades = %s" % (pdu.mac, pdu.numero_aleatori, pdu.dades)
            if self.debug == True:
                print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Finalitzat procés que atenia el paquet UDP "
            return -1
        else:#Dades correctes
            print "Dades Correctes"
            if self.controladors[nom]["estat"]=='DISCONNECTED':#Esta a l'estat DISCONNECTED
                num=str(randint(0,99999999))#numero aleatori de 8 digits generat
                if(len(num)<8):#Si el numero no es de 8 digits afegeix 0 per l'esquerra fins que hi hagi 8
                    zeros=8-len(num)
                    for x in range(zeros):
                        num='0'+num
                socket_udp_client=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)#crea el nou socket pel client
                socket_udp_client.bind(('',0))#El vincula per qualsevol port disponible
                port_udp=str(socket_udp_client.getsockname()[1])#Obtenim el nou port
                print "Nou port "+str(port_udp)
                pdu=PaquetUDP(SUBS_ACK,self.configuracio['MAC'],num,port_udp)#Crea paquet SUBS_ACK
                paquet=convert_pdu_to_struct(pdu)#converteix el paquet a struct
                mida=self.socket_udp.sendto(paquet, (ip, port))#envia el paquet
                if self.debug == True:
                    print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Enviat:103 bytes, tipus=SUBS_ACK, " \
                          + " mac = %s, rndm = %s , dades = %s" % (pdu.mac, pdu.numero_aleatori, pdu.dades)
                self.controladors[nom]['estat'] = 'WAIT_INFO'  # El client passa a l'estat WAIT_INFO
                print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "MSG.  --> Controlador " + nom + " passa a l'estat WAIT_INFO"
                i, o, e = select.select([socket_udp_client], [], [], s*t)#Espera s vegades t el paquet SUBS_INFO
                if(len(i)!=0):#S'ha rebut el paquet
                    (data, (ip, port)) = socket_udp_client.recvfrom(1 + 13 + 9 + 80)#Emmagatzema el struct
                    pdu = convert_to_Paquet_UDP(data)#converteix el struct
                    if self.debug==True:
                        print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Rebut:103 bytes, comanda=SUBS_INFO, " \
                              + " mac = %s, rndm = %s , dades = %s" % (pdu.mac, pdu.numero_aleatori, pdu.dades)
                    port_tcp=None
                    elements=None
                    if len(pdu.dades) != 0:#Si hi ha dades es queda amb el port i els dispositius
                        port_tcp, elements = pdu.dades.split(',')
                    if self.controlador_autoritzat(nom,pdu.mac)==True and num==pdu.numero_aleatori and \
                            port_tcp!=None and elements!=None:#Si el paquet SUBS_INFO es correcte
                        print "PAQUET SUBS_INFO correcte"
                        self.controladors[nom]['elements']=elements
                        self.controladors[nom]['ip']=ip
                        self.controladors[nom]['situacio']=situacio
                        self.controladors[nom]['rndm']=num
                        self.controladors[nom]['tcp']=port_tcp
                        self.controladors[nom]['udp']=port_udp
                        self.controladors[nom]['hello']=0
                        self.controladors[nom]['nohello'] = 0
                        # Crea paquet INFO_ACK
                        pdu = PaquetUDP(INFO_ACK, self.configuracio['MAC'], num, self.configuracio['TCP-port'])
                        #Converteix el paquet a struct
                        paquet = convert_pdu_to_struct(pdu)
                        #Envia el paquet
                        socket_udp_client.sendto(paquet, (ip, port))
                        if self.debug == True:
                            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Enviat:103 bytes, tipus=INFO_ACK, " \
                                  + " mac = %s, rndm = %s , dades = %s" % (pdu.mac, pdu.numero_aleatori, pdu.dades)
                            # El client passa a l'estat SUBSCRIBED per tant s'emmagatzema les dades
                        self.controladors[nom]['estat'] = 'SUBSCRIBED'
                        print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "MSG.  --> Controlador " + nom + " passa a l'estat SUBSCRIBED "
                        #Acaba el procés
                        socket_udp_client.close()
                        if self.debug == True:
                            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Finalitzat procés que atenia el paquet UDP "
                        return 0
                    else:
                        print "PAQUET SUBS_INFO Incorrecte"
                        #Crea el paquet SUBS_REJ
                        pdu = PaquetUDP(SUBS_REJ, self.configuracio['MAC'], "00000000","Dades identificació incorrectes en paquet SUBS_INFO")
                        #El converteix a struct
                        paquet = convert_pdu_to_struct(pdu)
                        mida=socket_udp_client.sendto(paquet, (ip, port))
                        if self.debug==True:
                            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Enviat:103 bytes, tipus=SUBS_REJ, " \
                                  + " mac = %s, rndm = %s , dades = %s" % (pdu.mac, pdu.numero_aleatori, pdu.dades)
                        self.controladors[nom]['estat'] = 'DISCONNECTED'
                        print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "MSG.  --> Controlador " + nom + " passa a l'estat DISCONNECTED"
                        #Finalitza procés
                        socket_udp_client.close()
                        if self.debug == True:
                            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Finalitzat procés que atenia el paquet UDP "
                        return -1


                else:
                    if self.debug==True:
                        print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Controlador " + nom + " no ha rebut paquet SUBS_INFO"

                    #Client passa a l'estat DISCONNECTED
                    self.controladors[nom]['estat']='DISCONNECTED'
                    print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "MSG.  --> Controlador " + nom + " passa a l'estat DISCONNECTED"
                    #Finalitza el procés
                    socket_udp_client.close()
                    if self.debug == True:
                        print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Finalitzat procés que atenia el paquet UDP "
                    return -1

    def respondre_paquets_hello(self,paquet_rebut,ip,port):
        socket_udp_client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # crea el nou socket pel client
        situacio=[]
        if len(str(paquet_rebut.dades))==21:#Comprova que el paquet contingui el nom i la situacio
            nom,situacio = str(paquet_rebut.dades).split(',')
        else:
            nom=str(paquet_rebut.dades)#Conte el nom del controlador
        if self.debug == True:
            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Rebut paquet HELLO del Controlador "\
            +nom +" ["+self.controladors[nom]['mac']+"]"
        self.controladors[nom]['nohello']=0#Inicialitza el numero de hello no rebuts
        # Vincula el socket al nou port udp per atendre al controlador
        socket_udp_client.bind(('', int(self.controladors[nom]['udp'])))
        #Comprova que el controlador estigui autoritzat
        autoritzat= self.controlador_autoritzat(nom,paquet_rebut.mac)
        #Comprova que la situacio tingui la longitud i el format correcte
        situacio_correcta=self.comprovar_situacio(situacio)
        # Si les dades son incorrectes o el controlador no està ni a l'estat SUBSCRIBED ni SEND_HELLO
        if autoritzat == False or situacio_correcta == False or (self.controladors[nom]['estat']!='SUBSCRIBED'\
                and self.controladors[nom]['estat']!='SEND_HELLO')or self.controladors[nom]['rndm']!=paquet_rebut.numero_aleatori:
            pdu = PaquetUDP(HELLO_REJ, self.configuracio['MAC'], self.controladors[nom]['rndm'],'Dades de identificacio incorrectes')
            if self.debug==True:
                print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Enviat:103 bytes, tipus=HELLO_REJ, " \
                      + " mac = %s, rndm = %s , dades = %s" % (pdu.mac, pdu.numero_aleatori, pdu.dades)
            self.controladors[nom]['estat'] = 'DISCONNECTED'
            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "MSG.  --> Controlador " + nom + " passa a l'estat DISCONNECTED"
        else:
            if self.controladors[nom]['hello'] == 0:#Si  ha rebut el primer hello
                self.controladors[nom]['estat'] = 'SEND_HELLO'
                print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "MSG.  --> Controlador " + nom + " passa a l'estat SEND_HELLO"
            self.controladors[nom]['hello'] += 1#Incrementa el numero de hello rebuts
            #Crea el HELLO de resposta
            pdu=PaquetUDP(HELLO,self.configuracio['MAC'],self.controladors[nom]['rndm'],paquet_rebut.dades)
            if self.debug == True:
                print time.strftime("%H:%M:%S", time.localtime()) + ' ' + "DEBUG.  --> Enviat:103 bytes, tipus=HELLO, " \
                  + " mac = %s, rndm = %s , dades = %s" % (pdu.mac, pdu.numero_aleatori, pdu.dades)
        #Converteix la pdu en struct
        paquet = convert_pdu_to_struct(pdu)
        mida = socket_udp_client.sendto(paquet, (ip, port))
        if self.debug == True:
            print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Finalitzat procés que atenia el paquet UDP "





    def controlador_autoritzat(self,nom,mac):# Funcio que comprova si un controlador esta en el fitxer d'autoritzats

        if self.controladors.has_key(nom):#Comprova si el nom del controlador està al diccionari
            return self.controladors[nom]["mac"]==mac#Comprova si la mac passada es igual a la del fitxer d'autoritzats
        else:
            return False
    #Comprova si la longitud de la situacio es la correcta i el seu format es correcte
    def comprovar_situacio(self,situacio):
        return len(situacio)==12 and situacio[0]=='B' and situacio[3]=='F' and situacio[6]=='R' \
               and situacio[9]=='Z'











#Converteix el paquet udp en struct
def convert_pdu_to_struct(pdu):
    myStruct = struct.Struct("B13s9s80s")
    paquet=myStruct.pack(pdu.tipus_paquet,pdu.mac,pdu.numero_aleatori,pdu.dades)
    return paquet
# converteix el struct en paquet udp
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
    return pdu
def main(opts):
    finish=False
    server=Server()#crea el servidor
    if opts.configuracio!=None:#si es pasa el arxiu de configuracio es el quin llegira
        server.fitxer_configuracio=opts.configuracio
    if opts.autoritzats!=None:#si es pasa el fitxer d'autoritzats es el quin llegira
        server.fitxer_autoritzats=opts.autoritzats
    if opts.debug==True:# Si es posa l'opcio de debug es mostraran missatges adicionals
        server.debug=True

    server.llegir_fitxer_configuracio()#Es llegeix el fitxer de configuracio
    server.llegir_fitxer_autoritzats()#Es llegeix els controladors autoritzats en el sistema
    server.setup()#Es creen els sockets i es vinculen al port
    p=threading.Thread(target=server.esperar_comandes)# Es crea un fil que esperi comandes
    p.start()#Es posa en execució el fil
    while finish==False:
        if p.isAlive()==False:# Si el thread que controla de comandes no esta viu acabem l'execucio
            if server.debug==True:
                print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Petició de finalització"

            sys.exit(0)
        i,o,e=select.select([server.socket_udp],[],[],0.0)#Es comprova si ha rebut un paquet
        if len(i)!=0:#si ha rebut paquet
            print "Connexio UDP"
            (data,(ip,port))=server.socket_udp.recvfrom(1+13+9+80)#s'emmagatzema el paquet
            pdu=convert_to_Paquet_UDP(data) #es converteix el paquet en un paquet pdu
            if int(pdu.tipus_paquet)==SUBS_REQ:# si el paquet es SUBS_REQ
                if server.debug==True:
                    print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Rebut:103 bytes, comanda=SUBS_REQ, "\
                    +" mac = %s, rndm = %s , dades = %s"%(pdu.mac,pdu.numero_aleatori,pdu.dades)
                    print time.strftime("%H:%M:%S", time.localtime()) + ' ' + "DEBUG.  --> Rebut paquet UDP creat procés per atendre"
                #Es crea un fil per atendre la petició de subscrició
                t=threading.Thread(target=server.atendre_peticions_de_subscripcio,args=(pdu,ip,port))
                t.start()


            elif int(pdu.tipus_paquet)==HELLO:
                #Si el paquet es de tipus HELLO es crea un fil per respondre paquets hello
                if server.debug == True:
                    print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Rebut:103 bytes, comanda=HELLO, " \
                          + " mac = %s, rndm = %s , dades = %s" % (pdu.mac, pdu.numero_aleatori, pdu.dades)
                    print time.strftime("%H:%M:%S",time.localtime()) + ' ' + "DEBUG.  --> Rebut paquet UDP creat procés per atendre"
                t = threading.Thread(target=server.respondre_paquets_hello, args=(pdu, ip, port))
                t.start()













if __name__=='__main__':
    #Arguments que permet el programa
    parser=argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="", epilog="")
    parser.add_argument('-c','--configuracio',type=str,
                        help="Arxiu de dades de configuracio del servidor")
    parser.add_argument('-u','--autoritzats',type=str,help="Arxiu de controladors autoritzats")
    parser.add_argument('-d','--debug',action='store_true', help="DEBUG")
    main(parser.parse_args())

