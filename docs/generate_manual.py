# -*- coding: utf-8 -*-
"""生成种子源模块控制器使用说明书 Word 文档"""

from docx import Document
from docx.shared import Pt, Cm, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement
import os

OUTPUT_PATH = r"f:\工作文档\2026\大功率可调恒流源\QT上位机\docs\种子源模块控制器_使用说明书.docx"


def set_cell_shading(cell, color_hex):
    """设置单元格底色"""
    shading = OxmlElement("w:shd")
    shading.set(qn("w:fill"), color_hex)
    shading.set(qn("w:val"), "clear")
    cell._tc.get_or_add_tcPr().append(shading)


def add_heading_styled(doc, text, level):
    """添加标题并设置中文字体"""
    heading = doc.add_heading(text, level=level)
    for run in heading.runs:
        run.font.name = "微软雅黑"
        run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
    return heading


def add_para(doc, text, bold=False, indent=False, space_after=Pt(4)):
    """添加段落"""
    p = doc.add_paragraph()
    if indent:
        p.paragraph_format.left_indent = Cm(0.75)
    p.paragraph_format.space_after = space_after
    p.paragraph_format.space_before = Pt(2)
    run = p.add_run(text)
    run.font.name = "微软雅黑"
    run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
    run.font.size = Pt(11)
    run.bold = bold
    return p


def add_bullet(doc, text, level=0):
    """添加列表项"""
    p = doc.add_paragraph(style="List Bullet")
    p.paragraph_format.left_indent = Cm(1.0 + level * 0.75)
    p.paragraph_format.space_after = Pt(2)
    p.paragraph_format.space_before = Pt(1)
    # 清除默认 run，重新添加
    p.clear()
    run = p.add_run(text)
    run.font.name = "微软雅黑"
    run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
    run.font.size = Pt(11)
    return p


def add_numbered(doc, text, level=0):
    """添加有序列表项"""
    p = doc.add_paragraph(style="List Number")
    p.paragraph_format.left_indent = Cm(1.0 + level * 0.75)
    p.paragraph_format.space_after = Pt(2)
    p.paragraph_format.space_before = Pt(1)
    p.clear()
    run = p.add_run(text)
    run.font.name = "微软雅黑"
    run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
    run.font.size = Pt(11)
    return p


