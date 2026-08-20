#include <stdint.h>
#include <stdbool.h>

#define MII_BMCR            0x00
#define MII_BMSR            0x01
#define MII_ANAR            0x04
#define MII_ANLPAR          0x05

#define BMCR_RESET          (1 << 15)
#define BMCR_ANENABLE       (1 << 12)
#define BMCR_ANRESTART      (1 << 9)

#define ANAR_PAUSE_ASYM     (1 << 11)
#define ANAR_PAUSE_SYM      (1 << 10)
#define ANAR_100BASE_TX_FD  (1 << 8)
#define ANAR_100BASE_TX_HD  (1 << 7)
#define ANAR_10BASE_T_FD    (1 << 6)
#define ANAR_10BASE_T_HD    (1 << 5)

#define MEMORY_PAGE_LIMIT   4096
#define CACHE_CRITICAL_MISS 85

volatile uint32_t* nic_mmio_base_address;

static void mdio_write_register(uint8_t phy_addr, uint8_t reg_offset, uint16_t value) {
    uint32_t mdio_command = 0;
    mdio_command |= (1 << 31);
    mdio_command |= (1 << 26);
    mdio_command |= (phy_addr & 0x1F) << 21;
    mdio_command |= (reg_offset & 0x1F) << 16;
    mdio_command |= (value & 0xFFFF);

    *(nic_mmio_base_address + 2) = mdio_command;

    while (*(nic_mmio_base_address + 2) & (1 << 31)) {
        __asm__ volatile("nop");
    }
}

static uint16_t mdio_read_register(uint8_t phy_addr, uint8_t reg_offset) {
    uint32_t mdio_command = 0;
    mdio_command |= (1 << 31);
    mdio_command |= (2 << 26);
    mdio_command |= (phy_addr & 0x1F) << 21;
    mdio_command |= (reg_offset & 0x1F) << 16;

    *(nic_mmio_base_address + 2) = mdio_command;

    while (*(nic_mmio_base_address + 2) & (1 << 31)) {
        __asm__ volatile("nop");
    }

    return (uint16_t)(*(nic_mmio_base_address + 3) & 0xFFFF);
}

void execute_hapg_governor_cycle(uint32_t current_rob_load, uint32_t cache_miss_rate, uint8_t target_phy_address) {
    if (current_rob_load >= MEMORY_PAGE_LIMIT || cache_miss_rate > CACHE_CRITICAL_MISS) {
        uint16_t advertisement_bits = mdio_read_register(target_phy_address, MII_ANAR);

        advertisement_bits |= ANAR_PAUSE_SYM | ANAR_PAUSE_ASYM;
        advertisement_bits &= ~(ANAR_100BASE_TX_FD | ANAR_100BASE_TX_HD);
        advertisement_bits |= ANAR_10BASE_T_FD;

        mdio_write_register(target_phy_address, MII_ANAR, advertisement_bits);

        uint16_t control_bits = mdio_read_register(target_phy_address, MII_BMCR);
        control_bits |= BMCR_ANRESTART;
        mdio_write_register(target_phy_address, MII_BMCR, control_bits);
    }
}
