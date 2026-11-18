<style>
@media print {
  @page {
    size: letter; /* or A4 */
    margin: 2in;  /* Adjust this value (e.g., 0.5in, 20mm, 2cm) */
  }
}
</style>

# CardioPack

[简体中文](./README.zh-CN.md)

CardioPack is intended to be an "umbrella" project that will contain potentially many sub-projects that are related to Cardiac Health.
The sub-project that initiated the idea behind CardioPack is the ECG monitoring system, and that is the one project that has the most maturity.

# Project requirements
The projects are developed with open-source tools. All of the projects were developed using the following:

* **Ubuntu**. Ubuntu was chosen as the development operating system in order to circumvent the quirkiness of the Microsoft Windows platform. (No slight against Microsoft is intended here). From a personal perspective, I have found that Linux generally speaking, has a very streamlined and easy interface for firmware development. Ubuntu can be downloaded from https://ubuntu.com/

* **Git**. Git is needed for software version control. https://git-scm.com/

* **Microsoft Visual Studio Code**. You can download it from https://code.visualstudio.com/

* **Espressif's ESP-IDF Visual Studio Code Extension**. After Visual Studio code is installed, you can retrieve & install the Extension via the Left Hand Side panel - you will find an icon with small blocks, with one of the blocks tilted on its side. In the Extensions search field, you can type ESP-IDF, and you will list of corresponding extension - the top one labeled "**ESP-IDF**" is the one to pick.

* There are tutorials available on Espressif's website that will be able to walk the user through a list of steps to bring up the necessary development (TBD). **Python** is also a very key package.

I had written up a version of the entire installation process, but it has aged considerably, and would have to be updated, probably with the assistance of AI. I may also ask AI to create an installation targeting an Ubuntu VM on the Windows 11 platform, thus maintaining an identical environment to what I currently have.

# AI is a significant contributor to the sub-projects
As I explain in detail below, AI was used extensively in the development. AI has received a lot of unfortunate unfair media coverage. AI is an excellent tool in the development process. The problems associated with AI are caused by who is controlling it, NOT with the tool itself.

# Overview of the ECG monitoring system, ESP32-ECGMonitor.
For now, please see ESP32-ECGMonitor project folder.

# Overview of the ECG simulator, ESP32-ECGSimulator.
For now, please see ESP32-ECGSimulator project folder.

# Cardiac Health information based projects.
This is intended to be a resource guide for public health issues related to cardiac health. There are several issues that I believe will come to the forefront of public health concerns that will face all countries across the globe.

Pollution has been impacting Cardiac Health across the globe in several areas. From an inquiry with DeepSeek chat:

* **Ischemic Heart Disease & Heart Attacks**: Both short-term and long-term exposures increase the risk of heart attacks. [^1],[^2],[^3]

* **Stroke**. Pollution is strongly linked to an increased risk of stroke. [^4],[^5]

* **Heart Failure & Arrhythmias**. The effects from exposure can range from increased risk of arrhythmias, to atrial fibrillation. [^6]

* **Atherosclerosis & Hypertension**. (see footnotes)

## Key Pollutants and Biological Mechanisms

* The Primary Culprits:
  * **Fine particulate matter (PM2.5)**
  * **Nitrogen dioxide (NO2)**
  * **Ozone (O3)**
  * **Sulfur dioxide (SO2)**
  * **Carbon monoxide (CO)**

* Mechanisms of Harm:


An in-depth discussion can be found in docs/Principle Cardiac Health impacts associated with pollution.odt.

## Cholesterol control to improve Cardiac Health.


## Microplastics and Cardiac Health.
The overwhelming amount of plastic waste has led to the phenomena of microplastics, and they are now found within the human population.

* Microplastics and Atherosclerosis. I first learned about this while I was in the hospital after undergoing a cardiac catheterization. [^MicroplasticCardiac]. 
**Please Note**: Personal responsibility: Every person must take for their health care. I was prescribed statins to control my cholesterol levels. However, the statins seemed to increase muscle pains and caused me to lose a great deal of sleep at night. I discontinued the statins due to the severe side effects, and the cholesterol levels undoubtedly contributed to the plaque buildup in my arteries. Fortunately, there are better ways to control cholesterol. In my case, I am using Alirocumab, an injectable, which is not a statin. (NOTE: Do not take my personal notes to be medical advice - your physician is the best judge).

## Wildfires and Cardiac Health.
This past summer (2026) witnessed a severe drop in Air Quality due to wild fires from Colorado and Canada. Air quality is expected to only get worse.

