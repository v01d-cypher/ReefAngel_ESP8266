# ReefAngel_ESP8266
Use an ESP8266 as a ReefAngel wifi module via serial communication

Reworked from code written by DrewPalmer04: http://forum.reefangel.com/viewtopic.php?f=3&t=2394&start=110

We would love to use ESP8266WebServer instead of raw ESP8266WiFi, but the data coming from the RA includes all headers, etc. that makes that complicated. So just send and receive bytes.
