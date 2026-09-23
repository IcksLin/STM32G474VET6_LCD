/**
 * @file    delay.h
 * @brief   基于 Cortex-M4 DWT 周期计数器的阻塞延时接口。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the Cortex-M4 DWT cycle counter used by the delay routines. */
void Delay_Init(void);

/* Blocking delays. Accuracy follows the current SystemCoreClock value. */
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);

/* Lower-case aliases for compatibility with common embedded code. */
#define delay_init Delay_Init
#define delay_us   Delay_us
#define delay_ms   Delay_ms

#ifdef __cplusplus
}
#endif

#endif /* DELAY_H */
