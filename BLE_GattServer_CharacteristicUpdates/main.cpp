/* mbed Microcontroller Library
 * Copyright (c) 2017-2019 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include<vector>
#include "platform/Callback.h"
#include "events/EventQueue.h"
#include "ble/BLE.h"
#include "gatt_server_process.h"
#include "mbed.h"
// #include "AnalogRead.h"

//#include "am_bsp.h"
using mbed::callback;
using namespace std::literals::chrono_literals;

mbed::DigitalOut led(LED1) ; 

//mbed::I2C  i2c(I2C_SDA , I2C_SCL);

bno055_interface* bno ; 
 
//mbed::AnalogIn Flex( A0 , 0.0 ) ; 
//mbed::PortIn(A0) ; 
//mbed::AnalogIn Flex[5] { {A0,0.0} ,{A1,0.0} , {A2,0.0} ,{A3,0.0} ,{A4,0.0}  }; 

/**
 * A Clock service that demonstrate the GattServer features.
 *
 * The clock service host three characteristics that model the current hour,
 * minute and second of the clock. The value of the second characteristic is
 * incremented automatically by the system.
 *
 * A client can subscribe to updates of the clock characteristics and get
 * notified when one of the value is changed. Clients can also change value of
 * the second, minute and hour characteristric.
 */
class ClockService : public ble::GattServer::EventHandler {
public:
    ClockService() :
        _second_char("8dd6a1b7-bc75-4741-8a26-264af75807de" , 0x0000), // "8dd6a1b7-bc75-4741-8a26-264af75807de"
        // _first_char("8dd6a1b7-bc75-4741-8a26-264af75807de"  , "test") , 
        _clock_service(
            /* uuid */ "51311102-030e-485f-b122-f8f381aa84ed" , // "51311102-030e-485f-b122-f8f381aa84ed"
            /* characteristics */ _clock_characteristics,
            /* numCharacteristics */ sizeof(_clock_characteristics) /
                                     sizeof(_clock_characteristics[0])
        )
    {
        /* update internal pointers (value, descriptors and characteristics array) */
        _clock_characteristics[0] = &_second_char;

        /* setup authorization handlers */
        _second_char.setWriteAuthorizationCallback(this, &ClockService::authorize_client_write);
    }

    void start(BLE &ble, events::EventQueue &event_queue)
    {   
        
        _server = &ble.gattServer();
        _event_queue = &event_queue;

        printf("Registering demo service\r\n");
        ble_error_t err = _server->addService(_clock_service);

        if (err) {
            printf("Error %u during demo service registration.\r\n", err);
            return;
        }

        /* register handlers */
        _server->setEventHandler(this);

        printf("clock service registered\r\n");
        printf("service handle: %u\r\n", _clock_service.getHandle());
        printf("second characteristic value handle %u\r\n", _second_char.getValueHandle());
        
        _event_queue->call_every(100ms, callback(this, &ClockService::ReadFlexSensors));
    }

    /* GattServer::EventHandler */
private:

    /**
     * Handler called when a notification or an indication has been sent.
     */
    void onDataSent(const GattDataSentCallbackParams &params) override
    {
        printf("sent updates\r\n");
    }

    /**
     * Handler called after an attribute has been written.
     */
    void onDataWritten(const GattWriteCallbackParams &params) override
    {
        printf("data written:\r\n");
        printf("connection handle: %u\r\n", params.connHandle);
        printf("attribute handle: %u", params.handle);
        // if (params.handle == _hour_char.getValueHandle()) {
        //     printf(" (hour characteristic)\r\n");
        // } else if (params.handle == _minute_char.getValueHandle()) {
        //     printf(" (minute characteristic)\r\n");
        // } else 
        if (params.handle == _second_char.getValueHandle()) {
            printf(" (second characteristic)\r\n");
        } else {
            printf("\r\n");
        }
        printf("write operation: %u\r\n", params.writeOp);
        printf("offset: %u\r\n", params.offset);
        printf("length: %u\r\n", params.len);
        printf("data: ");

        for (size_t i = 0; i < params.len; ++i) {
            printf("%02X", params.data[i]);
        }

        printf("\r\n");
    }

