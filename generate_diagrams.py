#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""生成6张系统架构图"""

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
import numpy as np
import os

# ============ 全局设置 ============
OUTPUT_DIR = r'f:\工作文档\2026\大功率可调恒流源\QT上位机\docs\images'
DPI = 300
FIG_WIDTH = 14
FONT_NAME = 'SimHei'

plt.rcParams['font.sans-serif'] = [FONT_NAME, 'Microsoft YaHei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['figure.facecolor'] = 'white'
plt.rcParams['savefig.facecolor'] = 'white'


def draw_rounded_rect(ax, x, y, w, h, color, text, fontsize=9, alpha=0.9, text_color='white', edge_color=None):
    """绘制圆角矩形并居中写文字"""
    box = FancyBboxPatch((x, y), w, h,
                         boxstyle="round,pad=0.02",
                         facecolor=color, edgecolor=edge_color or color,
                         alpha=alpha, linewidth=1.5, zorder=2)
    ax.add_patch(box)
    ax.text(x + w / 2, y + h / 2, text,
            ha='center', va='center', fontsize=fontsize,
            color=text_color, fontweight='bold', zorder=3,
            linespacing=1.4)
    return box


def draw_arrow(ax, x1, y1, x2, y2, color='#555555', style='->', lw=1.5, label='', label_fontsize=7, label_offset=(0, 0.05)):
    """绘制箭头"""
    ax.annotate('', xy=(x2, y2), xytext=(x1, y1),
                arrowprops=dict(arrowstyle=style, color=color, lw=lw, shrinkA=0, shrinkB=0),
                zorder=1)
    if label:
        mx = (x1 + x2) / 2 + label_offset[0]
        my = (y1 + y2) / 2 + label_offset[1]
        ax.text(mx, my, label, ha='center', va='center', fontsize=label_fontsize,
                color=color, fontstyle='italic', zorder=4,
                bbox=dict(boxstyle='round,pad=0.15', facecolor='white', edgecolor='none', alpha=0.8))


# ============================================================
# 图1: 系统分层架构图
# ============================================================
def draw_system_architecture():
    fig, ax = plt.subplots(1, 1, figsize=(FIG_WIDTH, 16))
    ax.set_xlim(-0.5, 10.5)
    ax.set_ylim(-0.5, 9.5)
    ax.axis('off')
    ax.set_title('系统分层架构图', fontsize=20, fontweight='bold', pad=20)

    layers = [
        ('UI层', 'MainWindow\n6个UI面板(QWidget)', '#B3D9FF'),
        ('可视化层', 'RealtimeCurveWidget\nStatusIndicatorWidget', '#99C2FF'),
        ('数据模型层', 'DeviceDataModel\nQAbstractTableModel', '#80AAFF'),
        ('通信层', 'CommunicationWorker\nQSerialPort', '#6699FF'),
        ('协议层', 'ICommand / CommandFactory\nIProtocolParser / SeedSourceProtocolParser', '#4D80FF'),
        ('报警层', 'AlarmManager\nAlarmRule / AlarmRecord', '#3366FF'),
        ('数据库层', 'DatabaseManager\nSQLite (QSqlDatabase)', '#1A4FFF'),
        ('配置层', 'ConfigManager\nQSettings / JSON', '#0033CC'),
    ]

    n = len(layers)
    box_w = 7.5
    box_h = 0.85
    x_start = 1.5
    y_start = 8.5
    gap = 0.15

    # 左侧标注
    ax.text(0.4, y_start + 0.2, '高层\n（业务逻辑）', ha='center', va='center',
            fontsize=11, fontweight='bold', color='#0033CC',
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#E6F0FF', edgecolor='#3366FF', lw=1.5))
    ax.text(0.4, y_start - (n - 1) * (box_h + gap) - 0.2, '底层\n（基础设施）', ha='center', va='center',
            fontsize=11, fontweight='bold', color='#0033CC',
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#E6F0FF', edgecolor='#3366FF', lw=1.5))

    # 左侧大箭头
    ax.annotate('', xy=(0.4, y_start - (n - 1) * (box_h + gap) + 0.2),
                xytext=(0.4, y_start - 0.2),
                arrowprops=dict(arrowstyle='->', color='#3366FF', lw=2.5))

    for i, (name, classes, color) in enumerate(layers):
        y = y_start - i * (box_h + gap)
        text_color = 'white' if i >= 4 else '#1a1a2e'
        draw_rounded_rect(ax, x_start, y, box_w, box_h, color,
                          f'{name}    {classes}', fontsize=10, text_color=text_color,
                          edge_color='#1a3a6a' if i >= 4 else '#4a7ab5')
        # 层间箭头
        if i < n - 1:
            y_arrow_top = y
            y_arrow_bot = y - gap
            ax.annotate('', xy=(x_start + box_w / 2, y_arrow_bot),
                        xytext=(x_start + box_w / 2, y_arrow_top),
                        arrowprops=dict(arrowstyle='->', color='#666', lw=1.5, shrinkA=2, shrinkB=2))

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, 'system_architecture.png')
    fig.savefig(path, dpi=DPI, bbox_inches='tight')
    plt.close(fig)
    print(f'已保存: {path}')


# ============================================================
# 图2: 多线程架构图
# ============================================================
def draw_thread_architecture():
    fig, ax = plt.subplots(1, 1, figsize=(FIG_WIDTH, 12))
    ax.set_xlim(-0.5, 14.5)
    ax.set_ylim(-1.5, 10.5)
    ax.axis('off')
    ax.set_title('多线程架构图', fontsize=20, fontweight='bold', pad=20)

    # 主线程框
    main_rect = FancyBboxPatch((0.5, 2.5), 5.5, 7.5,
                                boxstyle="round,pad=0.1",
                                facecolor='#E8F4FD', edgecolor='#2196F3',
                                linewidth=2.5, zorder=1)
    ax.add_patch(main_rect)
    ax.text(3.25, 9.7, '主线程（UI线程）', ha='center', va='center',
            fontsize=14, fontweight='bold', color='#1565C0', zorder=3)

    main_items = [
        'QApplication 事件循环',
        'MainWindow',
        '  ├─ ControlPanel',
        '  ├─ StatusPanel',
        '  ├─ CurvePanel',
        '  ├─ AlarmPanel',
        '  ├─ DatabasePanel',
        '  └─ LogPanel',
        'DeviceDataModel',
        'QTimer (pollTimer)',
    ]
    for i, item in enumerate(main_items):
        y = 9.2 - i * 0.6
        fs = 9 if item.startswith('  ') else 10
        ax.text(1.2, y, item, ha='left', va='center', fontsize=fs,
                color='#0D47A1', fontweight='bold' if not item.startswith('  ') else 'normal', zorder=3)

    # 通信线程框
    comm_rect = FancyBboxPatch((8.5, 2.5), 5.5, 7.5,
                                boxstyle="round,pad=0.1",
                                facecolor='#FFF3E0', edgecolor='#FF9800',
                                linewidth=2.5, zorder=1)
    ax.add_patch(comm_rect)
    ax.text(11.25, 9.7, '通信线程（CommunicationWorker）', ha='center', va='center',
            fontsize=13, fontweight='bold', color='#E65100', zorder=3)

    comm_items = [
        'QSerialPort',
        'QTimer (pollTimer 10ms)',
        'QTimer (connectionCheck 5s)',
        '命令队列 (QQueue)',
        '帧解析器 (FrameParser)',
    ]
    for i, item in enumerate(comm_items):
        y = 9.0 - i * 0.7
        ax.text(9.2, y, item, ha='left', va='center', fontsize=10,
                color='#BF360C', fontweight='bold', zorder=3)

    # 线程间箭头 - 主→通信
    signals_main_to_comm = [
        'connectRequested',
        'disconnectRequested',
        'sendCommandRequested',
    ]
    for i, sig in enumerate(signals_main_to_comm):
        y = 8.0 - i * 1.2
        ax.annotate('', xy=(8.5, y), xytext=(6.0, y),
                    arrowprops=dict(arrowstyle='->', color='#2E7D32', lw=2, shrinkA=3, shrinkB=3),
                    zorder=4)
        ax.text(7.25, y + 0.15, sig, ha='center', va='bottom', fontsize=8,
                color='#2E7D32', fontweight='bold', zorder=5,
                bbox=dict(boxstyle='round,pad=0.1', facecolor='#E8F5E9', edgecolor='none', alpha=0.9))

    # 线程间箭头 - 通信→主
    signals_comm_to_main = [
        'connectionStateChanged',
        'commandCompleted',
        'logMessage',
        'dataReceived',
    ]
    for i, sig in enumerate(signals_comm_to_main):
        y = 7.5 - i * 1.1
        ax.annotate('', xy=(6.0, y - 0.3), xytext=(8.5, y - 0.3),
                    arrowprops=dict(arrowstyle='->', color='#C62828', lw=2, shrinkA=3, shrinkB=3),
                    zorder=4)
        ax.text(7.25, y - 0.15, sig, ha='center', va='bottom', fontsize=8,
                color='#C62828', fontweight='bold', zorder=5,
                bbox=dict(boxstyle='round,pad=0.1', facecolor='#FFEBEE', edgecolor='none', alpha=0.9))

    # Qt::QueuedConnection 标注
    ax.text(7.25, 4.2, 'Qt::QueuedConnection', ha='center', va='center',
            fontsize=10, fontweight='bold', color='#6A1B9A',
            bbox=dict(boxstyle='round,pad=0.3', facecolor='#F3E5F5', edgecolor='#6A1B9A', lw=1.5),
            zorder=5)

    # SQLite数据库框
    db_rect = FancyBboxPatch((4.5, -1.2), 5.5, 1.5,
                              boxstyle="round,pad=0.1",
                              facecolor='#E8EAF6', edgecolor='#3F51B5',
                              linewidth=2.5, zorder=1)
    ax.add_patch(db_rect)
    ax.text(7.25, -0.45, 'SQLite 数据库\n(DatabaseManager)', ha='center', va='center',
            fontsize=11, fontweight='bold', color='#1A237E', zorder=3)

    # 主线程→数据库
    ax.annotate('', xy=(5.5, 0.3), xytext=(3.25, 2.5),
                arrowprops=dict(arrowstyle='->', color='#3F51B5', lw=2, shrinkA=3, shrinkB=3),
                zorder=4)
    # 通信线程→数据库
    ax.annotate('', xy=(9.0, 0.3), xytext=(11.25, 2.5),
                arrowprops=dict(arrowstyle='->', color='#3F51B5', lw=2, shrinkA=3, shrinkB=3),
                zorder=4)

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, 'thread_architecture.png')
    fig.savefig(path, dpi=DPI, bbox_inches='tight')
    plt.close(fig)
    print(f'已保存: {path}')


# ============================================================
# 图3: 数据流图
# ============================================================
def draw_data_flow():
    fig, ax = plt.subplots(1, 1, figsize=(FIG_WIDTH + 4, 14))
    ax.set_xlim(-1, 19)
    ax.set_ylim(-1, 14)
    ax.axis('off')
    ax.set_title('数据流图', fontsize=20, fontweight='bold', pad=20)

    # 定义步骤和颜色分类
    # 绿色=用户操作, 蓝色=主线程处理, 橙色=子线程处理, 紫色=数据更新
    steps = [
        ('用户操作', '#4CAF50', 'white', '用户操作'),
        ('UI面板信号', '#2196F3', 'white', '主线程'),
        ('MainWindow槽函数', '#2196F3', 'white', '主线程'),
        ('CommandFactory\n创建命令', '#2196F3', 'white', '主线程'),
        ('sendCommandRequested\n信号', '#2196F3', 'white', '主线程'),
        ('CommunicationWorker\n子线程', '#FF9800', 'white', '子线程'),
        ('命令队列', '#FF9800', 'white', '子线程'),
        ('command->execute()', '#FF9800', 'white', '子线程'),
        ('buildRequest()', '#FF9800', 'white', '子线程'),
        ('串口发送', '#FF9800', 'white', '子线程'),
        ('设备响应', '#4CAF50', 'white', '用户操作'),
        ('onSerialDataReady()', '#FF9800', 'white', '子线程'),
        ('findCompleteFrame()', '#FF9800', 'white', '子线程'),
        ('command->onResponse()', '#FF9800', 'white', '子线程'),
        ('parseResponse()', '#FF9800', 'white', '子线程'),
        ('commandCompleted信号', '#2196F3', 'white', '主线程'),
        ('MainWindow更新模型', '#2196F3', 'white', '主线程'),
        ('DeviceDataModel', '#9C27B0', 'white', '数据更新'),
        ('dataUpdated信号', '#9C27B0', 'white', '数据更新'),
        ('UI面板更新', '#9C27B0', 'white', '数据更新'),
    ]

    n = len(steps)
    cols = 4
    box_w = 3.2
    box_h = 0.9
    x_gap = 0.6
    y_gap = 0.5
    rows = (n + cols - 1) // cols

    # 图例
    legend_items = [
        ('用户操作', '#4CAF50'),
        ('主线程处理', '#2196F3'),
        ('子线程处理', '#FF9800'),
        ('数据更新', '#9C27B0'),
    ]
    for i, (label, color) in enumerate(legend_items):
        lx = 1 + i * 4
        ly = 13.2
        rect = FancyBboxPatch((lx, ly), 0.5, 0.4, boxstyle="round,pad=0.02",
                               facecolor=color, edgecolor='none', zorder=2)
        ax.add_patch(rect)
        ax.text(lx + 0.7, ly + 0.2, label, ha='left', va='center', fontsize=10, fontweight='bold', zorder=3)

    positions = []
    for idx, (text, color, text_color, category) in enumerate(steps):
        row = idx // cols
        col = idx % cols
        # 蛇形排列：偶数行从左到右，奇数行从右到左
        if row % 2 == 1:
            col = cols - 1 - col

        x = 0.5 + col * (box_w + x_gap)
        y = 12.0 - row * (box_h + y_gap)
        positions.append((x, y))

        draw_rounded_rect(ax, x, y, box_w, box_h, color, text,
                          fontsize=8, text_color=text_color, edge_color='#333')

    # 绘制箭头连接
    for i in range(n - 1):
        x1 = positions[i][0] + box_w / 2
        y1 = positions[i][1] + box_h / 2
        x2 = positions[i + 1][0] + box_w / 2
        y2 = positions[i + 1][1] + box_h / 2

        row_i = i // cols
        row_next = (i + 1) // cols

        if row_i == row_next:
            # 同行：水平箭头
            if positions[i + 1][0] > positions[i][0]:
                ax.annotate('', xy=(positions[i + 1][0], y2),
                            xytext=(positions[i][0] + box_w, y1),
                            arrowprops=dict(arrowstyle='->', color='#555', lw=1.5, shrinkA=2, shrinkB=2))
            else:
                ax.annotate('', xy=(positions[i + 1][0] + box_w, y2),
                            xytext=(positions[i][0], y1),
                            arrowprops=dict(arrowstyle='->', color='#555', lw=1.5, shrinkA=2, shrinkB=2))
        else:
            # 跨行：垂直+水平箭头
            ax.annotate('', xy=(x2, positions[i + 1][1] + box_h),
                        xytext=(x1, positions[i][1]),
                        arrowprops=dict(arrowstyle='->', color='#555', lw=1.5,
                                        connectionstyle='arc3,rad=0.2', shrinkA=2, shrinkB=2))

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, 'data_flow.png')
    fig.savefig(path, dpi=DPI, bbox_inches='tight')
    plt.close(fig)
    print(f'已保存: {path}')


