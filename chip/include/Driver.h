#pragma once
#include <cstdint>

/*!
    \class IDriver
    \brief Примитивный интерфейс драйвера для работы микросхемами через SPI
*/
class IDriver{
    public:
    //! Выбрать устройство (установить CS в низкий уровень)
    virtual void select() = 0;
    //! Снять выбор устройства (установить CS в высокий уровень)
    virtual void deselect() = 0;
    /*! \brief Передать и получить байт
        \param[in] byte Байт для передачи
        \return Принятый байт
    */
    virtual uint8_t transfer(uint8_t byte) = 0;
};