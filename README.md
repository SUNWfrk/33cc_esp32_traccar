There are a two steps before this works.

1. update in `33cc_esp32_traccar.ino`

```
#define USE_WIFI false
#define USE_GPRS true
```
to either use WIFI or GPRS


2. Copy `arduino_secrets.example` to `arduino_secrets.h` and update the file to your needs

Compile and upload this to your device, I'm using a `T-SIM7600E/G-H`

Example Serial log:

```
Starting modem... OK
Starting GPS module.... OK
Waiting for GPS Data for time config................... OK
Entering SIM PIN
Connecting to internet.provider.com. OK
Sendit value is set to: 0
Using initial GPS coordinates
Checking Accuracy..
Accuracy is: 1.70
lat is: 55.xxxxxx
lon is: 15.xxxxxx
Accuracy OK
Sending data
making POST request
Status code: 200
Sendit value is set to: 0
Checking Accuracy
Accuracy is: 1.20
lat is: 55.xxxxxx
lat high margin is: 55.xxxxxx
lat low margin is: 55.xxxxxx
lon is: 15.xxxxxx
lon high margin is: 15.xxxxxx
lon low margin is: 14.xxxxxx
Coordinates are between margins
Sending data
making POST request
Status code: 200
```