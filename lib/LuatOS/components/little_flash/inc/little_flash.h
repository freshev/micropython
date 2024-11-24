#ifndef _LITTLE_FLASH_H_
#define _LITTLE_FLASH_H_

#include "little_flash_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/*All nand flash uses BUF1 mode driver
    There is no unified standard and command for nand flash. When transplanting, you should pay attention to the status register and command register.*/


lf_err_t little_flash_init(void);

lf_err_t little_flash_deinit(void);

lf_err_t little_flash_device_init(little_flash_t *lf);

lf_err_t little_flash_device_deinit(little_flash_t *lf);

lf_err_t little_flash_erase(const little_flash_t *lf, uint32_t addr, uint32_t len);

lf_err_t little_flash_chip_erase(const little_flash_t *lf);

lf_err_t little_flash_write(const little_flash_t *lf, uint32_t addr, const uint8_t *data, uint32_t len);

lf_err_t little_flash_read(const little_flash_t *lf, uint32_t addr, uint8_t *data, uint32_t len);

lf_err_t little_flash_write_status(const little_flash_t *lf, uint8_t address, uint8_t status);

lf_err_t little_flash_read_status(const little_flash_t *lf, uint8_t address, uint8_t *status);


#ifdef __cplusplus
}
#endif

#endif /* _LITTLE_FLASH_H_ */