    /**
     * Handler called after an attribute has been read.
     */
    void onDataRead(const GattReadCallbackParams &params) override
    {
        printf("data read:\r\n");
        printf("connection handle: %u\r\n", params.connHandle);
        printf("attribute handle: %u", params.handle);
        // if (params.handle == _hour_char.getValueHandle()) {
        //     printf(" (hour characteristic)\r\n");
        // } else if (params.handle == _minute_char.getValueHandle()) {
        //     printf(" (minute characteristic)\r\n");
        // } else
         if (params.handle == _second_char.getValueHandle()) {
            printf(" (second characteristic)\r\n");
        } else {
            printf("\r\n");
        }
    }

    /**
     * Handler called after a client has subscribed to notification or indication.
     *
     * @param handle Handle of the characteristic value affected by the change.
     */
    void onUpdatesEnabled(const GattUpdatesEnabledCallbackParams &params) override
    {
        printf("update enabled on handle %d\r\n", params.attHandle);
    }

    /**
     * Handler called after a client has cancelled his subscription from
     * notification or indication.
     *
     * @param handle Handle of the characteristic value affected by the change.
     */
    void onUpdatesDisabled(const GattUpdatesDisabledCallbackParams &params) override
    {
        printf("update disabled on handle %d\r\n", params.attHandle);
    }

    /**
     * Handler called when an indication confirmation has been received.
     *
     * @param handle Handle of the characteristic value that has emitted the
     * indication.
     */
    void onConfirmationReceived(const GattConfirmationReceivedCallbackParams &params) override
    {
        printf("confirmation received on handle %d\r\n", params.attHandle);
    }

private:
    /**
     * Handler called when a write request is received.
     *
     * This handler verify that the value submitted by the client is valid before
     * authorizing the operation.
     */

    //mbed::DigitalOut led(LED1);
    void authorize_client_write(GattWriteAuthCallbackParams *e)
    {
        printf("characteristic %u write authorization\r\n", e->handle);

        if (e->offset != 0) {
            printf("Error invalid offset\r\n");
            e->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_OFFSET;
            return;
        }

        if (e->len != 1) {
            printf("Error invalid len\r\n");
            e->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_ATT_VAL_LENGTH;
            return;
        }

        if (e->data[0] >= 60)
        {
            printf("Error invalid data\r\n");
            e->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_WRITE_NOT_PERMITTED;
            return;
        }

        e->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    }

    /**
     * Increment the second counter.
     */
    void ReadFlexSensors(void)
    {
         led = !led;
        // std::string buffer = "" ; 
        std::vector<uint8_t> tempVector ; 

        int Flex[5] = { indexAnalogRead(11) , 
        indexAnalogRead(12) , indexAnalogRead(13) ,
        indexAnalogRead(29) , indexAnalogRead(32)} ; 

        for(int i = 0 ; i<5 ; i++){
            // buffer.append( to_string(Flex[i])) ; 
            // buffer.append(" # ") ; 

            // uint8_t hexBuffer[4] ;
            // memcpy((char*)hexBuffer,(char*)&Flex[i],sizeof(int));
            // for(int j=0 ; j< 4 ; j++)
            //     tempVector.push_back(hexBuffer[j]) ; 
            // tempVector.push_back(0x00) ;    

            tempVector.push_back(Flex[i]) ; 
            tempVector.push_back(0x0) ;  
        }

        // const int addr8bit = 0x48 << 1; // 8-bit I2C address, 0x90
        // char cmd[2] {0x01 , 0x00};
        // i2c.write(addr8bit, cmd, 2);

        // tempVector.push_back(bno->SensorRead()) ; 
        // tempVector.push_back(0x00) ; 
        // std::vector<uint8_t> tempVector(buffer.begin(), buffer.end());
        
        uint8_t *second = &tempVector[0];

        // ble_error_t err = _second_char.get(*_server, second[0]);
        // if (err) {
        //     printf("read of the second value returned error %u\r\n", err);
        //     return;
        // }

        ble_error_t err = _second_char.set(*_server, second);
        if (err) {
            printf("write of the second value returned error %u\r\n", err);
            return;
        }

        // if (second == 0) {
        //     increment_minute();
        // }
    }

