#include "gpio.h"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

gpio::gpio()
{
    this->gpionum = "4"; //GPIO4 is default
}

gpio::gpio(string gnum)
{
    this->gpionum = gnum;  //Instatiate GPIOClass object for GPIO pin number "gnum"
}

gpio::~gpio(){
    if (exported){
        //unexport_gpio();
    }
}

int gpio::export_gpio()
{
    string export_str = "/sys/class/gpio/export";
    ofstream exportgpio(export_str.c_str()); // Open "export" file. Convert C++ string to C string. Required for all Linux pathnames
    //if (exportgpio < 0){
    if (exportgpio.fail() || exportgpio.bad()){
        cout << " OPERATION FAILED: Unable to export GPIO"<< this->gpionum <<" ."<< endl;
        return -1;
    }
    exportgpio << this->gpionum ; //write GPIO number to export
    exportgpio.close(); //close export file
    exported = true;
    return 0;
}

int gpio::unexport_gpio()
{
    string unexport_str = "/sys/class/gpio/unexport";
    ofstream unexportgpio(unexport_str.c_str()); //Open unexport file
    //if (unexportgpio < 0){
    if (unexportgpio.fail() || unexportgpio.bad()){
        cout << " OPERATION FAILED: Unable to unexport GPIO"<< this->gpionum <<" ."<< endl;
        return -1;
    }

    unexportgpio << this->gpionum ; //write GPIO number to unexport
    unexportgpio.close(); //close unexport file
    return 0;
}

int gpio::setdir_gpio(string dir)
{

    string setdir_str ="/sys/class/gpio/gpio" + this->gpionum + "/direction";
    ofstream setdirgpio(setdir_str.c_str()); // open direction file for gpio
        //if (setdirgpio < 0){
        if (setdirgpio.fail() || setdirgpio.bad()){
            cout << " OPERATION FAILED: Unable to set direction of GPIO"<< this->gpionum <<" ."<< endl;
            return -1;
        }

        setdirgpio << dir; //write direction to direction file
        setdirgpio.close(); // close direction file
        return 0;
}

int gpio::setval_gpio(string val)
{

    string setval_str = "/sys/class/gpio/gpio" + this->gpionum + "/value";
    ofstream setvalgpio(setval_str.c_str()); // open value file for gpio
        //if (setvalgpio < 0){
        if (setvalgpio.fail() || setvalgpio.bad()){
            cout << " OPERATION FAILED: Unable to set the value of GPIO"<< this->gpionum <<" ."<< endl;
            return -1;
        }

        setvalgpio << val ;//write value to value file
        setvalgpio.close();// close value file
        return 0;
}

int gpio::getval_gpio(string& val){

    string getval_str = "/sys/class/gpio/gpio" + this->gpionum + "/value";
    //cout << "Reading the value of " << getval_str << endl;
    ifstream getvalgpio(getval_str.c_str());// open value file for gpio
    //if (getvalgpio < 0){
    if (getvalgpio.fail() || getvalgpio.bad()){
        cout << " OPERATION FAILED: Unable to get value of GPIO"<< this->gpionum <<" ."<< endl;
        return -1;
    }

    getvalgpio >> val ;  //read gpio value
   // cout << "GPIO value is " << val << endl;
    if(val != "0")
        val = "1";
    else
        val = "0";

    getvalgpio.close(); //close the value file
    return 0;
}

string gpio::get_gpionum(){

return this->gpionum;

}