# ============================================================
# 图4: 类继承关系图
# ============================================================
def draw_class_hierarchy():
    fig, ax = plt.subplots(1, 1, figsize=(FIG_WIDTH, 12))
    ax.set_xlim(-1, 15)
    ax.set_ylim(-1, 11)
    ax.axis('off')
    ax.set_title('类继承关系图', fontsize=20, fontweight='bold', pad=20)

    # UML类框绘制函数
    def draw_class_box(ax, x, y, w, h, name, methods, color, header_color):
        # 类名区域
        header_h = 0.5
        rect = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02",
                               facecolor='#FAFAFA', edgecolor=color, linewidth=2, zorder=2)
        ax.add_patch(rect)
        header = FancyBboxPatch((x, y + h - header_h), w, header_h, boxstyle="round,pad=0.02",
                                 facecolor=header_color, edgecolor=color, linewidth=2, zorder=3)
        ax.add_patch(header)
        ax.text(x + w / 2, y + h - header_h / 2, name, ha='center', va='center',
                fontsize=10, fontweight='bold', color='white', zorder=4)
        # 方法列表
        method_text = '\n'.join(methods)
        ax.text(x + 0.15, y + h - header_h - 0.15, method_text, ha='left', va='top',
                fontsize=7.5, color='#333', zorder=4, linespacing=1.3, family='monospace')

    # ICommand 基类
    draw_class_box(ax, 4.5, 8.5, 4.5, 2.0,
                   'ICommand (抽象基类)',
                   ['+ execute() : QByteArray',
                    '+ onResponse(data) : bool',
                    '+ commandCode() : quint8',
                    '+ address() : quint8',
                    '- m_address : quint8',
                    '- m_commandCode : quint8'],
                   '#1565C0', '#1565C0')

    # 4个子类
    subclasses = [
        ('ReadRegisterCommand', ['+ buildRequest() : QByteArray',
                                  '+ parseResponse() : QVariantMap',
                                  '+ registerAddress() : quint16'],
         0, 4.5, '#2E7D32'),
        ('WriteRegisterCommand', ['+ buildRequest() : QByteArray',
                                   '+ parseResponse() : QVariantMap',
                                   '+ value() : quint16',
                                   '- m_value : quint16'],
         3.8, 4.5, '#E65100'),
        ('ReadStatusCommand', ['+ buildRequest() : QByteArray',
                                '+ parseResponse() : QVariantMap',
                                '+ statusType() : StatusType'],
         7.6, 4.5, '#6A1B9A'),
        ('ControlDeviceCommand', ['+ buildRequest() : QByteArray',
                                   '+ parseResponse() : QVariantMap',
                                   '+ controlType() : ControlType',
                                   '- m_controlType : ControlType'],
         11.4, 4.5, '#C62828'),
    ]

    for name, methods, x, y, color in subclasses:
        draw_class_box(ax, x, y, 3.3, 2.2, name, methods, color, color)

    # 继承箭头（空心三角）
    for x_start in [1.65, 5.45, 9.25, 13.05]:
        ax.annotate('', xy=(6.75, 8.5), xytext=(x_start, 6.7),
                    arrowprops=dict(arrowstyle='-|>', color='#555', lw=1.5,
                                    shrinkA=2, shrinkB=2),
                    zorder=1)

    # CommandFactory
    draw_class_box(ax, 10, 8.5, 4.5, 1.8,
                   'CommandFactory',
                   ['+ createReadCmd(addr) : ICommand*',
                    '+ createWriteCmd(addr,val) : ICommand*',
                    '+ createStatusCmd(type) : ICommand*',
                    '+ createControlCmd(type) : ICommand*'],
                   '#00695C', '#00695C')

    # Factory虚线箭头指向4个子类
    for x_end in [1.65, 5.45, 9.25, 13.05]:
        ax.annotate('', xy=(x_end, 6.7), xytext=(10, 8.5),
                    arrowprops=dict(arrowstyle='->', color='#00695C', lw=1.2,
                                    linestyle='dashed', shrinkA=2, shrinkB=2),
                    zorder=1)
    ax.text(9.5, 7.8, '<<creates>>', ha='center', va='center', fontsize=8,
            color='#00695C', fontstyle='italic', zorder=5)

    # IProtocolParser
    draw_class_box(ax, 0, 0.5, 4.5, 1.5,
                   '<<interface>> IProtocolParser',
                   ['+ parseFrame(data) : ParsedFrame',
                    '+ validateChecksum(data) : bool'],
                   '#4527A0', '#4527A0')

    # SeedSourceProtocolParser
    draw_class_box(ax, 6, 0.5, 5.5, 2.0,
                   'SeedSourceProtocolParser',
                   ['+ parseFrame(data) : ParsedFrame',
                    '+ validateChecksum(data) : bool',
                    '+ findCompleteFrame(buf) : QByteArray',
                    '- m_buffer : QByteArray'],
                   '#283593', '#283593')

    # 实现箭头
    ax.annotate('', xy=(2.25, 2.0), xytext=(8.75, 2.5),
                arrowprops=dict(arrowstyle='-|>', color='#4527A0', lw=1.5,
                                linestyle='dashed', shrinkA=2, shrinkB=2),
                zorder=1)
    ax.text(5.5, 2.6, '<<implements>>', ha='center', va='center', fontsize=8,
            color='#4527A0', fontstyle='italic', zorder=5)

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, 'class_hierarchy.png')
    fig.savefig(path, dpi=DPI, bbox_inches='tight')
    plt.close(fig)
    print(f'已保存: {path}')


