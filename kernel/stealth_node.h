#ifndef _STEALTH_NODE_H
#define _STEALTH_NODE_H

#include <linux/device.h>
#include <linux/miscdevice.h>

/* 目标节点名称 */
#define TARGET_NODE "virtpipe-common-syzs"

/* 使用 MISC 主设备号，避免自定义 Major 号触发驱动列表检测 */
#define TARGET_MINOR 250

#endif /* _STEALTH_NODE_H */
