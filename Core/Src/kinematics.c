#include "kinematics.h"
#include "motor_ctrl.h"
#include <math.h>

// 全局变量
static odom_t odometry;           // 里程计数据
static wheel_speed_t wheel_data;   // 轮速数据

/**
 * @brief  初始化运动学模块
 */
void Kinematics_Init(void)
{
    // 初始化里程计数据
    odometry.x = 0.0f;
    odometry.y = 0.0f;
    odometry.angle = 0.0f;
    odometry.linear_vel = 0.0f;
    odometry.angular_vel = 0.0f;
    
    // 初始化轮速数据
    for (uint8_t i = 0; i < 4; i++)
    {
        wheel_data.wheel_speeds[i] = 0.0f;
        wheel_data.last_encoder[i] = 0.0f;
        wheel_data.encoder_counts[i] = 0.0f;
    }
}

/**
 * @brief  更新轮速
 * @param  wheel_id: 轮子编号
 * @param  encoder_count: 编码器计数
 */
void Kinematics_UpdateWheelSpeed(uint8_t wheel_id, float encoder_count)
{
    if (wheel_id < 4)
    {
        // 计算编码器变化量
        float delta_encoder = encoder_count - wheel_data.last_encoder[wheel_id];
        
        // 保存当前编码器值
        wheel_data.last_encoder[wheel_id] = encoder_count;
        wheel_data.encoder_counts[wheel_id] = encoder_count;
        
        // 计算轮速（弧度/秒）
        // 假设每次调用的时间间隔为20ms（50Hz）
        float dt = 0.02f;
        float wheel_rad = (delta_encoder / ENCODER_RESOLUTION) * 2.0f * PI;
        wheel_data.wheel_speeds[wheel_id] = wheel_rad / dt;
    }
}

/**
 * @brief  正运动学计算
 * @param  vx: X方向速度（输出）
 * @param  vy: Y方向速度（输出）
 * @param  wz: 角速度（输出）
 */
void Kinematics_Forward(float *vx, float *vy, float *wz)
{
    // 麦轮运动学模型（假设轮子方向与运动轴平行）
    // vx = (w1 + w2 + w3 + w4) * r / 4
    // vy = (-w1 + w2 + w3 - w4) * r / 4
    // wz = (-w1 + w2 - w3 + w4) * r / (4 * (l + w))
    
    float r = WHEEL_RADIUS;
    float l = WHEEL_BASE_LENGTH / 2.0f;
    float w = WHEEL_BASE_WIDTH / 2.0f;
    
    *vx = (wheel_data.wheel_speeds[KINEMATICS_WHEEL_FRONT_RIGHT] + 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_FRONT_LEFT] + 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_REAR_LEFT] + 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_REAR_RIGHT]) * r / 4.0f;
    
    *vy = (-wheel_data.wheel_speeds[KINEMATICS_WHEEL_FRONT_RIGHT] + 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_FRONT_LEFT] + 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_REAR_LEFT] - 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_REAR_RIGHT]) * r / 4.0f;
    
    *wz = (-wheel_data.wheel_speeds[KINEMATICS_WHEEL_FRONT_RIGHT] + 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_FRONT_LEFT] - 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_REAR_LEFT] + 
           wheel_data.wheel_speeds[KINEMATICS_WHEEL_REAR_RIGHT]) * r / (4.0f * (l + w));
    
    // 更新里程计速度
    odometry.linear_vel = sqrtf(*vx * *vx + *vy * *vy);
    odometry.angular_vel = *wz;
}

/**
 * @brief  逆运动学计算
 * @param  vx: X方向速度
 * @param  vy: Y方向速度
 * @param  wz: 角速度
 * @param  wheel_speeds: 轮速（输出）
 */
