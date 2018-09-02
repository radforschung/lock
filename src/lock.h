/**
 ******************************************************************************
 * @file    src/lock.h
 * @brief   Header for lock.c module
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _lock_lock_h
#define _lock_lock_h

/* Includes ------------------------------------------------------------------*/
#include <Bounce2.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

#define PIN_LOCK_MOTOR GPIO_NUM_13
// rotation switch: (PULLUP!) LOW: default, HIGH: motor presses against
// (rotation complete)
#define PIN_LOCK_ROTATION_SWITCH GPIO_NUM_2

/* Exported functions ------------------------------------------------------- */
class Lock {
public:
  Lock();
  void open();
  bool isOpen();
  bool motorIsParked();

private:
  Bounce debounceRotationSwitch;
  Bounce debounceLatchSwitch;
};

/* End of header guard ------------------------------------------------------ */
#endif // _lock_lock_h
