#include "menu_core.h"

#include <stddef.h>

#include "lcd_user.h"
#include "menu_user.h"
#include "menu_view.h"
#include "stm32g4xx_hal.h"

/** @brief 指向 menu_user.c 提供的用户菜单配置表。 */
static MenuItem *menu_items;
/** @brief 用户菜单配置表包含的菜单项数量。 */
static size_t menu_items_count;
/** @brief 当前被选中或正在运行的菜单项。 */
static MenuItem *current_item;
/** @brief true 表示按键动作应转交给叶子功能，而非菜单导航。 */
static bool function_active;
/** @brief true 表示帧缓存已有新内容，等待下一个周期提交。 */
static bool refresh_requested;
/** @brief 两次 LCD_Update 之间允许的最小间隔，单位 ms。 */
static uint32_t refresh_period_ms;
/** @brief 上一次检查并推进刷新周期时记录的 HAL 毫秒时基。 */
static uint32_t last_refresh_tick;
/**
 * @brief 按菜单编号查找菜单项。
 * @param id 目标菜单项编号。
 * @return 找到时返回菜单项指针，否则返回 NULL。
 */
static MenuItem *menu_find_by_id(int16_t id)
{
  for (size_t i = 0; i < menu_items_count; ++i) {
    if (menu_items[i].id == id) {
      return &menu_items[i];
    }
  }
  return NULL;
}

/**
 * @brief 查找指定父菜单的第一个直接子项。
 * @param parent_id 父菜单编号。
 * @return 找到时返回第一个子项指针，否则返回 NULL。
 */
static MenuItem *menu_find_first_child(int16_t parent_id)
{
  for (size_t i = 0; i < menu_items_count; ++i) {
    if (menu_items[i].parent_id == parent_id) {
      return &menu_items[i];
    }
  }
  return NULL;
}

/**
 * @brief 查找当前菜单项的前一个同级菜单项。
 * @param item 当前菜单项。
 * @return 前一个同级项；已经位于首项时保持当前项。
 */
static MenuItem *menu_find_previous_sibling(const MenuItem *item)
{
  /* previous 保存目标项之前最近遇到的同级项。 */
  MenuItem *previous = NULL;
  /* last 保存同级菜单末项，用于从首项向上循环。 */
  MenuItem *last = NULL;
  /* true 表示目标正是首个同级项，需要在遍历结束后返回末项。 */
  bool item_is_first = false;

  for (size_t i = 0; i < menu_items_count; ++i) {
    if (menu_items[i].parent_id != item->parent_id) {
      continue;
    }
    last = &menu_items[i];
    if (&menu_items[i] == item) {
      if (previous != NULL) {
        return previous;
      }
      item_is_first = true;
      continue;
    }
    previous = &menu_items[i];
  }
  if (item_is_first && (last != NULL)) {
    return last;
  }
  return current_item;
}

/**
 * @brief 查找当前菜单项的后一个同级菜单项。
 * @param item 当前菜单项。
 * @return 后一个同级项；已经位于末项时保持当前项。
 */
static MenuItem *menu_find_next_sibling(const MenuItem *item)
{
  /* first 保存同级菜单首项，用于从末项向下循环。 */
  MenuItem *first = NULL;
  /* 当前项被遇到后，下一个同父项即为普通导航目标。 */
  bool current_found = false;

  for (size_t i = 0; i < menu_items_count; ++i) {
    if (menu_items[i].parent_id != item->parent_id) {
      continue;
    }
    if (first == NULL) {
      first = &menu_items[i];
    }
    if (current_found) {
      return &menu_items[i];
    }
    if (&menu_items[i] == item) {
      current_found = true;
    }
  }
  if (current_found && (first != NULL)) {
    return first;
  }
  return current_item;
}

/**
 * @brief 根据 HAL 毫秒时基执行菜单画面的限频提交。
 * @return 无。
 * @note 只有刷新周期到达且 refresh_requested 已置位时才调用 LCD_Update；
 * 本函数不依赖 LCD 脏区实现。
 */
