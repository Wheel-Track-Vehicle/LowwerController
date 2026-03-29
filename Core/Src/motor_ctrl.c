#include "motor_ctrl.h"
// 最新测速值
float motor_rps = 0;       // 每秒旋转圈数
float motor_speed = 0;     // mm/s



extern UART_HandleTypeDef huart1;

/**
 * @brief 发送一个字节
 */

void hhSerialSendByte(uint8_t Byte){
    HAL_UART_Transmit(&huart1,&Byte,1,HAL_MAX_DELAY);
}

/**
 * @brief  发送一条字符串指令到一个电机
 */
static void Motor_SendCommand(const char *cmd)
{
    for (uint16_t i=0;cmd[i]!='\0';i++){
        hhSerialSendByte(cmd[i]);
    }

}

/**
 * @brief  格式化为固定宽度字符串（补0）
 */
static void format_number(char *buf, uint16_t num, uint8_t width)
{
    sprintf(buf, "%0*u", width, num);
}

/**
 * @brief  开环控制
 * @param  id: 电机ID (0-254)
 * @param  pwm: PWM值 (0500-2500)
 * @param  time: 时间(0000-9999)，0000为持续
 */
void Motor_OpenLoop(uint8_t id, uint16_t pwm, uint16_t time)
{
    char cmd[CMD_BUFFER_SIZE];
    char id_str[4], pwm_str[5], time_str[5];

    format_number(id_str, id, 3);
    format_number(pwm_str, pwm, 4);
    format_number(time_str, time, 4);

    sprintf(cmd, "#%sP%sT%s!", id_str, pwm_str, time_str);
    Motor_SendCommand(cmd);
}

/**
 * @brief  单电机闭环控制
 * @param  pwm: PWM值 (0500-2500)
 * @param  time: 时间(0000-9999)，0000为持续
 */
void Motor_CloseLoop(uint8_t id, uint16_t pwm, uint16_t time)
{
    char cmd[CMD_BUFFER_SIZE];
    char id_str[4], pwm_str[5], time_str[5];

    format_number(id_str, id, 3);
    format_number(pwm_str, pwm, 4);
    format_number(time_str, time, 4);

    sprintf(cmd, "#%sP%sB%s!", id_str, pwm_str, time_str);
    Motor_SendCommand(cmd);
}


/**
 * @brief  速度控制闭环模式（单位 mm/s）
 * @param  id   电机ID (0~254)
 * @param  speed_mm_s 目标线速度 (mm/s)，正反方向均可，反方向输入负数
 * @param  time_ms 运行时间 (ms)，0 表示持续
 */
void Motor_SetSpeed(uint8_t id, float speed_mm_s, uint16_t time_ms)
{
    // --- 1. 根据速度反推脉冲（一个 20ms 周期内的脉冲数）
    //     np = speed / 32.24
    float np = speed_mm_s / 32.24f;

    // --- 2. PWM = 1500 + np
    int pwm = (int)(1500 + np);

    // 限幅：1475～1525
    if (pwm > 1525) pwm = 1525;
    if (pwm < 1475) pwm = 1475;

    // --- 3. 构建 Bxxxx 时间（单位 s），协议为四位数字
    int time_s = time_ms / 1000;  // 协议要求以秒为单位
    if (time_ms == 0)
        time_s = 0;  // 持续模式

    // --- 4. 格式化指令
    char cmd[32];
    char id_str[4], pwm_str[5], time_str[5];

    format_number(id_str, id, 3);
    format_number(pwm_str, pwm, 4);
    format_number(time_str, time_s, 4);

    sprintf(cmd, "#%sP%sB%s!", id_str, pwm_str, time_str);

    // --- 5. 发送指令
    Motor_SendCommand(cmd);
}



/**
 * @brief  修改电机ID
 * @param old_id 旧id(若为未初始化id，则需取255)
 * @param new_id 新id
 */
void Motor_SetID(uint8_t old_id, uint8_t new_id)
{
    char cmd[CMD_BUFFER_SIZE];
    char old_str[4], new_str[4];
    format_number(old_str, old_id, 3);
    format_number(new_str, new_id, 3);
    sprintf(cmd, "#%sPID%s!", old_str, new_str);
    Motor_SendCommand(cmd);
}

/**
 * @brief  读取电机ID
 */
