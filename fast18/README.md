# The Experiments of the FAST 2018 Paper

The input files in the fast18 folder can be used to repeat the experiments of the FAST 2018 paper [1].

1. **Contention Effect on the write cache (data-cache-contention folder):** the _ssdconfig-datacache-contention.xml_ file should be fed into MQSim as the configuration file. Each of the 3 workload definition files should be fed into MQSim in three different experiments. These files and the corresponding output of execution are described below:<br />
   1.1. _workload-datacache-contention-flow-1.xml_: response time of flow-1 when it is executed alone<br />
   1.2. _workload-datacache-contention-flow-2.xml_: response time of flow-2 when it is executed alone<br />
   1.3. _workload-datacache-contention-flow-1-flow-2.xml_: response time of flow-1 and flow-2 when they are executed concurrently<br />
   To calculate slowdown, the response time of flow-1 and flow-2 in the concurrent execution (i.e., results of 1.3) should be divided to the response time of flow-1's alone execution (i.e., results of 1.1) and flow-2's alone execution (i.e., results of 1.2), respectively.

2. **Contention Effect in the SSD Back END (backend-contention folder):** the _ssdconfig-backend-contention.xml_ file should be fed into MQSim as the configuration file. Each of the 3 workload definition files should be fed into MQSim in three different experiments. These files and the corresponding output of execution are described below:<br />
   1.1. _workload-backend-contention-flow-1.xml_: response time of flow-1 when it is executed alone<br />
   1.2. _workload-backend-contention-flow-2.xml_: response time of flow-2 when it is executed alone<br />
   1.3. _workload-backend-contention-flow-1-flow-2.xml_: response time of flow-1 and flow-2 when they are executed concurrently<br />
   To calculate slowdown, the response time of flow-1 and flow-2 in the concurrent execution (i.e., results of 1.3) should be divided to the response time of flow-1's alone execution (i.e., results of 1.1) and flow-2's alone execution (i.e., results of 1.2), respectively.

3. **The effect of the QueueFetchSize parameter (queue-fetch-size folder):** the _ssdconfig-queue-fetch-size=16.xml_ and _ssdconfig-queue-fetch-size=1024.xml_ files define the SSD configuration parameters when the QueueFetchSize parameter is 16 and 1024, repectively. For each of these two configurations, each of the 3 workload definition files should be fed into MQSim in three different experiments. These files and their corresponding output of execution are described below:<br />
   2.1. _workload-queue-fetch-size-flow-1.xml_: response time of flow-1 when it is executed alone<br />
   2.2. _workload-queue-fetch-size-flow-2.xml_: response time of flow-2 when it is executed alone<br />
   2.3. _workload-queue-fetch-size-flow-1-flow-2.xml_: response time of flow-1 and flow-2 when they are executed concurrently<br />
   To calculate slowdown, the response time of flow-1 and flow-2 in the concurrent execution (i.e., results of 1.3) should be divided to the response time of flow-1's alone execution (i.e., results of 1.1) and flow-2's alone execution (i.e., results of 1.2), respectively.
