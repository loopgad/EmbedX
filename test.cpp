#include "test.h"
#include "usart_api.hpp"
uint8_t my_buf[12] = {0};
const uint8_t* temp_const_buf =  usart::UsartAPI::rx_ptr(1);
uint8_t my_buf_[64] = {0};
uint8_t data_length = 64;


extern "C" {

    void get_buf(void){
        usart::UsartAPI::recv(1, my_buf, sizeof(my_buf));
		std::memcpy(my_buf_, temp_const_buf, data_length);
		usart::UsartAPI::send(1, my_buf, sizeof(my_buf));
    }
}