#include "Driver.h"
#include "25LC040A.h"
#include "W25Q128.h"
#include <iostream>

/*!
    Реализация примитивного драйвера для примера
    Устанавливается бит WEL для разрешения записи
*/
class Driver : public IDriver{
    void select() override {};
    void deselect() override {};
    uint8_t transfer(uint8_t byte) override {return 0b00000010;};
};

int main(){
    Driver driver;
    //! 25LC040A
    {
        EEPROM25LC040A chip {&driver};
        bool bit = chip.readBit(0x00, 3);
        if (chip.checkError() != EEPROM25LC040A::error::OK) {
            std::cout << "Read bit error (EEPROM)" << "\n";
        }
        uint8_t byte = chip.readByte(0x00);
        if (chip.checkError() != EEPROM25LC040A::error::OK) {
            std::cout << "Read byte error (EEPROM)" << "\n";
        }
        uint8_t* buffer = new uint8_t[16];
        chip.readArray(0x00, 16, buffer);
        if (chip.checkError() != EEPROM25LC040A::error::OK) {
            std::cout << "Read array error (EEPROM)" << "\n";
        }

        chip.writeBit(0x00,3,true);
        if (chip.checkError() != EEPROM25LC040A::error::OK) {
            std::cout << "Write bit error (EEPROM)" << "\n";;
        }
        uint8_t data = 73;
        chip.writeByte(0x00, data);
        if (chip.checkError() != EEPROM25LC040A::error::OK) {
            std::cout << "Write byte error (EEPROM)" << "\n";
        }
        chip.writeArray(0x00,16,buffer);
        if (chip.checkError() != EEPROM25LC040A::error::OK) {
            std::cout << "Write array error (EEPROM)" << "\n";
        }
        delete[] buffer;
    }

    //!W25Q128
    {
        NORW25Q128 chip{&driver};
        bool bit = chip.readBit(0x00, 3);
        if (chip.checkError() != NORW25Q128::error::OK) {
            std::cout << "Read bit error (NOR)" << "\n";
        }
        uint8_t byte = chip.readByte(0x00);
        if (chip.checkError() != NORW25Q128::error::OK) {
            std::cout << "Read byte error (NOR)" << "\n";
        }
        uint8_t* buffer = new uint8_t[256];
        chip.readArray(0x00, 256, buffer);
        if (chip.checkError() != NORW25Q128::error::OK) {
            std::cout << "Read array error (NOR)" << "\n";
        }
        chip.eraseSector(0x00);
        if (chip.checkError() != NORW25Q128::error::OK) {
            std::cout << "Erase sector error (NOR)" << "\n";
        }
        chip.eraseBlock32(0x00);
        if (chip.checkError() != NORW25Q128::error::OK) {
            std::cout << "Erase block 32 error (NOR)" << "\n";
        }
        chip.eraseBlock64(0x00);
        if (chip.checkError() != NORW25Q128::error::OK) {
            std::cout << "Erase block 64 error (NOR)" << "\n";
        }
        chip.eraseChip();
        if (chip.checkError() != NORW25Q128::error::OK) {
            std::cout << "Erase chip error (NOR)" << "\n";
        }
        chip.pageProgram(0x00, buffer, 256);
        if (chip.checkError() != NORW25Q128::error::OK) {
            std::cout << "Page program error (NOR)" << "\n";
        }
        delete[] buffer;
    }
    return 0;
}