#ifndef FRAMECAN_H
#define FRAMECAN_H

#include <linux/can/raw.h>
#include <string.h>
#include "utils.h"

class frameCAN
{
    public:
        frameCAN();
        frameCAN(struct can_frame frame,CLIENT_TYPE ctype);
        struct can_frame getFrame();
        CLIENT_TYPE getClientType();
        virtual ~frameCAN();
    protected:
    private:
        struct can_frame frame;
        CLIENT_TYPE clientType;
};

#endif // FRAMECAN_H
