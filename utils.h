#ifndef __UTILS_H
#define __UTILS_H

#include <stdexcept>
#include <sstream>
#include <string>
#include <time.h>

//################# Config files tag definitions
#define TAG_CANID        "canid"
#define TAG_GRID_PORT    "cangrid_port"
#define TAG_TCP_PORT     "tcpport"
#define TAG_NN           "node_number"
#define TAG_AP_MODE      "ap_mode"
#define TAG_CAN_GRID     "can_grid"
#define TAG_SSID         "ap_ssid"
#define TAG_PASSWD       "ap_password"
#define TAG_ROUTER_SSID  "router_ssid"
#define TAG_ROUTER_PASSWD  "router_password"
#define TAG_LOGLEVEL     "loglevel"
#define TAG_LOGFILE      "logfile"
#define TAG_SERV_NAME    "service_name"
#define TAG_LOGAPPEND    "logappend"
#define TAG_LOGCONSOLE   "log_console"
#define TAG_TURNOUT      "turnout_file"
#define TAG_CANDEVICE    "candevice"
#define TAG_APCHANNEL    "ap_channel"
#define TAG_BP           "button_pin"
#define TAG_GL           "green_led_pin"
#define TAG_YL           "yellow_led_pin"
#define TAG_RL           "red_led_pin"
#define TAG_FNMOM        "fn_momentary"
#define TAG_WARN         "WARN"
#define TAG_INFO         "INFO"
#define TAG_DEBUG        "DEBUG"
#define TAG_NO_PASSWD    "ap_no_password"
#define TAG_CREATE_LOGFILE "create_log_file"
#define TAG_START_EVENT  "start_event_id"
#define TAG_NODE_MODE    "node_mode"
#define TAG_ORPHAN_TIMEOUT "orphan_timeout"
#define TAG_SHUTDOWN_CODE "shutdown_code"

using namespace std;

//byte definition
typedef unsigned char byte;

enum CLIENT_TYPE {ED,GRID,CBUS};
enum TURNOUT_STATE {CLOSED,THROWN,UNKNOWN};
enum SESSION_TYPE {MULTI_SESSION, MULTI_THROTTLE};
enum FN_STATE {OFF=0,ON};
enum FN_TYPE {MOMENTARY=0,SWITCH};
enum SCRIPT_ACTIONS {NONE=0, CONFIGURE=1, RESTART=2, SHUTDOWN=3};

#define INTERROR 323232

static inline void set_bit(int32_t *x, int bitNum) {
    *x |= (1L << bitNum);
}

static inline void clear_bit(int32_t *x, int bitNum) {
    *x &= ~ (1L << bitNum);
}

static inline void togle_bit(int32_t *x, int bitNum) {
    *x ^= (1L << bitNum);
}

static inline bool check_bit(int32_t *x, int bitNum) {
    int8_t bit = (*x >> bitNum) & 1;
    return (bit == 1);
}

static inline long elapsed_millis(struct timespec t, struct timespec t1){
    return t.tv_sec*1000 + t.tv_nsec/1.0e6 - t1.tv_sec*1000 - t1.tv_nsec/1.0e6;
}

//custom exception class
class my_exception : public std::runtime_error {
    std::string msg;
public:
    my_exception(const std::string &arg, const char *file, int line) :
    std::runtime_error(arg) {
        std::ostringstream o;
        o << file << ":" << line << ": " << arg;
        msg = o.str();
    }
    ~my_exception() throw() {}
    const char *what() const throw() {
        return msg.c_str();
    }
};
#define throw_line(arg) throw my_exception(arg, __FILE__, __LINE__);

#endif