def build_doc():
    doc = Document()

    # ── 全局默认字体 ──
    style = doc.styles["Normal"]
    style.font.name = "微软雅黑"
    style.font.size = Pt(11)
    style.element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")

    # ── 页边距 ──
    for section in doc.sections:
        section.top_margin = Cm(2.54)
        section.bottom_margin = Cm(2.54)
        section.left_margin = Cm(3.17)
        section.right_margin = Cm(3.17)

    # ═══════════════════════════════════════════
    # 封面标题
    # ═══════════════════════════════════════════
    for _ in range(6):
        doc.add_paragraph()

    title_p = doc.add_paragraph()
    title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title_run = title_p.add_run("种子源模块控制器")
    title_run.font.name = "微软雅黑"
    title_run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
    title_run.font.size = Pt(28)
    title_run.bold = True
    title_run.font.color.rgb = RGBColor(0x1F, 0x49, 0x7D)

    sub_p = doc.add_paragraph()
    sub_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    sub_run = sub_p.add_run("使 用 说 明 书")
    sub_run.font.name = "微软雅黑"
    sub_run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
    sub_run.font.size = Pt(22)
    sub_run.font.color.rgb = RGBColor(0x1F, 0x49, 0x7D)

    ver_p = doc.add_paragraph()
    ver_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    ver_p.paragraph_format.space_before = Pt(20)
    ver_run = ver_p.add_run("版本 v1.0.1")
    ver_run.font.name = "微软雅黑"
    ver_run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
    ver_run.font.size = Pt(14)
    ver_run.font.color.rgb = RGBColor(0x59, 0x56, 0x59)

    doc.add_page_break()

    # ═══════════════════════════════════════════
    # 1. 软件简介
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "1. 软件简介", level=1)
    add_bullet(doc, "软件名称：种子源模块控制器（SeedSourceControl）")
    add_bullet(doc, "版本：v1.0.1")
    add_bullet(doc, "运行环境：Windows 7/10/11（64位）")
    add_bullet(doc, "软件用途：用于控制和管理种子源模块设备，通过串口与设备通信，实现参数设置、实时监控、报警管理等功能")
    add_bullet(doc, "通信协议：《种子源模块通信协议V1.3》")

    # ═══════════════════════════════════════════
    # 2. 安装与启动
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "2. 安装与启动", level=1)

    add_heading_styled(doc, "2.1 系统要求", level=2)
    add_bullet(doc, "操作系统：Windows 7/10/11 64位")
    add_bullet(doc, "无需安装Qt环境（已集成运行时依赖）")

    add_heading_styled(doc, "2.2 启动方式", level=2)
    add_bullet(doc, "双击 SeedSourceControl.exe 即可启动")
    add_bullet(doc, "程序位于 build\\deploy\\Debug\\ 目录")
    add_bullet(doc, "程序以Windows GUI模式运行，不会弹出命令行窗口")

    add_heading_styled(doc, "2.3 目录说明", level=2)
    add_bullet(doc, "deploy\\Debug\\SeedSourceControl.exe — 主程序")
    add_bullet(doc, "deploy\\Debug\\platforms\\ — Qt平台插件")
    add_bullet(doc, "deploy\\Debug\\sqldrivers\\ — SQLite数据库驱动")
    add_bullet(doc, "deploy\\Debug\\imageformats\\ — 图像格式插件")

    # ═══════════════════════════════════════════
    # 3. 界面说明
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "3. 界面说明", level=1)

    add_heading_styled(doc, "3.1 主界面布局", level=2)
    add_para(doc, "主界面分为左右两个区域：")
    add_bullet(doc, "左侧区域（从上到下）：连接面板、控制面板、状态面板")
    add_bullet(doc, "右侧区域（从上到下）：实时图表面板、报警面板、日志面板")
    add_bullet(doc, "各区域可通过拖拽分割条调整大小")

    add_heading_styled(doc, "3.2 连接面板", level=2)
    add_bullet(doc, "串口选择：下拉选择可用的COM端口")
    add_bullet(doc, "波特率：支持9600/19200/38400/57600/115200等")
    add_bullet(doc, "数据位：5/6/7/8")
    add_bullet(doc, "校验位：无校验/偶校验/奇校验")
    add_bullet(doc, "停止位：1/1.5/2")
    add_bullet(doc, "流控制：无/硬件/软件")
    add_bullet(doc, "连接按钮：点击连接设备")
    add_bullet(doc, "断开按钮：点击断开设备")
    add_bullet(doc, "刷新按钮：刷新可用串口列表")

    add_heading_styled(doc, "3.3 控制面板", level=2)
    add_bullet(doc, "启动按钮：启动设备运行")
    add_bullet(doc, "停止按钮：停止设备运行")
    add_bullet(doc, "重置按钮：重置设备")
    add_bullet(doc, "校准按钮：执行设备校准")
    add_bullet(doc, "电流设置：设置目标电流值（0-1000mA）")
    add_bullet(doc, "温度设置：设置目标温度值（-40~125°C）")
    add_bullet(doc, "功率设置：设置目标功率值（0-10000mW）")

    add_heading_styled(doc, "3.4 状态面板", level=2)
    add_bullet(doc, "设备状态：显示当前设备运行状态")
    add_bullet(doc, "电流值：实时显示电流读数（mA）")
    add_bullet(doc, "温度值：实时显示温度读数（°C）")
    add_bullet(doc, "功率值：实时显示功率读数（mW）")
    add_bullet(doc, "报警状态：显示当前报警信息")

    add_heading_styled(doc, "3.5 实时图表面板", level=2)
    add_bullet(doc, "电流曲线：实时显示电流变化趋势")
    add_bullet(doc, "温度曲线：实时显示温度变化趋势")
    add_bullet(doc, "功率曲线：实时显示功率变化趋势")
    add_bullet(doc, "支持鼠标缩放和平移操作")

    add_heading_styled(doc, "3.6 报警面板", level=2)
    add_bullet(doc, "报警列表：显示所有报警记录")
    add_bullet(doc, "报警信息包括：时间、报警码、报警描述")
    add_bullet(doc, "清除按钮：清除所有报警记录")

    add_heading_styled(doc, "3.7 日志面板", level=2)
    add_bullet(doc, "日志级别：信息（Info）、警告（Warning）、错误（Error）")
    add_bullet(doc, "显示通信日志、操作日志、错误日志")
    add_bullet(doc, "清除按钮：清除所有日志")
    add_bullet(doc, "保存按钮：保存日志到文件")

    add_heading_styled(doc, "3.8 菜单栏", level=2)
    add_bullet(doc, "文件菜单：退出程序")
    add_bullet(doc, "视图菜单：视图选项")
    add_bullet(doc, "工具菜单：工具选项")
    add_bullet(doc, "帮助菜单：关于信息")

    add_heading_styled(doc, "3.9 工具栏", level=2)
    add_bullet(doc, "连接按钮：快速连接设备")
    add_bullet(doc, "断开按钮：快速断开设备")
    add_bullet(doc, "启动按钮：快速启动设备")
    add_bullet(doc, "停止按钮：快速停止设备")

    # ═══════════════════════════════════════════
    # 4. 操作流程
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "4. 操作流程", level=1)

    add_heading_styled(doc, "4.1 连接设备", level=2)
    add_numbered(doc, "在连接面板选择正确的串口（如COM3）")
    add_numbered(doc, "设置串口参数（波特率等，需与设备一致）")
    add_numbered(doc, '点击"连接"按钮')
    add_numbered(doc, '观察状态栏显示"已连接"，连接面板显示连接状态')

    add_heading_styled(doc, "4.2 启动设备", level=2)
    add_numbered(doc, "确认设备已连接")
    add_numbered(doc, '在控制面板点击"启动"按钮')
    add_numbered(doc, "观察状态面板显示设备运行状态")
    add_numbered(doc, "实时图表开始显示数据曲线")

    add_heading_styled(doc, "4.3 设置参数", level=2)
    add_numbered(doc, "确认设备已连接且正在运行")
    add_numbered(doc, "在控制面板调整电流/温度/功率参数")
    add_numbered(doc, "参数变更会自动发送到设备")
    add_numbered(doc, "观察状态面板和图表的实时变化")

    add_heading_styled(doc, "4.4 监控报警", level=2)
    add_numbered(doc, "当设备出现异常时，报警面板自动显示报警信息")
    add_numbered(doc, "报警类型包括：过流、过温、过功率、电压错误、通信错误、硬件故障")
    add_numbered(doc, "日志面板同时记录报警详情")

    add_heading_styled(doc, "4.5 停止和断开", level=2)
    add_numbered(doc, '点击"停止"按钮停止设备运行')
    add_numbered(doc, '点击"断开"按钮断开串口连接')
    add_numbered(doc, "数据会自动保存到SQLite数据库")

    # ═══════════════════════════════════════════
    # 5. 数据管理
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "5. 数据管理", level=1)

    add_heading_styled(doc, "5.1 数据存储", level=2)
    add_bullet(doc, "所有实时数据自动保存到SQLite数据库")
    add_bullet(doc, "数据库文件位置：用户AppData目录")
    add_bullet(doc, "包含数据表和报警表")

    add_heading_styled(doc, "5.2 数据导出", level=2)
    add_bullet(doc, "支持CSV格式导出")
    add_bullet(doc, "包含时间戳、电流、温度、功率、状态、报警信息")

    # ═══════════════════════════════════════════
    # 6. 配置说明
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "6. 配置说明", level=1)

    add_heading_styled(doc, "6.1 串口配置", level=2)
    add_bullet(doc, "串口参数通过界面设置")
    add_bullet(doc, "配置自动保存，下次启动自动加载")

    add_heading_styled(doc, "6.2 报警阈值配置", level=2)
    add_bullet(doc, "报警阈值配置文件：alarm_config.ini")
    add_bullet(doc, "可配置电流/温度/功率的上下限")
    add_bullet(doc, "可启用/禁用各类报警检测")

    add_heading_styled(doc, "6.3 应用配置", level=2)
    add_bullet(doc, "配置文件：config.ini")
    add_bullet(doc, "包含串口默认参数、轮询间隔、超时时间等")

    # ═══════════════════════════════════════════
    # 7. 常见问题
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "7. 常见问题", level=1)

    add_heading_styled(doc, "Q1: 无法找到串口", level=3)
    add_bullet(doc, "检查设备是否已连接")
    add_bullet(doc, "检查驱动是否安装")
    add_bullet(doc, '点击"刷新"按钮重新扫描')

    add_heading_styled(doc, "Q2: 连接失败", level=3)
    add_bullet(doc, "确认串口参数与设备一致")
    add_bullet(doc, "确认串口未被其他程序占用")
    add_bullet(doc, "检查设备是否上电")

    add_heading_styled(doc, "Q3: 数据不更新", level=3)
    add_bullet(doc, "确认设备已连接且已启动")
    add_bullet(doc, "检查串口通信是否正常")
    add_bullet(doc, "查看日志面板是否有错误信息")

    add_heading_styled(doc, "Q4: 程序退出时出现错误", level=3)
    add_bullet(doc, "请确保使用最新版本（v1.0.1+）")
    add_bullet(doc, "已修复退出时的线程清理问题")

    add_heading_styled(doc, "Q5: 报警阈值如何修改", level=3)
    add_bullet(doc, "通过alarm_config.ini文件修改")
    add_bullet(doc, "修改后重启程序生效")

    # ═══════════════════════════════════════════
    # 8. 技术参数
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "8. 技术参数", level=1)

    table_data = [
        ("参数", "范围", "说明"),
        ("电流", "0-1000 mA", "电流控制范围"),
        ("温度", "-40~125 °C", "温度控制范围"),
        ("功率", "0-10000 mW", "功率控制范围"),
        ("波特率", "9600-115200", "串口通信速率"),
        ("轮询间隔", "100 ms", "设备状态轮询周期"),
        ("命令超时", "2000 ms", "命令响应超时时间"),
    ]

    table = doc.add_table(rows=len(table_data), cols=3, style="Table Grid")
    table.alignment = WD_TABLE_ALIGNMENT.CENTER

    for i, row_data in enumerate(table_data):
        row = table.rows[i]
        for j, cell_text in enumerate(row_data):
            cell = row.cells[j]
            cell.text = ""
            p = cell.paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            run = p.add_run(cell_text)
            run.font.name = "微软雅黑"
            run._element.rPr.rFonts.set(qn("w:eastAsia"), "微软雅黑")
            run.font.size = Pt(11)
            if i == 0:
                run.bold = True
                run.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
                set_cell_shading(cell, "1F497D")
            else:
                if i % 2 == 0:
                    set_cell_shading(cell, "D6E4F0")

    # 设置列宽
    for row in table.rows:
        row.cells[0].width = Cm(4)
        row.cells[1].width = Cm(4.5)
        row.cells[2].width = Cm(5.5)

    # ═══════════════════════════════════════════
    # 9. 联系方式
    # ═══════════════════════════════════════════
    add_heading_styled(doc, "9. 联系方式", level=1)
    add_para(doc, "如有问题或建议，请联系开发团队。")

    # ── 保存 ──
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    doc.save(OUTPUT_PATH)
    print(f"文档已生成：{OUTPUT_PATH}")


if __name__ == "__main__":
    build_doc()
