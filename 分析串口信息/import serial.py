import sys
import serial
import numpy as np
import pyqtgraph as pg
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5.QtCore import QTimer, QThread, pyqtSignal
import re

# --- 配置参数 ---
SERIAL_PORT = 'COM3'  # 替换为你的串口号
BAUD_RATE = 115200    # 波特率
WINDOW_SIZE = 500     # 横轴显示的点数，越多波形越长
UPDATE_INTERVAL = 16  # 刷新间隔(ms)，16ms 约等于 60fps
# ----------------

class SerialReader(QThread):
    """串口读取线程，防止主界面卡顿"""
    data_received = pyqtSignal(list)

    def __init__(self, port, baud):
        super().__init__()
        try:
            self.ser = serial.Serial(port, baud, timeout=0.01)
        except Exception as e:
            print(f"串口打开失败: {e}")
            self.ser = None

    def run(self):
        while self.ser and self.ser.is_open:
            if self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if "Plot" in line:
                        # 提取数字
                        nums = re.findall(r"[-+]?\d*\.\d+|\d+", line)
                        if nums:
                            self.data_received.emit([float(x) for x in nums])
                except:
                    pass

class SmoothPlotter(QMainWindow):
    def __init__(self):
        super().__init__()
        
        # 初始化界面
        self.setWindowTitle("Smooth Serial Plotter (60 FPS)")
        self.resize(800, 600)
        
        # 创建绘图控件
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('k') # 黑色背景更专业
        self.plot_widget.showGrid(x=True, y=True, alpha=0.3)
        self.setCentralWidget(self.plot_widget)
        
        self.curves = []
        self.data_storage = []
        
        # 串口线程
        self.reader = SerialReader(SERIAL_PORT, BAUD_RATE)
        self.reader.data_received.connect(self.update_data)
        self.reader.start()

    def update_data(self, values):
        num_vars = len(values)
        
        # 第一次运行，初始化存储空间和线条
        if not self.data_storage:
            for i in range(num_vars):
                self.data_storage.append(np.zeros(WINDOW_SIZE))
                # 设置不同颜色
                pen = pg.mkPen(color=pg.intColor(i), width=2) 
                curve = self.plot_widget.plot(pen=pen, name=f"Var {i+1}")
                self.curves.append(curve)
        
        # 数据滚动更新
        for i in range(min(num_vars, len(self.data_storage))):
            self.data_storage[i] = np.roll(self.data_storage[i], -1)
            self.data_storage[i][-1] = values[i]
            # 实时更新波形数据
            self.curves[i].setData(self.data_storage[i])

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SmoothPlotter()
    window.show()
    sys.exit(app.exec_())