#include <string.h>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <vector>
#include <sstream>
//#include "/usr/include/mysql/mysql.h"
//#include <mysql/mysql.h>
#include "/usr/local/mysql5/include/mysql/mysql.h"
#include "Table.h"
#define PSEUDO 0
#define PASSWORD 1
#define MONEY 2
#define EMAIL 3
#define DOB 4
#define GENDER 5
#define LNAME 6
#define FNAME 7
#define ADRESS 8
#define CITY 9
#define ZIPCODE 10
#define COUNTRY 11
/////////////////////////////////////Variable globale//////////////////////////////
const char separateur[2]="&";
/****************************************************************************************/
/////////////////////////////////////Constructeur////////////////////////////////////
Table::Table(int maxJ, int miseDep){
  //Partie constructeur reseau
  brPub = Sock(SOCK_STREAM, (short)21346,0);
  if(brPub.good())
    descTable = brPub.getsDesc();
  else{
    cout<<"Probleme desc"<<endl;
    exit(1);
  }
  if(listen(descTable,10) == -1){
    cout<<"erreur de creation Boite Publique"<<endl;
  }
  isServeur=false;
  descMax=descTable;
  //Partie constructeur des joueurs
  nbJoueur=0;
  nbJMax = maxJ;
  joueurEnListe=0;
  nbJAllIn=0;
  prochainAMiser=0;
  dernierAMiser=0;
  joueurs=new Joueur*[nbJMax];
  for(int i=0;i<nbJMax;i++){
    joueurs[i]=NULL;
  }
  actif=new bool[nbJMax];
  for(int i=0;i<nbJMax;i++){
    actif[i]=false;
  }
  //Partie constructeur poker
  partieEnCours=false;
  dealer=0;
  aJouer=false;
  miseMin = miseDep;
  mise = miseMin;
  pot = 0;
  instanceAllIn=false;
  
}
Table::Table(int descE,int descR,int maxJ, int miseDep){
  cout<<"constructeur"<<endl;
  //Partie constructeur reseau
  descEnvoie=descE;
  descRecu=descR;
  isServeur=true;
  descMax=descRecu;
  brPub = Sock(SOCK_STREAM,(short)0,0);
  if(brPub.good())
    descTable = brPub.getsDesc();
  else{
    cout<<"Probleme desc"<<endl;
    exit(1);
  }
  if(listen(descTable,10) == -1){
    cout<<"erreur de creation Boite Publique"<<endl;
    exit(1);
  }
  char portChar[20]="";
  sprintf(portChar,"%d",brPub.returnPort());
  write(descEnvoie,portChar,20);
  if(descTable>descMax)
    descMax=descTable;
  //Partie constructeur des joueurs
  nbJoueur=0;
  nbJMax = maxJ;
  joueurEnListe=0;
  nbJAllIn=0;
  prochainAMiser=0;
  dernierAMiser=0;
  joueurs=new Joueur*[nbJMax];
  for(int i=0;i<nbJMax;i++){
    joueurs[i]=NULL;
  }
  actif=new bool[nbJMax];
  for(int i=0;i<nbJMax;i++){
    actif[i]=false;
  }
  //Partie constructeur poker
  partieEnCours=false;
  dealer=-1;
  petiteBlind=-1;
  grosseBlind=-1;
  aJouer=false;
  miseMin = miseDep;
  mise = miseMin;
  pot = 0;
  instanceAllIn=false;
  classementJoueurs=new int*[nbJMax];
  for(int i=0;i<nbJMax;i++){
    classementJoueurs[i]=new int[2];
  }

}
Table::~Table(){
  
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL){
      actif[i]=false;
      removeJoueur(i);
    }
  }
  for(int i=0;i<spectacteurs.size();i++){
    removeSpectateur(i);
  }
  for(int i=0;i<nbJMax;i++){
    delete[] classementJoueurs[i];
  }
  delete[] classementJoueurs;
  delete joueurs;
  delete actif;
}
/***********************************************************************************/
/////////////////////////////////                                        //////////////////
////////////////////////////////Ajouter/Suprimer/Verifier spec ou joueur//////////////////////
///////////////////////////////                                        //////////////////
void Table::addSpectateur(){
  struct sockaddr_in brCV;
  socklen_t lgbrCV=sizeof(struct sockaddr_in);
  int descBrCv = accept(descTable,(struct sockaddr *)&brCV, &lgbrCV);
  if(descBrCv==-1)
    cout<<"N'a pas accept�e le client"<<endl;
  else{
    char nom[52]="";
    int messCli = recv(descBrCv,nom,52,0);
    cout<<"Le nouveau client est:"<<nom<<endl;
    if(messCli!=0){
      //Partie constructeur du nouveau spectateur
      nom[messCli]='\0';
      string pseudo=string(nom);
      Spectateur *s=new Spectateur(descBrCv,pseudo);
      spectacteurs.push_back(s);
      //Partie reseau
      convertirTableEnChar(s);
      for(int i=0;i<nbJMax;i++){
	if(joueurs[i]!=NULL && actif[i]==false && joueurs[i]->getNom()==s->getNom()){
	  redonnerSonJeu(i,s);
	}
      }
      if(descBrCv>descMax)
	descMax=descBrCv;
    }
    else{
      cout<<"Probleme recv client"<<endl;
      close(descBrCv);
    }
  }
}
void Table::addJoueur(Spectateur* s, char* aConvertir){
  int place=-1;
  int jeton=-1;
  if(isServeur && convertirCharDeJoueur(aConvertir,s,place,jeton)){
    Joueur* j = new Joueur(*s, jeton);
    cout<<"Le nouveau joueur est: "<<j->getNom()<<endl;
    joueurs[place] = j;
    if(partieEnCours)
      actif[place]=true;
    else
      actif[place]=false;
    nbJoueur++;
    stringstream ss;
    ss<<place;
    string newJoueur = "NewJo";
    newJoueur+=separateur;
    newJoueur+=ss.str();
    newJoueur+=separateur;
    newJoueur+=j->getNom();
    newJoueur+=separateur;
    stringstream ssa;
    ssa<<j->getJeton();
    newJoueur+=ssa.str();
    newJoueur+=separateur;
    newJoueur+="0";
    newJoueur+=separateur;
    newJoueur+="t";
    newJoueur+=separateur;
    cout<<newJoueur<<endl;
    char nJ[100] = "";
    strcpy(nJ,newJoueur.c_str());
    for(int i=0; i<spectacteurs.size();i++){
      send(spectacteurs[i]->getDesc(),nJ, strlen(nJ),0);
    }
    char jEnPlus[]="NewJo&";
    write(descEnvoie,jEnPlus,10);
    if(!partieEnCours && nbJoueur>1){
      partieEnCours=true;
      lancerPartie();
    }
    else{
      if(!partieEnCours){
	//prochainAMiser=place;
	dealer=place;
      }
    }
  }
}
void Table::removeSpectateur(int placeSpec){
  int specDesc=spectacteurs[placeSpec]->getDesc();
  for(int i=0;i<nbJMax;i++){
    //Implémenter la méthode getDesc de la classe joueur!!!!!!!
    cout<<"C'est un joueur"<<endl;
    if(joueurs[i]!=NULL && specDesc==joueurs[i]->getDesc()){
      cout<<"Le joueur est parti"<<endl;
      removeJoueur(i);
    }
  }
  cout<<"Un spec est parti"<<endl;
  close(specDesc);
  delete spectacteurs[placeSpec];
  spectacteurs.erase(spectacteurs.begin()+placeSpec);
}
void Table::removeJoueur(int placeJ){
  if(actif[placeJ]){
    cout<<"la"<<endl;
    actif[placeJ]=false;
    close(joueurs[placeJ]->getDesc());
  }
  else{
    cout<<"OuLa"<<endl;
    modifBDD(placeJ);
    nbJoueur--;
    delete joueurs[placeJ];
    joueurs[placeJ]=NULL;
    envoieQuitteTable(placeJ);
    if(nbJoueur==0){
      dealer=-1;
    }
    else{
      if(placeJ==dealer){
	int place=placeJ+1;
	do{
	  if(joueurs[place]!=NULL){
	    cout<<"Affichage dealer après supression de l'ancien dealer: "<<place<<endl;
	    dealer=place;
	    place=placeJ;
	  }
	  else{
	    
	    place=(place+1)%nbJMax;
	  }
	}while(placeJ!=place);
      }
    }
    char jEnMoins[]="QuitJ&";
    write(descEnvoie,jEnMoins,10);
    cout<<"Joueur supprimé succés"<<endl;
  }
}
void Table::removeJNoMoney(){
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL && joueurs[i]->getJetonDebutDeManche()==0){
      removeJoueur(i);
    }
  }
}
 void Table::verifierPresenceJoueur(){
  
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL && (!actif[i] || !joueurs[i]->isActif())){
      removeJoueur(i);
    }
  }
}





