#ifndef INFO_H
#define INFO_H
#include <iostream>
using namespace std;

class Info{
 private:
  string nom;
  float argent;
  string email;
  //4
  string dOB;
  //5
  string gender;
  //6
  string lName;
  string fName;
  string adress;
  string city;
  string zipCode;
  string country;
 public:
  Info();
  Info(const string);
  Info(const char*);
  
  void setArgent(const float);
  void setEmail(const string);
  void setDOB(const string);
  void setGender(const string);
  void setLName(const string);
  void setFName(const string);
  void setAdress(const string);
  void setCity(const string);
  void setZipCode(const string);
  void setCountry(const string);

  string getNom()const;
  float getArgent()const;
  string getEmail()const;
  string getDOB()const;
  string getGender()const;
  string getLName()const;
  string getFName()const;
  string getAdress()const;
  string getCity()const;
  string getZipCode()const;
  string getCountry()const;

  string toString()const;
  
};
#endif
