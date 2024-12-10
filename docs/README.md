# Progress

[![readthedocs](https://shields.io/readthedocs/soc)](https://soc.readthedocs.io)
[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/ustc-ivclab/soc/main.svg)](https://results.pre-commit.ci/latest/github/ustc-ivclab/soc/main)
[![github/workflow](https://github.com/ustc-ivclab/soc/actions/workflows/main.yml/badge.svg)](https://github.com/ustc-ivclab/soc/actions)
[![codecov](https://codecov.io/gh/ustc-ivclab/soc/branch/main/graph/badge.svg)](https://codecov.io/gh/ustc-ivclab/soc)
[![DeepSource](https://deepsource.io/gh/ustc-ivclab/soc.svg/?show_trend=true)](https://deepsource.io/gh/ustc-ivclab/soc)

[![github/downloads](https://shields.io/github/downloads/ustc-ivclab/soc/total)](https://github.com/ustc-ivclab/soc/releases)
[![github/downloads/latest](https://shields.io/github/downloads/ustc-ivclab/soc/latest/total)](https://github.com/ustc-ivclab/soc/releases/latest)
[![github/issues](https://shields.io/github/issues/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/issues)
[![github/issues-closed](https://shields.io/github/issues-closed/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/issues?q=is%3Aissue+is%3Aclosed)
[![github/issues-pr](https://shields.io/github/issues-pr/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/pulls)
[![github/issues-pr-closed](https://shields.io/github/issues-pr-closed/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/pulls?q=is%3Apr+is%3Aclosed)
[![github/discussions](https://shields.io/github/discussions/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/discussions)
[![github/milestones](https://shields.io/github/milestones/all/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/milestones)
[![github/forks](https://shields.io/github/forks/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/network/members)
[![github/stars](https://shields.io/github/stars/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/stargazers)
[![github/watchers](https://shields.io/github/watchers/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/watchers)
[![github/contributors](https://shields.io/github/contributors/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/graphs/contributors)
[![github/commit-activity](https://shields.io/github/commit-activity/w/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/graphs/commit-activity)
[![github/last-commit](https://shields.io/github/last-commit/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/commits)
[![github/release-date](https://shields.io/github/release-date/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/releases/latest)

[![github/license](https://shields.io/github/license/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc/blob/main/LICENSE)
[![github/languages](https://shields.io/github/languages/count/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc)
[![github/languages/top](https://shields.io/github/languages/top/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc)
[![github/directory-file-count](https://shields.io/github/directory-file-count/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc)
[![github/code-size](https://shields.io/github/languages/code-size/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc)
[![github/repo-size](https://shields.io/github/repo-size/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc)
[![github/v](https://shields.io/github/v/release/ustc-ivclab/soc)](https://github.com/ustc-ivclab/soc)

- [x] 导出硬件描述文件：通过 EMIO 将 FPGA 引脚连到 arm 上，提供用于串口通讯的 UART2
- [x] 在 AMD Xilinx ZynqMP UltraScale+ 上部署 Linux
- [x] 通过 systemd 自启动本软件
- [x] cmake 构建
- [x] bitbake 打包
- [x] 多线程
- [ ] 与中心机通信：先在 PC 上实现一个程序模拟中心机通过串口发送图像和指令并接受码流，将 PC 和搭载板相连，测试搭载板上的串口通讯能否工作
- [ ] 与中心机通信： fork 1 个线程通过串口接受图像和指令，将图像推入缓冲区
- [ ] 通过神经网络压缩输入图像： fork 1 个线程从缓冲区弹出最早接收的图像，处理后将码流通过串口发送出去
- [ ] 量化
- [x] 算术编码: 由 @Dongcunhui 完成
- [x] 单元测试 google test ：在本机、 CI/CD 运行
- [x] 驱动: 由 wangyang 完成

## 文档

- [图像压缩软件设计报告大纲](resources/serial-transform-protocol.md)
- [搭载板图像加速处理软件与中心机的串口传输协议](resources/serial-transform-protocol.md)