/////////////////////////////////                            ////////////////////////////
////////////////////////////////Methode de lancement de  jeu/////////////////////////////
///////////////////////////////                            /////////////////////////
void Table::run(){
  cout<<"Table lancé"<<endl;
  while(isServeur){
    //Initialise à faux l'ensemble de lecture à surveiller et positionne à vrai les descripteurs à surveiller
    FD_ZERO(&desc_en_lect);
    FD_SET(descTable,&desc_en_lect);
    if(isServeur)
      FD_SET(descRecu,&desc_en_lect);
    for(int i=0;i<spectacteurs.size();i++){
      FD_SET(spectacteurs[i]->getDesc(),&desc_en_lect);
    }
    int sel=select(descMax+1,&desc_en_lect,NULL,NULL,NULL);
    cout<<"On a reçu un paquet "<<brPub.returnPort()<<endl;
    //Si erreur au select
    if(sel==-1){
      cout<<"Erreur au select"<<endl;
      erreur();
    }
    else{
      //si c'est un nouveau client se connectant sur la table
      if(FD_ISSET(descTable,&desc_en_lect)){
	cout<<"New spec"<<endl;
	addSpectateur();
      }
      else{
	if(isServeur && FD_ISSET(descRecu,&desc_en_lect)){
	    actionServeur();
	}
	else{
	  //Si c'est un client à l'origine de la sortie du select
	  int parcourSpec=0;
	  bool sortieParcours=true;
	  while(sortieParcours && parcourSpec!=nbSpec){
	    if(FD_ISSET(spectacteurs[parcourSpec]->getDesc(),&desc_en_lect)){
	      actionSpec(parcourSpec);
	      sortieParcours=false;
	    }
	    else{
	      parcourSpec++;
	    }
	  }
	}
      }
    }
  }
}
void Table::lancerPartie(){
  while(isServeur && nbJoueur>1){
    cout<<"let's go pour le poker"<<endl;
    preparerJoueur();
    petiteBlind=smallBlind();
    grosseBlind=bigBlind(petiteBlind);
    cout<<"La petiteBlind est: "<<petiteBlind<<endl;
    cout<<"La grosseBlind est: "<<grosseBlind<<endl;

    joueurEnListe=nbJoueur;
    cout<<"Parti lancé"<<endl;
    distribuer();
    joueurEnListe=nbJoueur;
    cout<<"Le nombre de joueur est de : " <<joueurEnListe<<endl;
    mise=miseMin;
    prochainAMiser=aMiser(grosseBlind);
    cout<<"Le premier à miser est: "<<prochainAMiser<<endl;
    lancerPartieReseau(petiteBlind,grosseBlind);
    if(joueurEnListe==1 || tourDeMise(2)){
      cout<<"On distribue le pot"<<endl;
      newManche();
      emporteLePot();
      pot=0;
    }
    else{
      sleep(1);
      newManche();
      poserFlop(prochainAMiser);
      if(joueurEnListe==1 || tourDeMise(1)){
	cout<<"On distribue le pot"<<endl;
	newManche();
        emporteLePot();
	pot = 0;
      }
      else{
	newManche();
	sleep(1);
	poserTurnRiver(prochainAMiser);
	if(joueurEnListe==1 || tourDeMise(1)){
	  newManche();
	  emporteLePot();
	  pot =0;
	}
	else{
	  sleep(1);
	  newManche();
	  poserTurnRiver(prochainAMiser);
	  if(joueurEnListe==1 || tourDeMise(1)){
	    newManche();
	    emporteLePot();
	    pot = 0;
	  }
	  else{
	    newManche();
	    cout<<"Avant SETCLASSEMENTJOUEUR"<<endl;
	    SetClassementJoueurs();
	    for(int i=0;i<nbJoueur;i++){
	      
	      cout<<"afficher gagnant:"<<classementJoueurs[i][0]<<" "<<classementJoueurs[i][1]<<endl;
	    }
	    cout<<"Affiche pot "<<pot<<endl;
	    pot = distribuerLePot();
	    for(int i=0;i<nbJMax;i++){
	      if(joueurs[i]!=NULL){
		cout<<"Le joueur en position "<<i<<" à " <<joueurs[i]->getJeton()<<" "<<joueurs[i]->getJetonDebutDeManche()<<endl;
	      }
	    }
	    cout<<"APRES POT"<<endl;
	  }
	}
      }
    }
    cout<<"Le poker est fini, c'était une très belle partie"<<endl;
    cout<<"On regarde si on peux relancer une partie"<<endl;
    verifierPresenceJoueur();
    nettoieTable();
    removeJNoMoney();
    newDealer();
    sleep(2);
  }
  if(nbJoueur==0)
    dealer=-1;
  partieEnCours=false;
}





/////////////                                                            ////////////////////////////////////////////////////
////////////Methode permettant de préparer une nouvelle partie de poker //////////////////////////////////////////////////////////////
///////////                                                            //////////////////////////////////////////////////////////////
//Prépare le joueur au démarache d'une partie
void Table::preparerJoueur(){
  cout<<"Debut preparerJoueur()"<<endl;
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL){
      joueurs[i]->resetJ();
      actif[i]=true;
      nbJAllIn=0;
    }
  }
}
//Choix du nouveau dealer
void Table::newDealer(){
  if(nbJoueur>0){
    dealer=(dealer+1)%nbJMax;
    while(joueurs[dealer]==NULL){
      dealer++;
      dealer=dealer%nbJMax;
    }
  }
}
//Détermine la small blind
int Table::smallBlind(){
  //////Partie Jeu
  int placeSmall=(dealer+1)%nbJMax;
  while(joueurs[placeSmall]==NULL){
    placeSmall++;
    placeSmall=placeSmall%nbJMax;
  }
  if(joueurs[placeSmall]->getJetonDebutDeManche()>miseMin/2){
    pot+=miseMin/2;
    //joueurs[placeSmall]->nouveauGain(-(miseMin/2));
    joueurs[placeSmall]->setJeton(joueurs[placeSmall]->getJeton()-(miseMin/2));
  }
  else{
    pot+=joueurs[placeSmall]->getJetonDebutDeManche();
    //joueurs[placeSmall]->setJetonDebutDeManche(0);
    joueurs[placeSmall]->setJeton(0);
    joueurs[placeSmall]->setAllIn(true);
    nbJAllIn++;
  }
  return placeSmall;
}
//Détermine la big blind
int Table::bigBlind(const int smallBlind){
  ///////Partie Jeu
  int placeBig=(smallBlind+1)%nbJMax;
  while(joueurs[placeBig]==NULL){
    placeBig++;
    placeBig=placeBig%nbJMax;
  }
  if(joueurs[placeBig]->getJetonDebutDeManche()>miseMin){
    pot+=miseMin;
    //joueurs[placeBig]->nouveauGain(-(miseMin));
    joueurs[placeBig]->setJeton(joueurs[placeBig]->getJeton()-miseMin);
  }
  else{
    pot+=joueurs[placeBig]->getJetonDebutDeManche();
    //joueurs[placeBig]->setJetonDebutDeManche(0);
    joueurs[placeBig]->setJeton(0);
    joueurs[placeBig]->setAllIn(true);
    nbJAllIn++;
  }
  return placeBig;
}
void Table::distribuer(){
  paquet.initialiserPaquet();
  donnerCartes();
}
void Table::donnerCartes(){
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL){
      char cartes[25]="Carte&";
      joueurs[i]->setCartes(paquet.sortirCarte(),0);
      joueurs[i]->setCartes(paquet.sortirCarte(),1);
      convertirJoueurEnCarte(i);
      cout<<"Les cartes d'un des joueurs: "<<cartes<<endl;
    }
  }
}
//Un joueur veut quitter la table pour la prochaine partie 
void Table::joueurQuitteTable(int place){
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL && spectacteurs[place]->getDesc()==joueurs[i]->getDesc()){
      joueurs[i]->setActif(false);
    }
  }
}