void Motor_ReadID(uint8_t id)
{
    char cmd[CMD_BUFFER_SIZE];
    char id_str[4];
    format_number(id_str, id, 3);
    sprintf(cmd, "#%sPID!", id_str);
    Motor_SendCommand(cmd);
}

/**
 * @brief  读取版本号
 */
void Motor_ReadVersion(uint8_t id)
{
    char cmd[CMD_BUFFER_SIZE];
    char id_str[4];
    format_number(id_str, id, 3);
    sprintf(cmd, "#%sPVER!", id_str);
    Motor_SendCommand(cmd);
}

/**
 * @brief  设置波特率
 * @param  baud_index: 1~8 对应不同速率
 */
void Motor_SetBaudrate(uint8_t id, uint8_t baud_index)
{
    char cmd[CMD_BUFFER_SIZE];
    char id_str[4], baud_str[2];
    format_number(id_str, id, 3);
    sprintf(baud_str, "%d", baud_index);
    sprintf(cmd, "#%sPBD%s!", id_str, baud_str);
    Motor_SendCommand(cmd);
}

/**
 * @brief  解析调试信息
 * 示例输入: "+0840+008"
 */
void Motor_ParseDebug(const char *msg)
{
    int pwm_val = 0;
    int pulse   = 0;

    if (sscanf(msg, "+%d+%d", &pwm_val, &pulse) == 2)
    {
        // 计算 RPS
        float rev_per_period = (float)pulse / (float)HALL_PER_REV;
        motor_rps = rev_per_period * PERIOD_COUNT;

        // 计算线速度 mm/s
        motor_speed = motor_rps * PI * WHEEL_DIAMETER_MM;

        printf("[DEBUG] pulse=%d  RPS=%.3f  Speed=%.2f mm/s\r\n",
            pulse, motor_rps, motor_speed);
    }
}

void Motor_EnableDebug(uint8_t id)
{
    char cmd[CMD_BUFFER_SIZE];
    char id_str[4];
    format_number(id_str, id, 3);
    sprintf(cmd, "#%sPDebug!", id_str);
    Motor_SendCommand(cmd);
}


/**
 * @brief  初始化接收并开启调试模式
 * @param id 需要读取速度的电机id
 */
void Motor_SpeedInit(uint8_t id)
{
    Motor_EnableDebug(id);

    HAL_Delay(20);

    // 开启串口接收中断
    HAL_UART_Receive_IT(&huart1, &rx_char, 1);

    printf("Motor Speed Measure Init OK.\r\n");
}

/**
 * @brief  接收来自上位机的指令控制电机转速
 * @param buf 上位机指令流
 */
void Process_Upper_Command(char *buf)
{
    uint8_t id;
    float speed;

    if (sscanf(buf, "SPEED %hhu %f", &id, &speed) == 2)
    {
        Motor_SetSpeed(id, speed, 0);
    }
}

/**
 * @brief  计算单个麦轮的速度分量
 * @param  vx: X方向速度分量 (-1000 ~ +1000)
 * @param  vy: Y方向速度分量 (-1000 ~ +1000)  
 * @param  wz: 旋转速度分量 (-1000 ~ +1000)
 * @param  wheel_angle: 轮子安装角度(弧度)
 * @retval 轮子PWM值 (0500-2500)
 */
static int16_t CalculateWheelSpeed(int16_t vx, int16_t vy, int16_t wz, float wheel_angle)
{
    // 麦轮运动学模型：V_wheel = Vx*cosθ + Vy*sinθ + Wz*R
    // 这里简化处理，R设为1
    float speed = vx * cosf(wheel_angle) + vy * sinf(wheel_angle) + wz;
    
    // 归一化到PWM范围
    int16_t pwm = (int16_t)((speed / 1000.0f) * (MAX_PWM_VALUE - MIN_PWM_VALUE) / 2 + 
                           (MAX_PWM_VALUE + MIN_PWM_VALUE) / 2);
    
    // 限制在有效范围内
    if (pwm > MAX_PWM_VALUE) pwm = MAX_PWM_VALUE;
    if (pwm < MIN_PWM_VALUE) pwm = MIN_PWM_VALUE;
    
    return pwm;
}

/**
 * @brief  四轮协同运动控制
 * @param  vx: X方向速度 (-1000 ~ +1000)
 * @param  vy: Y方向速度 (-1000 ~ +1000)
 * @param  wz: 旋转速度 (-1000 ~ +1000)
 * @param  time: 运动时间(ms)，0表示持续运动
 */
