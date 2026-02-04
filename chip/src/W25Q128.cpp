#include "W25Q128.h"
#include <cassert>
#include <cstdint>

NORW25Q128::NORW25Q128(IDriver* driver){
    assert(driver != nullptr);
    _driver = driver;
}
void NORW25Q128::wait(){
    while (readStatusReg1() & static_cast<uint8_t>(status::BUSY)) {
    }
}
NORW25Q128::error NORW25Q128::checkError(){ return _errorCode; }
bool NORW25Q128::writeEnable(){
    _driver->select();
    _driver->transfer(instruction::WRITE_ENABLE);
    _driver->deselect();
    return readStatusReg1() & static_cast<uint8_t>(status::WEL);
}

void NORW25Q128::sendAddress(uint32_t address){
    _driver->transfer((address >> 16) & 0xFF);
    _driver->transfer((address >> 8) & 0xFF);
    _driver->transfer(address & 0xFF);
}
uint8_t NORW25Q128::readStatusReg1(){
    _driver->select();
    _driver->transfer(instruction::READ_STATUS_REG1);
    uint8_t status = _driver->transfer(0xFF);
    _driver->deselect();
    return status;
}
uint8_t NORW25Q128::readByte(uint32_t address){
    if(address > MAX_ADDR){
        _errorCode = error::ADDRESS_OUT_OF_RANGE;
        return 0;
    }
    _driver->select();
    _driver->transfer(instruction::READ);
    sendAddress(address);
    uint8_t byte = _driver->transfer(0xFF);
    _driver->deselect();
    _errorCode = error::OK;
    return byte;
}
bool NORW25Q128::readBit(uint32_t address, uint8_t index){
    if(index > 7){
        _errorCode = error::INDEX_BIT_OUT_OF_RANGE;
        return false;
    }
    uint8_t byte = readByte(address);
    return (byte >> index) & 1;
}
void NORW25Q128::readArray(uint32_t address, uint16_t length, uint8_t* out){
    if (length == 0) { _errorCode = error::OK; return; }
    if(out == nullptr){
        _errorCode = error::NULL_POINTER;
        return;
    }
    if(address + length - 1 > MAX_ADDR){
        _errorCode = error::ADDRESS_OUT_OF_RANGE;
        return;
    }
    _driver->select();
    _driver->transfer(instruction::FAST_READ);
    sendAddress(address);
    _driver->transfer(0xFF);
    for(uint16_t i = 0; i < length; i++){
        out[i] = _driver->transfer(0xFF);
    }
    _driver->deselect();
    _errorCode = error::OK;
}
bool NORW25Q128::isProgramCompatible(uint32_t address, const uint8_t* data, uint16_t length)
{
    for (uint16_t i = 0; i < length; ++i) {
        uint8_t oldByte = readByte(address + i);
        if (_errorCode != error::OK) {
            return false;
        }
        if ((oldByte & data[i]) != data[i]) {
            return false;
        }
    }
    return true;
}

void NORW25Q128::pageProgram(uint32_t address, const uint8_t* data, uint16_t length){
    if (length == 0) { _errorCode = error::OK; return; }
    if(address + length - 1 > MAX_ADDR || length > 256){
        _errorCode = error::ADDRESS_OUT_OF_RANGE;
        return;
    }
    uint32_t page_off = address & 0xFFu;
    if (page_off + length > 256u) { 
        _errorCode = error::OUT_OF_PAGE; 
        return; 
    }
    if (data == nullptr) {
        _errorCode = error::NULL_POINTER;
        return;
    }
    if (!isProgramCompatible(address, data, length)) {
        _errorCode = error::NEEDS_ERASE;
        return;
    }
    if(!writeEnable()){
        _errorCode = error::WRITE_NOT_ENABLED;
        return;
    }
    _driver->select();
    _driver->transfer(instruction::PAGE_PROGRAM);
    sendAddress(address);
    for(uint16_t i = 0; i < length; i++){
        _driver->transfer(data[i]);
    }
    _driver->deselect();
    wait();
    _errorCode = error::OK;
}
void NORW25Q128::eraseSector(uint32_t address){
    if(address > MAX_ADDR){
        _errorCode = error::ADDRESS_OUT_OF_RANGE;
        return;
    }
    if(address % SECTOR_SIZE != 0){
        _errorCode = error::BAD_ADDRESS_ALIGNMENT;
        return;
    }
    if(!writeEnable()){
        _errorCode = error::WRITE_NOT_ENABLED;
        return;
    }
    _driver->select();
    _driver->transfer(instruction::SECTOR_ERASE);
    sendAddress(address);
    _driver->deselect();
    wait();
    _errorCode = error::OK;
}
void NORW25Q128::eraseBlock32(uint32_t address){
    if(address > MAX_ADDR){
        _errorCode = error::ADDRESS_OUT_OF_RANGE;
        return;
    }
    if(address % BLOCK_32K_SIZE != 0){
        _errorCode = error::BAD_ADDRESS_ALIGNMENT;
        return;
    }
    if(!writeEnable()){
        _errorCode = error::WRITE_NOT_ENABLED;
        return;
    }
    _driver->select();
    _driver->transfer(instruction::BLOCK_ERASE_32K);
    sendAddress(address);
    _driver->deselect();
    wait();
    _errorCode = error::OK;
}
void NORW25Q128::eraseBlock64(uint32_t address){
    if(address > MAX_ADDR){
        _errorCode = error::ADDRESS_OUT_OF_RANGE;
        return;
    }
    if(address % BLOCK_64K_SIZE != 0){
        _errorCode = error::BAD_ADDRESS_ALIGNMENT;
        return;
    }
    if(!writeEnable()){
        _errorCode = error::WRITE_NOT_ENABLED;
        return;
    }
    _driver->select();
    _driver->transfer(instruction::BLOCK_ERASE_64K);
    sendAddress(address);
    _driver->deselect();
    wait();
    _errorCode = error::OK;
}    
void NORW25Q128::eraseChip(){
    if(!writeEnable()){
        _errorCode = error::WRITE_NOT_ENABLED;
        return;
    }
    _driver->select();
    _driver->transfer(instruction::CHIP_ERASE);
    _driver->deselect();
    wait();
    _errorCode = error::OK;
}