void Kinematics_Inverse(float vx, float vy, float wz, float *wheel_speeds)
{
    // 麦轮运动学模型（假设轮子方向与运动轴平行）
    // w1 = (vx - vy - (l + w) * wz) / r
    // w2 = (vx + vy + (l + w) * wz) / r
    // w3 = (vx + vy - (l + w) * wz) / r
    // w4 = (vx - vy + (l + w) * wz) / r
    
    float r = WHEEL_RADIUS;
    float l = WHEEL_BASE_LENGTH / 2.0f;
    float w = WHEEL_BASE_WIDTH / 2.0f;
    
    wheel_speeds[KINEMATICS_WHEEL_FRONT_RIGHT] = (vx - vy - (l + w) * wz) / r;
    wheel_speeds[KINEMATICS_WHEEL_FRONT_LEFT] = (vx + vy + (l + w) * wz) / r;
    wheel_speeds[KINEMATICS_WHEEL_REAR_LEFT] = (vx + vy - (l + w) * wz) / r;
    wheel_speeds[KINEMATICS_WHEEL_REAR_RIGHT] = (vx - vy + (l + w) * wz) / r;
}

/**
 * @brief  更新里程计
 * @param  dt: 时间间隔（秒）
 */
void Kinematics_UpdateOdometry(float dt)
{
    float vx, vy, wz;
    
    // 计算机器人速度
    Kinematics_Forward(&vx, &vy, &wz);
    
    // 更新机器人位置（通过积分）
    odometry.x += (vx * cosf(odometry.angle) - vy * sinf(odometry.angle)) * dt;
    odometry.y += (vx * sinf(odometry.angle) + vy * cosf(odometry.angle)) * dt;
    odometry.angle += wz * dt;
    
    // 限制角度在0-2π范围内
    if (odometry.angle > 2.0f * PI)
        odometry.angle -= 2.0f * PI;
    else if (odometry.angle < 0.0f)
        odometry.angle += 2.0f * PI;
}

/**
 * @brief  获取里程计数据
 * @retval 里程计数据
 */
odom_t Kinematics_GetOdometry(void)
{
    return odometry;
}

// 应用层函数

/**
 * @brief  初始化应用层运动学
 */
void Kinematics_App_Init(void)
{
    // 初始化运动学模块
    Kinematics_Init();
    
    // 初始化电机控制
    // 这里可以添加电机初始化代码
}

/**
 * @brief  从电机数据更新
 * @param  wheel_id: 轮子编号
 * @param  pulse: 电机脉冲数
 */
void Kinematics_App_UpdateFromMotor(uint8_t wheel_id, int pulse)
{
    // 更新轮速
    Kinematics_UpdateWheelSpeed(wheel_id, (float)pulse);
}

/**
 * @brief  应用层更新里程计
 * @param  dt: 时间间隔（秒）
 */
void Kinematics_App_UpdateOdometry(float dt)
{
    // 更新里程计
    Kinematics_UpdateOdometry(dt);
    
    // 打印里程计数据（可选）
    // printf("Odometry: x=%.2f, y=%.2f, angle=%.2f, v=%.2f, w=%.2f\r\n", 
    //        odometry.x, odometry.y, odometry.angle, 
    //        odometry.linear_vel, odometry.angular_vel);
}

/**
 * @brief  应用速度控制
 * @param  vx: X方向速度
 * @param  vy: Y方向速度
 * @param  wz: 角速度
 */
void Kinematics_App_ApplyVelocity(float vx, float vy, float wz)
{
    // 将速度转换为OmniWheel_Move函数期望的范围（-1000到1000）
    int16_t vx_scaled = (int16_t)(vx * 1000.0f);
    int16_t vy_scaled = (int16_t)(vy * 1000.0f);
    int16_t wz_scaled = (int16_t)(wz * 1000.0f);
    
    // 限制速度范围
    if (vx_scaled > 1000) vx_scaled = 1000;
    if (vx_scaled < -1000) vx_scaled = -1000;
    if (vy_scaled > 1000) vy_scaled = 1000;
    if (vy_scaled < -1000) vy_scaled = -1000;
    if (wz_scaled > 1000) wz_scaled = 1000;
    if (wz_scaled < -1000) wz_scaled = -1000;
    
    // 应用速度控制
    OmniWheel_Move(vx_scaled, vy_scaled, wz_scaled, 0);
}

/**
 * @brief  执行位置控制
 */
void Kinematics_App_ExecutePositionControl(void)
{
    // 这里可以添加位置控制代码
    // 例如：PID控制、路径规划等
}