//////////////////////                         //////////////////////////////////
/////////////////////Methode de poker en cours/////////////////////////////////// 
////////////////////                      ///////////////////////////////////
//Lance la methode methodeMiser() pour chaque joueur en jeu
bool Table::tourDeMise(int verificateur){
  cout<<"tourDeMise(const int)"<<endl;
  if(nbJAllIn+1==joueurEnListe || nbJAllIn==joueurEnListe){
    return false;
  }
  if(verificateur==1){
    dernierAMiser=prochainAMiser;
  }
  if(verificateur==2){
    dernierAMiser=grosseBlind;  
  }
  aJouer=false;
  while(!aJouer){
    methodeMiser();
  }
  cout<<"Sortie premier boucle tour de mise"<<endl;
  while(prochainAMiser != -1 && prochainAMiser!=dernierAMiser && joueurEnListe!=1 && nbJAllIn!=nbJoueur){
    cout<<"Affiche prochain à Miser: "<<prochainAMiser<<endl;
    cout<<"Affiche dernier à Miser: "<<dernierAMiser<<endl;
    methodeMiser();
  }
  return joueurEnListe==1;
    
}
//Attends que le joueur correspondant mise
void Table::methodeMiser(){
  cout<<"methodeMiser()"<<endl;
  cout<<"Le prochaine à miser est: "<<prochainAMiser<<endl;
    if(!actif[prochainAMiser]){
      joueurMiseInactif(prochainAMiser);
      aJouer=true;
    }
    else{
      FD_ZERO(&desc_en_lect);
      FD_SET(descTable,&desc_en_lect);
      if(isServeur)
	FD_SET(descRecu,&desc_en_lect);
      cout<<"Avant le select de methodeMiser"<<endl;
      for(int i=0;i<spectacteurs.size();i++){
	FD_SET(spectacteurs[i]->getDesc(),&desc_en_lect);
      }
      cout<<"Après le select de methodeMiser"<<endl;
      int sel=select(descMax+1,&desc_en_lect,NULL,NULL,NULL);
      //Si erreur au select
      if(sel==-1){
	erreur();
      }
      else{
	if(FD_ISSET(descTable,&desc_en_lect)){
	    addSpectateur();
	}
	else{
 	  if(isServeur && FD_ISSET(descRecu,&desc_en_lect)){
	      actionServeur();
	  }
	  else{
	    cout<<"Action d'un spectateur"<<endl;
	    int parcourSpec=0;
	    bool sortieParcours=true;
	    while(sortieParcours && parcourSpec<spectacteurs.size()){
	      if(FD_ISSET(spectacteurs[parcourSpec]->getDesc(),&desc_en_lect)){
		actionSpecJoueur(parcourSpec);
		sortieParcours=false;
	      }
	      else{
		parcourSpec++;
	      }
	    }
	  }
	}
      }
    }
    cout<<"sortie methodeMiser()"<<endl;
}
//Cherche le prochain devant miser
int Table::aMiser(const int precMiseur){
  
  for(int i=(precMiseur+1)%nbJMax;i!=precMiseur;i=(i+1)%nbJMax){
    if(joueurs[i]!=NULL && !joueurs[i]->isFold() && !joueurs[i]->isAllIn())
      {
	return i;
      }
  }
  cout<<"aMiser retourne : -1"<<endl;
  return -1;
}
//Prépare une nouvelle manche
void Table::newManche(){
  cout<<"Une nouvelle manche se lance"<<endl;
  mise=0;
  while(!jAllIn.empty()){
    cout<<"On enleve un joueur ayant all IN"<<endl;
    int place=jAllIn.back();
    joueurs[place]->setPot(newPotSc());
    jAllIn.pop_back();
  }
  cout<<"On a fini de vider les joueur all In"<<endl;
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL)
      joueurs[i]->setJetonDebutDeManche(joueurs[i]->getJeton());
  }
  
  prochainAMiser=aMiser(dealer);
  cout<<"Fin de la prépartation de la nouvelle manche"<<endl;
}
//Pose le flop
void Table::poserFlop(int placeNext){
  milieu.push_back(paquet.sortirCarte());
  milieu.push_back(paquet.sortirCarte());
  milieu.push_back(paquet.sortirCarte());
  
  envoyerBoard(placeNext,0);
}
//Pose la turn ou la river
void Table::poserTurnRiver(int placeNext){
  milieu.push_back(paquet.sortirCarte());
  envoyerBoard(placeNext,milieu.size()-1);
}
//Determine le ou les gagnants. Mets les perdants à fold

