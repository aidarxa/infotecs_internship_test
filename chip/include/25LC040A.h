/*!
    \file 25LC040A.h
    \brief Обертка для работы с EEPROM 25LC040A через SPI драйврер
*/
#pragma once
#include "Driver.h"
#include <cstdint>
/*!
    \class EEPROM25LC040A
    \brief Обертка для работы с EEPROM 25LC040A через SPI драйврер
*/
class EEPROM25LC040A{
    //! Набор инструкций для работы с EEPROM
    enum instruction : uint8_t{
        READ = 0x03,                ///<Read data from memory array beginning at selected address
        WRITE = 0x02,               ///<Write data to memory array beginning at selected address
        WRDI = 0x04,                ///<Reset the write enable latch (disable write operations)
        WREN = 0x06,                ///<Set the write enable latch (enable write operations)
        RDSR = 0x05,                ///<Read STATUS register
        WRSR = 0x01                 ///<Write STATUS register
    };
    //! Биты регистра состояния
    enum class status:uint8_t{
        WIP = 0x01,                 ///<Write in progress
        WEL = 0x02,                 ///<Write enable latch
        BP0 = 0x04,                 ///<Block protect bit 0
        BP1 = 0x08,                 ///<Block protect bit 1
    };
    //! Максимальный адрес памяти
    static constexpr uint16_t MAX_ADDR = 0x1FF;
    //! Размер страницы (байты)
    static constexpr uint16_t PAGE_SIZE = 16;
    
    public:
    //! Список ошибок
     enum class error{
        OK,                         ///<Нет ошибки
        ADDRESS_OUT_OF_RANGE,       ///<Адрес выходит за пределы памяти
        INDEX_BIT_OUT_OF_RANGE,     ///<Индекс бита выходит за пределы байта
        WRITE_NOT_ENABLED,          ///<Попытка записи при отключенной возможности записи
        NULL_POINTER                ///<Передан нулевой указатель
    };

    private:
    //! Экземпляр драйвера
    IDriver* _driver;
    //! Текущая ошибка
    error _errorCode {error::OK};
    //! Ожидание окончания записи
    void wait();

    /*!
        Собрать и отправить инструкцию
        Так как адресация 9-ти битовая старший бит встраивается в инструкцию и отправляется вместе с ней
        \param[in] instr Инструкция
        \param[in] address 9-ти битовый адрес
    */
    void buildAndSendInstruction(instruction instr, uint16_t address);

    public:
    /*!
        Конструктор
        \param[in] driver Указатель на экземпляр драйвера
    */ 
    EEPROM25LC040A(IDriver* driver);
    /*! Функция проверки состояния ошибки

        Все функции кроме readStatus() и checkError() в случае успеха устанавливают error::OK
        Рекомендуется вызывать эту функцию после каждой операции чтения/записи для проверки успешности операции

        \return Код ошибки error
    */
    error checkError();
    /*!
        Запросить данные регистра состояния
        \return Байт данных регистра
    */
    uint8_t readStatus();
    /*!
        Прочитать байт
        \param[in] address Адрес байта
        \return Запрошнный байт
    */
    uint8_t readByte(uint16_t address);
    /*!
        Записать байт по адресу
        \param[in] address Адрес байта для записи
        \param[in] byte Байт данных
    */
    void writeByte(uint16_t address, uint8_t byte);
    /*!
        Прочитать бит
        \param[in] address Адрес байта
        \param[in] index Индекс бита (0-7)
        \return Значение бита (0 или 1)
    */
    bool readBit(uint16_t address, uint8_t index);
    /*!
        Записать бит
        
        Дорогая операция: чтение -> модификация -> запись байта

        \param[in] address Адрес байта
        \param[in] index Индекс бита (0-7)
        \param[in] value Значение бита (0 или 1)
    */
    void writeBit(uint16_t address, uint8_t index, bool value);
    /*!
        Прочитать массив байт
        \param[in] address Адрес начала чтения
        \param[in] length Длина массива в байтах
        \param[out] out Указатель на массив для записи данных
    */
    void readArray(uint16_t address, uint16_t length, uint8_t* out);
    /*!
        Записать массив байт
        \param[in] address Адрес начала записи
        \param[in] length Длина массива в байтах
        \param[in] data Указатель на массив с данными для записи
    */
    void writeArray(uint16_t address, uint16_t length,const uint8_t* data);
};