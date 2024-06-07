#MasterThesis
This project contains files for the Smart IoT based solution for our HVAC comfort in office rooms. The ESP XIAO C32 microcontroller used in the portable device is coded in Arduino IDE, the Node-red server used in this project is hosted in the Raspberry Pi located in the room.

How to get started with the project.

Pre-requisits :
=> Connect your laptop/Macbook to the Wifi near TUAS 3555, WiFiname "DNA-Mokkula-2G-7dTp7c" and WiFipw "41951404706" (dont use ""). If this is not available, it means the router is removed with the entire setup, so first get a Raspberry PI connected to the router with internet. To setup from scratch you need a raspberry Pi with Raspian OS installed in the SD card. The beginner guide can be found online on "How to setup raspberry Pi" Some useful links are as follows :

 General advise => https://www.raspberrypi.com/documentation/computers/getting-started.html#setting-up-your-raspberry-pi
 Specific advise :
1) How to setup Raspberry Pi : https://wiki.aalto.fi/display/ELECPROTO/Raspberry+Pi+Exercises
2) Get comfortable with ESP XIAO C32 : https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/     and    https://wiki.aalto.fi/display/ELECPROTO/Getting+started+with+the+ESP32

3) Setup Nodered in Raspberry Pi : https://wiki.aalto.fi/display/ELECPROTO/Setting+Up+Raspi+as+a+Cloud+Server+with+Node-RED
4) Wifi Connectivity of Raspberry Pi : https://www.seeedstudio.com/blog/2021/01/25/three-methods-to-configure-raspberry-pi-wifi/

For step 4, I have used wired connection of Raspberry Pi, please be flexible with your approach. 

I have not used Aalto open wifi but a router with sim card bearing the above credentials (Wifiname and Wifipw), if it does not apply to you, you can setup your own network and make the Wifi credentials accordingly, and also remember to change the credentials in the network.h file mentioned in Programming section.

Once all of these steps are completed(or not required) go to Conditional scenarios

Conditional scenarios:
1) If the Raspberry Pi is still located in the room, make sure that the node red server deployed is still working by visiting the existing Raspberry Pi IP  : http://192.168.1.101:1880/ (On the Wifi network as mentioned above with my credentials), if it is working and you can see Node red server, the system is running, and you can go to the programming section. 
  1.1) Maybe Node-red needs to be restarted/or not depending whether the Raspberry Pi lost power connection. if the IP is not responsive go to the terminal/command prompt of your system and start the node red, for this follow the instruction in file SSH.txt before Netatmo section, once Node red has started, you can identify the files as mentioned in the programming section below.

2) If step 1 has failed, it means the Pre requisits were not complied with, set it up by following all the 4 steps religiously and then use arduino and node red files in the programming section below.


Programming: 
1) Arduino IDE : For ESP 32 there are 2 files uploaded Prakhar_Update_1 and Prakhar_Update_2, The primary goal of the thesis to automate the HVAC Unit is available in Prakhar_Update_1. The voting mechanism is implemented in Prakhar_Update_2. The icons.h and network.h files are common for the sceanrios, so at a time you should have 3 files in your Arduino IDE Prakhar_Update_1/Prakhar_Update_2,icons.h,network.h based on your requirements.

2) Node red : The Back up file of 08/04 coincides with Prakhar_Update_1 use case, so the flow 4 and flow 5 in this file is to automate the HVAC unit. Flow 1 is disabled and not required and is archived,it can be used as reference for web page designs. The other file Back up file of 10/05 coincides with Prakhar_Update_2 and supports the voting sceanrios. The flow 4 and flow 5 are similar to the previous file,however with minute changes, the new piece of coding is added in flow 3.

Netatmo and Tuya cloud : No programming is done in the backend and hence there is no file, you need to setup your own account on each platform and develop applications on both the cloud platforms. Generate credentials and use them in your node red programming. I have used Postman to test my APIs of both platforms, I shall share it here.

Once all the steps of Pre-requisits are done (if required) and the Conditonal Scenario (1 or 2) have been encountered. The programming part is completed, then the system is ready to respond. 

Postman :

1) Tuya : https://api.postman.com/collections/10297344-a3015272-d4e2-4f72-af07-b856bf052182?access_key=PMAT-01HYX4KB9H1KWE1WH60TRJ2JP6

2) Netatmo : https://api.postman.com/collections/10297344-cbd28430-4ea8-4469-9f73-6b4f081e6d1a?access_key=PMAT-01HYX4NEHCNE1HBVRB3886YD8F

This is only an indication of how I have used APIs after developing the application on the respective cloud platforms. Once you have developed your applications on them, you can refer to this Postman API testing to see the structure of all the APIs.

SSH.text :
This file helps connecting the Node red remotely (without physical connection with Raspberry Pi) but in the same network of the Wifi through SSH client. The Netatmo section and Function on node red section are not important for this purpose but can be referred for code formatting assistance in Node red.


If at any time, you are stuck and you need my inputs.

You have my contact information

Name : Prakhar Prakash
email : prakhar.max@outlook.com
 
  
