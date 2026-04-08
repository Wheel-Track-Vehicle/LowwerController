#ifndef __KINEMATICS_H
#define __KINEMATICS_H
#include "stm32f4xx_hal.h"

// 机器人参数
#define WHEEL_RADIUS       0.04f   // 轮子半径（米），假设轮子直径80mm
#define WHEEL_BASE_WIDTH   0.2f    // 轮距宽度（米）
#define WHEEL_BASE_LENGTH  0.2f    // 轮距长度（米）
#define ENCODER_RESOLUTION 390.0f  // 编码器分辨率（每圈脉冲数）

// 轮子编号
typedef enum {
    KINEMATICS_WHEEL_FRONT_RIGHT = 0,
    KINEMATICS_WHEEL_FRONT_LEFT = 1,
    KINEMATICS_WHEEL_REAR_LEFT = 2,
    KINEMATICS_WHEEL_REAR_RIGHT = 3
} KinematicsWheelId;

// 运动学结构体
typedef struct {
    float x;             /** 机器人在x轴上的位置 */
    float y;             /** 机器人在y轴上的位置 */
    float angle;          /** 机器人当前角度 */
    float linear_vel;     /** 机器人当前线速度 */
    float angular_vel;    /** 机器人当前角速度 */
} odom_t;

// 轮速结构体
typedef struct {
    float wheel_speeds[4]; /** 四个轮子的速度（弧度/秒） */
    float last_encoder[4];  /** 上次编码器值 */
    float encoder_counts[4]; /** 当前编码器值 */
} wheel_speed_t;

// 运动学函数声明
void Kinematics_Init(void);
void Kinematics_UpdateWheelSpeed(uint8_t wheel_id, float encoder_count);
void Kinematics_Forward(float *vx, float *vy, float *wz);
void Kinematics_Inverse(float vx, float vy, float wz, float *wheel_speeds);
void Kinematics_UpdateOdometry(float dt);
odom_t Kinematics_GetOdometry(void);

// 应用层函数声明
void Kinematics_App_Init(void);
void Kinematics_App_UpdateFromMotor(uint8_t wheel_id, int pulse);
void Kinematics_App_UpdateOdometry(float dt);
void Kinematics_App_ApplyVelocity(float vx, float vy, float wz);
void Kinematics_App_ExecutePositionControl(void);

#endif