void OmniWheel_Move(int16_t vx, int16_t vy, int16_t wz, uint16_t time)
{
    // 计算四个轮子的速度
    int16_t wheel_speeds[4];
    
    wheel_speeds[0] = CalculateWheelSpeed(vx, vy, wz, WHEEL_ANGLE_FRONT_RIGHT); // 右前
    wheel_speeds[1] = CalculateWheelSpeed(vx, vy, wz, WHEEL_ANGLE_FRONT_LEFT);  // 左前  
    wheel_speeds[2] = CalculateWheelSpeed(vx, vy, wz, WHEEL_ANGLE_REAR_LEFT);   // 左后
    wheel_speeds[3] = CalculateWheelSpeed(vx, vy, wz, WHEEL_ANGLE_REAR_RIGHT);  // 右后
    
    // 发送控制命令到四个电机
    Motor_OpenLoop(WHEEL_FRONT_RIGHT, wheel_speeds[0], time);
    Motor_OpenLoop(WHEEL_FRONT_LEFT,  wheel_speeds[1], time);
    Motor_OpenLoop(WHEEL_REAR_LEFT,   wheel_speeds[2], time);
    Motor_OpenLoop(WHEEL_REAR_RIGHT,  wheel_speeds[3], time);
}

/**
 * @brief  前进
 * @param  speed: 速度值 (0-1000)
 * @param  time: 运动时间(ms)
 */
void OmniWheel_MoveForward(int16_t speed, uint16_t time)
{
    OmniWheel_Move(0, speed, 0, time);
}

/**
 * @brief  后退
 * @param  speed: 速度值 (0-1000)  
 * @param  time: 运动时间(ms)
 */
void OmniWheel_MoveBackward(int16_t speed, uint16_t time)
{
    OmniWheel_Move(0, -speed, 0, time);
}

/**
 * @brief  向右平移
 * @param  speed: 速度值 (0-1000)
 * @param  time: 运动时间(ms)
 */
void OmniWheel_MoveRight(int16_t speed, uint16_t time)
{
    OmniWheel_Move(speed, 0, 0, time);
}

/**
 * @brief  向左平移  
 * @param  speed: 速度值 (0-1000)
 * @param  time: 运动时间(ms)
 */
void OmniWheel_MoveLeft(int16_t speed, uint16_t time)
{
    OmniWheel_Move(-speed, 0, 0, time);
}

/**
 * @brief  顺时针旋转
 * @param  speed: 速度值 (0-1000)
 * @param  time: 运动时间(ms)
 */
void OmniWheel_RotateCW(int16_t speed, uint16_t time)
{
    OmniWheel_Move(0, 0, speed, time);
}

/**
 * @brief  逆时针旋转
 * @param  speed: 速度值 (0-1000)  
 * @param  time: 运动时间(ms)
 */
void OmniWheel_RotateCCW(int16_t speed, uint16_t time)
{
    OmniWheel_Move(0, 0, -speed, time);
}

/**
 * @brief  斜向移动（组合方向）
 * @param  angle: 移动角度 (0-360度，0度为正前方)
 * @param  speed: 速度值 (0-1000)
 * @param  time: 运动时间(ms)
 */
void OmniWheel_MoveAtAngle(float angle, int16_t speed, uint16_t time)
{
    float rad = angle * PI / 180.0f;
    int16_t vx = (int16_t)(speed * sinf(rad));
    int16_t vy = (int16_t)(speed * cosf(rad));
    
    OmniWheel_Move(vx, vy, 0, time);
}

/**
 * @brief  停止所有电机
 * @param  time: 停止时间(ms)，0表示立即停止
 */
void OmniWheel_Stop(uint16_t time)
{
    // 发送停止命令到所有电机（PWM=1500为中位）
    Motor_OpenLoop(WHEEL_FRONT_RIGHT, 1500, time);
    Motor_OpenLoop(WHEEL_FRONT_LEFT,  1500, time);  
    Motor_OpenLoop(WHEEL_REAR_LEFT,   1500, time);
    Motor_OpenLoop(WHEEL_REAR_RIGHT,  1500, time);
}

/**
 * @brief  急停（立即停止）
 */
void OmniWheel_EmergencyStop(void)
{
    OmniWheel_Stop(0);
}