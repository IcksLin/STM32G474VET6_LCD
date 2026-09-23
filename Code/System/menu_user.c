/**
 * @file    menu_user.c
 * @brief   用户菜单实现：扁平菜单表、步进电机与按键测试等页面业务和按键回调。
 * @note    用户自建代码，非 CubeMX 生成。
 */

#include "menu_user.h"

#include <stdbool.h>
#include <stdint.h>

#include "lcd_user.h"
#include "menu_view.h"
#include "zdt_x57.h"

static void menu_user_key_remap_test(MenuAction action);
static void menu_user_placeholder(MenuAction action);
static void menu_user_stepper_mode(MenuAction action);
static void menu_user_stepper_speed(MenuAction action);
static void menu_user_stepper_angle(MenuAction action);
static void menu_user_stepper_apply(MenuAction action);
static void menu_user_draw_stepper(const char *title);

/** @brief 用户维护的扁平菜单表；父子关系只由 id 和 parent_id 表达。 */
static MenuItem user_menu_items[] = {
    {0, -1, "Main Menu", NULL},
    {1, 0, "mode1", NULL},
    {2, 0, "stepper_test", NULL},
    {3, 0, "mode3", NULL},
    {4, 0, "mode4", NULL},
    {5, 0, "mode5", NULL},
    {6, 0, "mode6", NULL},
    {7, 0, "mode7", NULL},
    {8, 1, "key_remap_test", menu_user_key_remap_test},
    {9, 1, "imu_angle_display", menu_user_placeholder},
    {10, 2, "switch_mode", menu_user_stepper_mode},
    {11, 2, "set_speed", menu_user_stepper_speed},
    {12, 2, "set_angle", menu_user_stepper_angle},
    {13, 2, "run_motor", menu_user_stepper_apply},
};

MenuItem *MenuUser_GetItems(size_t *item_count)
{
  if (item_count != NULL) {
    *item_count = sizeof(user_menu_items) / sizeof(user_menu_items[0]);
  }
  return user_menu_items;
}

#define STEPPER_ADDRESS          1U     /**< 步进驱动器从站地址。 */
#define STEPPER_RAMP_RPM_S       1000U  /**< 速度命令使用的加减速斜率。 */
#define STEPPER_HOME_MODE        0U     /**< 回零模式（单圈就近）。 */
#define STEPPER_SPEED_STEP_RPM   50     /**< 每次按键调整的速度步长。 */
#define STEPPER_ANGLE_STEP_DEG   90     /**< 每次按键调整的角度步长。 */
#define STEPPER_SPEED_LIMIT_RPM  4000   /**< 允许设置的最大速度。 */
#define STEPPER_ANGLE_LIMIT_DEG  36000  /**< 允许设置的最大角度。 */

/** @brief 步进菜单支持的两种运动控制方式。 */
typedef enum {
  STEPPER_MODE_VELOCITY = 0, /**< 有符号速度连续旋转。 */
  STEPPER_MODE_POSITION      /**< 以限速运动到相对目标角度。 */
} StepperMode;

/** @brief 步进电机功能组共用的参数和最近一次命令状态。 */
typedef struct {
  StepperMode mode;      /**< 当前选择的速度或位置控制模式。 */
  int32_t speed_rpm;     /**< 速度目标，或位置模式最大转速。 */
  int32_t angle_degree;  /**< 位置模式的有符号相对目标角度。 */
  const char *status;    /**< 最近一次操作或发送结果。 */
} StepperMenuState;

/**
 * @brief 仅由下方步进电机菜单函数组使用的持久状态。
 * @note 紧邻对应函数组定义，以明确变量与业务功能的绑定关系。
 */
static StepperMenuState stepper = {
    STEPPER_MODE_VELOCITY, 300, 360, "CAN not bound"};

void MenuUser_Init(void)
{
  stepper.mode = STEPPER_MODE_VELOCITY;
  stepper.speed_rpm = 300;
  stepper.angle_degree = 360;
  stepper.status = ZDT_X57_IsCanReady() ? "CAN ready" : "CAN not bound";
}

