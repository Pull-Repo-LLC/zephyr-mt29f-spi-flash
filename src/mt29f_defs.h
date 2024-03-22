
#pragma once

#define COMMAND_RESET                       0xFF //addres byte = 0 ,dummy bytes = 0 ,data bytes = 0          **RESET THE DEVICE
                                                                                                             
#define COMMAND_GET_FEATURE                 0x0F //addres byte = 1 ,dummy bytes = 0 ,data bytes = 1          **GET FEATURE
#define COMMAND_SET_FEATURE                 0x1F //addres byte = 1 ,dummy bytes = 0 ,data bytes = 1          **Set features
                                                                                                             
#define COMMAND_READ_ID                     0x9F //addres byte = 0 ,dummy bytes = 1 ,data bytes = 2          **Read device ID
                                                                                                             
#define COMMAND_PAGE_READ                   0x13 //addres byte = 3 ,dummy bytes = 0 ,data bytes = 0          **Array read
#define COMMAND_READ_PAGE_CACHE_RANDOM      0x30 //addres byte = 3 ,dummy bytes = 0 ,data bytes = 0          **Cache read
#define COMMAND_READ_PAGE_CACHE_LAST        0x3F //addres byte = 0 ,dummy bytes = 0 ,data bytes = 0          **Ending of cache read
                                            
#define COMMAND_READ_FROM_CACHE_x1          0x03 //addres byte = 2 ,dummy bytes = 1 ,data bytes = 1 to 4352  **Output cache data on column address
#define COMMAND_READ_FROM_CACHE_x2          0x3B //addres byte = 2 ,dummy bytes = 1 ,data bytes = 1 to 4352  **Output cache data on IO[1:0]
#define COMMAND_READ_FROM_CACHE_x4          0x6B //addres byte = 2 ,dummy bytes = 1 ,data bytes = 1 to 4352  **Output cache data on IO[3:0]
                                            
#define COMMAND_READ_FROM_CACHE_Dual_IO     0xBB //addres byte = 2 ,dummy bytes = 1 ,data bytes = 1 to 4352  **Input address/Output cache data on IO[1:0]
#define COMMAND_READ_FROM_CACHE_Quad_IO     0xEB //addres byte = 2 ,dummy bytes = 2 ,data bytes = 1 to 4352  **Input address/Output cache data on IO[3:0]
                                                                                                             
#define COMMAND_WRITE_ENABLE                0x06 //addres byte = 0 ,dummy bytes = 0 ,data bytes = 0          **Sets the WEL bit in the status register to 1; required to enable operations that change the content of the m mory array
#define COMMAND_WRITE_DISABLE               0x04 //addres byte = 0 ,dummy bytes = 0 ,data bytes = 0          **Clears the WEL bit in the status register to 0; required to disable operations that change the contet of the memory array
                                            
#define COMMAND_BLOCK_ERASE                 0xD8 //addres byte = 3 ,dummy bytes = 0 ,data bytes = 0          **Block erase
#define COMMAND_PROGRAM_EXECUTE             0x10 //addres byte = 3 ,dummy bytes = 0 ,data bytes = 0          **Array program
                                            
#define COMMAND_PROGRAM_LOAD_x1             0x02 //addres byte = 2 ,dummy bytes = 0 ,data bytes = 1 to 4352  **Load program data into cache register on SI
#define COMMAND_PROGRAM_LOAD_x2             0xA2 //addres byte = 2 ,dummy bytes = 0 ,data bytes = 1 to 4352  **Load program data into cache register on SI[1:0]
#define COMMAND_PROGRAM_LOAD_x4             0x32 //addres byte = 2 ,dummy bytes = 0 ,data bytes = 1 to 4352  **Load program data into cache register on SI[3:0]
                                            
#define COMMAND_PROGRAM_LOAD_RANDOM_DATA_x1 0x84 //addres byte = 2 ,dummy bytes = 0 ,data bytes = 1 to 4352  **Overwrite cache register with input data on SI
#define COMMAND_PROGRAM_LOAD_RANDOM_DATA_x2 0x44 //addres byte = 2 ,dummy bytes = 0 ,data bytes = 1 to 4352  **Overwrite cache register with input data on SI[1:0]
#define COMMAND_PROGRAM_LOAD_RANDOM_DATA_x4 0x34 //addres byte = 2 ,dummy bytes = 0 ,data bytes = 1 to 4352  **Overwrite cache register with input data on SI[3:0]
                                            
#define COMMAND_PROTECT                     0x2C //addres byte = 3 ,dummy bytes = 0 ,data bytes = 0          **Permanently protect a specific group of blocks

#define BLOCK_POS  6UL
#define PAGE_POS   0UL

#define FEATURE_RX_LEN      0x03
#define FEATURE_DATA_INDEX  0x02
                                                   
#define REG_BLOCK_LOCK     0xA0                    
#define REG_CONFIGURATION  0xB0                    
#define REG_STATUS         0xC0                    
#define REG_DIE_SELECT     0xD0

#define STATUS_BIT_OIP_MASK              0x01
#define STATUS_BIT_WEL_MASK              0x02
#define STATUS_BIT_ERASE_FAIL_MASK       0x04
#define STATUS_BIT_PROGRAM_FAIL_MASK     0x08
#define STATUS_BIT_ECCS0_MASK            0x10
#define STATUS_BIT_ECCS1_MASK            0x20
#define STATUS_BIT_ECCS2_MASK            0x40
#define STATUS_BIT_CACHE_READ_BUSY_MASK  0x80

#define ECC_STATUS_OK     0b000
#define ECC_STATUS_1_3_NR 0b001
#define ECC_STATUS_4_6_R  0b011
#define ECC_STATUS_7_8_R  0b101
#define ECC_STATUS_NOT_OK 0b010

#define SEC_STATUS_BIT_QE       0x01
#define SEC_STATUS_BIT_CONT     0x04
#define SEC_STATUS_BIT_ECC_EN   0x10
#define SEC_STATUS_BIT_OTP_EN   0x40
#define SEC_STATUS_BIT_OTP_PROT 0x80

#define PROT_STATUS_BIT_SP      0x01
#define PROT_STATUS_BIT_COMPLE  0x02
#define PROT_STATUS_BIT_INVERT  0x04
#define PROT_STATUS_BIT_BP0     0x08
#define PROT_STATUS_BIT_BP1     0x10
#define PROT_STATUS_BIT_BP2     0x20
#define PROT_STATUS_BIT_BPRWD   0x80

#define PROT_STATUS_BP_MASK     0x38

#define DIE_0 0x00
#define DIE_1 0x40

#define MANUFACTURER_ID  0x2C
#define DEVICE_ID        0x46

#define BLOCK_OFFSET  0x40000
#define PAGE_OFFSET   0x1000
#define BLOCK_MASK    0x3FFFF
#define PAGE_MASK     0xFFF
