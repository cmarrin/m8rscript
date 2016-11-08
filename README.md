# m8rscript, A new IoT scripting language
> Caution: MaterScript is very much a work in progress. You can do a few things with it, like get the current time and print on Mac. But it's not up on ESP8266 yet and for all I know may never be. Right now it's just a proof of concept. Proceed at your own risk.

See https://github.com/cmarrin/m8rscript/wiki for an overview of m8rscript and its features.

##Building

The 2.0 version of the ESP8266 SDK is included in the repository, as well as versions of esptool and loader scripts used to link the system. You only need to get a copy of the ESP8266 Arduino repository and fetch the build tools. I recommend cloning into $(HOME)/esp8266 so you don't have to change the Makefile:

~~~~
cd ~/
git clone https://github.com/esp8266/Arduino.git esp8266
cd esp8266/tools
python get.py
~~~~

Then go to the m8rscript/esp folder and type make. You should get a successful build and a printout of the code and data sizes. Before you try to upload you'll need to change the UPLOAD_PORT variable in esp/Makefile to your serial device. Or you can put an UPLOAD_PORT variable into your environment and it will pick that up. Then you just make sure you ESP8266 is plugged in, powered on and set to upload mode and:

~~~~
make upload
~~~~

In the current version this should start the LED blinking (assuming you're using an ESP-12). This is just the heartbeat and I'll be changing it to something much less intrusive soon.

Next you can go into the mac folder, open the xcodeproj file, build that and try to connect!

###More Later...
