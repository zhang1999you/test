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

interface BleWriteItem {
  text: string;
  onSuccess?: () => void;
  onFail?: () => void;
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
    runCommandPending: false,
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
    logScrollTop: 0,

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
  _logScrollTopValue: 0,
  _logAutoScrollMinItems: 10,
  _bleValueChangeHandler: null as any,
  _bluetoothDeviceFoundHandler: null as any,
  _bluetoothAdapterOpen: false,
  _bleClosing: false,

  _plotSamples: [] as PlotSample[],
  _plotDrawTimer: 0 as any,
  _plotMaxPoints: 100,
  _plotCanvasNode: null as any,
  _plotCanvasContext: null as any,
  _plotCanvasDpr: 1,
  _gyroCalibrationTimer: 0 as any,
  _gyroCommandAckTimer: 0 as any,

  _controlWriteQueue: [] as BleWriteItem[],
  _normalWriteQueue: [] as BleWriteItem[],
  _pendingMotionWrite: null as BleWriteItem | null,
  _bleWriteBusy: false,
  _bleWriteTimeout: 0 as any,
  _bleWriteGeneration: 0,

  _commandSequence: 0,
  _runAckTimer: 0 as any,
  _runCommandId: 0,
  _runCommandTarget: -1,
  _runCommandAttempts: 0,
  _runCommandOnConfirmed: null as (() => void) | null,
  _runCommandOnFailed: null as (() => void) | null,

  _gyroCommandId: 0,
  _gyroCommandAttempts: 0,