//Distribue le pot et nettoie la table
int Table::distribuerLePot(){
  cout<<"teste 0 "<<pot<<endl;
  int restePot=0;
  int cpt=-1;
  while(pot>0){
    cout<<"On vide le pot"<<endl;
    cpt++;
    
    int pointMax=classementJoueurs[cpt][1];
    int deb=cpt;
    
    int cptNoAllIn=0;
    int sommeMax=0;
    while(cpt<nbJoueur && pointMax==classementJoueurs[cpt][1]){
      int place=classementJoueurs[cpt][0];
    
      if(joueurs[place]->isAllIn()){
	if(sommeMax!=-1)
	  sommeMax+=joueurs[place]->getPot();
      }
      else{
	sommeMax=-1;
      }
      cpt++;
    }      
    int potProvisoire;
    if(sommeMax>=pot || sommeMax==-1){
      potProvisoire=pot;
    }
    else{
      potProvisoire=sommeMax;
    }
    pot-=potProvisoire;
    int nb=0;
    while(potProvisoire-restePot>0){
      cout<<"On vide le pot provisoire"<<endl;
      potProvisoire+=restePot;
      restePot=0;
      int sommeGG=potProvisoire/(cpt-deb+nb);
      restePot+=potProvisoire%(cpt-deb+nb);
      for(int i=deb;i<cpt;i++){
	int place=classementJoueurs[i][0];
	cout<<"Afficher place du gagnant "<<place<<endl;
	if(joueurs[place]->isAllIn()){
	  cout<<"Teste1"<<endl;
	  if(joueurs[place]->getPot()>=sommeGG){
	    potProvisoire-=sommeGG;
	    joueurs[place]->nouveauGain(sommeGG);
	    
	  }
	  else{
	    potProvisoire-=joueurs[place]->getPot();
	    joueurs[place]->nouveauGain(joueurs[place]->getPot());
	    joueurs[place]->setPot(0);
	    nb++;
	  }
	}
	else{
	  cout<<"Teste 2"<<endl;
	  potProvisoire-=sommeGG;
	  joueurs[place]->nouveauGain(sommeGG);
	}
      }
    }
  }
  cout<<"Après distribution du pot"<<endl;
  //Envoyer les gagnants/Perdant ici.........
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL && joueurs[i]->getJetonDebutDeManche()!=joueurs[i]->getJeton()){
      gagnantPerdantSomme(i);
      joueurs[i]->setJeton(joueurs[i]->getJetonDebutDeManche());
      joueurs[i]->setJetonDebutDePartie(joueurs[i]->getJetonDebutDeManche());
    }
    else{
      if(joueurs[i]!=NULL){
	joueurs[i]->setJetonDebutDePartie(joueurs[i]->getJetonDebutDeManche());
      }
    }
  }
  cout<<"Avant le retourne de dustribution du pot"<<endl;
  return restePot;
}
void Table::emporteLePot(){
  for(int place=0;place<nbJMax;place++){
    if(joueurs[place]!=NULL && !joueurs[place]->isFold()){
      joueurs[place]->setJetonDebutDeManche(joueurs[place]->getJeton());
      joueurs[place]->nouveauGain(pot);
      joueurs[place]->setJeton(joueurs[place]->getJetonDebutDeManche());
      pot=0;
      char gagnantChar[25]="Gagna&";
      char placeChar[5]="";
      sprintf(placeChar,"%d",place);
      strcat(gagnantChar,placeChar);
      strcat(gagnantChar,separateur);
      char argentChar[15]="";
      sprintf(argentChar,"%d",joueurs[place]->getJetonDebutDeManche());
      strcat(gagnantChar,argentChar);
      strcat(gagnantChar,separateur);
      messageSpec(gagnantChar,25);
      return;
    }
  }
}

void Table::nettoieTable(){
  nbJAllIn=0;
  instanceAllIn=false;
  petiteBlind=-1;
  grosseBlind=-1;
  for(int i=0;i<nbJMax;i++){
    actif[i]=false;
  }
  mise=miseMin;
  while(!milieu.empty()){
    milieu.pop_back();
  }
  while(!jAllIn.empty())
    jAllIn.pop_back();
}
/************************************************************************************/








//////////////////////////                   //////////////////////////////////////
/////////////////////////Les méthodes de mise////////////////////////////////////////
////////////////////////                    ////////////////////////////////////////
void Table::joueurFold(int place){
  joueurEnListe--;
  joueurs[place]->setJetonDebutDeManche(joueurs[place]->getJeton());
  joueurs[place]->setFold(true);
  if(prochainAMiser==dernierAMiser){
    prochainAMiser=-1;
  }
  miserReseau(place,-100);
}
void Table::joueurAllIn(int place){
  joueurs[place]->setAllIn(true);
  nbJAllIn++;
  jAllIn.push_back(place);
  pot+=joueurs[place]->getJeton();
  miserReseau(place,joueurs[place]->getJeton()); //Faut envoyer mise d'all in avant de mettre les jeton à 0
  joueurs[place]->setJeton(0);
}
void Table::joueurMise(char *miseChar,int place){
  cout<<"Debut joueurMise(char*,int)"<<endl;
  char miseJChar[15]="";
  for(int i=0;miseChar[i]!='&';i++){
    miseJChar[i]=miseChar[i];
  }
  int miseJ=atoi(miseJChar);
  cout<<"La mise du joueur est de: "<<miseJ<<endl;
  cout<<"place: "<<place<<endl;
  cout<<"La joueur actuel "<<joueurs[place]->miseDeCetteManche()<<endl;
  int miseEffectue=joueurs[place]->miseDeCetteManche()+miseJ;
  cout<<"Mise effectue sans *100 "<<miseEffectue<<endl; 
  prochainAMiser=aMiser(place);
  if(joueurs[dernierAMiser]->isAllIn())
    dernierAMiser=place;
  if(miseEffectue==mise){
    cout<<"Joueur check"<<endl;
    pot+=miseJ;
    joueurs[place]->setJeton(joueurs[place]->getJeton()-miseJ);
    miserReseau(place,miseJ);
  } 
  else{
    if(miseEffectue>mise){
      if(miseEffectue>=joueurs[place]->getJetonDebutDeManche()){
	cout<<"Joueur All in"<<endl;
	joueurAllIn(place);
	dernierAMiser=place;
	
      }
      else{
	cout<<"Joueur mise"<<endl;
	pot+=miseJ;
	joueurs[place]->setJeton(joueurs[place]->getJeton()-miseJ);
	dernierAMiser=place;
	mise=miseEffectue;
	cout<<"Affiche mise après mise d'un joueur: "<<mise<<endl;
	miserReseau(place,miseJ);
      }
    }
    else{
      if(miseEffectue>=joueurs[place]->getJetonDebutDeManche()){
	cout<<"Joueur All in"<<endl;
	joueurAllIn(place);
	//dernierAMiser=place;
      }
      else{
	cout<<"Joueur fold"<<endl;
	joueurFold(place);
      }
    }
  }
  cout<<"Fin joueurMise(char*,int)"<<endl;
}
//Le joueur est inactif et on choisie la mise pour lui
void Table::joueurMiseInactif(int place){
  prochainAMiser=aMiser(place);
  if(mise==joueurs[place]->miseDeCetteManche()){
    miserReseau(place,0);
  }
  else{
    joueurFold(place);
  }
}
// Creer le pot parralèle d'un joueur all In
int Table::newPotSc(){
  int placeJAllIn=jAllIn.back();
  int potSc=pot;
  int sommeAllIn=joueurs[placeJAllIn]->miseDeCetteManche();
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL){
      int sommeI=joueurs[i]->miseDeCetteManche();
      if(i!=placeJAllIn && sommeI>sommeAllIn){
	potSc=potSc-(sommeI-sommeAllIn);
      }
    }
  }
  return potSc;
}

/***********************************************************************************/








////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////                         ///////////////////////////////////////////
/////////////////////////////                         ////////////////////////////////////
////////////////////////////  Methode  pour le RESEAU /////////////////////////////
///////////////////////////                         //////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////





/////Permet d'envoyer un message au spectateurs///////////////////////////////////////
void Table::messageSpec(char* aEnvoyer,int taille){
  cout<<"Debut messageSpec(char*,int)"<<endl;
  for(int i=0;i<spectacteurs.size();i++){
    send(spectacteurs[i]->getDesc(),aEnvoyer, taille, 0);
  }
  cout<<"Fin messageSpec(char*,int)"<<endl;
}
/*******************************************************************************/




