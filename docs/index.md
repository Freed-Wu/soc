# deep-space-detection

[![readthedocs](https://shields.io/readthedocs/deep-space-detection)](https://deep-space-detection.readthedocs.io)
[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/Freed-Wu/deep-space-detection/main.svg)](https://results.pre-commit.ci/latest/github/Freed-Wu/deep-space-detection/main)
[![github/workflow](https://github.com/Freed-Wu/deep-space-detection/actions/workflows/main.yml/badge.svg)](https://github.com/Freed-Wu/deep-space-detection/actions)
[![codecov](https://codecov.io/gh/Freed-Wu/deep-space-detection/branch/main/graph/badge.svg)](https://codecov.io/gh/Freed-Wu/deep-space-detection)
[![DeepSource](https://deepsource.io/gh/Freed-Wu/deep-space-detection.svg/?show_trend=true)](https://deepsource.io/gh/Freed-Wu/deep-space-detection)

[![github/downloads](https://shields.io/github/downloads/Freed-Wu/deep-space-detection/total)](https://github.com/Freed-Wu/deep-space-detection/releases)
[![github/downloads/latest](https://shields.io/github/downloads/Freed-Wu/deep-space-detection/latest/total)](https://github.com/Freed-Wu/deep-space-detection/releases/latest)
[![github/issues](https://shields.io/github/issues/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/issues)
[![github/issues-closed](https://shields.io/github/issues-closed/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/issues?q=is%3Aissue+is%3Aclosed)
[![github/issues-pr](https://shields.io/github/issues-pr/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/pulls)
[![github/issues-pr-closed](https://shields.io/github/issues-pr-closed/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/pulls?q=is%3Apr+is%3Aclosed)
[![github/discussions](https://shields.io/github/discussions/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/discussions)
[![github/milestones](https://shields.io/github/milestones/all/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/milestones)
[![github/forks](https://shields.io/github/forks/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/network/members)
[![github/stars](https://shields.io/github/stars/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/stargazers)
[![github/watchers](https://shields.io/github/watchers/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/watchers)
[![github/contributors](https://shields.io/github/contributors/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/graphs/contributors)
[![github/commit-activity](https://shields.io/github/commit-activity/w/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/graphs/commit-activity)
[![github/last-commit](https://shields.io/github/last-commit/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/commits)
[![github/release-date](https://shields.io/github/release-date/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/releases/latest)

[![github/license](https://shields.io/github/license/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection/blob/main/LICENSE)
[![github/languages](https://shields.io/github/languages/count/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection)
[![github/languages/top](https://shields.io/github/languages/top/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection)
[![github/directory-file-count](https://shields.io/github/directory-file-count/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection)
[![github/code-size](https://shields.io/github/languages/code-size/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection)
[![github/repo-size](https://shields.io/github/repo-size/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection)
[![github/v](https://shields.io/github/v/release/Freed-Wu/deep-space-detection)](https://github.com/Freed-Wu/deep-space-detection)

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
