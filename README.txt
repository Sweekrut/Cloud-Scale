Project: CloudScale - Team 4
Team members: 	Sweekrut Suhas Joshi
				Suhaskrishna Gopalkrishna
				Bharath Banglaore Veeranna


General dependencies required:
	g++, python, numpy, matplotlib, fastdtw, python-pearson package


For the CloudScale system deployment, configure the following setup on 4 VCL machines:

Host-1: Dom-0 (CloudScale Monitor and Resource Scaler), VM-1 (HAProxy Webserver), VM-2 (RuBIS PHP Server-1), VM-3 (RuBIS PHP Server-2), VM-4 (RuBIS DB Server)
Host-2: RuBIS Client running the modified RuBIS client
Host-3: Predictor, SLA notification listening Server
Host-4: Target Host for migration


NOTE: If the below diagram is not aligned properly, please refer the image readme_setup.png provided in the same folder


												VM Migration connection b/w sender and receiver
											       |------------------------------------------------------------Host-4
				Dom-0 (Host-1) ------------------------------------------------|
				   |							       | Communication between Dom-0 monitor and Host-3 Predictor
			Connection towards Dom-0 Monitor				       |------------------------------------------------------------Host-3
  ------------------------------------------------------------------												|
  |				  |  |				   |												|
  |				  |  |				   |												|
  |				  |  |				   |												|
  |				  |  |				   |												|
  |  Proxy-Con  |-----------------VM-2------------------|	   |												|
VM-1------------|		     |	(App to DB Con)	|---------VM-4												|
  |		|-----------------VM-3------------------|													|
  |																				|
  |				Host-1																|
  |																				|
  |(RuBIS server-client Con)																	|
  |									Connection to SLA Violation Notification Server						|
Host-2-----------------------------------------------------------------------------------------------------------------------------------------------------------




	   |
------ or  | "Denotes UDP/TCP connection"
	   |


Pre Configuration:

	Host-2:
	Configure RuBIS client. Update the Stats.java, ClientEmulator.java, RUBiSProperties.java and rubis.properties files in the standard RuBIS client directory with the ones provided.
	The rubis.properties file can be updated with the appropriate SLA Notification server IP(Host-3 IP in this case) and Threshold time. Follow the standard procedure to setup PHP based RUBiS client. 


	Host-1:
	Setup xen. Create the base VM images to be run. VM memory used 512 MB. VM disk space used 2GB.
	The scripts to setup XEN and configure the VMs are provided in the folder XEN Setup
	Run the script ./installXen.sh to install Xen on the host. ./configureVM will setup the network configurations required by the VM
	Copy the file Monitor/client.cpp to each of the VMs and compile the same to generate the binary file
	Setup the HAProxy web server to balance load from Host-2 RuBIS client to VM-2 and VM-3. VM-2 and VM-3 are configured as RuBIS PHP application servers.
	VM-4 is configured as RUBiS Database server. Follow standard procedure to set these machine up. Make sure the DB communication between the application servers and database servers are established.
	Make sure there is interconnectivity between all VMs as well.

	In dom-0:
	Enable the appropriate settings for establishing TCP connection to migration target host - Host-4 (changes in xend-config file). Host-4 and Host-1 should have shared file system for the VMs. Use NFS to achieve this.

Run monitor and scaling scripts

	Install libxenstat library in the domain-0. To install the libxenstat library, execute the script ./installLibXenStat.sh file provided in the Xen Setup folder. This will install libxenstat library required for monitoring tool
	Copy the contents of the folder Monitor into the Domain-0 of the host. Run the make command to build the binaries.
	Execute the binary to start the monitoring tool
	Copy the files in Migration Folder to this host. Modify the password in the "live.sh" script to the Host-4's user password. Modify the user "sgopalk" in "mig.sh" and "target_nat.sh" to Host-4's user.
	NAT rules in target_nat.sh and live.sh need to be modified as per the user's topology.
	Run the migHandler python script after modifying the Web_IP in the script to VM-1 Web server's IP address. 

	In VM-1 (HAProxy Server):
	Copy haproxy.cfg, LoadPHPSer1.cfg, LoadPHPSer2.cfg into /etc/haproxy/ folder. Modify PHP1 and PHP2 server IPs to VM-2's and VM-3's IP respectively.
	Copy and modify loadBalance.py script to have the UDP_IP field as VM-1's IP address. Run the loadBalance.py python script to run the server that listens to load balance changing control messages from dom-0.


Host-3:
	Set up the prediction server and SLA Notification Server.
	Prediction Server:
	Copy the contents of the folder Prediction in this folder. Run make command to generate the binary.
	Copy XEN Setup folder to this host and run ./installLibXenStat.sh to install required libraries.
	run the binary file ./handler to start the execution of the prediction server

	SLA Notification Server:
	Run the SLAViolateRetrain.py python script provided. This automatically runs the SLA notification server and also signals the predictor if retraining has to be initiated.


Host-4:
	Setup xen. Have the changes enabled for migration in the xend-config file to start listening for migration requests.
	Have the Host-1 VM's root directories, network mounted on this host.



Order of steps to follow to run the system:
	Setup the prediction server and run the ./handler binary
	Setup the Domain-0 hosts and start up the VMs
	Start the monitor tool in all the hosts.
	Login to each of the VMs and execute the binary ./client
	CloudScale setup is complete!