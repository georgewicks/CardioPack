<style>
@media print {
  @page {
    size: letter; /* or A4 */
    margin: 2in;  /* Adjust this value (e.g., 0.5in, 20mm, 2cm) */
  }
}
</style>

# Your Document Heading Starts Here
This text will now respect the 1-inch margins when the print preview renders.

# CardioPack
CardioPack is intended to be an "umbrella" project that will contain potentially many sub-projects that are related to Cardiac Health.
The sub-project that initiated the idea behind CardioPack is the ECG monitoring system.

# Project requirments
THe projects are developed with open-source tools. All of the projects were developed using the following:
* Ubuntu 24.04. Ubuntu was chosen as the development operating system in order to circumvent the quirkiness of the Microsoft Windows platform. No slight against Microsoft is intended here. From personal perspective, I have that Linux in general has a very streamlined and easy interface for firmware development. Ubuntu can be downloaded from https://ubuntu.com/
* Git. Git is needed for software version control. https://git-scm.com/
* Microsoft Visual Studio Code. You can download it from https://code.visualstudio.com/
* Espressif's ESP-IDF Visual Studio Code Extension. After Visual Studio code is installed, you can retrieve & install the Extension via the Left Hand Side panel - you will find an icon with small blocks, with one of the blocks tilted on its side. In the Extensions search field, you can type ESP-IDF, and you will list of corresponding extension - the top one labeled "ESP-IDF" is the one to pick.
* There are tutorials available from the Espressif website that will be able to walk the user through a list of steps to do in order to bring up the necessary development (TBD).

# AI is a significant contributor to the sub-projects
As explained in detail below, AI was used extensively in the development. AI has received a unfortunate and unfair media coverage. AI is an excellent tool that can benefit the development process. The problems associated with AI are actually caused by who is controlling it, NOT with the tool itself.

# Overview of the ECG monitoring system, ESP32-ECGMonitor.

# Overview of the ECG simulator, ESP32-ECGSimulator.

# Cardiac Health information based projects.
This is intended to be a resource guide for public health issues related to cardiac health. There are several issues that I believe will come to the forefront of public health concerns that will face all countries across the globe.

Pollution has been impacting Cardiac Health across the globe in several areas. 

## Microplastics and Cardiac Health.
The overwhelming amount of plastic waste has led to the phenomena of microplastics, and they are now found in within the human population.
* Microplastics and Atherosclerosis. I first learned about this while I was in the hospital recovering after the cardiac catheterization. [^MicroplasticCardiac]

## Wildfires and Cardiac Health.
This past summer has witnessed a severe drop in Air Quality for the Midwest region due to wild fires from Colorado and Canada. It is expected to only get worse.
One of the target audiences for a portable and cheap ECG monitoring system are the fire fighters, especially those in the "Hotshot" crews employed by the US Forest Service. They are our first line of defence, and they can suffer greatly due to the dangers present. They are under considerable distress, that can severely impact their health, especially their heart health. 

# A little bit of history
* I have been working in firmware for a few decades, working on imaging systems, signal analysis applications, along with GUI development. While I never worked formally on biomedical devices, there was always a very strong interest in that development work. 
* I had read several biomedical textbooks, and one of which has always held great interest for me, namely "Biomedical Digital Signal Processing: C Language Examples and Laboratory Experiments for the IBM PC", edited by  Willis J. Tompkins. This is a fascinating lab based experimenters book on biomedical instrumentation. The section on how signal processing applied to QRS detection for electrocardiography held particular interest for me.
* Years passed, and while I continued to review books and videos on the subject, I was never directly involved in biomedical instrumentation projects, per se. I retired in 2023, at the age of 67.

## Personal health crisis
* In April of 2024, while working in the garden in my backyard, I developed crushing chest sensations, and my neighbor, Isaac, rushed me to the hospital. Fortunately, I was diagnosed with serious heart blockage and I received Cardiac Catheterization at Evanston Hospital. 
* I had been working part-time on the beginnings of a ECG monitor on and off for a few years, but certainly not on a full time basis. Post heart attack recovery led me to refocus my energies on the ECG monitoring system,

## What I had been able to accomplish
* At the core of the project, the computing platform that I was working on is the Espressif ESP32 plaform. There are many development boards that are available, and they are all reasonably priced. The chief development IDE is Microsoft Visual Studio Code, which is freely available. Espressif has created the ESP-IDF extension for VSCode (Microsoft Visual Studio Code), which enables the developer to create and maintain ESP32 development on many platforms, including Windows, macOS, as well as Linux based operating systems.

## Getting AI to assist me in development.
* I had been able to shake out most of the various sensor boards and other devices, however, due to other health related issues, such as GI problems, spinal stenosis issues, and then severe cataract problems, my work was then slowing down. I reluctantly decided to look into the use of AI assistance, and I was pleasantly surprised by it. It has helped immensely which helped me overcome physical disabilities
* The result of this AI interaction led me to a rather promising package that may prove to be a good platform that can be expanded upon.

# Footnotes

[^MicroplasticCardiac]: Microplastics are associated with elevated atherosclerotic risk and increased vascular complexity in acute coronary syndrome patients. Yunxiao Yang, Feng Zhang, Zhili Jiang, Zhiyong Du, Sheng Liu, Ming Zhang, Yanyan Jin, Yanwen Qin,
Xiubin Yang, Chenggang Wang and Hai Gao. Particle and Fibre Toxicology. (2024) 21:34