# ============================================================
# 图5: 信号槽连接图
# ============================================================
def draw_signal_slot():
    fig, ax = plt.subplots(1, 1, figsize=(FIG_WIDTH, 14))
    ax.set_xlim(-2, 16)
    ax.set_ylim(-2, 14)
    ax.axis('off')
    ax.set_title('信号槽连接图', fontsize=20, fontweight='bold', pad=20)

    # 中心 MainWindow
    center_x, center_y = 7, 7
    mw_w, mw_h = 3.5, 2.0
    draw_rounded_rect(ax, center_x - mw_w / 2, center_y - mw_h / 2, mw_w, mw_h,
                      '#1565C0', 'MainWindow', fontsize=14, text_color='white', edge_color='#0D47A1')

    # 周围组件
    components = [
        ('ControlPanel', 7, 12.5, '#4CAF50', 'UI信号'),
        ('StatusPanel', 12.5, 10, '#4CAF50', 'UI信号'),
        ('CurvePanel', 13.5, 6, '#4CAF50', 'UI信号'),
        ('AlarmPanel', 12, 2, '#4CAF50', 'UI信号'),
        ('DatabasePanel', 2, 2, '#4CAF50', 'UI信号'),
        ('LogPanel', 0.5, 6, '#4CAF50', 'UI信号'),
        ('CommunicationWorker', 1, 10.5, '#FF9800', '通信信号'),
        ('DeviceDataModel', 7, 1, '#2196F3', '数据信号'),
        ('DatabaseManager', -1, 4, '#795548', '数据信号'),
        ('AlarmManager', 13, 4, '#F44336', '报警信号'),
    ]

    comp_w, comp_h = 2.8, 1.2
    for name, cx, cy, color, category in components:
        draw_rounded_rect(ax, cx - comp_w / 2, cy - comp_h / 2, comp_w, comp_h,
                          color, name, fontsize=9, text_color='white', edge_color='#333')

    # 信号槽连接定义
    # (from_comp_idx, to_comp_idx, signal, slot, color)
    connections = [
        (0, -1, 'setCurrentRequested', 'onSetCurrent', '#4CAF50'),      # ControlPanel → MainWindow
        (1, -1, 'refreshRequested', 'onRefreshStatus', '#4CAF50'),      # StatusPanel → MainWindow
        (2, -1, 'curveConfigChanged', 'onCurveConfigChanged', '#4CAF50'),  # CurvePanel → MainWindow
        (3, -1, 'alarmConfigChanged', 'onAlarmConfigChanged', '#4CAF50'),  # AlarmPanel → MainWindow
        (4, -1, 'queryRequested', 'onDatabaseQuery', '#4CAF50'),        # DatabasePanel → MainWindow
        (5, -1, 'clearLogRequested', 'onClearLog', '#4CAF50'),          # LogPanel → MainWindow
        (-1, 6, 'sendCommandRequested', 'onSendCommand', '#FF9800'),    # MainWindow → CommunicationWorker
        (-1, 6, 'connectRequested', 'onConnect', '#FF9800'),            # MainWindow → CommunicationWorker
        (6, -1, 'commandCompleted', 'onCommandCompleted', '#FF9800'),   # CommunicationWorker → MainWindow
        (6, -1, 'connectionStateChanged', 'onConnectionChanged', '#FF9800'),  # CommunicationWorker → MainWindow
        (-1, 7, 'dataUpdated', 'onDataUpdated', '#2196F3'),             # MainWindow → DeviceDataModel (via model)
        (7, -1, 'dataUpdated', 'onModelUpdated', '#2196F3'),            # DeviceDataModel → MainWindow
        (-1, 8, 'saveDataRequested', 'onSaveData', '#795548'),          # MainWindow → DatabaseManager
        (8, -1, 'queryResultReady', 'onQueryResult', '#795548'),        # DatabaseManager → MainWindow
        (-1, 9, 'checkAlarmRequested', 'onCheckAlarm', '#F44336'),      # MainWindow → AlarmManager
        (9, -1, 'alarmTriggered', 'onAlarmTriggered', '#F44336'),       # AlarmManager → MainWindow
    ]

    # 图例
    legend_items = [
        ('UI信号', '#4CAF50'),
        ('通信信号', '#FF9800'),
        ('数据信号', '#2196F3'),
        ('报警信号', '#F44336'),
    ]
    for i, (label, color) in enumerate(legend_items):
        lx = 2 + i * 3.5
        ly = -1.2
        ax.plot([lx, lx + 0.6], [ly, ly], color=color, lw=2.5, zorder=5)
        ax.text(lx + 0.8, ly, label, ha='left', va='center', fontsize=10, fontweight='bold', zorder=5)

    # 绘制连接线
    for from_idx, to_idx, signal_name, slot_name, color in connections:
        if from_idx == -1:
            # MainWindow → component
            fx, fy = center_x, center_y
            tcx, tcy = components[to_idx][1], components[to_idx][2]
        else:
            fx, fy = components[from_idx][1], components[from_idx][2]
            tcx, tcy = center_x, center_y

        # 计算方向
        dx = tcx - fx
        dy = tcy - fy
        dist = np.sqrt(dx ** 2 + dy ** 2)
        if dist == 0:
            continue

        # 缩短箭头避免重叠
        shrink = 1.2
        sx = fx + dx / dist * shrink
        sy = fy + dy / dist * shrink
        ex = tcx - dx / dist * shrink
        ey = tcy - dy / dist * shrink

        ax.annotate('', xy=(ex, ey), xytext=(sx, sy),
                    arrowprops=dict(arrowstyle='->', color=color, lw=1.3,
                                    shrinkA=0, shrinkB=0, alpha=0.7),
                    zorder=1)

        # 标注信号/槽
        mx = (sx + ex) / 2
        my = (sy + ey) / 2
        label_text = f'{signal_name}\n→ {slot_name}'
        ax.text(mx, my, label_text, ha='center', va='center', fontsize=6,
                color=color, fontweight='bold', zorder=5,
                bbox=dict(boxstyle='round,pad=0.1', facecolor='white', edgecolor='none', alpha=0.85))

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, 'signal_slot.png')
    fig.savefig(path, dpi=DPI, bbox_inches='tight')
    plt.close(fig)
    print(f'已保存: {path}')


