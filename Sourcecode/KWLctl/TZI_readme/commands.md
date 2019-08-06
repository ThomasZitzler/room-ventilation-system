mosquitto_pub -t d15/set/kwl/calibratefans -m YES


mosquitto_pub -t d15/set/kwl/fan1/standardspeed -m 1300
mosquitto_pub -t d15/set/kwl/fan2/standardspeed -m 1300


mosquitto_pub -t d15/set/kwl/lueftungsstufe -m 2




mosquitto_pub -t d15/debugset/kwl/fan1/getvalues -m on
mosquitto_pub -t d15/debugset/kwl/fan2/getvalues -m on

mosquitto_sub -v -h localhost -t "d15/debugstate/#" > /tmp/debug.log &

 scp pi@raspberrypi:/tmp/debug.log debug.log