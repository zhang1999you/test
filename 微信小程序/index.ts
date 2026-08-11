interface JoyConfig {
  centerX: number;
  centerY: number;
  maxR: number;
}

interface MsgItem {
  time: string;
  text: string;
}

interface PlotSample {
  angleAcc: number;
  angleGyro: number;
  angle: number;
}

Page({
  data: {
    stickX: 0,
    stickY: 0,
    speed: 0,
    turn: 0,

    devices: [] as any[],
    isScanning: false,
    connected: false,
    statusMsg: '蓝牙未连接',

    receivedMsgs: [] as MsgItem[],
    isReceiving: true,

    runFlag: 0,
    gyroCalibrating: false,
    gyroCalibrationStatus: '未标定（断电后需重新标定）',
    angleAccOffset: 0,

    anglePID: {
      kp: 4.1,
      ki: 0.5,
      kd: 0.1
    },

    speedPID: {
      kp: 0.0,
      ki: 0.0,
      kd: 0.0
    },

    turnPID: {
      kp: 2.0,
      ki: 0.0,
      kd: 0.0
    },

    scrollIntoViewId: '',

    plotCanvasW: 0,
    plotCanvasH: 0,

    plotShowAcc: true,
    plotShowGyro: true,
    plotShowAngle: true,
    plotCurrentText: '等待 Plot 数据...'
  },

  _joyConfig: { centerX: 0, centerY: 0, maxR: 120 } as JoyConfig,
  _lastSendTime: 0,
  _sendTimer: null as any, // 用于持续循环发送数据的定时器

  _deviceId: '',
  _serviceId: '',
  _characteristicId: '',

  _rxBuffer: '',
  _pendingLogs: [] as MsgItem[],
  _flushTimer: 0 as any,
  _bleValueChangeHandler: null as any,

  _plotSamples: [] as PlotSample[],
  _plotDrawTimer: 0 as any,
  _plotMaxPoints: 100,
  _gyroCalibrationTimer: 0 as any,

  onReady() {
    this.initJoystick();
    this.initPlotCanvasSize();
  },

  onUnload() {
    this.stopSendTimer(); // 页面销毁时确保清除定时器，防止内存泄漏
    this.closeBLE();
  },

  // 触摸开始：立刻启动循环发送定时器
  onTouchStart() {
    if (this.data.gyroCalibrating) return;
    this.startSendTimer();
  },

  initJoystick() {
    wx.createSelectorQuery()
      .select('#joy-base')
      .boundingClientRect((rect: any) => {
        if (rect) {
          this._joyConfig = {
            centerX: rect.left + rect.width / 2,
            centerY: rect.top + rect.height / 2,
            maxR: rect.width / 2.2
          };
        }
      })
      .exec();
  },

  initPlotCanvasSize() {
    const sys = wx.getSystemInfoSync();
    const rpxToPx = sys.windowWidth / 750; // 获取 rpx 转 px 的精确比例

    // 对应 less 中 .plot-container 的 width: 96% 和 padding: 14rpx
    const containerWidthPx = sys.windowWidth * 0.96;
    const paddingPx = 14 * 2 * rpxToPx;
    const w = Math.floor(containerWidthPx - paddingPx);
    
    // 对应 less 中 .plot-canvas 的 height: 320rpx
    const h = Math.floor(320 * rpxToPx);

    this.setData(
      {
        plotCanvasW: w,
        plotCanvasH: h
      },
      () => {
        this.drawPlotChart();
      }
    );
  },

  // 触摸移动：只负责高频计算并更新界面坐标及数据，发送动作交由定时器接管
  onTouchMove(e: any) {
    if (this.data.gyroCalibrating) return;

    const touch = e.touches[0];

    let dx = touch.clientX - this._joyConfig.centerX;
    let dy = touch.clientY - this._joyConfig.centerY;

    const distance = Math.sqrt(dx * dx + dy * dy);

    if (distance > this._joyConfig.maxR) {
      dx = (dx / distance) * this._joyConfig.maxR;
      dy = (dy / distance) * this._joyConfig.maxR;
    }

    const speed = Math.round((-dy / this._joyConfig.maxR) * 100);
    const turn = Math.round((dx / this._joyConfig.maxR) * 100);

    this.setData({
      stickX: dx,
      stickY: dy,
      speed,
      turn
    });

    // 防御性启动：确保意外情况下定时器依然在运行
    if (!this._sendTimer) {
      this.startSendTimer();
    }
  },

  // 触摸结束：关闭定时器，摇杆数据清零，并发送最后一次(0,0)停机指令
  onTouchEnd() {
    // 1. 停止循环发送
    this.stopSendTimer();

    // 2. 摇杆复位归零
    this.setData({
      stickX: 0,
      stickY: 0,
      speed: 0,
      turn: 0
    });

    if (this.data.gyroCalibrating) return;

    // 3. 连续发送三次归零码文本，每次间隔 50ms 防止蓝牙底层丢包或粘包
    let sendCount = 0;
    const sendZeroInterval = () => {
      if (sendCount < 3) {
        this.sendToCar(0, 0);
        sendCount++;
        
        // 如果还没发满3次，隔 50ms 后再次触发自身
        if (sendCount < 3) {
          setTimeout(sendZeroInterval, 50);
        }
      }
    };
    
    // 立即启动第一次发送
    sendZeroInterval();
  },

  // 启动循环发送定时器
  startSendTimer() {
    if (this._sendTimer || this.data.gyroCalibrating) return; // 避免重复创建

    // 每隔 60ms 自动获取 data 中最新的 speed 和 turn 发送给小车
    this._sendTimer = setInterval(() => {
      this.sendToCar(this.data.speed, this.data.turn);
    }, 60); 
  },

  // 停止循环发送定时器
  stopSendTimer() {
    if (this._sendTimer) {
      clearInterval(this._sendTimer);
      this._sendTimer = null;
    }
  },

// 修改后：高效率、低长度的紧凑明文协议
  sendToCar(speed: number, turn: number) {
    if (!this.data.connected) return;

    // 格式如 s:50,t:60\r\n  最长只有 14 字节，确保永不分包丢包
    const cmd = `s:${speed},t:${turn}\r\n`;
    this.sendString(cmd);
  },

  sendString(str: string, onSuccess?: () => void, onFail?: () => void) {
    if (!this.data.connected) {
      if (onFail) onFail();
      return;
    }

    const buffer = new ArrayBuffer(str.length);
    const dataView = new DataView(buffer);

    for (let i = 0; i < str.length; i++) {
      dataView.setUint8(i, str.charCodeAt(i));
    }

    wx.writeBLECharacteristicValue({
      deviceId: this._deviceId,
      serviceId: this._serviceId,
      characteristicId: this._characteristicId,
      value: buffer,
      success: () => {
        if (onSuccess) onSuccess();
      },
      fail: () => {
        if (onFail) onFail();
      }
    });
  },
  copyReceivedLogs() {
    const logs = this.data.receivedMsgs;
  
    if (logs.length === 0) {
      wx.showToast({
        title: '暂无日志',
        icon: 'none'
      });
      return;
    }
  
    const content = logs
      .map((item) => `[${item.time}] ${item.text}`)
      .join('\n');
  
    wx.setClipboardData({
      data: content,
      success: () => {
        wx.showToast({
          title: `已复制 ${logs.length} 条`,
          icon: 'success'
        });
      }
    });
  },
  toggleReceive() {
    const next = !this.data.isReceiving;
    this.setData({ isReceiving: next });

    wx.showToast({
      title: next ? '已开启接收' : '已停止接收',
      icon: 'none'
    });
  },

  toggleRunFlag() {
    if (this.data.gyroCalibrating) {
      wx.showToast({
        title: '标定中，请勿启动',
        icon: 'none'
      });
      return;
    }

    const next = this.data.runFlag ? 0 : 1;
    this.setData({ runFlag: next });
    this.sendString(`runFlag:${next}\r\n`);
  },

  _clearGyroCalibrationTimer() {
    if (this._gyroCalibrationTimer) {
      clearTimeout(this._gyroCalibrationTimer);
      this._gyroCalibrationTimer = 0;
    }
  },

  startGyroCalibration() {
    if (!this.data.connected || this.data.gyroCalibrating) return;

    this.stopSendTimer();
    this._clearGyroCalibrationTimer();
    this.setData({
      runFlag: 0,
      stickX: 0,
      stickY: 0,
      speed: 0,
      turn: 0,
      isReceiving: true,
      gyroCalibrating: true,
      gyroCalibrationStatus: '正在停车，请固定小车'
    });

    this.sendString(
      'runFlag:0\r\n',
      () => {
        wx.showModal({
          title: '标定 gx 零飘',
          content: '请将小车平放在稳定表面，标定期间电机会关闭。确认后保持完全静止约 3 秒。',
          confirmText: '开始标定',
          success: (res) => {
            if (!res.confirm) {
              this.setData({
                gyroCalibrating: false,
                gyroCalibrationStatus: '已取消标定，小车保持停止'
              });
              return;
            }

            if (!this.data.connected) {
              this.setData({
                gyroCalibrating: false,
                gyroCalibrationStatus: '连接已断开，标定取消'
              });
              return;
            }

            this.setData({
              gyroCalibrationStatus: '标定中，请勿移动小车'
            });

            this._gyroCalibrationTimer = setTimeout(() => {
              this._gyroCalibrationTimer = 0;
              this.setData({
                gyroCalibrating: false,
                gyroCalibrationStatus: '标定超时，请保持静止后重试'
              });
              wx.showToast({
                title: '标定超时',
                icon: 'none'
              });
            }, 8000);

            this.sendString(
              'gyroCalibrate:1\r\n',
              undefined,
              () => {
                this._clearGyroCalibrationTimer();
                this.setData({
                  gyroCalibrating: false,
                  gyroCalibrationStatus: '标定命令发送失败，请重试'
                });
                wx.showToast({
                  title: '发送失败',
                  icon: 'none'
                });
              }
            );
          }
        });
      },
      () => {
        this.setData({
          gyroCalibrating: false,
          gyroCalibrationStatus: '停车命令发送失败，请检查连接'
        });
        wx.showToast({
          title: '发送失败',
          icon: 'none'
        });
      }
    );
  },

  _handleGyroCalibrationLine(text: string) {
    if (/^GYRO_CAL:START$/i.test(text)) {
      this.setData({
        gyroCalibrating: true,
        gyroCalibrationStatus: '正在静置并采样，请勿移动小车'
      });
      return true;
    }

    if (/^GYRO_CAL:BUSY$/i.test(text)) {
      this.setData({
        gyroCalibrating: true,
        gyroCalibrationStatus: '设备正在标定，请继续保持静止'
      });
      return true;
    }

    const done = text.match(
      /^GYRO_CAL:DONE,offset:([+-]?(?:\d+(?:\.\d*)?|\.\d+))$/i
    );
    if (done) {
      this._clearGyroCalibrationTimer();
      const offset = Number(done[1]);
      this.setData({
        gyroCalibrating: false,
        gyroCalibrationStatus: `标定完成，gx 零偏 ${offset.toFixed(2)}`
      });
      wx.showToast({
        title: '标定完成',
        icon: 'success'
      });
      return true;
    }

    if (/^GYRO_CAL:FAIL,MOVED$/i.test(text)) {
      this._clearGyroCalibrationTimer();
      this.setData({
        gyroCalibrating: false,
        gyroCalibrationStatus: '标定失败：检测到移动，请重试'
      });
      wx.showToast({
        title: '小车发生移动',
        icon: 'none'
      });
      return true;
    }

    return false;
  },

  onAngleAccOffsetChange(e: any) {
    const val = Number(Number(e.detail.value).toFixed(1));
    this.setData({ angleAccOffset: val });
    
    this.sendString(`accOff:${val}\r\n`);
  },

  onPidChange(e: any) {
    const { type, param } = e.currentTarget.dataset;
    const val = Number(e.detail.value.toFixed(1));
    const cmd = `${type}.${param}:${val}\r\n`;
    this.sendString(cmd);
  },

  togglePlotSeries(e: any) {
    const key = e.currentTarget.dataset.key;

    if (key === 'angleAcc') {
      this.setData({ plotShowAcc: !this.data.plotShowAcc });
    } else if (key === 'angleGyro') {
      this.setData({ plotShowGyro: !this.data.plotShowGyro });
    } else if (key === 'angle') {
      this.setData({ plotShowAngle: !this.data.plotShowAngle });
    }

    this.drawPlotChart();
  },

  _parsePlotLine(text: string) {
    const m = text.match(
      /Plot[:：]\s*([+-]?(?:\d+\.?\d*|\.\d+))\s+([+-]?(?:\d+\.?\d*|\.\d+))\s+([+-]?(?:\d+\.?\d*|\.\d+))/i
    );

    if (!m) return null;

    const angleAcc = Number(m[1]);
    const angleGyro = Number(m[2]);
    const angle = Number(m[3]);

    if ([angleAcc, angleGyro, angle].some(v => Number.isNaN(v))) {
      return null;
    }

    return { angleAcc, angleGyro, angle };
  },

  _enqueueLog(line: string, timeStr: string) {
    this._pendingLogs.push({ time: timeStr, text: line });
    this._scheduleFlushLogs();
  },

  _scheduleFlushLogs() {
    if (this._flushTimer) return;

    this._flushTimer = setTimeout(() => {
      this._flushTimer = 0;

      if (this._pendingLogs.length === 0) return;

      const newLogs = this.data.receivedMsgs.concat(this._pendingLogs);
      this._pendingLogs = [];

      const finalLogs = newLogs.length > 80
        ? newLogs.slice(newLogs.length - 80)
        : newLogs;

      const lastIndex = finalLogs.length - 1;

      this.setData({
        receivedMsgs: finalLogs,
        scrollIntoViewId: lastIndex >= 0 ? `log-${lastIndex}` : ''
      });
    }, 80);
  },

  _enqueuePlotSample(sample: PlotSample) {
    this._plotSamples.push(sample);

    if (this._plotSamples.length > this._plotMaxPoints) {
      this._plotSamples = this._plotSamples.slice(-this._plotMaxPoints);
    }

    this.setData({
      plotCurrentText: `当前值  angleAcc=${sample.angleAcc.toFixed(2)}  angleGyro=${sample.angleGyro.toFixed(2)}  angle=${sample.angle.toFixed(2)}`
    });

    this._schedulePlotDraw();
  },

  _schedulePlotDraw() {
    if (this._plotDrawTimer) return;

    this._plotDrawTimer = setTimeout(() => {
      this._plotDrawTimer = 0;
      this.drawPlotChart();
    }, 16);
  },

  drawPlotChart() {
    const w = this.data.plotCanvasW;
    const h = this.data.plotCanvasH;
  
    if (!w || !h) return;
  
    const ctx = wx.createCanvasContext('plot-canvas', this);
  
    const left = 60;
    const right = 15;
    const top = 30;
    const bottom = 20;
  
    const plotW = w - left - right;
    const plotH = h - top - bottom;
  
    ctx.clearRect(0, 0, w, h);
    ctx.setFillStyle('#141414');
    ctx.fillRect(0, 0, w, h);
  
    const selected: Array<{
      key: keyof PlotSample;
      label: string;
      color: string;
      enabled: boolean;
    }> = [
      { key: 'angleAcc', label: 'angleAcc', color: '#ff4d4f', enabled: this.data.plotShowAcc },
      { key: 'angleGyro', label: 'angleGyro', color: '#40c057', enabled: this.data.plotShowGyro },
      { key: 'angle', label: 'angle', color: '#4dabf7', enabled: this.data.plotShowAngle }
    ];
  
    const active = selected.filter(s => s.enabled);
  
    if (active.length === 0) {
      ctx.setFillStyle('#888');
      ctx.setFontSize(13);
      ctx.setTextAlign('center');
      ctx.fillText('请选择要显示的曲线', w / 2, h / 2);
      ctx.draw();
      return;
    }
  
    const samples = this._plotSamples;
  
    if (samples.length === 0) {
      ctx.setFillStyle('#888');
      ctx.setFontSize(13);
      ctx.setTextAlign('center');
      ctx.fillText('等待 Plot 数据...', w / 2, h / 2);
      ctx.draw();
      return;
    }
  
    let minV = Infinity;
    let maxV = -Infinity;
  
    for (const s of samples) {
      for (const series of active) {
        const v = s[series.key];
        if (v < minV) minV = v;
        if (v > maxV) maxV = v;
      }
    }
  
    if (!isFinite(minV) || !isFinite(maxV)) {
      ctx.draw();
      return;
    }
  
    if (Math.abs(maxV - minV) < 1e-4) {
      maxV += 1.0;
      minV -= 1.0;
    } else {
      const pad = (maxV - minV) * 0.15;
      maxV += pad;
      minV -= pad;
    }
  
    ctx.setLineWidth(1);
  
    const gridCount = 4; 
    for (let i = 0; i <= gridCount; i++) {
      const ratio = i / gridCount;
      const y = top + plotH - ratio * plotH;
      const currentVal = minV + ratio * (maxV - minV);
  
      ctx.setStrokeStyle(i === 0 || i === gridCount ? '#333333' : '#222222');
      ctx.beginPath();
      ctx.moveTo(left, y);
      ctx.lineTo(left + plotW, y);
      ctx.stroke();
  
      ctx.setTextAlign('right');
      ctx.setTextBaseline('middle');
      ctx.setFontSize(10);
      ctx.setFillStyle('#888888');
      ctx.fillText(currentVal.toFixed(1), left - 8, y);
    }
  
    const xGridCount = 6;
    ctx.setStrokeStyle('#222222');
    for (let j = 1; j < xGridCount; j++) {
      const x = left + (j / xGridCount) * plotW;
      ctx.beginPath();
      ctx.moveTo(x, top);
      ctx.lineTo(x, top + plotH);
      ctx.stroke();
    }
  
    ctx.setStrokeStyle('#333333');
    ctx.beginPath();
    ctx.moveTo(left, top);
    ctx.lineTo(left, top + plotH);
    ctx.lineTo(left + plotW, top + plotH);
    ctx.stroke();
  
    const maxPoints = this._plotMaxPoints;
    const xStep = plotW / (maxPoints - 1);
    const n = samples.length;
  
    const yOf = (v: number) => {
      return top + plotH - ((v - minV) / (maxV - minV)) * plotH;
    };
  
    active.forEach((series) => {
      ctx.setStrokeStyle(series.color);
      ctx.setLineWidth(2);
      ctx.setLineJoin('round');
      ctx.beginPath();
  
      samples.forEach((s, i) => {
        const x = left + (maxPoints - n + i) * xStep;
        const y = yOf(s[series.key]);
  
        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      });
  
      ctx.stroke();
    });
  
    ctx.setTextAlign('left');
    ctx.setTextBaseline('top');
    ctx.setFontSize(11);
  
    selected.forEach((series, idx) => {
      const xPos = left + idx * 85;
      if (series.enabled) {
        ctx.setFillStyle(series.color);
        ctx.fillRect(xPos, 8, 10, 6);
        ctx.setFillStyle('#cfcfcf');
        ctx.fillText(series.label, xPos + 14, 6);
      } else {
        ctx.setFillStyle('#333333');
        ctx.fillRect(xPos, 8, 10, 6);
        ctx.setFillStyle('#555555');
        ctx.fillText(series.label, xPos + 14, 6);
      }
    });
  
    ctx.draw();
  },

  initReceiveListener() {
    try {
      if (this._bleValueChangeHandler) {
        wx.offBLECharacteristicValueChange(this._bleValueChangeHandler);
      }
    } catch (e) {}

    this._bleValueChangeHandler = (res: any) => {
      const chunkText = Array.from(new Uint8Array(res.value))
        .map((b) => String.fromCharCode(b))
        .join('');

      this._rxBuffer += chunkText;

      const parts = this._rxBuffer.split(/\r\n|\n|\r/);
      this._rxBuffer = parts.pop() || '';

      const now = new Date();
      const pad2 = (n: number) => String(n).padStart(2, '0');
      const timeStr =
        `${pad2(now.getHours())}:` +
        `${pad2(now.getMinutes())}:` +
        `${pad2(now.getSeconds())}`;

      for (const part of parts) {
        const text = part.trim();
        if (text.length === 0) continue;

        if (this._handleGyroCalibrationLine(text)) {
          this._enqueueLog(text, timeStr);
          continue;
        }

        if (!this.data.isReceiving) continue;

        const plot = this._parsePlotLine(text);
        if (plot) {
          this._enqueuePlotSample(plot);
          continue;
        }

        this._enqueueLog(text, timeStr);
      }
    };

    wx.onBLECharacteristicValueChange(this._bleValueChangeHandler);

    wx.notifyBLECharacteristicValueChange({
      state: true,
      deviceId: this._deviceId,
      serviceId: this._serviceId,
      characteristicId: this._characteristicId
    });
  },

  startConnect() {
    if (this.data.connected) {
      this.closeBLE();
      return;
    }

    wx.openBluetoothAdapter({
      success: () => {
        this.startDiscovery();
      },
      fail: () => {
        wx.showToast({
          title: '请检查蓝牙开关',
          icon: 'none'
        });
      }
    });
  },

  startDiscovery() {
    this.setData({
      isScanning: true,
      devices: []
    });

    wx.startBluetoothDevicesDiscovery({
      success: () => {
        wx.onBluetoothDeviceFound((res) => {
          res.devices.forEach((device) => {
            if (
              (device.name || device.localName) &&
              !this.data.devices.some((d) => d.deviceId === device.deviceId)
            ) {
              this.setData({
                devices: [...this.data.devices, device]
              });
            }
          });
        });
      }
    });
  },

  handleConnect(e: any) {
    const { id: deviceId } = e.currentTarget.dataset;

    wx.stopBluetoothDevicesDiscovery();

    wx.showLoading({
      title: '连接中...'
    });

    wx.createBLEConnection({
      deviceId,
      success: () => {
        this._deviceId = deviceId;
        this.getServiceAndChar(deviceId);
      },
      fail: () => {
        wx.hideLoading();
      }
    });
  },

  getServiceAndChar(deviceId: string) {
    wx.getBLEDeviceServices({
      deviceId,
      success: (res) => {
        const targetService = res.services.find(
          (s) => s.uuid.indexOf('FFE0') !== -1
        );

        const sId = targetService
          ? targetService.uuid
          : res.services[0].uuid;

        this._serviceId = sId;

        wx.getBLEDeviceCharacteristics({
          deviceId,
          serviceId: sId,
          success: (cRes) => {
            const targetChar = cRes.characteristics.find(
              (c) => c.uuid.indexOf('FFE1') !== -1
            );

            const cId = targetChar
              ? targetChar.uuid
              : cRes.characteristics[0].uuid;

            this._characteristicId = cId;

            this.setData({
              connected: true,
              statusMsg: '连接成功',
              isScanning: false
            });

            wx.hideLoading();
            wx.setBLEMTU({
              deviceId: this._deviceId,
              mtu: 128,
              complete: () => {
                this.initReceiveListener();
                this.drawPlotChart();
              }
            });
            this.initReceiveListener();
            this.drawPlotChart();
          }
        });
      }
    });
  },

  closeBLE() {
    this.stopSendTimer();
    this._clearGyroCalibrationTimer();

    try {
      if (this._bleValueChangeHandler) {
        wx.offBLECharacteristicValueChange(this._bleValueChangeHandler);
      }
    } catch (e) {}

    if (this._deviceId) {
      wx.closeBLEConnection({
        deviceId: this._deviceId
      });
    }

    if (this._flushTimer) {
      clearTimeout(this._flushTimer);
      this._flushTimer = 0;
    }

    if (this._plotDrawTimer) {
      clearTimeout(this._plotDrawTimer);
      this._plotDrawTimer = 0;
    }

    this._rxBuffer = '';
    this._pendingLogs = [];
    this._plotSamples = [];

    this.setData({
      connected: false,
      statusMsg: '蓝牙未连接',
      runFlag: 0,
      stickX: 0,
      stickY: 0,
      speed: 0,
      turn: 0,
      gyroCalibrating: false,
      gyroCalibrationStatus: '未标定（断电后需重新标定）',
      devices: [],
      receivedMsgs: [],
      scrollIntoViewId: ''
    });

    this.drawPlotChart();
  }
});
