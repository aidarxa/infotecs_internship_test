/*!
    \file W25Q128.h
    \brief Обертка для работы с NOR Flash памятью W25Q128 через SPI драйвер
*/
#pragma once
#include "Driver.h"
#include <cstdint>
/*!
    \class NORW25Q128
    \brief Обертка для работы с NOR Flash памятью W25Q128 через SPI драйвер
*/
class NORW25Q128{
    //! Набор инструкций для работы с EEPROM
    enum instruction : uint8_t{
        READ = 0x03,                ///<Read data from memory array beginning at selected address
        FAST_READ = 0x0B,           ///<Fast read data
        PAGE_PROGRAM = 0x02,        ///<Аllows from one byte to 256 bytes (a page) of data to be programmed
        SECTOR_ERASE = 0x20,        ///<Sets all memory within a specified sector (4K-bytes) to the erased state of all 1s (FFh).
        BLOCK_ERASE_32K = 0x52,     ///<Sets all memory within a specified block (32K-bytes) to the erased state of all 1s (FFh).
        BLOCK_ERASE_64K = 0xD8,     ///<Sets all memory within a specified block (64K-bytes) to the erased state of all 1s (FFh).
        CHIP_ERASE = 0xC7,          ///<Sets all memory within the device to the erased state of all 1s (FFh).
        WRITE_ENABLE = 0x06,        ///<Sets the Write Enable Latch (WEL) bit in the Status Register to 1
        WRITE_DISABLE = 0x04,       ///<Sets the Write Enable Latch (WEL) bit in the Status Register to 0
        READ_STATUS_REG1 = 0x05     ///<Allow the 8-bit Status Registers to be read.
    };
    //! Биты регистра состояния
    enum class status:uint8_t{
        BUSY = 0x01,                ///<Erase/Write in progress
        WEL = 0x02,                 ///<Write enable latch
        BP0 = 0x04,                 ///<Block protect bit 0
        BP1 = 0x08,                 ///<Block protect bit 1
        BP2 = 0x10,                 ///<Block protect bit 2
        TB = 0x20,                  ///<Top/Bottom bit
        SEC = 0x40,                 ///<Sector/Block erase bit
        SRP0 = 0x80,                ///<Status register protect bit 0
    };
    //! Размер сектора (байты)
    static constexpr uint32_t SECTOR_SIZE = 4u * 1024u;
    //! Размер блока 32 (байты)
    static constexpr uint32_t BLOCK_32K_SIZE = 32u * 1024u;
    //! Размер блока 64 (байты)
    static constexpr uint32_t BLOCK_64K_SIZE = 64u * 1024u;
    //! Максимальный адрес памяти
    static constexpr uint32_t MAX_ADDR = 0xFFFFFF;
    public:
    //! Список ошибок
     enum class error{
        OK,                         ///<Нет ошибки
        ADDRESS_OUT_OF_RANGE,       ///<Адрес выходит за пределы памяти
        BAD_ADDRESS_ALIGNMENT,      ///<Адрес не выровнен по границе страницы/сектора/блока
        INDEX_BIT_OUT_OF_RANGE,     ///<Индекс бита выходит за пределы байта
        WRITE_NOT_ENABLED,          ///<Попытка записи при отключенной возможности записи
        NULL_POINTER,               ///<Передан нулевой указатель
        OUT_OF_PAGE,                ///<Данные не помещаются в страницу
        NEEDS_ERASE                 ///<Данные незвоможно записать (нужна очистка)
    };
    private:
    //! Экземпляр драйвера
    IDriver* _driver;
    //! Текущая ошибка
    error _errorCode {error::OK};
    //! Ожидание окончания записи
    void wait();
    ///! Установка разрешения на запись
    bool writeEnable();
    /*!
        Отправка 24-битного адреса
        \param[in] address Адрес для отправки
    */
    void sendAddress(uint32_t address);
    /*!
        Проверить можно ли записать данные без стирания
        \param[in] address Адрес начала записи
        \param[in] data Указатель на данные
        \param[in] length Длина данных
        \return true - можно записать, false - нужна очистка
    */
    bool isProgramCompatible(uint32_t address, const uint8_t* data, uint16_t length);
    public:
    /*!
        Конструктор
        \param[in] driver Указатель на экземпляр драйвера
    */ 
    NORW25Q128(IDriver* driver);
    /*! Функция проверки состояния ошибки

        Все функции кроме readStatusReg1() и checkError() в случае успеха устанавливают error::OK
        Рекомендуется вызывать эту функцию после каждой операции чтения/записи для проверки успешности операции

        \return Код ошибки error
    */
    error checkError();
    /*!
        Запросить данные регистра состояния 1
        \return Байт данных регистра
    */
    uint8_t readStatusReg1();
    /*!
        Прочитать байт
        
        Использует стандартное чтение (READ)
        \param[in] address Адрес байта
        \return Запрошнный байт
    */
    uint8_t readByte(uint32_t address);
    /*!
        Прочитать бит
        
        Использует стандартное чтение (READ)
        \param[in] address Адрес байта
        \param[in] index Индекс бита (0-7)
        \return Значение бита (0 или 1)
    */
    bool readBit(uint32_t address, uint8_t index);
    /*!
        Прочитать массив байт
        
        Использует быстрое чтение (FAST READ)
        \param[in] address Адрес начала чтения
        \param[in] length Длина массива в байтах
        \param[out] out Указатель на массив для записи данных
    */
    void readArray(uint32_t address, uint16_t length, uint8_t* out);
    /*!
        Записать страницу (до 256 байт)
        \param[in] address Адрес начала записи
        \param[in] data Указатель на данные для записи
        \param[in] length Длина данных в байтах (макс. 256)
    */
    void pageProgram(uint32_t address, const uint8_t* data, uint16_t length);

    /*!
        Стереть сектор (4Кбайт)
        \param[in] address Адрес начала сектора
    */
    void eraseSector(uint32_t address);
    /*!
        Стереть блок 32Кбайт
        \param[in] address Адрес начала блока
    */
    void eraseBlock32(uint32_t address);
    /*!
        Стереть блок 64Кбайт
        \param[in] address Адрес начала блока
    */
    void eraseBlock64(uint32_t address);
    /*!
        Очистить чип
    */
    void eraseChip();
};