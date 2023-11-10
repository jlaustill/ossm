//
// Created by jlaustill on 11/1/23.
//

#ifndef OPCM_OBD2_H
#define OPCM_OBD2_H
#include <FlexCAN_T4.h>
#include <AppData.h>

class OBD2 {
public:
    static void initialize(AppData *currentData);
    static void sendData(const CAN_message_t &msg);

private:
    static AppData *appData;
};


#endif //OPCM_OBD2_H