bool MenuUser_HandleGlobalAction(const MenuItem *item, MenuAction action)
{
  bool stop_ok;
  bool home_ok;

  if ((item == NULL) || (action != MENU_ACTION_BACK_LONG) ||
      ((item->id != 2) && (item->parent_id != 2))) {
    return false;
  }

  /*
   * 长按返回先停止当前轨迹，再触发驱动器已配置的单圈就近回零。
   * 两条命令分别发送，保证停止失败时仍会尝试回零。
   */
  stop_ok = ZDT_X57_Stop(STEPPER_ADDRESS);
  home_ok = ZDT_X57_Home(STEPPER_ADDRESS, STEPPER_HOME_MODE);
  stepper.status = (stop_ok && home_ok) ? "STOP + HOME sent"
                                       : "CAN send failed";
  menu_user_draw_stepper("Emergency home");
  return true;
}

/**
 * @brief 绘制步进电机参数和通信状态并请求限频刷新。
 * @param title 当前功能页标题。
 * @return 无。
 */
static void menu_user_draw_stepper(const char *title)
{
  MenuView_User_StepperControl_Common(
      title,
      stepper.mode == STEPPER_MODE_VELOCITY ? "Velocity" : "Position",
      stepper.speed_rpm, stepper.angle_degree, stepper.status);
  Menu_RequestRefresh();
}

/**
 * @brief 切换速度/位置模式。
 * @param action 菜单核心分发的按键动作。
 * @return 无。
 */
static void menu_user_stepper_mode(MenuAction action)
{
  if ((action == MENU_ACTION_BACK) || (action == MENU_ACTION_BACK_LONG)) {
    Menu_ExitFunction();
    return;
  }
  if ((action == MENU_ACTION_OK) || (action == MENU_ACTION_OK_LONG)) {
    stepper.mode = stepper.mode == STEPPER_MODE_VELOCITY
                       ? STEPPER_MODE_POSITION
                       : STEPPER_MODE_VELOCITY;
    stepper.status = "Mode switched";
  }
  menu_user_draw_stepper("Switch mode");
}

/**
 * @brief 设置速度；确认键在速度模式下立即发送速度命令。
 * @param action 菜单核心分发的按键动作。
 * @return 无。
 */
static void menu_user_stepper_speed(MenuAction action)
{
  if ((action == MENU_ACTION_BACK) || (action == MENU_ACTION_BACK_LONG)) {
    Menu_ExitFunction();
    return;
  }
  if ((action == MENU_ACTION_UP) || (action == MENU_ACTION_UP_LONG)) {
    stepper.speed_rpm += STEPPER_SPEED_STEP_RPM;
  } else if ((action == MENU_ACTION_DOWN) ||
             (action == MENU_ACTION_DOWN_LONG)) {
    stepper.speed_rpm -= STEPPER_SPEED_STEP_RPM;
  } else if ((action == MENU_ACTION_OK) ||
             (action == MENU_ACTION_OK_LONG)) {
    if (stepper.mode == STEPPER_MODE_VELOCITY) {
      stepper.status =
          ZDT_X57_RunVelocity(STEPPER_ADDRESS, (float)stepper.speed_rpm,
                              STEPPER_RAMP_RPM_S)
              ? "Velocity sent"
              : "CAN send failed";
    } else {
      stepper.status = "Speed limit saved";
    }
  }
  if (stepper.speed_rpm > STEPPER_SPEED_LIMIT_RPM) {
    stepper.speed_rpm = STEPPER_SPEED_LIMIT_RPM;
  } else if (stepper.speed_rpm < -STEPPER_SPEED_LIMIT_RPM) {
    stepper.speed_rpm = -STEPPER_SPEED_LIMIT_RPM;
  }
  menu_user_draw_stepper("Set speed");
}

/**
 * @brief 设置有符号相对目标角度；确认键在位置模式下发送位置命令。
 * @param action 菜单核心分发的按键动作。
 * @return 无。
 */
