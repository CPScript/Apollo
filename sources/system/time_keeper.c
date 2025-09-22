#include "time_keeper.h"
#include <stdint.h>

#define CMOS_ADDRESS_PORT 0x70
#define CMOS_DATA_PORT 0x71

#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09
#define RTC_CENTURY 0x32
#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static uint8_t read_rtc_register(uint8_t reg) {
    outb(CMOS_ADDRESS_PORT, reg);
    return inb(CMOS_DATA_PORT);
}

static int bcd_to_binary(uint8_t bcd_value) {
    return ((bcd_value >> 4) * 10) + (bcd_value & 0x0F);
}

static bool rtc_update_in_progress(void) {
    return (read_rtc_register(RTC_STATUS_A) & 0x80) != 0;
}

void time_keeper_get_datetime(int* year, int* month, int* day, int* hour, int* minute, int* second) {
    uint8_t century;
    uint8_t last_second, last_minute, last_hour, last_day, last_month, last_year, last_century;
    
    while (rtc_update_in_progress()) {
        __asm__ volatile("pause");
    }
    
    *second = read_rtc_register(RTC_SECONDS);
    *minute = read_rtc_register(RTC_MINUTES);
    *hour = read_rtc_register(RTC_HOURS);
    *day = read_rtc_register(RTC_DAY);
    *month = read_rtc_register(RTC_MONTH);
    *year = read_rtc_register(RTC_YEAR);
    century = read_rtc_register(RTC_CENTURY);
    
    do {
        last_second = *second;
        last_minute = *minute;
        last_hour = *hour;
        last_day = *day;
        last_month = *month;
        last_year = *year;
        last_century = century;
        
        while (rtc_update_in_progress()) {
            __asm__ volatile("pause");
        }
        
        *second = read_rtc_register(RTC_SECONDS);
        *minute = read_rtc_register(RTC_MINUTES);
        *hour = read_rtc_register(RTC_HOURS);
        *day = read_rtc_register(RTC_DAY);
        *month = read_rtc_register(RTC_MONTH);
        *year = read_rtc_register(RTC_YEAR);
        century = read_rtc_register(RTC_CENTURY);
        
    } while ((last_second != *second) || (last_minute != *minute) || 
             (last_hour != *hour) || (last_day != *day) || 
             (last_month != *month) || (last_year != *year) || 
             (last_century != century));
    
    uint8_t status_b = read_rtc_register(RTC_STATUS_B);
    
    if (!(status_b & 0x04)) {
        *second = bcd_to_binary(*second);
        *minute = bcd_to_binary(*minute);
        *hour = bcd_to_binary(*hour);
        *day = bcd_to_binary(*day);
        *month = bcd_to_binary(*month);
        *year = bcd_to_binary(*year);
        century = bcd_to_binary(century);
    }
    
    if (!(status_b & 0x02) && (*hour & 0x80)) {
        *hour = ((*hour & 0x7F) + 12) % 24;
    }
    
    if (century != 0) {
        *year += century * 100;
    } else {
        *year += (*year >= 70) ? 1900 : 2000;
    }
}

void time_keeper_initialize(void) {
    uint8_t status_b = read_rtc_register(RTC_STATUS_B);
    status_b |= 0x02; // 24-hour format
    status_b |= 0x04; // Binary format
    
    outb(CMOS_ADDRESS_PORT, RTC_STATUS_B);
    outb(CMOS_DATA_PORT, status_b);
    
    read_rtc_register(0x0C);
}

uint64_t time_keeper_get_uptime_seconds(void) {
    // In the feuture, this would track time since boot
    static uint64_t boot_time = 0;
    static bool initialized = false;
    
    if (!initialized) {
        int year, month, day, hour, minute, second;
        time_keeper_get_datetime(&year, &month, &day, &hour, &minute, &second);
        
        boot_time = (uint64_t)year * 365 * 24 * 3600 + 
                   (uint64_t)month * 30 * 24 * 3600 + 
                   (uint64_t)day * 24 * 3600 + 
                   (uint64_t)hour * 3600 + 
                   (uint64_t)minute * 60 + 
                   (uint64_t)second;
        initialized = true;
        return 0;
    }
    
    int year, month, day, hour, minute, second;
    time_keeper_get_datetime(&year, &month, &day, &hour, &minute, &second);
    
    uint64_t current_time = (uint64_t)year * 365 * 24 * 3600 + 
                           (uint64_t)month * 30 * 24 * 3600 + 
                           (uint64_t)day * 24 * 3600 + 
                           (uint64_t)hour * 3600 + 
                           (uint64_t)minute * 60 + 
                           (uint64_t)second;
    
    return current_time - boot_time;
}