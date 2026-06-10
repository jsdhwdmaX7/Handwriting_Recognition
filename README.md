# Handwriting_Recognition
Handwriting_Recognition嵌入式系统大作业

基于 STM32F767 与 X-CUBE-AI 的手写数字识别系统。

## 项目简介

本项目通过训练 MNIST 手写数字识别模型，并通过 X-CUBE-AI 将模型部署到 STM32F767 开发板，实现嵌入式端的数字识别功能。

## 项目结构

```text
Handwriting_Recognition/    STM32工程源码
MNIST/                      数据集
model.py                    模型训练代码
model.h5                    训练完成模型
嵌入式大作业报告.docx        实验报告
演示视频.mp4                演示视频
```

## 开发环境

- STM32F767
- STM32CubeMX
- Keil MDK
- X-CUBE-AI
- Python
- TensorFlow / Keras

## 功能实现

- MNIST数据集训练
- X-CUBE-AI模型转换
- STM32端推理部署
- 串口输出识别结果
