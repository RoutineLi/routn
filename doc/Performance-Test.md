### routn框架性能测试(基于ab)
使用工具：apache-bench httpd-tools(ab)
Ubuntu: sudo apt-get install apache2-utils
CentOs: sudo yum -y install httpd-tools

ab -n 1000000 -c 200 http://172.20.10.3:8083/routn/
100w请求 200连接

### 压测数据

## 单线程
```															
Connection = close；		

	Server Software:        routn/1.0.0											
	Server Hostname:        172.20.10.3									
	Server Port:            8083									
													
	Document Path:          /routn								
	Document Length:        138 bytes							

	Concurrency Level:      200
	Time taken for tests:   58.262 seconds
	Complete requests:      1000000
	Failed requests:        0
	Non-2xx responses:      1000000
	Total transferred:      250000000 bytes
	HTML transferred:       138000000 bytes
	Requests per second:    17163.78 [#/sec] (mean)
	Time per request:       11.652 [ms] (mean)
	Time per request:       0.058 [ms] (mean, across all concurrent requests)
	Transfer rate:          4190.38 [Kbytes/sec] received

	Connection Times (ms)
		      min  mean[+/-sd] median   max
	Connect:        0    0   0.3      0       6
	Processing:     2   11  51.0     10    3615
	Waiting:        0   11  51.0     10    3615
	Total:          7   12  51.0     10    3615

	Percentage of the requests served within a certain time (ms)
	  50%     10
	  66%     11
	  75%     11
	  80%     12
	  90%     13
	  95%     14
	  98%     15
	  99%     15
	 100%   3615 (longest request)
```

## 双线程
```
connection = close;
	Server Software:        routn/1.0.0
	Server Hostname:        172.20.10.3
	Server Port:            8083

	Document Path:          /routn/
	Document Length:        138 bytes

	Concurrency Level:      200
	Time taken for tests:   73.047 seconds
	Complete requests:      1000000
	Failed requests:        0
	Non-2xx responses:      1000000
	Total transferred:      250000000 bytes
	HTML transferred:       138000000 bytes
	Requests per second:    13689.89 [#/sec] (mean)
	Time per request:       14.609 [ms] (mean)
	Time per request:       0.073 [ms] (mean, across all concurrent requests)
	Transfer rate:          3342.26 [Kbytes/sec] received

	Connection Times (ms)
				min  mean[+/-sd] median   max
	Connect:        0    6  43.5      5    3701
	Processing:     0    9  58.9      8    3705
	Waiting:        0    7  49.4      6    3702
	Total:          2   15  73.3     13    3710

	Percentage of the requests served within a certain time (ms)
	50%     13
	66%     14
	75%     14
	80%     15
	90%     16
	95%     17
	98%     20
	99%     24
	100%   3710 (longest request)
```

## 多线程
```

Server Software:        routn/1.0.0
Server Hostname:        172.20.10.3
Server Port:            8083

Document Path:          /routn/
Document Length:        138 bytes

Concurrency Level:      200
Time taken for tests:   61.400 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      250000000 bytes
HTML transferred:       138000000 bytes
Requests per second:    16286.60 [#/sec] (mean)
Time per request:       12.280 [ms] (mean)
Time per request:       0.061 [ms] (mean, across all concurrent requests)
Transfer rate:          3976.22 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    5  30.9      5    3526
Processing:     2    7  39.1      7    3529
Waiting:        0    5  35.1      5    3527
Total:          4   12  49.8     11    3534

Percentage of the requests served within a certain time (ms)
  50%     11
  66%     12
  75%     12
  80%     12
  90%     13
  95%     14
  98%     16
  99%     19
 100%   3534 (longest request)


```


```
connection = keep-alive;
	Server Software:        routn/1.0.0
	Server Hostname:        172.20.10.3
	Server Port:            8083

	Document Path:          /routn/
	Document Length:        138 bytes

	Concurrency Level:      200
	Time taken for tests:   20.683 seconds
	Complete requests:      1000000
	Failed requests:        0
	Non-2xx responses:      1000000
	Keep-Alive requests:    1000000
	Total transferred:      255000000 bytes
	HTML transferred:       138000000 bytes
	Requests per second:    48349.13 [#/sec] (mean)
	Time per request:       4.137 [ms] (mean)
	Time per request:       0.021 [ms] (mean, across all concurrent requests)
	Transfer rate:          12040.07 [Kbytes/sec] received

	Connection Times (ms)
				min  mean[+/-sd] median   max
	Connect:        0    0   0.1      0       6
	Processing:     0    4   1.2      4      38
	Waiting:        0    4   1.2      4      38
	Total:          0    4   1.2      4      40

	Percentage of the requests served within a certain time (ms)
	50%      4
	66%      4
	75%      5
	80%      5
	90%      6
	95%      6
	98%      7
	99%      7
	100%     40 (longest request)
```