/////////////////////Envoie les infos de la table                     /////////////
/////////////////////Protocole NewJo&placeDansTable&pseudo&jeton&mise&/////////////
void Table::convertirTableEnChar(Spectateur* s){
  char aEnvoyer[255]="";
  char type[7]="NewJo";
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL){
      strcat(aEnvoyer,type);
      strcat(aEnvoyer,separateur);
      char placeChar[3]="";
      sprintf(placeChar,"%d",i);
      strcat(aEnvoyer,placeChar);
      strcat(aEnvoyer,separateur);
      strcat(aEnvoyer,joueurs[i]->getNom().c_str());
      strcat(aEnvoyer,separateur);
      char jetonChar[20]="";
      sprintf(jetonChar,"%d",joueurs[i]->getJeton());
      strcat(aEnvoyer,jetonChar);
      strcat(aEnvoyer,separateur);
      char miseChar[20]="";
      sprintf(miseChar,"%d",joueurs[i]->miseDeCetteManche());
      strcat(aEnvoyer,miseChar);
      strcat(aEnvoyer,separateur);
      if(joueurs[i]->isFold()){
	char foldChar[]="t";
	strcat(aEnvoyer,foldChar);
	strcat(aEnvoyer,separateur);
      }
      else{
	char foldChar[]="f";
	strcat(aEnvoyer,foldChar);
	strcat(aEnvoyer,separateur);
      }
    }
  }
  char typeInfo[7]="InfoT";
  strcat(aEnvoyer,typeInfo);
  strcat(aEnvoyer,separateur);
  char potChar[20]="";
  sprintf(potChar,"%d",pot);
  strcat(aEnvoyer,potChar);
  strcat(aEnvoyer,separateur);
  char dealerChar[20]="";
  sprintf(dealerChar,"%d",dealer);
  strcat(aEnvoyer,dealerChar);
  strcat(aEnvoyer,separateur);
  char smallChar[20]="";
  sprintf(smallChar,"%d",petiteBlind);
  strcat(aEnvoyer,smallChar);
  strcat(aEnvoyer,separateur);
  char bigChar[20]="";
  sprintf(bigChar,"%d",grosseBlind);
  strcat(aEnvoyer,bigChar);
  strcat(aEnvoyer,separateur);
  char blindChar[20]="";
  cout<<"La mise min est de: "<<miseMin<<endl;
  sprintf(blindChar,"%d",miseMin);
  strcat(aEnvoyer,blindChar);
  strcat(aEnvoyer,separateur);
  char joueurJoueChar[20]="";
  sprintf(joueurJoueChar,"%d",prochainAMiser);
  strcat(aEnvoyer,joueurJoueChar);
  strcat(aEnvoyer,separateur);
  char typeMilieu[7]="Milie";
  for(int i=0;i<milieu.size();i++){
    strcat(aEnvoyer,typeMilieu);
    strcat(aEnvoyer,separateur);
    char couleurChar[4]="";
    sprintf(couleurChar,"%d",milieu[i].getCouleur()*100+milieu[i].getValeur());
    strcat(aEnvoyer,couleurChar);
    strcat(aEnvoyer,separateur);
    char suivantChar[4]="";
    sprintf(suivantChar,"%d",prochainAMiser);
    strcat(aEnvoyer,suivantChar);
    strcat(aEnvoyer,separateur);
  }
  cout<<aEnvoyer<<endl;
  send(s->getDesc(),aEnvoyer,255,0);
}
//////La table previent les specs que un joueur vient de quitter la table
void Table::envoieQuitteTable(int placeJ){
  cout<<"Debut envoieQuitteTable(int)"<<endl;
  char jQuitter[9]="JQuit&";
  char place[2]="";
  sprintf(place,"%d",placeJ);
  strcat(jQuitter,place);
  strcat(jQuitter,separateur);
  messageSpec(jQuitter,9);
  cout<<"Fin envoieQuitteTable(int)"<<endl;
}
//Pour les reconexion en cours de jeu 
void Table::redonnerSonJeu(int place,Spectateur* s){
  char cartes[25]="Carte&";
  actif[place]=true;
  joueurs[place]->setDesc(s->getDesc());
  convertirJoueurEnCarte(place);
  //send(joueurs[place]->getDesc(),cartes,25,0);
}



//Pour transmettre les informations du jeu en reseau
///////////////////Previens qu'une partie va être lancé////////////////////////
void Table::lancerPartieReseau(int petiteBlind,int grosseBlind){
  char convDealChar[30]="Deale&";
  char dealerChar[3]="";
  sprintf(dealerChar,"%d",dealer);
  strcat(convDealChar,dealerChar);
  strcat(convDealChar,separateur);
  char pBChar[3]="";
  sprintf(pBChar,"%d",petiteBlind);
  strcat(convDealChar,pBChar);
  strcat(convDealChar,separateur);
  if(joueurs[petiteBlind]->getJeton()==0){
    char misePBChar[10]="";
    sprintf(misePBChar,"%d",joueurs[petiteBlind]->getJetonDebutDeManche());
    strcat(convDealChar,misePBChar);
    strcat(convDealChar,separateur);
  }
  else{
    char misePBChar[10]="";
    sprintf(misePBChar,"%d",miseMin/2);
    strcat(convDealChar,misePBChar);
    strcat(convDealChar,separateur);
    }
  char gBChar[3]="";
  sprintf(gBChar,"%d",grosseBlind);
  strcat(convDealChar,gBChar);
  strcat(convDealChar,separateur);
  //prochainAMiser=aMiser(grosseBlind);
  //cout<<"Affiche prochain à jouer: "<<prochainAMiser<<endl;
  if(joueurs[grosseBlind]->getJeton()==0){
    char miseGBChar[10]="";
    sprintf(miseGBChar,"%d",joueurs[grosseBlind]->getJetonDebutDeManche());
    strcat(convDealChar,miseGBChar);
    strcat(convDealChar,separateur);
  }
  else{
    char miseGBChar[10]="";
    sprintf(miseGBChar,"%d",miseMin);
    strcat(convDealChar,miseGBChar);
    strcat(convDealChar,separateur);
  }
  char nextChar[3]="";
  sprintf(nextChar,"%d",prochainAMiser);
  strcat(convDealChar,nextChar);
  strcat(convDealChar,separateur);
  messageSpec(convDealChar,30);
}
/************************************************************************************/
////////////////////////Envoie les cartes au joueur concerné////////////////////////
////////////////////////Protocole Carte&place&valeur&valeur////////////////////////
void Table::convertirJoueurEnCarte(int place){
  char cartes[25]="Carte&";
  char entier[5]="";
  sprintf(entier,"%d",place);
  strcat(cartes,entier);
  strcat(cartes,separateur);
  sprintf(entier,"%d",joueurs[place]->getCartes(0).getCouleur()*100+joueurs[place]->getCartes(0).getValeur());
  strcat(cartes,entier);
  strcat(cartes,separateur);
  sprintf(entier,"%d",joueurs[place]->getCartes(1).getCouleur()*100+joueurs[place]->getCartes(1).getValeur());
  strcat(cartes,entier);
  strcat(cartes,separateur);
  send(joueurs[place]->getDesc(),cartes,25,0);
}
/////////////////////Previens qu'un joueur viens de miser///////////////////////////
/////////////////////Protocole Miser&place&mise&next&
void Table::miserReseau(int place,int miseJ){
  char joueurMise[30]="Miser&";
  char placeChar[3]="";
  sprintf(placeChar,"%d",place);
  strcat(joueurMise,placeChar);
  strcat(joueurMise,separateur);
  char miseChar[20]="";
  sprintf(miseChar,"%d",miseJ);
  strcat(joueurMise,miseChar);
  strcat(joueurMise,separateur);
  char nextChar[3]="";
  sprintf(nextChar,"%d",prochainAMiser);
  strcat(joueurMise,nextChar);
  strcat(joueurMise,separateur);
  messageSpec(joueurMise,30);
}
//////////////////Envoie les classementJoueurs et les sommes gagné///////////////////////////
/////////////////Protocole Gagna&place&somme&/////////////////////////////////////
void Table::gagnantPerdantSomme(int place){
  if(joueurs[place]->getJetonDebutDePartie()>=joueurs[place]->getJetonDebutDeManche()){
    char perdantChar[25]="Perdu&";
    char placeChar[3]="";
    sprintf(placeChar,"%d",place);
    strcat(perdantChar,placeChar);
    strcat(perdantChar,separateur);
    char sommePerdu[10]="";
    sprintf(sommePerdu,"%d",joueurs[place]->getJetonDebutDeManche());
    strcat(perdantChar,sommePerdu);
    strcat(perdantChar,separateur);
    messageSpec(perdantChar,25);
  }
  else{
    char gagnantChar[25]="Gagna&";
    char placeChar[3]="";
    sprintf(placeChar,"%d",place);
    strcat(gagnantChar,placeChar);
    strcat(gagnantChar,separateur);
    char sommeGagne[10]="";
    sprintf(sommeGagne,"%d",joueurs[place]->getJetonDebutDeManche());
    strcat(gagnantChar,sommeGagne);
    strcat(gagnantChar,separateur);
    messageSpec(gagnantChar,25);
  }
}
///////////////////////////Pour transmettre le board////////////////////////////////////////////
//////////////////////////Protocole Milie&valeur&next&//////////////////////////
void Table::envoyerBoard(int next,int debBoard){
  char board[50]="";
  char type[]="Milie";
  for(int i=debBoard;i<milieu.size();i++){
    strcat(board,type);
    strcat(board,separateur);
    char couleur[5]="";
    sprintf(couleur,"%d",milieu[i].getCouleur()*100+milieu[i].getValeur());
    strcat(board,couleur);
    strcat(board,separateur);
    char placeChar[3]="";
    sprintf(placeChar,"%d",next);
    strcat(board,placeChar);
    strcat(board,separateur);
  }
  messageSpec(board,50);
}
/**********************************************************************************/