static void menu_service_refresh(void)
{
  /* HAL_GetTick 的无符号减法能够正确跨越 uint32_t 回绕点。 */
  uint32_t now = HAL_GetTick();

  if ((uint32_t)(now - last_refresh_tick) < refresh_period_ms) {
    return;
  }

  last_refresh_tick = now;
  if (refresh_requested) {
    refresh_requested = false;
    (void)LCD_Update();
  }
}

/**
 * @brief 初始化菜单核心与刷新周期。
 * @param period_ms 屏幕提交的最小周期，单位 ms。
 * @return 无。
 */
void Menu_Init(uint32_t period_ms)
{
  MenuUser_Init();
  menu_items = MenuUser_GetItems(&menu_items_count);
  current_item = menu_items;
  function_active = false;
  refresh_period_ms = period_ms == 0U ? 1U : period_ms;
  refresh_requested = false;
  MenuKey_Init();
  Menu_Draw();
  (void)LCD_Update();
  refresh_requested = false;
  last_refresh_tick = HAL_GetTick();
}

/**
 * @brief 处理一次按键事件并维护菜单和刷新状态。
 * @return 无。
 */
void Menu_Process(void)
{
  /* 每次主循环最多消费一个由 TIM17 扫描完成的按键事件。 */
  MenuAction action = MenuKey_GetAction();

  if (action != MENU_ACTION_WAITING) {
    if (function_active) {
      if (current_item->handler != NULL) {
        current_item->handler(action);
      }
      if (!function_active) {
        Menu_Draw();
      }
    } else {
      /* 默认保持当前项，仅在导航确实成功时重新绘制菜单。 */
      MenuItem *next_item = current_item;

      switch (action) {
        case MENU_ACTION_UP:
        case MENU_ACTION_UP_LONG:
          next_item = menu_find_previous_sibling(current_item);
          break;
        case MENU_ACTION_DOWN:
        case MENU_ACTION_DOWN_LONG:
          next_item = menu_find_next_sibling(current_item);
          break;
        case MENU_ACTION_OK:
        case MENU_ACTION_OK_LONG: {
          /* 有子项时进入子菜单，否则进入当前叶子功能。 */
          MenuItem *child = menu_find_first_child(current_item->id);
          if (child != NULL) {
            next_item = child;
          } else if (current_item->handler != NULL) {
            function_active = true;
            current_item->handler(action);
          }
          break;
        }
        case MENU_ACTION_BACK:
        case MENU_ACTION_BACK_LONG: {
          /* 根菜单的 parent_id 为 -1，因此找不到父项时保持不动。 */
          MenuItem *parent = menu_find_by_id(current_item->parent_id);
          if (parent != NULL) {
            next_item = parent;
          }
          break;
        }
        default:
          break;
      }

      if (next_item != current_item) {
        current_item = next_item;
        Menu_Draw();
      }
    }
  }

  menu_service_refresh();
}

/**
 * @brief 调用视图层绘制当前菜单并请求周期刷新。
 * @return 无。
 */
void Menu_Draw(void)
{
  MenuView_System_Navigation_Common(menu_items, menu_items_count,
                                    current_item);
  Menu_RequestRefresh();
}

/**
 * @brief 设置菜单刷新请求标志。
 * @return 无。
 */
void Menu_RequestRefresh(void)
{
  refresh_requested = true;
}

/**
 * @brief 清除叶子功能交互状态。
 * @return 无。
 */
void Menu_ExitFunction(void)
{
  function_active = false;
}

/**
 * @brief 设置指定菜单项的功能处理回调。
 * @param id 菜单项编号。
 * @param handler 新的按键处理函数，可为 NULL。
 * @return true 设置成功；false 菜单编号不存在。
 */
bool Menu_SetHandler(int16_t id, MenuHandler handler)
{
  /* 运行时只替换回调，不改变静态层级和菜单名称。 */
  MenuItem *item = menu_find_by_id(id);
  if (item == NULL) {
    return false;
  }
  item->handler = handler;
  return true;
}

/**
 * @brief 返回当前选中的菜单项。
 * @return 当前菜单项只读指针。
 */
const MenuItem *Menu_GetCurrentItem(void)
{
  return current_item;
}
