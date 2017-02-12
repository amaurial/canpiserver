#include <string>
#include <regex>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
//#include "libconfig.h++"

//#define S "M[TA]+\\+[SL][0-9]+<;>.*"
using namespace std;
//using namespace libconfig;

string cleanchar(string val,char c){
   int i = val.find(c);
   string s = val;
   while (i > 0){
      s.erase(i,1);
      cout << "cleaning:" << s << endl;
      i = s.find(c);
   }
   return s;
}

string cleanString(string val){
   int i;
   string s;
   s = cleanchar(val, '"');
   s = cleanchar(s,';');
   s = cleanchar(s,' ');
   return s;
}

pair <string,string> getpair(string val){
   pair <string,string> ret;
   string key;
   string value;
   string s;

   s = cleanString(val);
   cout << "cleaned:" << s << endl;
   int i = val.find("=");
   if (i > 0){
      key = s.substr(0,i);
      value = s.substr(i + 1, s.length());
      cout << "key: " << key << endl;    
      cout << "value: " << value << endl;    
      ret = std::make_pair(key, value);
      return ret;
   }
   return std::make_pair("0","0");
}


int main (){

/*
   string s = "MT+L300<;>L300";
   string se = S;
   regex e = regex (se);
   cout << "start" << endl;
   if (regex_match(s,e)){
      cout << "got a match" << endl;
   }


   Config cfg;
   string key,value;
    try
    {
       cfg.readFile("teste.cfg");

       cout << "key: ";
       cin >> key;
       cout <<"value: ";
       cin >> value;
       cout << "key :" << key << " value: " << value << endl;

       if (cfg.exists(key)){
            libconfig::Setting &varkey = cfg.lookup(key);
            varkey = atoi(value.c_str());
            cfg.writeFile("teste.cfg");
       }
    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "File I/O error" << std::endl;
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
	          << " - " << pex.getError() << std::endl;
    }
*/

    ifstream myfile;
    string line;
    map <string,string> m;
    pair <string,string> p;
    myfile.open ("canpi.cfg",ios::in);
    if (myfile.is_open ()){
        while ( getline (myfile, line) ){
            cout << line << endl;
            p = getpair (line);
            if ( (get<0>(p)) != "0") m.insert(p);
        }
        myfile.close();
    }
    else{
       cout << "Failed to open the file" << endl;
    }
    
    if (m.size() > 0){
       map<string,string>::iterator it = m.begin();
       for (it = m.begin(); it != m.end(); it++)
           cout << "key:" << it->first << " value:" << it->second << endl;
       
    }

}
