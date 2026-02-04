#include "25LC040A.h"
#include <algorithm>
#include <cassert>

EEPROM25LC040A::EEPROM25LC040A(IDriver* driver){
    assert(driver != nullptr);
    _driver = driver;
}
void EEPROM25LC040A::wait(){
    while (readStatus() & uint8_t(status::WIP)) {
    }
}
void EEPROM25LC040A::buildAndSendInstruction(instruction instr, uint16_t address){
    uint8_t cmd = instr;
    //! Если старший бит - 1 вставляем в инструкцию
    if((address >> 8) & 0x01){
        cmd |= 0x08;
    }
    _driver->transfer(cmd);
}
uint8_t EEPROM25LC040A::readStatus(){
    _driver->select();
    _driver->transfer(instruction::RDSR);
    uint8_t status = _driver->transfer(0xFF);
    _driver->deselect();
    return status;
}
uint8_t EEPROM25LC040A::readByte(uint16_t address){
    if(address > MAX_ADDR) {
        _errorCode = error::ADDRESS_OUT_OF_RANGE; 
        return 0;
    }
    _driver->select();
    buildAndSendInstruction(READ,address);
    _driver->transfer(static_cast<uint8_t>(address & 0xFF));
    uint8_t data = _driver->transfer(0xFF);
    _driver->deselect();
    _errorCode = error::OK;
    return data;
}
void EEPROM25LC040A::writeByte(uint16_t address, uint8_t byte){
    if(address > MAX_ADDR) {
        _errorCode = error::ADDRESS_OUT_OF_RANGE; 
        return;
    }
    _driver->select();
    _driver->transfer(instruction::WREN);
    _driver->deselect();
    uint8_t state = readStatus();
    if ((state & static_cast<uint8_t>(status::WEL)) == 0){
        _errorCode = error::WRITE_NOT_ENABLED;
        return;
    }
    _driver->select();
    buildAndSendInstruction(WRITE,address);
    _driver->transfer(static_cast<uint8_t>(address & 0xFF));
    _driver->transfer(byte);
    _driver->deselect();

    wait();
    _errorCode = error::OK;
}

bool EEPROM25LC040A::readBit(uint16_t address, uint8_t index){
    if(address > MAX_ADDR) {
        _errorCode = error::ADDRESS_OUT_OF_RANGE; 
        return 0;
    }
    if(index > 7) {
        _errorCode = error::INDEX_BIT_OUT_OF_RANGE; 
        return 0;
    }
    return readByte(address) >> index & 0x01;
}
void EEPROM25LC040A::writeBit(uint16_t address, uint8_t index, bool value){
    if(address > MAX_ADDR) {
        _errorCode = error::ADDRESS_OUT_OF_RANGE; 
        return;
    }
    if(index > 7) {
        _errorCode = error::INDEX_BIT_OUT_OF_RANGE; 
        return;
    }
    uint8_t byte = readByte(address);
    if(value){
        byte |= (1 << index);
    } else {
        byte &= ~(1 << index);
    }
    writeByte(address, byte);
}
void EEPROM25LC040A::readArray(uint16_t address, uint16_t length, uint8_t* out){
    if (length == 0) { _errorCode = error::OK; return; }
    if(address + length - 1 > MAX_ADDR) {
        _errorCode = error::ADDRESS_OUT_OF_RANGE; 
        return;
    }
    if (out == nullptr) {
        _errorCode = error::NULL_POINTER;
        return;
    }
    _driver->select();
    buildAndSendInstruction(READ,address);
    _driver->transfer(static_cast<uint8_t>(address & 0xFF));

    for (uint16_t i{0}; i<length; i++) {
        out[i] = _driver->transfer(0xFF);
    }
    _driver->deselect();
    _errorCode = error::OK;
}
void EEPROM25LC040A::writeArray(uint16_t address, uint16_t length,const uint8_t* data){
    if (length == 0) { _errorCode = error::OK; return; }
    if(address + length - 1 > MAX_ADDR) {
        _errorCode = error::ADDRESS_OUT_OF_RANGE; 
        return;
    }
    if (data == nullptr) {
        _errorCode = error::NULL_POINTER;
        return;
    }
    uint16_t offset{0};
    //! Запись ведется блоками, максимум в размер страницы
    while(length > 0){
        uint16_t page_offset = address % PAGE_SIZE;
        uint16_t chunk = std::min(static_cast<uint16_t>(PAGE_SIZE - page_offset), length);
        _driver->select();
        _driver->transfer(instruction::WREN);
        _driver->deselect();
        uint8_t state = readStatus();
        if ((state & static_cast<uint8_t>(status::WEL)) == 0){
            _errorCode = error::WRITE_NOT_ENABLED;
            return;
        }
        _driver->select();
        buildAndSendInstruction(WRITE,address);
        _driver->transfer(static_cast<uint8_t>(address & 0xFF));
        for (uint16_t j{0}; j < chunk; j++) {
            _driver->transfer(data[offset+j]);
        }
        _driver->deselect();
        wait();
        address += chunk; offset += chunk; length -= chunk;
    }
    _errorCode = error::OK;
}
EEPROM25LC040A::error EEPROM25LC040A::checkError(){
    return _errorCode;
}