    /**
     * Increment the minute counter.
     */
    // void increment_minute(void)
    // {
    //     uint8_t minute = 0;
    //     ble_error_t err = _minute_char.get(*_server, minute);
    //     if (err) {
    //         printf("read of the minute value returned error %u\r\n", err);
    //         return;
    //     }

    //     minute = (minute + 1) % 60;

    //     err = _minute_char.set(*_server, minute);
    //     if (err) {
    //         printf("write of the minute value returned error %u\r\n", err);
    //         return;
    //     }

    //     if (minute == 0) {
    //         increment_hour();
    //     }
    // }

    /**
     * Increment the hour counter.
     */
    // void increment_hour(void)
    // {
    //     uint8_t hour = 0;
    //     ble_error_t err = _hour_char.get(*_server, hour);
    //     if (err) {
    //         printf("read of the hour value returned error %u\r\n", err);
    //         return;
    //     }

    //     hour = (hour + 1) % 24;

    //     err = _hour_char.set(*_server, hour);
    //     if (err) {
    //         printf("write of the hour value returned error %u\r\n", err);
    //         return;
    //     }
    // }

private:
    /**
     * Read, Write, Notify, Indicate  Characteristic declaration helper.
     *
     * @tparam T type of data held by the characteristic.
     */
    template<typename T>
    class ReadWriteNotifyIndicateCharacteristic : public GattCharacteristic {
    public:
        /**
         * Construct a characteristic that can be read or written and emit
         * notification or indication.
         *
         * @param[in] uuid The UUID of the characteristic.
         * @param[in] initial_value Initial value contained by the characteristic.
         */
            //GattAttribute *descriptors[1] {GattCharacteristic::BLE_GATT_FORMAT_UTF8S} ; 

        ReadWriteNotifyIndicateCharacteristic(const UUID & uuid, const T& initial_value) :
            GattCharacteristic(
                /* UUID */ uuid,
                /* Initial value */ _value,
                /* Value size */ 20,
                /* Value capacity */ 20,
                /* Properties */ GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ |
                                 GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY |
                                 GattCharacteristic::BLE_GATT_FORMAT_UTF8S | 
                                 GattCharacteristic::BLE_GATT_FORMAT_SINT32,
                /* Descriptors */  nullptr,
                /* Num descriptors */ 0,
                /* variable len */ true
            ),
            _value(initial_value) {
        }

        /**
         * Get the value of this characteristic.
         *
         * @param[in] server GattServer instance that contain the characteristic
         * value.
         * @param[in] dst Variable that will receive the characteristic value.
         *
         * @return BLE_ERROR_NONE in case of success or an appropriate error code.
         */
        ble_error_t get(GattServer &server, T& dst) const
        {
            uint16_t value_length = sizeof(dst);
            return server.read(getValueHandle(), &dst, &value_length);
        }

        /**
         * Assign a new value to this characteristic.
         *
         * @param[in] server GattServer instance that will receive the new value.
         * @param[in] value The new value to set.
         * @param[in] local_only Flag that determine if the change should be kept
         * locally or forwarded to subscribed clients.
         */
        ble_error_t set(GattServer &server, const uint8_t* value, bool local_only = false) const
        {
            return server.write(getValueHandle(), value, sizeof(value) * 4, local_only);
        }

    private:
        uint8_t* _value;
    };

private:
    GattServer *_server = nullptr;
    events::EventQueue *_event_queue = nullptr;

    GattService _clock_service;
    GattCharacteristic* _clock_characteristics[1];    
    ReadWriteNotifyIndicateCharacteristic<uint8_t*> _second_char;
    // ReadOnlyGattCharacteristic<string> _first_char ; 
};

int main() {
    unsigned int rr =  initializeADC() ; 
    int aa  = indexAnalogRead(11) ; 


    bno = new bno055_interface() ; 

    BLE &ble = BLE::Instance();
    events::EventQueue event_queue;
    ClockService demo_service;

    /* this process will handle basic ble setup and advertising for us */
    GattServerProcess ble_process(event_queue, ble);

    /* once it's done it will let us continue with our demo */
    ble_process.on_init(callback(&demo_service, &ClockService::start));

    ble_process.start();

    return 0;
}
