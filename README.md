# RTOS Analysis

#### An analysis of various performance metrics between RTOS kernels

<a name="readme-top"></a>

<!-- rtos gif -->
![rtos](https://github.com/UreshiiPanda/rtos_analysis/assets/39992411/c630b322-caf0-42cd-8f45-70081342af0e)


<!-- ABOUT THE PROJECT -->
## About The Project

This project was aimed at learning more about RTOS kernels and embedded programming in general. A teammate and I used Miro Samek's
fantastic video course [Embedded Video Course](https://www.state-machine.com/video-course) as our guide to developing a minimalistic RTOS
kernel which was then compared to other open source RTOS kernels such as FreeRTOS and QPC/QXK. From round-robin scheduler to priority-based scheduler, 
the scheduling, concurrency model, and interrupt mechanisms of the minimal RTOS was tweaked to improve its thread-handling, context-switching, and overall performance.
Each RTOS was built/loaded in the ARM KEIL IDE and flashed onto a TivaC Launchpad microcontroller equipped with an ARM Cortex M-4 CPU. Performance metrics were 
measured with a logic analyzer in order to analyze the differences of the RTOS kernels, of different concurrency models, and of various interrupt implementations. 
The primary metrics we focused on consisted of: Interrupt Latency, Interrupt Overhead, Scheduling Latency, and overall RTOS Kernel Overhead. Lastly, these 
measurements were collected via Sigrok PulseView software and compiled into a Jupyter Lab workflow for simple data visualizations.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## How to Run

A lot is involved in setting up an embedded project at home, and the steps are best explained on the video course here [Embedded Video Course](https://www.state-machine.com/video-course), but the following steps outline the general process which can be carried out
on a Windows machine or on an Intel-based machine with a Windows VM:


### Installation / Execution Steps

1. Order a TivaC Launchpad (by Texas Instruments) and any logic analyzer (I used HiLetGo USB analyzer off of Amazon)
2. Navigate to the video course link to download the Tiva/Stellaris ICDI Drivers
3. Once installed, open Device Manager and look for a device called “Stellaris In-Circuit Debug Interface” with the Tiva-C launchpad connected and powered on
4. To set up the development environment, navigate to [KEIL IDE](https://www.keil.arm.com/) and download the “Keil uVision” IDE
5. Setup and activate a KEIL license and then download the Tiva/Stellaris Extension for your "Keil uVision" IDE
7. Clone this repo
   ```sh
      git clone https://github.com/UreshiiPanda/rtos_analysis.git
   ```
8. Navigate to the “tm4c123-keil” directory and open the uVision5 project file
9. With the project file open, and the Tiva-C Launchpad connected and powered on, press F7 to build/compile the project and F8 to flash the program onto the board
10. At this point, the program should be running on the board and you should see the RGB LED lit on your TivaC board
11. To set up the logic analyzer, navigate to [Sigrok Pulseview Logic Analysis Software](https://www.sigrok.org/wiki/Downloads) and download the Windows version
12. Next, open the application called “Zadig (Pulse View)” from your Windows start menu. Once the application opens, click “Install Driver”
13. You can now open the PulseView softwarem, and in the device connection dropdown menu, select “Saleae Logic” to connect to your Logic Analyzer
14. To view the GPIO outputs of your Tiva-C launchpad with the logic analyzer, connect the test leads to the logic analyzer’s channel headers ensuring to connect the two GND pins to the GND pins on the analyzer
15. Connect the other ends of the leads to whichever GPIO pins you wish to monitor on your TivaC board
16. With the PulseView Software configured and the board running, click the “Run” button to begin capturing traces of your program operating



<p align="right">(<a href="#readme-top">back to top</a>)</p>