## 使用腾讯Libco接口实现的协程(仅测试长链接)
```
	Server Software:        routn/1.0.0
	Server Hostname:        172.20.10.3
	Server Port:            8083

	Document Path:          /routn/
	Document Length:        138 bytes

	Concurrency Level:      200
	Time taken for tests:   20.285 seconds
	Complete requests:      1000000
	Failed requests:        0
	Non-2xx responses:      1000000
	Keep-Alive requests:    1000000
	Total transferred:      255000000 bytes
	HTML transferred:       138000000 bytes
	Requests per second:    49297.89 [#/sec] (mean)
	Time per request:       4.057 [ms] (mean)
	Time per request:       0.020 [ms] (mean, across all concurrent requests)
	Transfer rate:          12276.33 [Kbytes/sec] received

	Connection Times (ms)
				min  mean[+/-sd] median   max
	Connect:        0    0   0.1      0       7
	Processing:     0    4   1.2      4      42
	Waiting:        0    4   1.2      4      42
	Total:          0    4   1.2      4      45

	Percentage of the requests served within a certain time (ms)
	50%      4
	66%      4
	75%      5
	80%      5
	90%      5
	95%      6
	98%      7
	99%      7
	100%     45 (longest request)

```


## 对照组-Nginx-1.18.0

```				[nginx] 

	Server Software:        nginx/1.18.0
	Server Hostname:        172.20.10.3
	Server Port:            80

	Document Path:          /
	Document Length:        612 bytes

	Concurrency Level:      200
	Time taken for tests:   80.459 seconds
	Complete requests:      1000000
	Failed requests:        0
	Total transferred:      854000000 bytes
	HTML transferred:       612000000 bytes
	Requests per second:    12428.65 [#/sec] (mean)
	Time per request:       16.092 [ms] (mean)
	Time per request:       0.080 [ms] (mean, across all concurrent requests)
	Transfer rate:          10365.30 [Kbytes/sec] received

	Connection Times (ms)
		      min  mean[+/-sd] median   max
	Connect:        0    7  43.5      6    3902
	Processing:     2    9  58.3      8    3904
	Waiting:        0    7  44.6      6    3902
	Total:          8   16  72.7     14    3909

	Percentage of the requests served within a certain time (ms)
	  50%     14
	  66%     15
	  75%     15
	  80%     16
	  90%     16
	  95%     17
	  98%     18
	  99%     19
	 100%   3909 (longest request)
```

```
Connection = keep-alive

	Server Software:        nginx/1.18.0
	Server Hostname:        172.20.10.3
	Server Port:            80

	Document Path:          /routn/
	Document Length:        162 bytes

	Concurrency Level:      200
	Time taken for tests:   13.444 seconds
	Complete requests:      1000000
	Failed requests:        0
	Non-2xx responses:      1000000
	Keep-Alive requests:    990093
	Total transferred:      325950465 bytes
	HTML transferred:       162000000 bytes
	Requests per second:    74382.39 [#/sec] (mean)
	Time per request:       2.689 [ms] (mean)
	Time per request:       0.013 [ms] (mean, across all concurrent requests)
	Transfer rate:          23676.73 [Kbytes/sec] received

	Connection Times (ms)
				min  mean[+/-sd] median   max
	Connect:        0    0   0.4      0      93
	Processing:     0    3   2.1      2     100
	Waiting:        0    3   2.0      2      83
	Total:          0    3   2.1      2     120

	Percentage of the requests served within a certain time (ms)
	50%      2
	66%      3
	75%      3
	80%      3
	90%      4
	95%      5
	98%      7
	99%      9
	100%    120 (longest request)
```

## Routn/1.0.4----更新协程池(仅测试长连接)
 # 基于ucontext
```
	Server Software:        routn/1.0.4
	Server Hostname:        172.20.10.3
	Server Port:            8083

	Document Path:          /routn
	Document Length:        138 bytes

	Concurrency Level:      200
	Time taken for tests:   1.762 seconds
	Complete requests:      100000
	Failed requests:        0
	Non-2xx responses:      100000
	Keep-Alive requests:    100000
	Total transferred:      25500000 bytes
	HTML transferred:       13800000 bytes
	Requests per second:    56762.10 [#/sec] (mean)
	Time per request:       3.523 [ms] (mean)
	Time per request:       0.018 [ms] (mean, across all concurrent requests)
	Transfer rate:          14135.09 [Kbytes/sec] received

	Connection Times (ms)
				min  mean[+/-sd] median   max
	Connect:        0    0   0.2      0       6
	Processing:     0    4   1.9      3      41
	Waiting:        0    3   1.9      3      41
	Total:          0    4   2.0      3      44

	Percentage of the requests served within a certain time (ms)
	50%      3
	66%      4
	75%      4
	80%      4
	90%      5
	95%      6
	98%      7
	99%      9
	100%     44 (longest request)
```
 # 基于libco
```

```