#ifndef MSGDATA_H
#define MSGDATA_H

//software vesion
#define SOFT_VERSION "VN2.0"
// no locos
#define ROASTER_INFO  "RL0]\\["
//first info to send to ED
//#define START_INFO  "VN2.0\n\rRL0]|[\n\rPPA1\n\rPTT]\[Turnouts}|{Turnout]\[Closed}|{2]\[Thrown}|{4\n\rPRT]\[Routes}|{Route]\[Active}|{2]\[Inactive}|{4\n\rRCC0\n\rPW12080\n\r"
#define START_INFO_RL "RL"//"0]\\["
#define START_INFO_PPA "PPA1"
#define START_INFO_PRT "PRT"//"]\\[Routes}|{Route]\\[Active}|{2]\\[Inactive}|{4"
#define START_INFO_PRL "PRL"//"]\\["
#define START_INFO_RCC "RCC0"
#define START_INFO_PW "PW12080"
//
#define DELIM_BRACET  "]\\["
//
#define DELIM_BTLT  "<;>"
//
#define DELIM_KEY  "}|{"
//
#define EMPTY_LABELS  "<;>]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[]\\[\n"
//MTS3<......MTLS1<;>]\[]\[]

//regex to identify a set speed message
#define RE_SPEED "M[STA]+[SL\\*]([0-9]+)?<;>[VX]([0-9]+)?"
//regex to identify a create session message
#define RE_SESSION  "M[STA]+\\+[SL][0-9]+<;>.*"
//regex to identify a release session message
#define RE_REL_SESSION "M[STGA]+\\-[SL\\*]([0-9]+)?<;>.*"
//regex to identify a create session message
#define RE_DIR "M[STA]+[SL\\*]([0-9]+)?<;>R[0-1]"
//regex to identify a query speed
#define RE_QRY_SPEED "M[STA]+[SL\\*]([0-9]+)?<;>qV"
//regex to identify a query direction
#define RE_QRY_DIRECTION "M[STA]+[SL\\*]([0-9]+)?<;>qR"
//regex to identify a query direction
#define RE_FUNC "M[STA]+[SL\\*]([0-9]+)?<;>F[0-9]+"
//regex to identify turnout messages
#define RE_TURNOUT "PTA[0-9]MT\\+[0-9]+;\\-[0-9]+"
//PTATMT8 PTACMT8
//regex to identify turnout messages
#define RE_TURNOUT_GENERIC "PTA[TC24]MT[0-9]+"
//regex to identify idle
#define RE_IDLE "M[STA]+[SL\\*]([0-9]+)?<;>I"

#endif //MSGDATA_H