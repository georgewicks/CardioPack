<style>
@media print {
  @page {
    size: letter; /* 或 A4 */
    margin: 2in;  /* 可调整此值，例如 0.5in、20mm、2cm */
  }
}
</style>

# CardioPack

[English](./README.md)

CardioPack 旨在成为一个“伞形”项目，容纳多个与心脏健康相关的子项目。

促成 CardioPack 构想的子项目是 ECG 监护系统；目前，这也是其中成熟度最高的项目。

# 项目要求

这些项目使用开源工具开发。所有项目都使用了以下工具：

* **Ubuntu**。选择 Ubuntu 作为开发操作系统，是为了避开 Microsoft Windows 平台的一些使用上的不便。这并非有意贬低 Microsoft。就个人体验而言，Linux 整体上为固件开发提供了简洁、易用的界面。可从 https://ubuntu.com/ 下载 Ubuntu。
* **Git**。Git 用于软件版本控制。https://git-scm.com/
* **Microsoft Visual Studio Code**。可从 https://code.visualstudio.com/ 下载。
* **Espressif 的 ESP-IDF Visual Studio Code 扩展**。安装 Visual Studio Code 后，可通过左侧边栏查找并安装该扩展：找到由多个小方块组成、其中一个方块倾斜的图标。在“扩展”搜索框中输入 ESP-IDF，搜索结果会列出相关扩展；选择顶部名称为“**ESP-IDF**”的扩展。
* Espressif 网站上有一些教程，可按步骤指导完成必要的开发环境配置（待补充）。**Python** 也是非常重要的软件包。

我曾编写过一份完整的安装流程说明，但它已经相当过时，需要更新，可能需要借助 AI。我也可能请 AI 编写一份在 Windows 11 平台的 Ubuntu 虚拟机中安装的指南，以便维持与我当前环境相同的开发环境。

# AI 对子项目有重要贡献

如下文所述，开发过程中广泛使用了 AI。媒体对 AI 有过不少令人遗憾且不公平的报道。AI 是一种能够帮助开发工作的优秀工具。问题实际上出在人们如何控制 AI，而不在工具本身。

# ECG 监护系统概述：ESP32-ECGMonitor

目前请参阅 [ESP32-ECGMonitor 项目目录](./ESP32-ECGMonitor/)。

# ECG 模拟器概述：ESP32-ECGSimulator

目前请参阅 [ESP32-ECGSimulator 项目目录](./ESP32-ECGSimulator/)。

# 心脏健康信息相关项目

本项目旨在为与心脏健康相关的公共卫生议题提供资源指南。我认为，若干议题将成为全球各国日益关注的公共卫生问题。

污染正在通过多个方面影响全球的心脏健康。以下内容整理自对 DeepSeek 聊天助手的咨询：

* **缺血性心脏病与心脏病发作**：短期和长期接触污染物都会增加心脏病发作的风险。[^1] [^2] [^3]
* **脑卒中**：污染与脑卒中风险增加密切相关。[^4] [^5]
* **心力衰竭与心律失常**：接触污染物的影响包括心律失常风险增加，甚至可能出现心房颤动。[^6]
* **动脉粥样硬化与高血压**：（参见脚注）

## 主要污染物与生物学机制

* **主要污染物**：
  * **细颗粒物（PM2.5）**
  * **二氧化氮（NO2）**
  * **臭氧（O3）**
  * **二氧化硫（SO2）**
  * **一氧化碳（CO）**
* **危害机制**：

更深入的讨论请参阅 [与空气污染相关的主要心脏健康影响](./docs/Principle%20Cardiac%20Health%20impacts%20associated%20with%20pollution.odt)。

## 通过控制胆固醇改善心脏健康

## 微塑料与心脏健康

大量塑料废弃物导致了微塑料现象；如今，已在人群中发现微塑料。

* **微塑料与动脉粥样硬化。** 我是在接受心脏导管手术后住院期间第一次了解到这一点的。[^MicroplasticCardiac]

**请注意**：每个人都必须对自己的健康照护承担责任。我曾被处方他汀类药物来控制胆固醇水平。不过，我感觉他汀类药物似乎加重了肌肉疼痛，也让我夜间严重失眠。由于副作用严重，我停用了他汀类药物；而胆固醇水平无疑促成了我动脉中的斑块形成。幸运的是，还有其他控制胆固醇的方法。就我个人而言，我目前使用阿利西尤单抗（Alirocumab），这是一种注射剂，并非他汀类药物。（注意：请勿将我的个人记录视为医疗建议；是否适合某种治疗，应由医生判断。）

## 野火与心脏健康

2026 年夏天，来自科罗拉多州和加拿大的野火导致空气质量大幅下降。预计空气质量还会进一步恶化。