///////////////////////                                        ////////////////////
//////////////////////Methode de convertion reçu par un client/////////////////////
/////////////////////                                         //////////////////////
//////////////////////////////Demande d'un spec//////////////////////////////////////
void Table::actionSpec(int placeSpec){
  cout<<"Je suis dans actionSpec"<<endl;
  char recuSpec[256]="";
  int taille=recv(spectacteurs[placeSpec]->getDesc(),recuSpec,256,0);
  if(taille==0){
    cout<<"Un client se deconnecte"<<endl;
    removeSpectateur(placeSpec);
  }
  else{
    //Protocole 'Jouer'
    if(recuSpec[0]=='J' && recuSpec[1]=='o' && recuSpec[2]=='u' && recuSpec[3]=='e' && recuSpec[4]=='r'){
      cout<<"Jouer"<<endl;
      addJoueur(spectacteurs[placeSpec],recuSpec+6);
    }
    else{
      if(recuSpec[0]=='Q' && recuSpec[1]=='u' && recuSpec[2]=='i' && recuSpec[3]=='t' && recuSpec[4]=='T'){
	joueurQuitteTable(placeSpec);
      }
    }
  }
}

//////////////////////////////Demande d'un joueur ou un spec///////////////////////
void Table::actionSpecJoueur(int placeSpec){
  cout<<"ActionSpecJoueur(int)"<<endl;
  if(spectacteurs[placeSpec]->getDesc()==joueurs[prochainAMiser]->getDesc()){
    cout<<"Action du joueur en cour de jeu"<<endl;
    char recuSpec[256]="";
    int taille=recv(spectacteurs[placeSpec]->getDesc(),recuSpec,256,0);
    if(taille==0){
      removeSpectateur(placeSpec);
    }
    else{
      //Protocole 'Fold'
      if(recuSpec[0]=='F' && recuSpec[1]=='o' && recuSpec[2]=='l' && recuSpec[3]=='d' && recuSpec[4]=='d'){
	joueurFold(prochainAMiser);
	aJouer=true;
      }
      else{
	//Protocole 'Miser'->Miser ou check ou fold
	if(recuSpec[0]=='M' && recuSpec[1]=='i' && recuSpec[2]=='s' && recuSpec[3]=='e' && recuSpec[4]=='r'){
	  joueurMise(recuSpec+6,prochainAMiser);
	  aJouer=true;
	}
	else{
	  if(recuSpec[0]=='A' && recuSpec[1]=='b' && recuSpec[2]=='s' && recuSpec[3]=='e' && recuSpec[4]=='n'){
	    
	  }
	}
      }
    }
  }
  else{
    actionSpec(placeSpec);
  }
  cout<<"Fin actionSpecJoueur(int)"<<endl;
}


bool Table::convertirCharDeJoueur(char* aConvertir,Spectateur* s,int& place,int& jeton){
  char placeCar[2]="";
  int cpt=0;
  while(aConvertir[cpt]!='&'){
    placeCar[cpt]=aConvertir[cpt];
    cpt++;
  }
  cout<<placeCar<<endl;
  placeCar[cpt]='\0';
  cout<<"place: "<<placeCar<<endl;
  place=atoi(placeCar);
  if(place>nbJMax || joueurs[place]!=NULL){
    return false;
  }
  cpt++;
  jeton=atoi(aConvertir+cpt);
  cout<<"jeton: "<<jeton<<endl;
  if(jeton<miseMin)
    return false;
  return ajouterJetonTable(s,jeton);
  
}














//////////////////////////////           //////////////////////////////////////////
//////////////////////////////PARTIE BDD///////////////////////////////////////////
//////////////////////////////         ////////////////////////////////////////////
void Table::modifBDD(int placeJ){
  MYSQL utilisateur;
  mysql_init(&utilisateur);
  bool joueurExiste=false;
  const char* option="option";
  mysql_options(&utilisateur,MYSQL_READ_DEFAULT_GROUP,option);
    //Chez la fac
  if(mysql_real_connect(&utilisateur,"venus","flucia","flucia","flucia",0,NULL,0)){
  //Chez jm
  //if(mysql_real_connect(&utilisateur,"localhost","root","azertySQL","pokerL3",0,NULL,0)){   
    cout<<"bouh1"<<endl;
    mysql_query(&utilisateur,"SELECT * FROM infoclients");
    MYSQL_RES * result =NULL;
    MYSQL_ROW row;
    
    unsigned int i=0;
    unsigned int nb_champs=0;

    result = mysql_store_result(&utilisateur);
    
    nb_champs=mysql_num_fields(result);
    cout<<"bouh3"<<endl;
    while(!joueurExiste && (row = mysql_fetch_row(result))){
      
      if(row[PSEUDO] == joueurs[placeJ]->getNom()){
	joueurExiste=true;
      }
    }
    cout<<"bouh4"<<endl;
    int argent=0;
    stringstream ss(row[MONEY]);
    ss >> argent;
    ostringstream os;
    os<<joueurs[placeJ]->getJetonDebutDeManche()+argent;
    string requete="update infoclients set money='"+os.str()+"'where Username='"+joueurs[placeJ]->getNom()+"'";
    mysql_query(&utilisateur,requete.c_str());
    mysql_close(&utilisateur);
    cout<<"bouh5"<<endl;
  }
  else{
    cout<<"Probleme bdd"<<endl;
  }
}
bool Table::ajouterJetonTable(Spectateur* s,int& jeton){
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL){
      if(s->getNom()==joueurs[i]->getNom()){
	return false;
      }
    }
  }
  MYSQL utilisateur;
  mysql_init(&utilisateur);
  bool joueurExiste=false;
  const char* option="option";
  mysql_options(&utilisateur,MYSQL_READ_DEFAULT_GROUP,option);
    //Chez la fac
  if(mysql_real_connect(&utilisateur,"venus","flucia","flucia","flucia",0,NULL,0)){
  //Chez jm
  //if(mysql_real_connect(&utilisateur,"localhost","root","azertySQL","pokerL3",0,NULL,0)){   
    mysql_query(&utilisateur,"SELECT * FROM infoclients");

        
    MYSQL_RES * result =NULL;
    MYSQL_ROW row;

    unsigned int i=0;
    unsigned int nb_champs=0;

    result = mysql_store_result(&utilisateur);
    
    nb_champs=mysql_num_fields(result);
    while(!joueurExiste && (row = mysql_fetch_row(result))){
      
      if(row[PSEUDO] == s->getNom()){
	joueurExiste=true;
      }
    }
    cout<<"colonne 2"<<row[MONEY]<<endl;
    stringstream ss(row[MONEY]);
    int argent=0;
    if(joueurExiste){
      ss >> argent;
      cout<<ss<<endl;
    }
    else{
      return false;
    }
    cout<<"Argent "<<argent<<endl;
    if(argent<jeton){
      mysql_close(&utilisateur);
      return false;
    }
    else{
      ostringstream os;
      os<<argent-jeton;
      cout<<os<<endl;
      string requete="update infoclients set money='"+os.str()+"' where Username='"+s->getNom()+"'";
      mysql_query(&utilisateur,requete.c_str());
      mysql_close(&utilisateur);
      return true;
    }
  
  }
  else{
    cout<<"Erreur de la connexion à la base de donnée"<<endl;
    return false;
  }
}
/***********************************************************************************/







 ///////////////////////                                      ////////////////////////
  //////////////////////PARTIE SERVEUR COMUNIQUANT PAR UN TUBE////////////////////////
  /////////////////////                                      /////////////////////////

