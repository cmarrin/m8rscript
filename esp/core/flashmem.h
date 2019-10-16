// Based on NodeMCU platform_flash
// https://github.com/nodemcu/nodemcu-firmware

#ifndef SYSTEM_FLASHMEM_H_
#define SYSTEM_FLASHMEM_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include "spiffs_config.h"
#include <ets_sys.h>
#include <c_types.h>
#include <osapi.h>
#include <stdint.h>
#include <spi_flash.h>

typedef signed long s32_t;
typedef unsigned long u32_t;
typedef signed short s16_t;
typedef unsigned short u16_t;
typedef signed char s8_t;
typedef unsigned char u8_t;

extern u32_t _SPIFFS_start;
extern u32_t _SPIFFS_end;
extern u32_t _SPIFFS_page;
extern u32_t _SPIFFS_block;

#define SPIFFS_OK                       0
#define SPIFFS_ERR_NOT_MOUNTED          -10000
#define SPIFFS_ERR_FULL                 -10001
#define SPIFFS_ERR_NOT_FOUND            -10002
#define SPIFFS_ERR_END_OF_OBJECT        -10003
#define SPIFFS_ERR_DELETED              -10004
#define SPIFFS_ERR_NOT_FINALIZED        -10005
#define SPIFFS_ERR_NOT_INDEX            -10006
#define SPIFFS_ERR_OUT_OF_FILE_DESCS    -10007
#define SPIFFS_ERR_FILE_CLOSED          -10008
#define SPIFFS_ERR_FILE_DELETED         -10009
#define SPIFFS_ERR_BAD_DESCRIPTOR       -10010
#define SPIFFS_ERR_IS_INDEX             -10011
#define SPIFFS_ERR_IS_FREE              -10012
#define SPIFFS_ERR_INDEX_SPAN_MISMATCH  -10013
#define SPIFFS_ERR_DATA_SPAN_MISMATCH   -10014
#define SPIFFS_ERR_INDEX_REF_FREE       -10015
#define SPIFFS_ERR_INDEX_REF_LU         -10016
#define SPIFFS_ERR_INDEX_REF_INVALID    -10017
#define SPIFFS_ERR_INDEX_FREE           -10018
#define SPIFFS_ERR_INDEX_LU             -10019
#define SPIFFS_ERR_INDEX_INVALID        -10020
#define SPIFFS_ERR_NOT_WRITABLE         -10021
#define SPIFFS_ERR_NOT_READABLE         -10022
#define SPIFFS_ERR_CONFLICTING_NAME     -10023
#define SPIFFS_ERR_NOT_CONFIGURED       -10024

#define SPIFFS_ERR_NOT_A_FS             -10025
#define SPIFFS_ERR_MOUNTED              -10026
#define SPIFFS_ERR_ERASE_FAIL           -10027
#define SPIFFS_ERR_MAGIC_NOT_POSSIBLE   -10028

#define SPIFFS_ERR_NO_DELETED_BLOCKS    -10029

#define SPIFFS_ERR_FILE_EXISTS          -10030

#define SPIFFS_ERR_NOT_A_FILE           -10031
#define SPIFFS_ERR_RO_NOT_IMPL          -10032
#define SPIFFS_ERR_RO_ABORTED_OPERATION -10033
#define SPIFFS_ERR_PROBE_TOO_FEW_BLOCKS -10034
#define SPIFFS_ERR_PROBE_NOT_A_FS       -10035
#define SPIFFS_ERR_NAME_TOO_LONG        -10036

#define SPIFFS_ERR_IX_MAP_UNMAPPED      -10037
#define SPIFFS_ERR_IX_MAP_MAPPED        -10038
#define SPIFFS_ERR_IX_MAP_BAD_RANGE     -10039

#define SPIFFS_ERR_INTERNAL             -10050

#define SPIFFS_ERR_TEST                 -10100

#define STORE_TYPEDEF_ATTR  __attribute__((aligned(4),packed))

#define INTERNAL_FLASH_WRITE_UNIT_SIZE  4
#define INTERNAL_FLASH_READ_UNIT_SIZE	4

#define FLASH_TOTAL_SEC_COUNT 	(flashmem_get_size_sectors())

#define SYS_PARAM_SEC_COUNT 4
#define FLASH_WORK_SEC_COUNT (FLASH_TOTAL_SEC_COUNT - SYS_PARAM_SEC_COUNT)

#define INTERNAL_FLASH_SECTOR_SIZE      SPI_FLASH_SEC_SIZE
#define INTERNAL_FLASH_SIZE             ( (FLASH_WORK_SEC_COUNT) * INTERNAL_FLASH_SECTOR_SIZE )
#define INTERNAL_FLASH_START_ADDRESS    0x40200000

typedef struct
{
    uint8_t unknown0;
    uint8_t unknown1;
    enum
    {
        MODE_QIO = 0,
        MODE_QOUT = 1,
        MODE_DIO = 2,
        MODE_DOUT = 15,
    } mode : 8;
    enum
    {
        SPEED_40MHZ = 0,
        SPEED_26MHZ = 1,
        SPEED_20MHZ = 2,
        SPEED_80MHZ = 15,
    } speed : 4;
    enum
    {
        SIZE_4MBIT = 0,
        SIZE_2MBIT = 1,
        SIZE_8MBIT = 2,
        SIZE_16MBIT = 3,
        SIZE_32MBIT = 4,
    } size : 4;
} STORE_TYPEDEF_ATTR SPIFlashInfo;

extern uint32_t flashmem_write( const void *from, uint32_t toaddr, uint32_t size );
extern uint32_t flashmem_read( void *to, uint32_t fromaddr, uint32_t size );
extern bool flashmem_erase_sector( uint32_t sector_id );

extern SPIFlashInfo flashmem_get_info();
extern uint8_t flashmem_get_size_type();
extern uint32_t flashmem_get_size_bytes();
extern uint16_t flashmem_get_size_sectors();
uint32_t flashmem_find_sector( uint32_t address, uint32_t *pstart, uint32_t *pend );
uint32_t flashmem_get_sector_of_address( uint32_t addr );

extern uint32_t flashmem_write_internal( const void *from, uint32_t toaddr, uint32_t size );
extern uint32_t flashmem_read_internal( void *to, uint32_t fromaddr, uint32_t size );
extern uint32_t flashmem_get_first_free_block_address();

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_FLASHMEM_H_ */
