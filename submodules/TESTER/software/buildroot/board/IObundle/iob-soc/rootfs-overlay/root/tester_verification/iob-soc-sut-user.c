#include "iob-soc-sut-user.h"

int iob_soc_sut_set_reg(uint32_t num, uint32_t value){
    int ret = 0;
    switch (num) {
        case 1:
            ret = iob_sysfs_write_file(IOB_SOC_SUT_SYSFILE_REG1, value);
            break;
        case 2:
            ret = iob_sysfs_write_file(IOB_SOC_SUT_SYSFILE_REG2, value);
            break;
        default:
            perror("[Tester|User] Invalid write register number");
            ret = -1;
            break;
    }
    if (ret == -1) {
        perror("[Tester|User] Failed to write register");
    }
    return ret;
}

int iob_soc_sut_get_reg(uint32_t num, uint32_t *value) {
    int ret = 0;
    switch (num) {
        case 3:
            ret = iob_sysfs_read_file(IOB_SOC_SUT_SYSFILE_REG3, value);
            break;
        case 4:
            ret = iob_sysfs_read_file(IOB_SOC_SUT_SYSFILE_REG4, value);
            break;
        case 5:
            ret = iob_sysfs_read_file(IOB_SOC_SUT_SYSFILE_REG4, value);
            break;
        default:
            perror("[Tester|User] Invalid read register number");
            ret = -1;
            break;
    }
    if (ret == -1) {
        perror("[Tester|User] Failed to read register");
    }
    return ret;
}
