#include "event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

#define QUEUE_SIZE_PER_PRIO 16  // 每个优先级队列深度
#define MAX_SUBSCRIBERS_PER_EVENT 8



typedef struct event_bus_t {
    // 4 个优先级队列（FreeRTOS Queue）
    QueueHandle_t queues[4];

    // 订阅者表：每个事件类型最多 8 个订阅者
    struct subscriber {
        event_handler_t handler;
        void* ctx;
    } subscribers[EVT_MAX][MAX_SUBSCRIBERS_PER_EVENT];

    uint8_t sub_count[EVT_MAX]; // 每个事件类型的订阅者数量
}event_bus_t;

//事件优先级映射表
static const sys_event_prio_t s_event_prio_map[EVT_MAX] = {
    //最高优先级，处理系统事件
    [EVT_SYS_LOW_BATTERY]   = EVT_PRIO_CRITICAL,
    [EVT_SYS_LOW_ALARM]     = EVT_PRIO_CRITICAL,
    //高优先级
    [EVT_FINGER_SUCCESS]    = EVT_PRIO_HIGH,
    [EVT_FACE_SUCCESS]      = EVT_PRIO_HIGH,
    [EVT_PWD_ENTER_SUCCESS] = EVT_PRIO_HIGH,
    [EVT_MOTOR_OPEN]        = EVT_PRIO_HIGH,
    [EVT_MOTOR_LOCK]        = EVT_PRIO_HIGH,
    //正常优先级
    [EVT_FINGER_TOUCH]      = EVT_PRIO_NORMAL,
    [EVT_FACE_DETECT]       = EVT_PRIO_NORMAL,
    [EVT_PWD_KEY_PRESSED]   = EVT_PRIO_NORMAL,
    [EVT_UI_SHOW_NORMAL]    = EVT_PRIO_NORMAL,
    [EVT_PWD_CMD_ENROLL]    = EVT_PRIO_NORMAL,
    [EVT_FACE_CMD_ENROLL]   = EVT_PRIO_NORMAL,
    [EVT_FINGER_CMD_ENROLL] = EVT_PRIO_NORMAL,
    [EVT_FINGER_FAIL]       = EVT_PRIO_NORMAL,
    [EVT_FACE_FAIL]         = EVT_PRIO_NORMAL,
    [EVT_PWD_ENTER_FAIL]    = EVT_PRIO_NORMAL,
    //低优先级
    [EVT_UI_SHOW_HOME]      = EVT_PRIO_LOW,
    
};

// ------------------- 辅助函数 -------------------

static inline void fill_event(sys_event_t* ev, sys_event_type_t type, const void* data, size_t len) {
    ev->type = type;
    ev->prio = s_event_prio_map[type]; // 默认优先级，publish 时可覆盖
    ev->timestamp_ms = esp_timer_get_time() / 1000;
    ev->payload_len = (len > SYS_EVENT_PAYLOAD_MAX) ? SYS_EVENT_PAYLOAD_MAX : len;
    if (data && ev->payload_len > 0) {
        memcpy(ev->payload, data, ev->payload_len);
    } else {
        ev->payload_len = 0;
    }
}

// ------------------- 公共 API 实现 -------------------

sys_event_bus_handle_t event_bus_create(void)
{
    sys_event_bus_handle_t bus = heap_caps_calloc(1, sizeof(event_bus_t), MALLOC_CAP_INTERNAL);
    if (!bus) {
        ESP_LOGE("event_bus", "Failed to alloc bus");
        return NULL;
    }
 
    //创建4个优先级队列
    for (int i = 0; i < 4; i++) {
        bus->queues[i] = xQueueCreate(QUEUE_SIZE_PER_PRIO, sizeof(sys_event_t));
        if (!bus->queues[i]) {
            ESP_LOGE("event_bus", "Failed to create queue %d", i);
            event_bus_delete(bus);
            return NULL;
        }
    }

    // 初始化订阅者表
    memset(bus->subscribers, 0, sizeof(bus->subscribers));
    memset(bus->sub_count, 0, sizeof(bus->sub_count));

    ESP_LOGI("event_bus", "Created with %d queues", 4);
    return bus;
}

void event_bus_delete(event_bus_t* bus_handle) {
    if (!bus_handle) return;
    for (int i = 0; i < 4; i++) {
        if (bus_handle->queues[i]) {
            vQueueDelete(bus_handle->queues[i]);
        }
    }
    free(bus_handle);
}

bool event_bus_publish(sys_event_bus_handle_t bus_handle, sys_event_type_t type, const void *data, size_t len)
{
    if (!bus_handle || type >= EVT_MAX || len > SYS_EVENT_PAYLOAD_MAX) {
        return false;
    }
    //填充发布事件
    sys_event_t ev;
    fill_event(&ev, type, data, len);

    // 发布到对应优先级队列
    BaseType_t ret = xQueueSendToBack(bus_handle->queues[ev.prio], &ev, 0);     //不阻塞等待发向队列尾部
    if (ret != pdTRUE) {
        ESP_LOGW("event_bus", "Queue %d full for event %d", ev.prio, type);
    }
    return (ret == pdTRUE);
}

void event_bus_subscribe(sys_event_bus_handle_t bus_handle, sys_event_type_t type, event_handler_t handler, void *ctx)
{
    if (!bus_handle || !handler || type >= EVT_MAX) return;

    // 防重复订阅
    for (int i = 0; i < bus_handle->sub_count[type]; i++) {
        if (bus_handle->subscribers[type][i].handler == handler && 
            bus_handle->subscribers[type][i].ctx == ctx) {
                return;
            }
    }
    //添加总线订阅者
    if (bus_handle->sub_count[type] < MAX_SUBSCRIBERS_PER_EVENT) {
        bus_handle->subscribers[type][bus_handle->sub_count[type]] = (struct subscriber){
            .handler = handler,
            .ctx = ctx
        };
        bus_handle->sub_count[type]++;
        ESP_LOGD("event_bus", "Subscribed to event %d (total: %d)", type, bus_handle->sub_count[type]);
    } else {
        ESP_LOGE("event_bus", "Max subscribers reached for event %d", type);
    }
}

void event_bus_process(sys_event_bus_handle_t bus_handle)
{
    if (!bus_handle) return;

    // 从高优先级到低优先级处理
    for (int prio = 0; prio < 4; prio++) {
        sys_event_t ev;
        while (xQueueReceive(bus_handle->queues[prio], &ev, 0) == pdTRUE) {
            // 分发给所有订阅者
            for (int i = 0; i < bus_handle->sub_count[ev.type]; i++) {
                bus_handle->subscribers[ev.type][i].handler(&ev, bus_handle->subscribers[ev.type][i].ctx);
            }
        }
    }
}