# ============================================================
# 图6: 通信协议帧结构图
# ============================================================
def draw_protocol_frame():
    fig, ax = plt.subplots(1, 1, figsize=(FIG_WIDTH, 8))
    ax.set_xlim(-0.5, 13)
    ax.set_ylim(-3, 5)
    ax.axis('off')
    ax.set_title('通信协议帧结构图', fontsize=20, fontweight='bold', pad=20)

    # 帧字段定义: (名称, 相对宽度, 颜色, 字节数, 含义)
    fields = [
        ('帧头\n0xAA', 1.0, '#E53935', '1字节', '标识帧起始'),
        ('地址码', 1.0, '#FB8C00', '1字节', '设备地址(0x01~0xFE)'),
        ('命令码', 1.0, '#43A047', '1字节', '功能码(读/写/状态/控制)'),
        ('数据长度\nL', 1.0, '#1E88E5', '1字节', '数据字段字节数(0~255)'),
        ('数据\n×L', 2.5, '#8E24AA', 'L字节', '有效数据载荷'),
        ('校验和', 1.0, '#00897B', '1字节', '地址码~数据的异或和'),
        ('帧尾\n0x55', 1.0, '#546E7A', '1字节', '标识帧结束'),
    ]

    total_w = sum(f[1] for f in fields)
    x_start = (13 - total_w) / 2
    y_base = 1.5
    field_h = 1.8

    x = x_start
    for name, w, color, bytes_str, meaning in fields:
        # 字段矩形
        rect = FancyBboxPatch((x, y_base), w - 0.03, field_h, boxstyle="round,pad=0.02",
                               facecolor=color, edgecolor='#333', linewidth=1.5, alpha=0.9, zorder=2)
        ax.add_patch(rect)
        ax.text(x + w / 2, y_base + field_h / 2, name, ha='center', va='center',
                fontsize=10, fontweight='bold', color='white', zorder=3)

        # 下方标注字节数
        ax.text(x + w / 2, y_base - 0.4, bytes_str, ha='center', va='center',
                fontsize=9, color='#333', fontweight='bold', zorder=3)
        # 下方标注含义
        ax.text(x + w / 2, y_base - 0.9, meaning, ha='center', va='center',
                fontsize=8, color='#666', zorder=3)

        x += w

    # 上方校验和范围标注
    # 校验和覆盖：地址码到数据（第2~5个字段）
    checksum_start = x_start + fields[0][1]  # 地址码开始
    checksum_end = x_start + fields[0][1] + fields[1][1] + fields[2][1] + fields[3][1] + fields[4][1]  # 数据结束

    bracket_y = y_base + field_h + 0.3
    ax.annotate('', xy=(checksum_start, bracket_y), xytext=(checksum_start, bracket_y + 0.5),
                arrowprops=dict(arrowstyle='-', color='#D32F2F', lw=2), zorder=4)
    ax.annotate('', xy=(checksum_end, bracket_y), xytext=(checksum_end, bracket_y + 0.5),
                arrowprops=dict(arrowstyle='-', color='#D32F2F', lw=2), zorder=4)
    ax.plot([checksum_start, checksum_end], [bracket_y + 0.5, bracket_y + 0.5],
            color='#D32F2F', lw=2, zorder=4)
    ax.text((checksum_start + checksum_end) / 2, bracket_y + 0.7,
            '校验和计算范围（异或和：地址码 ⊕ 命令码 ⊕ 数据长度 ⊕ 数据）',
            ha='center', va='center', fontsize=10, fontweight='bold', color='#D32F2F', zorder=5)

    # 底部总帧格式说明
    ax.text(6.5, -2.2,
            '完整帧格式：[0xAA] [地址码] [命令码] [数据长度L] [数据×L] [校验和] [0x55]    '
            '最小帧长：7字节（L=0时）',
            ha='center', va='center', fontsize=10, color='#333',
            bbox=dict(boxstyle='round,pad=0.4', facecolor='#F5F5F5', edgecolor='#999', lw=1),
            zorder=5)

    plt.tight_layout()
    path = os.path.join(OUTPUT_DIR, 'protocol_frame.png')
    fig.savefig(path, dpi=DPI, bbox_inches='tight')
    plt.close(fig)
    print(f'已保存: {path}')


# ============ 主函数 ============
if __name__ == '__main__':
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    print('开始生成架构图...')
    draw_system_architecture()
    draw_thread_architecture()
    draw_data_flow()
    draw_class_hierarchy()
    draw_signal_slot()
    draw_protocol_frame()
    print('所有架构图生成完成！')
