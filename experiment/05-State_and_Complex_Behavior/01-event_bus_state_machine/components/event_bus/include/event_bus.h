#pragma once 

#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SYS_EVENT_PAYLOAD_MAX 64

//事件总线句柄
typedef struct event_bus_t *sys_event_bus_handle_t;

//总事件定义
typedef enum {
    //系统级事件定义
    EVT_SYS_LOW_BATTERY = 0x00,
    EVT_SYS_LOW_ALARM = 0x01,
    //输入设备事件定义
    //指纹事件定义
    EVT_FINGER_TOUCH = 0x10,
    EVT_FINGER_SUCCESS = 0x11,
    EVT_FINGER_FAIL = 0x12,
    EVT_FINGER_CMD_ENROLL = 0x13,
    //人脸事件定义
    EVT_FACE_DETECT = 0x20,
    EVT_FACE_SUCCESS = 0x21,
    EVT_FACE_FAIL = 0x22,
    EVT_FACE_CMD_ENROLL = 0x23,
    //密码事件定义
    EVT_PWD_KEY_PRESSED = 0x30,
    EVT_PWD_ENTER_SUCCESS = 0x31,
    EVT_PWD_ENTER_FAIL = 0x32,
    EVT_PWD_CMD_ENROLL = 0x33,
    //执行设备事件定义
    //电机事件y定
    EVT_MOTOR_OPEN = 0x40,
    EVT_MOTOR_LOCK = 0x41,
    //UI事件定义
    EVT_UI_SHOW_HOME = 0x50,
    EVT_UI_SHOW_NORMAL = 0x51,
    //事件边界
    EVT_MAX,
} sys_event_type_t;

//事件优先级
typedef enum {
    EVT_PRIO_CRITICAL = 0,      //防撬锁
    EVT_PRIO_HIGH,              //开锁
    EVT_PRIO_NORMAL,            //状态更新
    EVT_PRIO_LOW,               //日志
} sys_event_prio_t;

//事件总线结构体
typedef struct {
    sys_event_type_t type;      //事件类型
    sys_event_prio_t prio;      //事件优先级
    uint32_t timestamp_ms;      //时间戳
    uint8_t payload[SYS_EVENT_PAYLOAD_MAX];
    uint8_t payload_len;
} sys_event_t;

//事件处理回调
typedef void (*event_handler_t)(const sys_event_t *ev, void *ctx);


/**
 * @brief 创建事件总线实例
 * @return 成功返回指针，失败返回 NULL
 */
sys_event_bus_handle_t event_bus_create(void); 

/**
 * @brief 销毁事件总线（释放队列）
 */
void event_bus_delete(sys_event_bus_handle_t bus_handle);

/**
 * @brief 发布事件（线程安全）
 * @param bus 事件总线句柄
 * @param type 事件类型
 * @param data 可选数据指针（可为 NULL）
 * @param len 数据长度（<=64）
 * @return true: 成功入队；false: 队列满或参数错误
 */
bool event_bus_publish(sys_event_bus_handle_t bus_handle, sys_event_type_t type, const void *data, size_t len);

/**
 * @brief 订阅特定事件类型
 * @param bus 事件总线句柄
 * @param type 要监听的事件类型
 * @param handler 事件回调函数
 * @param ctx 用户上下文（传给 handler 的第二个参数）
 */
void event_bus_subscribe(sys_event_bus_handle_t bus_handle, sys_event_type_t type, event_handler_t handler, void *ctx);

/**
 * @brief 处理所有待处理事件（主循环中调用）
 * @note 从高优先级到低优先级依次处理
 */
void event_bus_process(sys_event_bus_handle_t bus_handle);



#ifdef __cplusplus
}
#endif