static void menu_user_stepper_angle(MenuAction action)
{
  if ((action == MENU_ACTION_BACK) || (action == MENU_ACTION_BACK_LONG)) {
    Menu_ExitFunction();
    return;
  }
  if ((action == MENU_ACTION_UP) || (action == MENU_ACTION_UP_LONG)) {
    stepper.angle_degree += STEPPER_ANGLE_STEP_DEG;
  } else if ((action == MENU_ACTION_DOWN) ||
             (action == MENU_ACTION_DOWN_LONG)) {
    stepper.angle_degree -= STEPPER_ANGLE_STEP_DEG;
  } else if ((action == MENU_ACTION_OK) ||
             (action == MENU_ACTION_OK_LONG)) {
    if (stepper.mode == STEPPER_MODE_POSITION) {
      float max_speed = (float)(stepper.speed_rpm < 0
                                    ? -stepper.speed_rpm
                                    : stepper.speed_rpm);
      stepper.status =
          ZDT_X57_RunPosition(STEPPER_ADDRESS, (float)stepper.angle_degree,
                              max_speed, false)
              ? "Position sent"
              : "CAN send failed";
    } else {
      stepper.status = "Switch to Position";
    }
  }
  if (stepper.angle_degree > STEPPER_ANGLE_LIMIT_DEG) {
    stepper.angle_degree = STEPPER_ANGLE_LIMIT_DEG;
  } else if (stepper.angle_degree < -STEPPER_ANGLE_LIMIT_DEG) {
    stepper.angle_degree = -STEPPER_ANGLE_LIMIT_DEG;
  }
  menu_user_draw_stepper("Set angle");
}

/**
 * @brief 按当前模式发送速度或位置命令。
 * @param action 菜单核心分发的按键动作。
 * @return 无。
 */
static void menu_user_stepper_apply(MenuAction action)
{
  bool sent = false;

  if ((action == MENU_ACTION_BACK) || (action == MENU_ACTION_BACK_LONG)) {
    Menu_ExitFunction();
    return;
  }
  if ((action == MENU_ACTION_OK) || (action == MENU_ACTION_OK_LONG)) {
    if (stepper.mode == STEPPER_MODE_VELOCITY) {
      sent = ZDT_X57_RunVelocity(STEPPER_ADDRESS, (float)stepper.speed_rpm,
                                 STEPPER_RAMP_RPM_S);
    } else {
      float max_speed = (float)(stepper.speed_rpm < 0
                                    ? -stepper.speed_rpm
                                    : stepper.speed_rpm);
      sent = ZDT_X57_RunPosition(STEPPER_ADDRESS,
                                 (float)stepper.angle_degree, max_speed, false);
    }
    stepper.status = sent ? "Motion command sent" : "CAN send failed";
  }
  menu_user_draw_stepper("Run motor");
}

/** @brief 仅由用户页面模板使用的演示状态。 */
static int32_t template_value;

void MenuUser_PageTemplate(MenuAction action)
{
  bool changed = false;

  switch (action) {
    case MENU_ACTION_UP:
    case MENU_ACTION_UP_LONG:
      ++template_value;
      changed = true;
      break;
    case MENU_ACTION_DOWN:
    case MENU_ACTION_DOWN_LONG:
      --template_value;
      changed = true;
      break;
    case MENU_ACTION_OK:
    case MENU_ACTION_OK_LONG:
      template_value = 0;
      changed = true;
      break;
    case MENU_ACTION_BACK:
    case MENU_ACTION_BACK_LONG:
      Menu_ExitFunction();
      return;
    default:
      break;
  }
  if (changed) {
    MenuView_User_FunctionPage_Common("User Template", "UP/DOWN: change",
                                      "OK: reset");
    LCD_Printf(16, 130, "Value: %ld", (long)template_value);
    Menu_RequestRefresh();
  }
}

/** @brief 仅由下方按键映射测试函数使用的可增减测试值。 */
static int32_t key_test_value;

static void menu_user_key_remap_test(MenuAction action)
{
  switch (action) {
    case MENU_ACTION_UP:
    case MENU_ACTION_UP_LONG:
      ++key_test_value;
      break;
    case MENU_ACTION_DOWN:
    case MENU_ACTION_DOWN_LONG:
      --key_test_value;
      break;
    case MENU_ACTION_OK:
    case MENU_ACTION_OK_LONG:
      key_test_value = 0;
      break;
    case MENU_ACTION_BACK:
    case MENU_ACTION_BACK_LONG:
      Menu_ExitFunction();
      return;
    default:
      break;
  }
  MenuView_User_KeyRemapTest_8(key_test_value);
  Menu_RequestRefresh();
}

static void menu_user_placeholder(MenuAction action)
{
  if ((action == MENU_ACTION_BACK) || (action == MENU_ACTION_BACK_LONG)) {
    Menu_ExitFunction();
    return;
  }
  MenuView_User_FunctionPage_Common(Menu_GetCurrentItem()->name,
                                    "Callback not installed",
                                    "Edit menu_user.c");
  Menu_RequestRefresh();
}
