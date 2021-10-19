## Binance C++ API
API Library for Binance Bitcoin Exchange.

This version included updated versions of LibCurl, LibWebSockets, LibWolfSSL, and JsonCpp:

Thread safety : Lag free : Support multi-threads websockets connections and more..

### Note:
```
LibWebSockets and LibCurl use LibWolfSSL with a builtin CA-cert.
```
### Prerequisites:
```
sudo apt-get install build-essential cmake g++ libssl-dev
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