* This is why one of the target audiences for a portable and cheap ECG monitoring system are fire fighters, especially those in the "Hotshot" crews employed by the US Forest Service. They are our first line of defense, and can suffer greatly due to the dangers present. They are under considerable distress, that can severely impact their health, especially their heart health. One of the additional features for the ECG system that recently included is the 3-axis accelerometer sensor. This was added because we wanted to improve upon ECG signal processing - adjusting baseline wander using motion compensation. Besides that, the data recordings will provide data to show the physical stress levels for the patient, i.e., the fire fighter, so that medical personnel can make studies to see if there is a correlation between the physical events and the cardiac system events. Compiled data could eventually lead to quicker medical intervention, based on the analysis of the accumulated data. (NOTE: This will mean that the extra physical data collected will need to be stored in a separate data file, apart from the ISHNE file - there is a real-time clock on the system, so they will all have timestamped records). 
In progress: an AI chat document on China and forest fires.


# A little bit of history

* I have been working in firmware for a few decades, working on imaging systems, signal analysis applications, along with GUI development. While I never worked formally on biomedical devices, there was always a very strong interest in that development work. 

* I had read several biomedical textbooks, and one of which has always held great interest for me, namely "Biomedical Digital Signal Processing: C Language Examples and Laboratory Experiments for the IBM PC", edited by  Willis J. Tompkins. This is a fascinating lab based experimenters book on biomedical instrumentation. The section on how signal processing applied to QRS detection for electrocardiography held particular interest for me.

* Years passed, and while I continued to review books and videos on the subject, I was never directly involved in biomedical instrumentation projects, per se. I retired in 2023, at the age of 67.

## Personal health crisis

* In April of 2024, while working in the garden in my backyard, I developed crushing chest sensations, and my neighbor, Isaac, rushed me to the hospital. Fortunately, while I was diagnosed with a serious heart blockage, I then received Cardiac Catheterization at Evanston Hospital. 

* Now, I had been working part-time on the beginnings of a ECG monitor on and off for a few years, but certainly not on a full time basis. Post heart attack recovery led me to refocus my energies on the ECG monitoring system,

## What I had been able to accomplish

* At the core of the project, the computing platform that I was working on is the Espressif ESP32 platform. There are many development boards that are available, and they are all reasonably priced. The chief development IDE is Microsoft Visual Studio Code, which is freely available. Espressif has created the ESP-IDF extension for VSCode (Microsoft Visual Studio Code), which enables the developer to create and maintain ESP32 development on many platforms, including Windows, macOS, as well as Linux based operating systems.

## Getting AI to assist me in development.

* I had been able to shake out most of the various sensor boards and other devices, however, due to other health related issues, such as GI problems, spinal stenosis issues, and then severe cataract problems, my work was then slowing down. I reluctantly decided to look into the use of AI assistance, and I was pleasantly surprised by it. It has helped immensely which helped me overcome physical disabilities

* The result of this AI interaction led me to a rather promising package that may prove to be a good platform that can be expanded upon.

# Footnotes

[^MicroplasticCardiac]: Microplastics are associated with elevated atherosclerotic risk and increased vascular complexity in acute coronary syndrome patients. Yunxiao Yang, Feng Zhang, Zhili Jiang, Zhiyong Du, Sheng Liu, Ming Zhang, Yanyan Jin, Yanwen Qin,
Xiubin Yang, Chenggang Wang and Hai Gao. Particle and Fibre Toxicology. (2024) 21:34
[^1]: "Air Pollution, Heart Disease and Stroke", https://www.heart.org/en/health-topics/consumer-healthcare/what-is-cardiovascular-disease/air-pollution-and-heart-disease-stroke
[^2]: "Air pollution exposure may cause heart attack within an hour", Laura Williamson, https://www.heart.org/es/news/2022/04/22/air-pollution-exposure-may-cause-heart-attack-within-an-hour
[^3]: "Exposure to Moderate Levels of Air Pollution May Cause Heart Damage", https://www.rsna.org/news/2026/june/air-pollution-heart-damage#:~:text=It%20is%20one%20of%20the,to%20the%20World%20Health%20Organization.
[^4]: "7 million premature deaths annually linked to air pollution", https://www.who.int/news/item/25-03-2014-7-million-premature-deaths-annually-linked-to-air-pollution#1
[^5]: "Long-term exposure to low levels of air pollution increases risk of heart and lung disease", https://newsroom.heart.org/news/long-term-exposure-to-low-levels-of-air-pollution-increases-risk-of-heart-and-lung-disease?print=1
[^6]: "Even low levels of air pollution can increase heart, lung risks in older adults", https://www.heart.org/en/news/2021/02/22/even-low-levels-of-air-pollution-can-increase-heart-lung-risks-in-older-adults
