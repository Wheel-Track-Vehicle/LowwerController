#ifndef __MOTOR_CTRL_H
#define __MOTOR_CTRL_H

#include "stm32f4xx_hal.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define MOTOR_UART   &huart1  
#define CMD_BUFFER_SIZE 64
#define HALL_PER_REV      390           // 30*13
#define WHEEL_DIAMETER_MM 80.0f
#define PI                3.14f
#define PERIOD_COUNT      50.0f         // 1秒内的20ms周期数
#define WHEEL_ANGLE_FRONT_RIGHT    (PI / 4)     // 45°
#define WHEEL_ANGLE_FRONT_LEFT     (3 * PI / 4) // 135°
#define WHEEL_ANGLE_REAR_LEFT      (5 * PI / 4) // 225° 
#define WHEEL_ANGLE_REAR_RIGHT     (7 * PI / 4) // 315°
// 运动控制参数
#define MAX_PWM_VALUE      2500    // 最大PWM值
#define MIN_PWM_VALUE      0500    // 最小PWM值
#define DEFAULT_MOVE_TIME  1000    // 默认运动时间(ms)

typedef enum {
    WHEEL_FRONT_RIGHT = 3,  // 右前轮 - ID 1
    WHEEL_FRONT_LEFT  = 1,  // 左前轮 - ID 2  
    WHEEL_REAR_LEFT   = 2,  // 左后轮 - ID 3
    WHEEL_REAR_RIGHT  = 4   // 右后轮 - ID 4
} WheelPosition;

static uint8_t rx_char;
static char rx_buffer[64];
static uint8_t rx_index = 0;



// 电机控制函数
void Motor_ParseDebug(const char *msg);
void Motor_OpenLoop(uint8_t id, uint16_t pwm, uint16_t time);
void Motor_CloseLoop(uint8_t id, uint16_t pwm, uint16_t time);
void Motor_SetID(uint8_t old_id, uint8_t new_id);
void Motor_ReadID(uint8_t id);
void Motor_ReadVersion(uint8_t id);
void Motor_SetBaudrate(uint8_t id, uint8_t baud_index);
void Motor_EnableDebug(uint8_t id);
void Motor_SetSpeed(uint8_t id, float speed_mm_s, uint16_t time_ms);
void Motor_SpeedInit(uint8_t id);
void Process_Upper_Command(char *buf);


// 调试信息解析函数
void Motor_ParseDebugMessage(const char *msg);


// 基本运动控制
void OmniWheel_Move(int16_t vx, int16_t vy, int16_t wz, uint16_t time);
void OmniWheel_MoveForward(int16_t speed, uint16_t time);
void OmniWheel_MoveBackward(int16_t speed, uint16_t time);  
void OmniWheel_MoveRight(int16_t speed, uint16_t time);
void OmniWheel_MoveLeft(int16_t speed, uint16_t time);
void OmniWheel_RotateCW(int16_t speed, uint16_t time);
void OmniWheel_RotateCCW(int16_t speed, uint16_t time);
void OmniWheel_MoveAtAngle(float angle, int16_t speed, uint16_t time);

// 停止控制
void OmniWheel_Stop(uint16_t time);
void OmniWheel_EmergencyStop(void);

#endif
