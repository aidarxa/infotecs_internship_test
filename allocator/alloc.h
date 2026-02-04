/*!
    \file alloc.h
    \brief Заголовочный файл кастомного аллокатора памяти

    Файл содержит реализацию кастомного аллокатора с сегментно-страничной организацией.
    Аллокатор поддерживает выделение и освобождение памяти для маленьких (до 15 байт) и больших (до 180 байт) блоков.

    Маленькие блоки выравниваются до 16 байт, большие - до 192 байт.
    Общий размер кучи составляет 64 КБ, разбитых на страницы по 1 КБ каждая.
    Каждая страница может находится в одном из трех состояний: свободная, для маленьких блоков или для больших блоков.

    Пример использования:

    \code 
    int main(){
        alloc_init();
        uint8_t* ptr = custom_malloc(13);
        if (ptr == NULL) {
            printf("Allocation failed\n");
            return -1;
        }
        else {
            printf("Allocation succeeded: %p\n", ptr);
        }
        custom_free(ptr);
        return 0;
    }
    \endcode
*/
#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>    
#include <string.h>

/*! 
    \brief Размер кучи 
*/
#define HEAP_SIZE (64 * 1024)
/*! 
    \brief Размер страницы (степень двойки в байтах)
*/
#define PAGE_SIZE 1024
#if (HEAP_SIZE % PAGE_SIZE != 0)
#error "HEAP_SIZE must be multiple of PAGE_SIZE"
#endif
/*! 
    \brief Размер сдвига для получения индекса страницы (2^x)
*/
#define PAGE_SHIFT 10
#if (1 << PAGE_SHIFT != PAGE_SIZE)
#error "PAGE_SIZE must be a power of two"
#endif

/*! 
    \brief Размер заголовка
*/
#define PAGE_HEADER_SIZE 16
/*! 
    \brief Размер маленького блока
*/
#define BLOCK_SMALL 15
/*! 
    \brief Размер большого блока
*/
#define BLOCK_BIG 180
/*! 
    \brief Размер маленького блока с выравниванием
*/
#define ALIGNED_BLOCK_SMALL 16
/*! 
    \brief Размер большого блока с выравниванием
*/
#define ALIGNED_BLOCK_BIG 192
/*! 
    \brief Количество маленьких сегментов
*/
#define SMALL_SEGMENTS ((PAGE_SIZE - PAGE_HEADER_SIZE) / ALIGNED_BLOCK_SMALL)
/*! 
    \brief Количество больших сегментов
*/
#define BIG_SEGMENTS ((PAGE_SIZE - PAGE_HEADER_SIZE) / ALIGNED_BLOCK_BIG)
/*! 
    \brief Количество страниц
*/
#define PAGE_COUNT (HEAP_SIZE / PAGE_SIZE)

//! Набор состояний страницы
typedef enum {
    PAGE_FREE,      ///< Страница свободна
    PAGE_SMALL,     ///< Страница используется для маленьких блоков
    PAGE_BIG        ///< Страница используется для больших блоков
} page_type_t;

/*!
    \struct h_page
    \brief Описатель страницы памяти

    Хранит информацию о количестве использованных сегментов и типе страницы.
*/
typedef struct {
    uint16_t used_segments;
    page_type_t type;
} h_page;

//! Куча
static uint8_t heap[HEAP_SIZE];
//! Таблица страниц
static h_page page_table[PAGE_COUNT];