* 这也是便携、低成本 ECG 监护系统将消防员列为目标用户群体之一的原因，尤其是美国林务局雇用的“Hotshot”消防队员。他们是第一线的防护力量，也可能因面临的危险而遭受严重影响。他们承受着相当大的压力，这可能严重影响健康，尤其是心脏健康。最近为 ECG 系统加入的功能之一是三轴加速度计传感器。加入该传感器是为了改进 ECG 信号处理，例如通过运动补偿来调整基线漂移。此外，记录的数据可用于呈现患者（即消防员）的身体压力水平，以便医疗人员研究身体活动与心脏事件之间是否存在关联。对汇总数据的分析最终可能有助于更快地进行医疗干预。（注意：这意味着额外采集的身体活动数据需要存储在独立于 ISHNE 文件的另一个数据文件中。系统配有实时时钟，因此所有记录都将带有时间戳。）

进行中：一份关于中国与森林火灾的 AI 对话文档。

# 一些背景

* 我从事固件开发已有数十年，工作涉及成像系统、信号分析应用以及 GUI 开发。虽然我从未正式从事生物医学设备工作，但我一直对这个领域有浓厚兴趣。
* 我读过几本生物医学教材，其中一本一直令我特别感兴趣：《Biomedical Digital Signal Processing: C Language Examples and Laboratory Experiments for the IBM PC》，由 Willis J. Tompkins 编者。这是一本引人入胜、以实验为基础的生物医学仪器教材。书中关于如何将信号处理应用于心电图 QRS 检测的章节尤其令我感兴趣。
* 多年过去了，尽管我继续阅读相关书籍、观看相关视频，却始终没有直接参与生物医学仪器项目。2023 年，我在 67 岁时退休。

## 个人健康危机

* 2024 年 4 月，我在后院花园劳作时感到胸部剧烈压迫。邻居 Isaac 赶紧把我送到了医院。幸运的是，虽然我被诊断出有严重的心脏血管阻塞，但随后在 Evanston 医院接受了心脏导管手术。
* 几年来，我断断续续地兼职着手开发 ECG 监护仪，但并非全职投入。心脏病发作后的康复经历促使我重新把精力集中到 ECG 监护系统上。

## 已完成的工作

* 项目的核心计算平台是 Espressif ESP32。市面上有许多价格合理的开发板可供选择。主要开发 IDE 是免费的 Microsoft Visual Studio Code。Espressif 为 VSCode（Microsoft Visual Studio Code）开发了 ESP-IDF 扩展，使开发者能够在包括 Windows、macOS 和 Linux 在内的多个平台上开发和维护 ESP32 项目。

## 借助 AI 协助开发

* 我已经完成了对大多数传感器板和其他设备的初步调试。不过，由于其他健康问题，例如胃肠道问题、椎管狭窄以及严重的白内障，我的工作进度开始放缓。我不太情愿地开始尝试使用 AI 协助开发，结果令我惊喜。AI 帮助很大，也帮助我克服了身体上的限制。
* 这次与 AI 合作的成果是一个颇有前景的软件包，未来可能发展成为可扩展的平台。

# 脚注

[^MicroplasticCardiac]: 微塑料与急性冠脉综合征患者动脉粥样硬化风险升高及血管复杂性增加相关。作者：Yunxiao Yang、Feng Zhang、Zhili Jiang、Zhiyong Du、Sheng Liu、Ming Zhang、Yanyan Jin、Yanwen Qin、Xiubin Yang、Chenggang Wang 和 Hai Gao。发表于 *Particle and Fibre Toxicology*（2024），21:34。
[^1]: “空气污染、心脏病与脑卒中”，https://www.heart.org/en/health-topics/consumer-healthcare/what-is-cardiovascular-disease/air-pollution-and-heart-disease-stroke
[^2]: Laura Williamson，“空气污染暴露可能在一小时内引发心脏病发作”，https://www.heart.org/es/news/2022/04/22/air-pollution-exposure-may-cause-heart-attack-within-an-hour
[^3]: “暴露于中等程度的空气污染可能损害心脏”，https://www.rsna.org/news/2026/june/air-pollution-heart-damage#:~:text=It%20is%20one%20of%20the,to%20the%20World%20Health%20Organization.
[^4]: “每年有 700 万例过早死亡与空气污染相关”，https://www.who.int/news/item/25-03-2014-7-million-premature-deaths-annually-linked-to-air-pollution#1
[^5]: “长期暴露于低浓度空气污染会增加心脏病和肺病风险”，https://newsroom.heart.org/news/long-term-exposure-to-low-levels-of-air-pollution-increases-risk-of-heart-and-lung-disease?print=1
[^6]: “即使低浓度空气污染也会增加老年人的心肺风险”，https://www.heart.org/en/news/2021/02/22/even-low-levels-of-air-pollution-can-increase-heart-lung-risks-in-older-adults