void Table::actionServeur(){
  char buf[100]="";
  if(read(descRecu,buf,100) == 0){
    cout<<"Le serveur est partie. Finisez votre dernière partie et supprimer les tables"<<endl;
    close(descRecu);
    close(descEnvoie);
    isServeur=false;
  }
}










///////////////////////////////      //////////////////////////////////////////////
//////////////////////////////ERREUR////////////////////////////////////////////////
/////////////////////////////      ////////////////////////////////////////////////
void Table::erreur(){
  while(!spectacteurs.empty()){
    close(spectacteurs.back()->getDesc());
    delete spectacteurs.back();
    spectacteurs.pop_back();
  }
  for(int i=0;i<nbJMax;i++){
    if(joueurs[i]!=NULL)
      delete joueurs[i];
    
  }
  delete joueurs;
  close(descTable);
  exit(1);
}
/************************************************************************************/




int Table::EssayerMain3CartesMilieu(int array[],int player)
{
  cout<<"Debut EssayerMain3CartesMilieu"<<endl;
	Carte mains[5];

         for (int i=1;i<4;i++)
                mains[i-1] = milieu[array[i]];

         for (int i=0;i<2;i++)
	  {
             //   mains[i+3] = joueurs[player].cartes[i];
		mains[i+3] = joueurs[player]->getCartes(i);
	  }
	 cout<<"fin EssayerMain3CartesMilieu"<<endl;
         return ObtenirPointMain(mains);
}

int Table::EssayerMain4CartesMilieu(int array[],int player)
{
  cout<<"Debut EssayeMain4CartesMilieu"<<endl;

	Carte mains[5];
	int ptC1=0;
	int ptC2=0;

         for (int i=1;i<5;i++)
	{
                mains[i-1] = milieu[array[i]];
	}

	// mains[4] = joueurs[player].cartes[0];
	 mains[4] = joueurs[player]->getCartes(0);
	 ptC1 = ObtenirPointMain(mains);
	 mains[4] = joueurs[player]->getCartes(1);
     //    mains[4] = joueurs[player]->cartes[1];
	 ptC2 = ObtenirPointMain(mains);

	 if (ptC1 > ptC2)
	{	
		//joueurs[player].carteMax = joueurs[player].cartes[0];
	  joueurs[player]->setCarteMax(joueurs[player]->getCartes(0));
	  return ptC1;
	}
	 else
	   {
	     joueurs[player]->setCarteMax(joueurs[player]->getCartes(1));
	     //joueurs[player].carteMax = joueurs[player].cartes[1];
	     return ptC2;
	   }
	 cout<<"Fin EssayerMain4CarteMilieu"<<endl;
}



void Table::RemplissageJeuxMax()
{
  cout<<"Debut RemplissagejeuxMax"<<endl;
  for (int q=0;q<nbJMax;q++)
    {
      if(joueurs[q]!=NULL){
	if (joueurs[q]->getValeurMain3() > joueurs[q]->getValeurMain4())
	  {
	    for (int i=0;i<3;i++)
	      {
		joueurs[q]->setCartesMax(i,joueurs[q]->getMainMax3(i));
	      }
	    int k=0;
	    for (int i=3;i<5;i++)
	      {
		joueurs[q]->setCartesMax(i,joueurs[q]->getCartes(k));
		k++;
	      }
	  }
	else
	  {
	    for (int i=0;i<4;i++)
	      {
		joueurs[q]->setCartesMax(i,joueurs[q]->getMainMax4(i));
	      }
	    joueurs[q]->setCartesMax(4,joueurs[q]->getCarteMax());
	    
	    
	  }
      }
    }
  cout<<"Fin RemplissagejeuxMax"<<endl;
}



//Cette fonction permet d'evaluer les mains maximun des joueurs en prenant en compte 4 cartes du board et les deux cartes des joueurs
// Elle remplis l'attribut valeurMain4 qui contiendra le nombre de point maximun que le joueur peut avoir ainsi que mainMax4 qui contiendra les 4 meilleurs cartes pour ce nombre de points
void Table::EvaluerMain2()
{
  cout<<"Debut Main2"<<endl;
  int stack[10],k;
  int PointsCourant;
  
  for (int q=0;q<nbJMax;q++){
    //joueurs[q].valeurMain4 = 0;
    if(joueurs[q]!=NULL){
      joueurs[q]->setValeurMain4(0);
      if (!joueurs[q]->isFold()){
	stack[0]=-1; /* -1 is not considered as part of the set */
	k = 0;
	while(1){
	  if (stack[k]<4){
	    stack[k+1] = stack[k] + 1;
	    k++;
	  }
	  
	  else{
	    stack[k-1]++;
	    k--;
	  }
	  
	  if (k==0)
	    break;

	  if (k==4){
	    PointsCourant = EssayerMain4CartesMilieu(stack,q);
	    if (PointsCourant>joueurs[q]->getValeurMain4()){
	      joueurs[q]->setValeurMain4(PointsCourant);
	      for (int x=0;x<4;x++){
		//   joueurs[q]->mainMax4[x] = milieu[stack[x+1]];
		joueurs[q]->setMainMax4(x,milieu[stack[x+1]]);
	      }
	      
	      
	    }
	  }
	  
	}
      }
    }
  }
  cout<<"Fin EvaluerMain2"<<endl;
}



//Cette fonction permet d'evaluer les mains maximun des joueurs en prenant en compte 3 cartes du board et les deux cartes des joueurs
// Elle remplis l'attribut valeurMain3 qui contiendra le nombre de point maximun que le joueur peut avoir ainsi que mainMax3 qui contiendra les 3 meilleurs cartes pour ce nombre dvector<int>e points
void Table::EvaluerMain()
{
  cout<<"Debut EvaluerMain()"<<endl;
  int stack[10],k;
  int PointsCourant;
  
  cout<<"TESTE 0"<<endl;
  for (int q=0;q<nbJMax;q++){
    if(joueurs[q]!=NULL){
      joueurs[q]->setValeurMain3(0);
      cout<<"TTTTTTEEEESSSTEE"<<endl;
      if (!joueurs[q]->isFold()){
	cout<<"TESTE 00"<<endl;
	stack[0]=-1; /* -1 is not considered as part of the set */
	k = 0;
	cout<<"TESTE 1"<<endl;
	while(1){
	  if (stack[k]<4){
	    stack[k+1] = stack[k] + 1;
	    k++;
	  }
	  
	  else{
	    stack[k-1]++;
	    k--;
	  }
	  
	  if (k==0)
	    break;
	  
	  if (k==3){
	    cout<<"TESTE 2"<<endl;
	    PointsCourant = EssayerMain3CartesMilieu(stack,q);
	    if (PointsCourant>joueurs[q]->getValeurMain3()){
	      joueurs[q]->setValeurMain3(PointsCourant);
	      for (int x=0;x<3;x++){
		//  joueurs[q].mainMax3[x] = milieu[stack[x+1]];
		joueurs[q]->setMainMax3(x,milieu[stack[x+1]]);
	      }
			    
	    }
	  }
	  
	}
      }
      
    }
  }
  cout<<"Fin EvaluterMain()"<<endl;
}



