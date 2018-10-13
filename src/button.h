/**
 ******************************************************************************
 * @file    src/button.h
 * @brief   Header for button.c module
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _lock_button_h
#define _lock_button_h

/* Includes ------------------------------------------------------------------*/
#include <Bounce2.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

#define PIN_BUTTON GPIO_NUM_26
#define PIN_BUTTON_SEL GPIO_SEL_26

/* Exported functions ------------------------------------------------------- */

void button_task(void *ignore);

/* End of header guard ------------------------------------------------------ */
#endif // _lock_button_h
