#include <string>
#include <regex>
#include <iostream>
#define S "M[STA]+[SL\\*]([0-9]+)?<;>I"
using namespace std;


int main (){

   string s = "MTAL300<;>I";
   string se = S;
   regex e = regex (se);
   cout << "start" << endl;
   if (regex_match(s,e)){
      cout << "got a match" << endl;
   }
}