  _motionStopAckTimer: 0 as any,
  _motionStopId: 0,
  _motionStopAttempts: 0,

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
    this._cancelMotionStopCommand();
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
        wx.nextTick(() => this.initPlotCanvasNode());
      }
    );
  },

  initPlotCanvasNode() {
    if (!this.data.connected) return;

    wx.createSelectorQuery()
      .select('#plot-canvas')
      .fields({ node: true, size: true }, (res: any) => {
        if (!res || !res.node || !res.width || !res.height) return;

        const canvas = res.node;
        const sys = wx.getSystemInfoSync();
        const dpr = sys.pixelRatio || 1;
        const ctx = canvas.getContext('2d');

        canvas.width = Math.round(res.width * dpr);
        canvas.height = Math.round(res.height * dpr);
        ctx.scale(dpr, dpr);

        this._plotCanvasNode = canvas;
        this._plotCanvasContext = ctx;
        this._plotCanvasDpr = dpr;
        this.drawPlotChart();
      })
      .exec();
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

    // 清除尚未发出的旧摇杆帧，改发带 ACK 的高优先级停车命令。
    this._pendingMotionWrite = null;
    this._sendMotionStopCommand();
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
    // 摇杆帧只保留最新值，避免 BLE 短时拥塞后继续执行过期动作。
    this._pendingMotionWrite = { text: cmd };
    this._drainBleWriteQueue();
  },

  sendString(
    str: string,
    onSuccess?: () => void,
    onFail?: () => void,
    highPriority = false
  ) {
    if (!this.data.connected) {
      if (onFail) onFail();
      return;
    }

    const item: BleWriteItem = { text: str, onSuccess, onFail };
    if (highPriority) {
      this._controlWriteQueue.push(item);
    } else {
      this._normalWriteQueue.push(item);
    }
    this._drainBleWriteQueue();
  },

  _stringToBuffer(str: string) {
    const buffer = new ArrayBuffer(str.length);
    const dataView = new DataView(buffer);

    for (let i = 0; i < str.length; i++) {
      dataView.setUint8(i, str.charCodeAt(i));
    }

    return buffer;
  },

  _drainBleWriteQueue() {
    if (this._bleWriteBusy || !this.data.connected) return;

    const item = this._controlWriteQueue.shift() ||
      this._pendingMotionWrite ||
      this._normalWriteQueue.shift();

    if (!item) return;

    if (item === this._pendingMotionWrite) {
      this._pendingMotionWrite = null;
    }

    this._bleWriteBusy = true;
    const generation = this._bleWriteGeneration;
    let settled = false;

    const finish = (success: boolean) => {
      if (settled) return;
      settled = true;

      // 连接已经关闭或重建时，忽略上一条连接迟到的回调。
      if (generation !== this._bleWriteGeneration) return;

      if (this._bleWriteTimeout) {
        clearTimeout(this._bleWriteTimeout);
        this._bleWriteTimeout = 0;
      }

      this._bleWriteBusy = false;
      if (success) {
        if (item.onSuccess) item.onSuccess();
      } else if (item.onFail) {
        item.onFail();
      }

      this._drainBleWriteQueue();
    };

    // 防止极少数情况下微信 BLE API 不回调，导致整个发送队列永久卡死。
    this._bleWriteTimeout = setTimeout(() => finish(false), 1200);

    wx.writeBLECharacteristicValue({
      deviceId: this._deviceId,
      serviceId: this._serviceId,
      characteristicId: this._characteristicId,
      value: this._stringToBuffer(item.text),
      success: () => finish(true),
      fail: () => finish(false)
    });
  },

  _nextCommandId() {
    this._commandSequence = (this._commandSequence % 65535) + 1;
    return this._commandSequence;
  },

  _clearMotionStopAckTimer() {
    if (this._motionStopAckTimer) {
      clearTimeout(this._motionStopAckTimer);
      this._motionStopAckTimer = 0;
    }
  },

  _removeQueuedMotionStop(commandId: number) {
    const commandText = `X,${commandId}\n`;
    this._controlWriteQueue = this._controlWriteQueue.filter(
      (item) => item.text !== commandText
    );
  },

  _cancelMotionStopCommand() {
    if (this._motionStopId !== 0) {
      this._removeQueuedMotionStop(this._motionStopId);
    }
    this._clearMotionStopAckTimer();
    this._motionStopId = 0;
    this._motionStopAttempts = 0;
  },

  _attemptMotionStopCommand() {
    if (this._motionStopId === 0 || !this.data.connected) return;

    if (this._motionStopAttempts >= 3) {
      // 兼容尚未升级的旧固件；新固件还有 500ms 遥控看门狗兜底。
      this.sendString('s:0,t:0\r\n', undefined, undefined, true);
      this._cancelMotionStopCommand();
      return;
    }

    this._motionStopAttempts += 1;
    this._clearMotionStopAckTimer();
    const commandId = this._motionStopId;

    this.sendString(
      `X,${commandId}\n`,
      () => {
        if (this._motionStopId !== commandId) return;
        this._motionStopAckTimer = setTimeout(() => {
          this._motionStopAckTimer = 0;
          this._attemptMotionStopCommand();
        }, 500);
      },
      () => {
        if (this._motionStopId !== commandId) return;
        this._motionStopAckTimer = setTimeout(() => {
          this._motionStopAckTimer = 0;
          this._attemptMotionStopCommand();
        }, 100);
      },
      true
    );
  },

  _sendMotionStopCommand() {
    if (!this.data.connected) return;

    this._cancelMotionStopCommand();
    this._motionStopId = this._nextCommandId();
    this._motionStopAttempts = 0;
    this._attemptMotionStopCommand();
  },

  _handleMotionStopAckLine(text: string) {
    const ack = text.match(/^A,X,(\d+)$/i);
    if (!ack) return false;

    const commandId = Number(ack[1]);
    if (commandId === this._motionStopId) {
      this._removeQueuedMotionStop(commandId);
      this._clearMotionStopAckTimer();
      this._motionStopId = 0;
      this._motionStopAttempts = 0;
    }
    return true;
  },

  _clearRunAckTimer() {
    if (this._runAckTimer) {
      clearTimeout(this._runAckTimer);
      this._runAckTimer = 0;
    }
  },

  _resetRunCommandState() {
    this._clearRunAckTimer();
    this._runCommandId = 0;
    this._runCommandTarget = -1;
    this._runCommandAttempts = 0;
    this._runCommandOnConfirmed = null;
    this._runCommandOnFailed = null;
    this.setData({ runCommandPending: false });
  },

  _failRunCommand(message: string) {
    const onFailed = this._runCommandOnFailed;
    const failedTarget = this._runCommandTarget;
    const commandText = `R,${this._runCommandId},${this._runCommandTarget}\n`;
    this._controlWriteQueue = this._controlWriteQueue.filter(
      (item) => item.text !== commandText
    );
    this._resetRunCommandState();

    // 启动命令没有收到确认时按安全侧处理，再补发一次停车命令。
    if (failedTarget === 1 && this.data.connected) {
      const stopId = this._nextCommandId();
      this.sendString(`R,${stopId},0\n`, undefined, undefined, true);
      this.setData({ runFlag: 0 });
    }

    if (onFailed) {
      onFailed();
    } else {
      wx.showToast({ title: message, icon: 'none' });
    }
  },

  _attemptRunCommand() {
    if (this._runCommandId === 0) return;

    if (!this.data.connected) {
      this._failRunCommand('蓝牙连接已断开');
      return;
    }

    if (this._runCommandAttempts >= 3) {
      this._failRunCommand('小车未确认运行命令');
      return;
    }

    this._runCommandAttempts += 1;
    this._clearRunAckTimer();
    const commandId = this._runCommandId;
    this.sendString(
      `R,${commandId},${this._runCommandTarget}\n`,
      () => {
        if (this._runCommandId !== commandId) return;
        this._runAckTimer = setTimeout(() => {
          this._runAckTimer = 0;
          this._attemptRunCommand();
        }, 700);
      },
      () => {
        if (this._runCommandId !== commandId) return;
        this._runAckTimer = setTimeout(() => {
          this._runAckTimer = 0;
          this._attemptRunCommand();
        }, 100);
      },
      true
    );
  },

  _sendRunCommand(
    target: number,
    onConfirmed?: () => void,
    onFailed?: () => void
  ) {
    if (this.data.runCommandPending || !this.data.connected) {
      if (onFailed) onFailed();
      return;
    }

    this._runCommandId = this._nextCommandId();
    this._runCommandTarget = target ? 1 : 0;
    this._runCommandAttempts = 0;
    this._runCommandOnConfirmed = onConfirmed || null;
    this._runCommandOnFailed = onFailed || null;
    this.setData({ runCommandPending: true });
    this._attemptRunCommand();
  },

  _handleRunAckLine(text: string) {
    const compact = text.match(/^A,R,(\d+),([01])$/i);
    const legacy = text.match(/^ACK:RUN:([01])$/i);
    if (!compact && !legacy) return false;

    if (this._runCommandId === 0) return true;

    if (compact && Number(compact[1]) !== this._runCommandId) {
      return true;
    }

    const actual = Number(compact ? compact[2] : legacy![1]);
    const expected = this._runCommandTarget;
    const commandText = `R,${this._runCommandId},${expected}\n`;
    const onConfirmed = this._runCommandOnConfirmed;
    const onFailed = this._runCommandOnFailed;

    this._controlWriteQueue = this._controlWriteQueue.filter(
      (item) => item.text !== commandText
    );
    this._resetRunCommandState();
    this.setData({ runFlag: actual });

    if (actual === expected) {
      if (onConfirmed) onConfirmed();
    } else if (onFailed) {
      onFailed();
    } else {
      wx.showToast({
        title: expected ? '标定中，无法启动' : '停止命令未生效',
        icon: 'none'
      });
    }
    return true;
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

    if (!this.data.connected || this.data.runCommandPending) return;

    const next = this.data.runFlag ? 0 : 1;
    if (!next) {
      this.stopSendTimer();
      this._pendingMotionWrite = null;
      this.setData({ stickX: 0, stickY: 0, speed: 0, turn: 0 });
    }

    this._sendRunCommand(next, () => {
      wx.showToast({
        title: next ? '小车已开始运行' : '小车已停止',
        icon: 'none'
      });
    });
  },

  _clearGyroCalibrationTimer() {
    if (this._gyroCalibrationTimer) {
      clearTimeout(this._gyroCalibrationTimer);
      this._gyroCalibrationTimer = 0;
    }
  },

  _clearGyroCommandAckTimer() {
    if (this._gyroCommandAckTimer) {
      clearTimeout(this._gyroCommandAckTimer);
      this._gyroCommandAckTimer = 0;
    }
  },

  _failGyroCalibrationCommand(status: string, toast: string) {
    const commandText = `C,${this._gyroCommandId}\n`;
    this._controlWriteQueue = this._controlWriteQueue.filter(
      (item) => item.text !== commandText
    );
    this._clearGyroCommandAckTimer();
    this._clearGyroCalibrationTimer();
    this._gyroCommandId = 0;
    this._gyroCommandAttempts = 0;
    this.setData({
      gyroCalibrating: false,
      gyroCalibrationStatus: status
    });
    wx.showToast({ title: toast, icon: 'none' });
  },

  _attemptGyroCalibrationCommand() {
    if (this._gyroCommandId === 0) return;

    if (!this.data.connected) {
      this._failGyroCalibrationCommand('连接已断开，标定取消', '连接已断开');
      return;
    }

    if (this._gyroCommandAttempts >= 3) {
      this._failGyroCalibrationCommand(
        '标定命令无响应，请保持静止后重试',
        '标定命令无响应'
      );
      return;
    }

    this._gyroCommandAttempts += 1;
    this._clearGyroCommandAckTimer();
    const commandId = this._gyroCommandId;
    this.sendString(
      `C,${commandId}\n`,
      () => {
        if (this._gyroCommandId !== commandId) return;
        this._gyroCommandAckTimer = setTimeout(() => {
          this._gyroCommandAckTimer = 0;
          this._attemptGyroCalibrationCommand();
        }, 1200);
      },
      () => {
        if (this._gyroCommandId !== commandId) return;
        this._gyroCommandAckTimer = setTimeout(() => {
          this._gyroCommandAckTimer = 0;
          this._attemptGyroCalibrationCommand();
        }, 100);
      },
      true
    );
  },

  _acceptGyroCalibrationCommand(isBusy: boolean) {
    const commandText = `C,${this._gyroCommandId}\n`;
    this._controlWriteQueue = this._controlWriteQueue.filter(
      (item) => item.text !== commandText
    );
    this._clearGyroCommandAckTimer();
    this._clearGyroCalibrationTimer();
    this.setData({
      gyroCalibrating: true,
      gyroCalibrationStatus: isBusy
        ? '设备正在标定，请继续保持静止'
        : '正在静置并采样，请勿移动小车'
    });

    this._gyroCalibrationTimer = setTimeout(() => {
      this._gyroCalibrationTimer = 0;
      this._failGyroCalibrationCommand(
        '标定过程超时，请保持静止后重试',
        '标定超时'
      );
    }, 10000);
  },

  startGyroCalibration() {
    if (
      !this.data.connected ||
      this.data.gyroCalibrating ||
      this.data.runCommandPending
    ) return;

    this.stopSendTimer();
    this._clearGyroCalibrationTimer();
    this._clearGyroCommandAckTimer();
    this._pendingMotionWrite = null;
    this.setData({
      stickX: 0,
      stickY: 0,
      speed: 0,
      turn: 0,
      isReceiving: true,
      gyroCalibrating: true,
      gyroCalibrationStatus: '正在停车，请固定小车'
    });

    this._sendRunCommand(
      0,
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
              gyroCalibrationStatus: '正在发送标定命令...'
            });

            this._gyroCommandId = this._nextCommandId();
            this._gyroCommandAttempts = 0;
            this._attemptGyroCalibrationCommand();
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
    const compact = text.match(
      /^A,C,(\d+),(S|B|D|F)(?:,([+-]?(?:\d+(?:\.\d*)?|\.\d+)|MOVED))?$/i
    );
    if (compact) {
      const commandId = Number(compact[1]);
      if (commandId !== this._gyroCommandId) return true;

      const state = compact[2].toUpperCase();
      if (state === 'S' || state === 'B') {
        this._acceptGyroCalibrationCommand(state === 'B');
        return true;
      }

      this._clearGyroCommandAckTimer();
      this._clearGyroCalibrationTimer();
      this._gyroCommandId = 0;
      this._gyroCommandAttempts = 0;

      if (state === 'D') {
        const offset = Number(compact[3]);
        this.setData({
          gyroCalibrating: false,
          gyroCalibrationStatus: `标定完成，gx 零偏 ${offset.toFixed(2)}`
        });
        wx.showToast({ title: '标定完成', icon: 'success' });
      } else {
        this.setData({
          gyroCalibrating: false,
          gyroCalibrationStatus: '标定失败：检测到移动，请重试'
        });
        wx.showToast({ title: '小车发生移动', icon: 'none' });
      }
      return true;
    }

    if (/^GYRO_CAL:START$/i.test(text)) {
      this._acceptGyroCalibrationCommand(false);
      this.setData({
        gyroCalibrating: true,
        gyroCalibrationStatus: '正在静置并采样，请勿移动小车'
      });
      return true;
    }

    if (/^GYRO_CAL:BUSY$/i.test(text)) {
      this._acceptGyroCalibrationCommand(true);
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
      this._clearGyroCommandAckTimer();
      this._clearGyroCalibrationTimer();
      this._gyroCommandId = 0;
      this._gyroCommandAttempts = 0;
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
      this._clearGyroCommandAckTimer();
      this._clearGyroCalibrationTimer();
      this._gyroCommandId = 0;
      this._gyroCommandAttempts = 0;
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

      const updateData: Record<string, any> = {
        receivedMsgs: finalLogs
      };

      if (finalLogs.length >= this._logAutoScrollMinItems) {
        // 使用单调递增的 scroll-top，避免安卓在内容未满时反复重新定位。
        this._logScrollTopValue += 1000;
        updateData.logScrollTop = this._logScrollTopValue;
      }

      // 日志框未满时不更新 scroll-top，避免安卓把内层滚动变化传递给主页面。
      this.setData(updateData);
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
  
    if (!w || !h || !this._plotCanvasContext) return;
  
    const ctx = this._plotCanvasContext as any;
  
    const left = 60;
    const right = 15;
    const top = 30;
    const bottom = 20;
  
    const plotW = w - left - right;
    const plotH = h - top - bottom;
  
    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = '#141414';
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
      ctx.fillStyle = '#888';
      ctx.font = '13px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText('请选择要显示的曲线', w / 2, h / 2);
      return;
    }
  
    const samples = this._plotSamples;
  
    if (samples.length === 0) {
      ctx.fillStyle = '#888';
      ctx.font = '13px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText('等待 Plot 数据...', w / 2, h / 2);
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
  
    ctx.lineWidth = 1;
  
    const gridCount = 4; 
    for (let i = 0; i <= gridCount; i++) {
      const ratio = i / gridCount;
      const y = top + plotH - ratio * plotH;
      const currentVal = minV + ratio * (maxV - minV);
  
      ctx.strokeStyle = i === 0 || i === gridCount ? '#333333' : '#222222';
      ctx.beginPath();
      ctx.moveTo(left, y);
      ctx.lineTo(left + plotW, y);
      ctx.stroke();
  
      ctx.textAlign = 'right';
      ctx.textBaseline = 'middle';
      ctx.font = '10px sans-serif';
      ctx.fillStyle = '#888888';
      ctx.fillText(currentVal.toFixed(1), left - 8, y);
    }
  
    const xGridCount = 6;
    ctx.strokeStyle = '#222222';
    for (let j = 1; j < xGridCount; j++) {
      const x = left + (j / xGridCount) * plotW;
      ctx.beginPath();
      ctx.moveTo(x, top);
      ctx.lineTo(x, top + plotH);
      ctx.stroke();
    }
  
    ctx.strokeStyle = '#333333';
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
      ctx.strokeStyle = series.color;
      ctx.lineWidth = 2;
      ctx.lineJoin = 'round';
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
  
    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';
    ctx.font = '11px sans-serif';
  
    selected.forEach((series, idx) => {
      const xPos = left + idx * 85;
      if (series.enabled) {
        ctx.fillStyle = series.color;
        ctx.fillRect(xPos, 8, 10, 6);
        ctx.fillStyle = '#cfcfcf';
        ctx.fillText(series.label, xPos + 14, 6);
      } else {
        ctx.fillStyle = '#333333';
        ctx.fillRect(xPos, 8, 10, 6);
        ctx.fillStyle = '#555555';
        ctx.fillText(series.label, xPos + 14, 6);
      }
    });
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

        if (
          this._handleMotionStopAckLine(text) ||
          this._handleRunAckLine(text) ||
          this._handleGyroCalibrationLine(text)
        ) {
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

  _removeBluetoothDeviceFoundListener() {
    if (!this._bluetoothDeviceFoundHandler) return;

    try {
      wx.offBluetoothDeviceFound(this._bluetoothDeviceFoundHandler);
    } catch (e) {}
    this._bluetoothDeviceFoundHandler = null;
  },

  _stopBluetoothDiscovery(onComplete?: () => void) {
    this._removeBluetoothDeviceFoundListener();

    wx.stopBluetoothDevicesDiscovery({
      complete: () => {
        this.setData({ isScanning: false });
        if (onComplete) onComplete();
      }
    });
  },

  startConnect() {
    if (this._bleClosing) {
      wx.showToast({ title: '蓝牙正在重置，请稍候', icon: 'none' });
      return;
    }

    if (this.data.connected) {
      this.closeBLE();
      return;
    }

    if (this.data.isScanning) {
      this._stopBluetoothDiscovery();
      return;
    }

    wx.openBluetoothAdapter({
      success: () => {
        this._bluetoothAdapterOpen = true;
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
    this._stopBluetoothDiscovery(() => {
      this.setData({
        isScanning: true,
        devices: [],
        statusMsg: '正在搜索蓝牙设备'
      });

      this._bluetoothDeviceFoundHandler = (res: any) => {
        const found = res.devices.filter(
          (device: any) => device.name || device.localName
        );
        if (found.length === 0) return;

        const byId: { [key: string]: any } = {};
        this.data.devices.forEach((device: any) => {
          byId[device.deviceId] = device;
        });
        found.forEach((device: any) => {
          byId[device.deviceId] = device;
        });
        this.setData({ devices: Object.keys(byId).map(id => byId[id]) });
      };

      wx.onBluetoothDeviceFound(this._bluetoothDeviceFoundHandler);

      wx.startBluetoothDevicesDiscovery({
        allowDuplicatesKey: false,
        success: () => {},
        fail: () => {
          this._removeBluetoothDeviceFoundListener();
          this.setData({
            isScanning: false,
            statusMsg: '搜索失败，请重新打开蓝牙'
          });
          wx.showToast({ title: '蓝牙搜索失败', icon: 'none' });
        }
      });
    });
  },

  handleConnect(e: any) {
    const { id: deviceId } = e.currentTarget.dataset;

    this._stopBluetoothDiscovery();

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
        this.setData({ statusMsg: '连接失败，请重新搜索' });
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
            this.initReceiveListener();
            wx.nextTick(() => this.initPlotCanvasNode());

            /* MTU 调整是优化项，初始化监听/画布不依赖其回调。 */
            wx.setBLEMTU({
              deviceId: this._deviceId,
              mtu: 128
            });
          }
        });
      }
    });
  },

  closeBLE() {
    if (this._bleClosing) return;
    this._bleClosing = true;
    this.setData({ statusMsg: '正在断开并重置蓝牙...' });

    this.stopSendTimer();
    this._cancelMotionStopCommand();
    this._clearRunAckTimer();
    this._clearGyroCalibrationTimer();
    this._clearGyroCommandAckTimer();

    if (this._bleWriteTimeout) {
      clearTimeout(this._bleWriteTimeout);
      this._bleWriteTimeout = 0;
    }

    this._controlWriteQueue = [];
    this._normalWriteQueue = [];
    this._pendingMotionWrite = null;
    this._bleWriteBusy = false;
    this._bleWriteGeneration += 1;
    this._runCommandId = 0;
    this._runCommandTarget = -1;
    this._runCommandAttempts = 0;
    this._runCommandOnConfirmed = null;
    this._runCommandOnFailed = null;
    this._gyroCommandId = 0;
    this._gyroCommandAttempts = 0;
    this._motionStopId = 0;
    this._motionStopAttempts = 0;
    this._plotCanvasNode = null;
    this._plotCanvasContext = null;
    this._logScrollTopValue = 0;

    this._stopBluetoothDiscovery();

    try {
      if (this._bleValueChangeHandler) {
        wx.offBLECharacteristicValueChange(this._bleValueChangeHandler);
      }
    } catch (e) {}

    if (this._deviceId) {
      wx.closeBLEConnection({
        deviceId: this._deviceId,
        complete: () => {
          this._deviceId = '';
          this._serviceId = '';
          this._characteristicId = '';

          if (this._bluetoothAdapterOpen) {
            wx.closeBluetoothAdapter({
              complete: () => {
                this._bluetoothAdapterOpen = false;
                this._bleClosing = false;
                this.setData({ statusMsg: '蓝牙已断开，可重新搜索' });
              }
            });
          } else {
            this._bleClosing = false;
          }
        }
      });
    } else if (this._bluetoothAdapterOpen) {
      wx.closeBluetoothAdapter({
        complete: () => {
          this._bluetoothAdapterOpen = false;
          this._bleClosing = false;
          this.setData({ statusMsg: '蓝牙已断开，可重新搜索' });
        }
      });
    } else {
      this._bleClosing = false;
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
      isScanning: false,
      runFlag: 0,
      runCommandPending: false,
      stickX: 0,
      stickY: 0,
      speed: 0,
      turn: 0,
      gyroCalibrating: false,
      gyroCalibrationStatus: '未标定（断电后需重新标定）',
      devices: [],
      receivedMsgs: [],
      scrollIntoViewId: '',
      logScrollTop: 0
    });
  }
});
