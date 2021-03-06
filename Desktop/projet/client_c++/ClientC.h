#ifndef CLIENTC_H
#define CLIENTC_H
#include <iostream>
#include "sock/sock.h"
#include "sock/sockdist.h"
#include "utilisateur/Info.h"
using namespace std;

class ClientC{
 private:
  Sock brPri;
  SockDist brPub;
  struct sockaddr_in* adrBrPub;
  int lgAdrBrPub;
  
  int descbreCli;
  int descLectMax;

  fd_set desc_en_lect;
  
  Info joueur;
  
 public:
  ClientC(char*);
  
  void entrePseudoMdp(char*);
  
  void lanceClient();
  
  void recuServeur();
  void recuTchat(char*);

  void envoieServeur(const int);
  void envoieTchat();
  void envoieModif();
};
#endif