/*!
    \brief Получить базовый адрес страницы по её индексу.
    \param[in] page_index Индекс страницы
    \return Указатель на базовый адрес страницы
*/
static inline uint8_t* page_base_addr(size_t page_index) {
    return &heap[page_index << PAGE_SHIFT];
}
/*!
    \brief Получить адрес заголовка страницы по её индексу.
    \param[in] page_index Индекс страницы
    \return Указатель на адрес заголовка страницы

    Alias для page_base_addr
*/
static inline uint8_t* page_header_addr(size_t page_index) {
    return page_base_addr(page_index);
}
/*!
    \brief Получить адрес начала данных в странице по её индексу.
    \param[in] page_index Индекс страницы
    \return Указатель на адрес данных страницы
*/
static inline uint8_t* page_data_addr(size_t page_index) {
    return page_base_addr(page_index) + PAGE_HEADER_SIZE;
}
/*!
    \brief Найти перывый свободный блок
    \param[in] bitmap Указатель на битовую карту
    \param[in] bit_count Количество бит для проверки
    \return Индекс первого свободного бита или UINT16_MAX, если свободных нет
*/
static uint16_t find_free_block(const uint8_t* bitmap, uint16_t bit_count) {
    for(uint16_t i = 0; i < bit_count; i++) {
        uint8_t byte = bitmap[i >> 3];
        uint8_t mask = (uint8_t)(1 << (i & 7));
        if ((byte & mask) == 0) {
            return i;
        }
    }
    return UINT16_MAX;
}
/*!
    \brief  Установить бит в состояние 1 (блок занят)
    \param[in] bitmap Указатель на битовую карту
    \param[in] index Индекс бита для установки
*/
static void bitmap_set(uint8_t* bitmap, uint16_t index) {
    bitmap[index >> 3] |= (1 << (index & 7));
}
/*!
    \brief  Установить бит в состояние 0 (блок свободен)
    \param[in] bitmap Указатель на битовую карту
    \param[in] index Индекс бита для очистки
*/
static void bitmap_clear(uint8_t* bitmap, uint16_t index) {
    bitmap[index >> 3] &= ~(1 << (index & 7));
}
/*!
    \brief Проверить состояние бита (блока)
    \param[in] bitmap Указатель на битовую карту
    \param[in] index Индекс бита для проверки
    \return true, если бит установлен (занят), false - если сброшен (свободен)
*/
static bool bitmap_is_set(const uint8_t* bitmap, uint16_t index) {
    return (bitmap[index >> 3] & (1 << (index & 7))) != 0;
}
/*!
    \brief Установить страницу в состояние свободной
    \param[in] page_index Индекс страницы
*/
static void page_set_free(size_t page_index) {
    page_table[page_index].type = PAGE_FREE;
    page_table[page_index].used_segments = 0;
    memset(page_header_addr(page_index), 0, PAGE_HEADER_SIZE);
}
/*!
    \brief Установить страницу в состояние для маленьких блоков
    \param[in] page_index Индекс страницы
*/
static void page_set_small(size_t page_index) {
    page_table[page_index].type = PAGE_SMALL;
    page_table[page_index].used_segments = 0;
}
/*!
    \brief Установить страницу в состояние для больших блоков
    \param[in] page_index Индекс страницы
*/
static void page_set_big(size_t page_index) {
    page_table[page_index].type = PAGE_BIG;
    page_table[page_index].used_segments = 0;
}
/*!
    \brief Инициализировать аллокатор

    Устанавливает все страницы в состояние свободных.

    \warning Перед первым использованием аллокатора требуется вызвать alloc_init()
*/
void alloc_init() {
    for(size_t i = 0; i < PAGE_COUNT; ++i) {
        page_set_free(i);
    }
}
/*!
    \brief Выделить маленький блок
    \return Адрес начала блока или NULL, в случае когда свободного блока нет
*/
static void* alloc_small(){
    for(size_t page = 0; page < PAGE_COUNT; page++){
        if(page_table[page].type == PAGE_SMALL && page_table[page].used_segments < SMALL_SEGMENTS){
            uint8_t* bitmap = page_header_addr(page);
            uint16_t free_block = find_free_block(bitmap, SMALL_SEGMENTS);
            if(free_block != UINT16_MAX){
                bitmap_set(bitmap, free_block);
                page_table[page].used_segments++;
                return (void*)(page_data_addr(page) + free_block * ALIGNED_BLOCK_SMALL);
            }
        }
    }
    for(size_t page = 0; page < PAGE_COUNT; page++){
        if(page_table[page].type == PAGE_FREE){
            page_set_small(page);
            uint8_t* bitmap = page_header_addr(page);
            bitmap_set(bitmap, 0);
            page_table[page].used_segments = 1;
            return (void*)(page_data_addr(page));
        }
    }
    return NULL;
}
/*!
    \brief Выделить большой блок
    \return Адрес начала блока или NULL, в случае когда свободного блока нет
*/
static void* alloc_big(){
    for (size_t page = 0; page < PAGE_COUNT; page++) {
        if(page_table[page].type == PAGE_BIG && page_table[page].used_segments < BIG_SEGMENTS){
            uint8_t* bitmap = page_header_addr(page);
            uint16_t free_block = find_free_block(bitmap, BIG_SEGMENTS);
            if(free_block != UINT16_MAX){
                bitmap_set(bitmap, free_block);
                page_table[page].used_segments++;
                return (void*)(page_data_addr(page) + free_block * ALIGNED_BLOCK_BIG);
            }
        }
    }
    for (size_t page = 0; page < PAGE_COUNT; page++) {
        if(page_table[page].type == PAGE_FREE){
            page_set_big(page);
            uint8_t* bitmap = page_header_addr(page);
            bitmap_set(bitmap,0);
            page_table[page].used_segments = 1;
            return (void*)(page_data_addr(page));
        }
    }
    return NULL;
}
/*!
    \brief Выделить память
    \param[in] Размер запрашивоемой памяти
    \return Адрес начала блока или NULL, в случае если:
    - Было запрошено 0 байт памяти
    - Было запрошено > BLOCK_BIG
    - Свободный блок памяти не был найден
*/
void* custom_malloc(size_t size){
    if (size == 0) {
        return NULL;
    }
    if (size <= BLOCK_SMALL) {
        return alloc_small();
    }else if (size <= BLOCK_BIG) {
        return alloc_big();
    }
    return NULL;
};
/*!
    \brief Очистить память
    \param[in] Указатель на блок
*/
void custom_free(void* ptr){
    uint8_t* pointer = (uint8_t*)ptr;
    if(pointer == NULL || pointer < &heap[0] || pointer >= &heap[HEAP_SIZE]){
        return;
    }
    size_t offset = pointer - (uint8_t*)&heap;                
    size_t page_index = offset >> PAGE_SHIFT;
    if(page_table[page_index].used_segments == 0){
        return;
    }
    if(page_table[page_index].type == PAGE_SMALL){
        size_t offset_in_page = pointer - page_data_addr(page_index);
        if(offset_in_page % ALIGNED_BLOCK_SMALL != 0){
            return;
        }

        size_t bit_index = offset_in_page / ALIGNED_BLOCK_SMALL;
        if (bit_index >= SMALL_SEGMENTS) return;
        uint8_t* bitmap = page_header_addr(page_index);
        if(!bitmap_is_set(bitmap, bit_index)){
            return;
        }
        bitmap_clear(bitmap, bit_index);
        page_table[page_index].used_segments--;
        if(page_table[page_index].used_segments == 0){
            page_set_free(page_index);
        }
    }else if (page_table[page_index].type == PAGE_BIG) {
        size_t offset_in_page = pointer - page_data_addr(page_index);
        if(offset_in_page % ALIGNED_BLOCK_BIG != 0){
            return;
        }
        size_t bit_index = offset_in_page / ALIGNED_BLOCK_BIG;
        if (bit_index >= BIG_SEGMENTS) return;
        uint8_t* bitmap = page_header_addr(page_index);
        if(!bitmap_is_set(bitmap, bit_index)){
            return;
        }
        bitmap_clear(bitmap, bit_index);
        page_table[page_index].used_segments--;
        if(page_table[page_index].used_segments == 0){
            page_set_free(page_index);
        }
    }
}

#endif // ALLOC_H