void Table::SetClassementJoueurs()
{
  cout<<"Debut SetClassementJoueurs()"<<endl;
  this->EvaluerMain();
  this->EvaluerMain2();
  this->RemplissageJeuxMax();
  //cout<<"HELLO world: "<<endl;
  int tab_joueurs[nbJMax][2];
  
  for (int i=0;i<nbJMax;i++)
    {
	  if(joueurs[i]!=NULL){
	    tab_joueurs[i][0] = i;
	    tab_joueurs[i][1] = -1;
	  }
	  else{
	    tab_joueurs[i][0]=-1;
	    tab_joueurs[i][1]=-1;
	  }
    }
  
  cout<<"Detection de la main max"<<endl;
  for (int i=0;i<nbJMax;i++)
	{
	  if(joueurs[i]!=NULL && !joueurs[i]->isFold() ){
	    if (joueurs[i]->getValeurMain3() > joueurs[i]->getValeurMain4() )
	    joueurs[i]->setValeurMainMax(joueurs[i]->getValeurMain3());
	    else
	      joueurs[i]->setValeurMainMax(joueurs[i]->getValeurMain4());
	  }
	}
  
  cout<<"remplissage tableau num_joueur_point"<<endl;
  for (int i=0;i<nbJMax;i++)
    {
      if(joueurs[i]!=NULL && !joueurs[i]->isFold()){
	  tab_joueurs[i][0] = i;
	  tab_joueurs[i][1] = joueurs[i]->getValeurMainMax();
	}
    }
  cout<<"fin remplissage tableau num_joueur_point"<<endl;
  int tmp[2];
  for (int i=0;i<nbJMax;i++)
    {
      for(int j=i+1;j<nbJMax;j++){
	if(tab_joueurs[j][1] > tab_joueurs[i][1])
	  {
	    tmp[0] = tab_joueurs[i][0];
	    tmp[1] = tab_joueurs[i][1];
	    tab_joueurs[i][0] = tab_joueurs[j][0];
	    tab_joueurs[i][1] = tab_joueurs[j][1];
	    tab_joueurs[j][0] = tmp[0];
	    tab_joueurs[j][1] = tmp[1];
	  }
      }
    }
  for(int i=0;i<nbJMax;i++){
    cout<<"place: "<<tab_joueurs[i][0]<<" "<<tab_joueurs[i][1]<<endl;
  }
  cout<<"copie du tableau dans l'attribut classement"<<endl;
  int j=0;
  for (int i=0;i<nbJoueur;i++)
    {
      cout<<"Classement joueurs "<<tab_joueurs[i][0]<<tab_joueurs[i][1]<<endl;
      classementJoueurs[j][0] = tab_joueurs[i][0];
      classementJoueurs[j][1] = tab_joueurs[i][1];
      j++;
     
    }
  
  cout<<"Fin setClassementJoueur()"<<endl;
}








int Table::ObtenirPointMain(Carte main[])
{
  cout<<"Debut ObtenirPointMain"<<endl;
	Carte main_copie[5];
	main_copie[0] = main[0];
	main_copie[1] = main[1];
	main_copie[2] = main[2];
	main_copie[3] = main[3];
	main_copie[4] = main[4];
    	int points;
    	int valMax = 0;
    //Classement des cartes de la plus petite � la plus grande
    qsort(main_copie,5,sizeof(Carte),comparerCartes);
     for (int i=0;i<5;i++)
     {
	valMax += main_copie[i].getValeur();

     }



     //Test du trie
        for (int i=0;i<5;i++)
             {
                  cout<<"getValeur() carte : "<<main[i].getValeur()<<" ";
             }
    int suite,couleur,brelan,carre,full,paires,CarteHaute;
    suite = couleur = brelan = carre = full = paires = CarteHaute = 0;
    int cpt = 0 ;

    //Verification pour voir si il y a un carr�
    for (int i=0;i<2;i++){
        cpt = i;
        while (  cpt<i+3 && (main_copie[cpt].getValeur() == main_copie[cpt+1].getValeur()))
              cpt++;
        if (cpt==i+3){
           carre = 1;
           CarteHaute = main_copie[i].getValeur();
        }
    }

    //Verification d'une getCouleur()
    cpt = 0;
    while (cpt<4 && main_copie[cpt].getCouleur()==main_copie[cpt+1].getCouleur())
          cpt++;
    if (cpt==4){
       couleur = 1;
    }

    //Verification d'une suite
    cpt=0;
    while (cpt<4 && main_copie[cpt].getValeur() == main_copie[cpt+1].getValeur()-1)
    {
          cpt++;
    }
    if (cpt==4)
       suite = 1;



    //Verification d'un brelan ou d'un full
    if (!carre){
       for (int i=0;i<3;i++){
           cpt = i;
           while (cpt<i+2 && (main_copie[cpt].getValeur()==main_copie[cpt+1].getValeur()))
           {
                 cpt++;
           }
           if (cpt==i+2){
              //Brelan a �t� detecter
              brelan = 1;
              CarteHaute=main_copie[i].getValeur();
              //Verification d'un full

              //Brelan sur  les 3 premieres cartes donc on verifie les deux dernieres
              if (i==0){
                 if (main_copie[3].getValeur()==main_copie[4].getValeur())
                    full=1;
              }
              //Brelan sur les cartes du milieu
              else if(i==1){
                   if (main_copie[0].getValeur()==main_copie[4].getValeur())
                      full=1;
              }
              else{
              //Brelan sur les 3 dernieres cartes
                   if (main_copie[0].getValeur()==main_copie[1].getValeur())
                      full=1;
              }
           }
       }
    }
    //Quinte Flush
    if (suite&&couleur)
         return 300 + main_copie[4].getValeur() + valMax;
    else if(carre)
         return 270 + CarteHaute + valMax;
    else if(full)
         return 250 + CarteHaute + valMax;
    else if(couleur)
         return 220 + valMax;
    else if(suite)
    {
         return 200 + main_copie[4].getValeur() + valMax;
    }
    else if(brelan)
         return 150 + CarteHaute + valMax;

    int valPaire =0 ;
    //Verification d'une ou des paires
    for (cpt=0;cpt<4;cpt++){
        if (main_copie[cpt].getValeur()==main_copie[cpt+1].getValeur()){
	   valPaire += main_copie[cpt].getValeur();
           paires++;
	   
           if (main_copie[cpt].getValeur()>CarteHaute)
              CarteHaute = main_copie[cpt].getValeur();
        }
    }
    if (paires==2)
    {
        return  100 + valPaire + valMax;
    }
    else if(paires)
         return 30 + valMax;
    else
         return main_copie[4].getValeur()+valMax;

}

int comparerCartes(const void *cartes1, const void *cartes2){
  cout<<"Debut comparerCartes"<<endl;
  return (*(Carte *)cartes1).getValeur() - (*(Carte *)cartes2).getValeur();
}
