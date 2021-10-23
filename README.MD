## Binance C++ API
API Library for Binance Bitcoin Exchange.

This version included updated versions of LibCurl, LibWebSockets, LibWolfSSL, JsonCpp and Libplot:

Thread safety : Lag free : Support multi-threads websockets connections and more..

CommandLine Plotting: ./exampleWatch

 ![](Screencast.gif)  ![](Screencast2.gif)

### Note:
```
LibWebSockets and LibCurl use LibWolfSSL with a builtin CA-cert.
libplot as standalone libXplot
```
### Prerequisites:
```
sudo apt-get install build-essential cmake g++ gcc libssl-dev
```

### Building:

```
git clone --recurse-submodules https://github.com/mussonero/binance-cxx-api-v2.git
cd binance-cxx-api-v2
mkdir build
cd build/
cmake ..
make
./bin/httpPerformanceTest | grep ":ms"
```
### output:
```
LINKBTC:getDepth time duration->249:ms
ETHTUSD:getDepth time duration->246:ms
XRPBNB:getDepth time duration->247:ms
LINKETH:getDepth time duration->245:ms
BNBUSDT:getDepth time duration->248:ms
Thread 0 finished in->1237:ms  =
LINKBTC:getDepth time duration->246:ms
ETHTUSD:getDepth time duration->246:ms
XRPBNB:getDepth time duration->246:ms
LINKETH:getDepth time duration->247:ms
BNBUSDT:getDepth time duration->246:ms
Thread 1 finished in->1234:ms  =
LINKBTC:getDepth time duration->246:ms
ETHTUSD:getDepth time duration->245:ms
XRPBNB:getDepth time duration->244:ms
LINKETH:getDepth time duration->245:ms
BNBUSDT:getDepth time duration->244:ms
Thread 2 finished in->1227:ms 
~~~
~~~
```

#
thanks

original libbinance-cxx-api from https://github.com/dmikushin/binance-cxx-api 

original Libplot from  https://github.com/annacrombie/plot
#