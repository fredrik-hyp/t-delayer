#!/usr/bin/php
<?php
    /* Imports delayerdb streaminterface config from CSV file
    */

    require_once("/var/db/dbstreaminterfaces.php");
    require_once("/var/db/dbstreaminterfaceconfigs.php");
    require_once("/var/db/dbstreaminterfacesources.php");
    require_once("/var/db/dbstreaminterfacesinks.php");
    require_once("/var/db/dbstreaminterfacebackupsources.php");
    require_once("/var/db/dbstreaminterfacebackupsinks.php");
    require_once("/var/db/dbclientcertificates.php");
    require_once("/var/db/dbvirtualsources.php");
    
    define("MAX_BACKUP_INTERFACES", 8);
    define("CSV_CONFIG_VERSION", 2);
    
    function fixBool($bool)
    {
        if($bool == "yes" || $bool === true)
        {
            return 't';
        }
        
        return 'f';
    }
    
    function certificateExists($certid)
    {
        $cert = DBFetchClientCertificateById($certid);
        if(sizeof($cert) < 1)
        {
            return false;
        }
        
        return true;
    }
    
    function virtualsourceExists($vsourceid)
    {
        $virtualsource = DBFetchVirtualSourceById($vsourceid);
        if(sizeof($virtualsource) < 1)
        {
            return false;
        }
        
        return true;
    }
    
    function parseLineVersion1($line)
    {
        $line = trim($line);
        $result = array();
        
        $parts = explode(",", $line);
        if($parts === false || sizeof($line) == 0)
        {
            return false;
        }

        $result['name'] = $parts[0];
        $result['usertext'] = $parts[1];
        $result['mode'] = $parts[2];
        $result['autostart'] = $parts[3];
        $result['msbuffer'] = $parts[4];
        $result['legacy'] = $parts[5];
        $result['ttl'] = $parts[6];
        $result['hostcertificate_id'] = $parts[7];
        $result['talkbackcertificate_id'] = $parts[8];
        $result['sourcetype'] = $parts[9];
        $result['sourcehost'] = $parts[10];
        $result['sourceport'] = $parts[11];
        $result['sourcetalkbackhost'] = $parts[12];
        $result['sourcetalkbackport'] = $parts[13];
        $result['sinktype'] = $parts[14];
        $result['sinkhost'] = $parts[15];
        $result['sinkport'] = $parts[16];
        $result['sinktalkbackhost'] = $parts[17];
        $result['sinktalkbackport'] = $parts[18];
        $result['virtualsource_id'] = $parts[19];
        $result['streammissfailovertimeout'] = $parts[20];
        $result['retrytime'] = $parts[21];


        $backsrcs = array();
        $backdests = array();
        
        $offset = 21;
        for($i=0 ;$i<MAX_BACKUP_INTERFACES; $i++)
        {
            $bakoff = $i*22;
            $backsrc = array();
            $backsrc['name'] = $parts[$offset+1+$bakoff];
            $backsrc['priority'] = $parts[$offset+2+$bakoff];
            $backsrc['maxretrytime'] = $parts[$offset+3+$bakoff];
            $backsrc['streammissfailovertimeout'] = $parts[$offset+4+$bakoff];
            $backsrc['hostclientcertificate_id'] = $parts[$offset+5+$bakoff];
            $backsrc['talkbackclientcertificate_id'] = $parts[$offset+6+$bakoff];
            $backsrc['sourcetype'] = $parts[$offset+7+$bakoff];
            $backsrc['host'] = $parts[$offset+8+$bakoff];
            $backsrc['port'] = $parts[$offset+9+$bakoff];
            $backsrc['talkbackhost'] = $parts[$offset+10+$bakoff];
            $backsrc['talkbackport'] = $parts[$offset+11+$bakoff];
            $backsrcs[$i] = $backsrc;
            
            $backdest = array();
            $backdest['name'] = $parts[$offset+12+$bakoff];
            $backdest['priority'] = $parts[$offset+13+$bakoff];
            $backdest['maxretrytime'] = $parts[$offset+14+$bakoff];
            $backdest['streammissfailovertimeout'] = $parts[$offset+15+$bakoff];
            $backdest['hostclientcertificate_id'] = $parts[$offset+16+$bakoff];
            $backdest['talkbackclientcertificate_id'] = $parts[$offset+17+$bakoff];
            $backdest['sinktype'] = $parts[$offset+18+$bakoff];
            $backdest['host'] = $parts[$offset+19+$bakoff];
            $backdest['port'] = $parts[$offset+20+$bakoff];
            $backdest['talkbackhost'] = $parts[$offset+21+$bakoff];
            $backdest['talkbackport'] = $parts[$offset+22+$bakoff];
            
            $backdests[$i] = $backdest;    
        }
        
    
        $result['backupsources'] = $backsrcs;
        $result['backupdestinations'] = $backdests;
        return $result;
    }

    function parseLineVersion2($line)
    {
        $line = trim($line);
        $result = array();
        
        $parts = explode(",", $line);
        if($parts === false || sizeof($line) == 0)
        {
            return false;
        }

        $result['name'] = $parts[0];
        $result['usertext'] = $parts[1];
        $result['mode'] = $parts[2];
        $result['autostart'] = $parts[3];
        $result['mcinusecheck'] = $parts[4];
        $result['multipath'] = $parts[5];
        $result['mploadbalance'] = $parts[6];
        $result['msbuffer'] = $parts[7];
        $result['legacy'] = $parts[8];
        $result['ttl'] = $parts[9];
        $result['hostcertificate_id'] = $parts[10];
        $result['talkbackcertificate_id'] = $parts[11];
        $result['sourcetype'] = $parts[12];
        $result['sourcehost'] = $parts[13];
        $result['sourceport'] = $parts[14];
        $result['sourcetalkbackhost'] = $parts[15];
        $result['sourcetalkbackport'] = $parts[16];
        $result['sinktype'] = $parts[17];
        $result['sinkhost'] = $parts[18];
        $result['sinkport'] = $parts[19];
        $result['sinktalkbackhost'] = $parts[20];
        $result['sinktalkbackport'] = $parts[21];
        $result['virtualsource_id'] = $parts[22];
        $result['streammissfailovertimeout'] = $parts[23];
        $result['retrytime'] = $parts[24];


        $backsrcs = array();
        $backdests = array();
        
        $offset = 24;
        for($i=0 ;$i<MAX_BACKUP_INTERFACES; $i++)
        {
            $bakoff = $i*22;
            $backsrc = array();
            $backsrc['name'] = $parts[$offset+1+$bakoff];
            $backsrc['priority'] = $parts[$offset+2+$bakoff];
            $backsrc['maxretrytime'] = $parts[$offset+3+$bakoff];
            $backsrc['streammissfailovertimeout'] = $parts[$offset+4+$bakoff];
            $backsrc['hostclientcertificate_id'] = $parts[$offset+5+$bakoff];
            $backsrc['talkbackclientcertificate_id'] = $parts[$offset+6+$bakoff];
            $backsrc['sourcetype'] = $parts[$offset+7+$bakoff];
            $backsrc['host'] = $parts[$offset+8+$bakoff];
            $backsrc['port'] = $parts[$offset+9+$bakoff];
            $backsrc['talkbackhost'] = $parts[$offset+10+$bakoff];
            $backsrc['talkbackport'] = $parts[$offset+11+$bakoff];
            $backsrcs[$i] = $backsrc;
            
            $backdest = array();
            $backdest['name'] = $parts[$offset+12+$bakoff];
            $backdest['priority'] = $parts[$offset+13+$bakoff];
            $backdest['maxretrytime'] = $parts[$offset+14+$bakoff];
            $backdest['streammissfailovertimeout'] = $parts[$offset+15+$bakoff];
            $backdest['hostclientcertificate_id'] = $parts[$offset+16+$bakoff];
            $backdest['talkbackclientcertificate_id'] = $parts[$offset+17+$bakoff];
            $backdest['sinktype'] = $parts[$offset+18+$bakoff];
            $backdest['host'] = $parts[$offset+19+$bakoff];
            $backdest['port'] = $parts[$offset+20+$bakoff];
            $backdest['talkbackhost'] = $parts[$offset+21+$bakoff];
            $backdest['talkbackport'] = $parts[$offset+22+$bakoff];
            
            $backdests[$i] = $backdest;    
        }
        
    
        $result['backupsources'] = $backsrcs;
        $result['backupdestinations'] = $backdests;
        return $result;
    }
    
    function validateInterfaceSettings($interface)
    {
        
        if($interface['hostcertificate_id'] != "")
        {
            if(!certificateExists($interface['hostcertificate_id']))
            {
                $interface['hostcertificate_id'] = "";
            }
        }
        
        if($interface['talkbackcertificate_id'] != "")
        {
            if(!certificateExists($interface['talkbackcertificate_id']))
            {
                $interface['talkbackcertificate_id'] = "";
            }
        }
        
        if($interface['virtualsource_id'] != "")
        {
            if(!virtualsourceExists($interface['virtualsource_id'] != ""))
            {
                $interface['virtualsource_id'] = "";
            }
        }
        
        for($i=0; $i < MAX_BACKUP_INTERFACES; $i++)
        {
            $baksrc = $interface['backupsources'][$i];
            $baksink = $interface['backupdestinations'][$i];

            if($baksrc['hostclientcertificate_id'] != "")
            {
                if(!certificateExists($baksrc['hostclientcertificate_id']))
                {
                    $interface['backupsources'][$i]['hostclientcertificate_id'] = "";
                }
            }

            if($baksrc['talkbackclientcertificate_id'] != "")
            {
                if(!certificateExists($baksrc['talkbackclientcertificate_id']))
                {
                    $interface['backupsources'][$i]['talkbackclientcertificate_id'] = "";
                }
            }

            if($baksink['hostclientcertificate_id'] != "")
            {
                if(!certificateExists($baksink['hostclientcertificate_id']))
                {
                    $interface['backupdestinations'][$i]['hostclientcertificate_id'] = "";
                }
            }

            if($baksink['talkbackclientcertificate_id'] != "")
            {
                if(!certificateExists($baksink['talkbackclientcertificate_id']))
                {
                    $interface['backupdestinations'][$i]['talkbackclientcertificate_id'] = "";
                }
            }
            
        }
        
        return $interface;
    }
    
    function writeConfig($interfaces)
    {
        $dbconn = dbconnect();
        if($dbconn === false)
        {
            return false;
        }
        
        foreach($interfaces as $idx => $interface)
        {
            $id = $idx + 1;
            $stinterface = DBFetchStreaminterfaceByName($interface['name']);
            if(sizeof($stinterface) < 1)
            {
                print("Could not find interface with name " . $interface['name'] . "!\n");
                continue;
            }
            
            $stinterfaceconfig = DBFetchStreaminterfaceConfigByStreaminterfaceId($stinterface['id']);
            if(sizeof($stinterfaceconfig) < 1)
            {
                print("Could not find config for " . $interface['name'] . "!\n");
                continue;
            }
            
//            $id, $name, $priority, $sourcetype, $host, $port, $talkbackhost, $talkbackport, $virtualsourceid, $hostclientcertid, $talkbackclientcertid, $retrytime, $streammiss

            $stinterfacesource = DBFetchStreaminterfaceSourceByStreaminterfaceConfigId($stinterfaceconfig['id']);
            
            if(sizeof($stinterfacesource) < 1)
            {
                print("Could not find source for " . $interface['name'] . "!\n");
                continue;
            }
            
            $stinterfacesink = DBFetchStreaminterfaceSinkByStreaminterfaceConfigId($stinterfaceconfig['id']);
            if(sizeof($stinterfacesource) < 1)
            {
                print("Could not find sink for " . $interface['name'] . "!\n");
                continue;
            }
            
            $interface = validateInterfaceSettings($interface);
            
            $autostart = fixBool($interface['autostart']);
            $legacy = fixBool($interface['legacy']);
            $mcinusecheck = fixBool($interface['mcinusecheck']);
            $multipath = fixBool($interface['multipath']);
            $mploadbalance = fixBool($interface['mploadbalance']);
            
            DBDeleteStreaminterfaceBackupSourceByStreaminterfaceSourceId($stinterfacesource['id']);
            DBDeleteStreaminterfaceBackupSinkByStreaminterfaceSinkId($stinterfacesink['id']);
            
            $ret = DBUpdateStreaminterface($stinterface['id'], $interface['name'], $interface['usertext']);
            if($ret == 0)
            {
                print("Error updating streaminterface!\n");
            }
            
            // DBUpdateStreaminterfaceConfig($id, $streaminterfaceid, $typeofconfig, $autostart, $msbuffer, $resendmode, $mode, $nosessionid)
            $ret = DBUpdateStreaminterfaceConfig($stinterfaceconfig['id'], $stinterface['id'], "stored", $autostart, $interface['msbuffer'], "normal", $interface['mode'], $legacy, $multipath, $mploadbalance);
            if($ret == 0)
            {
                print("Error updating streaminterfaceconfig!\n");
            }
            DBSetStreaminterfaceConfigMcInUseCheck($stinterface['id'], $mcinusecheck);
            //  DBUpdateStreaminterfaceSource($id, $name, $priority, $sourcetype, $host, $port, $talkbackhost, $talkbackport, $virtualsourceid, $hostclientcertid, $talkbackclientcertid, $retrytime, $streammiss)
            if($interface['mode'] == "transmitter")
            {
                $streammisstimeout = "NULL";
                $srcretrytime = "NULL";
                $sinkretrytime = $interface['retrytime'];
            }
            else
            {
                $streammisstimeout = $interface['streammissfailovertimeout'];
                $srcretrytime = $interface['retrytime'];    
                $sinkretrytime = "NULL";
            }
            
            $ret = DBUpdateStreaminterfaceSource($stinterfacesource['id'], "sourcename", "NULL", $interface['sourcetype'], $interface['sourcehost'], $interface['sourceport'], $interface['sourcetalkbackhost'], $interface['sourcetalkbackport'], $interface['virtualsource_id'], $interface['hostcertificate_id'], $interface['talkbackcertificate_id'], $srcretrytime,  $streammisstimeout);

            if($ret == 0)
            {
                print("Error updating streaminterfacesource!\n");
            }

            // DBUpdateStreaminterfaceSink($id, $name, $priority, $sinktype, $host, $port, $talkbackhost, $talkbackport, $ttl, $hostclientcertid, $talkbackclientcertid, $retrytime)
            $ret = DBUpdateStreaminterfaceSink($stinterfacesink['id'], "sinkname", "NULL", $interface['sinktype'], $interface['sinkhost'], $interface['sinkport'], $interface['sinktalkbackhost'], $interface['sinktalkbackport'],1, $interface['hostcertificate_id'], $interface['talkbackcertificate_id'],$sinkretrytime);
            if($ret == 0)
            {
                print("Error updating streaminterfacesink!\n");
            }
            
            if($interface['mode'] == "receiver")
            {
                foreach($interface['backupsources'] as $idx => $baksource)
                {
                    if(($baksource['priority']) == "")
                    {
                      continue;  
                    }
                    $ret = DBAddStreaminterfaceBackupSource($stinterfacesource['id'], $baksource['name'], $baksource['priority'], $baksource['maxretrytime'], $baksource['streammissfailovertimeout'], $baksource['sourcetype'], $baksource['host'], $baksource['port'], $baksource['talkbackhost'], $baksource['talkbackport'], $baksource['hostclientcertificate_id'], $baksource['talkbackclientcertificate_id']);
                    if($ret < 1){
                        print("Error updating streaminterfacebackupsource!\n");
                    }
                }
            }
            else
            {                
                foreach($interface['backupdestinations'] as $idx => $baksink)
                {
                    if(($baksink['priority']) == "")
                    {
                      continue;  
                    }
                    $ret = DBAddStreaminterfaceBackupSink($stinterfacesink['id'], $baksink['name'], $baksink['priority'], $baksink['maxretrytime'], $baksink['streammissfailovertimeout'], $baksink['sinktype'], $baksink['host'], $baksink['port'], $baksink['talkbackhost'], $baksink['talkbackport'], $baksink['hostclientcertificate_id'], $baksink['talkbackclientcertificate_id']);                    
                    if($ret < 1){
                        print("Error updating streaminterfacebackupsink!\n");
                    }
                }
            }
        }
    }
    
    
    function parseVersion1($lines, $maxnrofinterfaces)
    {
        $titlesfound = false;
		$handledinterfaces = 0;
        $interfaces = array();
		$handledinterfaces = 0;
        foreach($lines as $idx => $line)
        {
            if(trim($line) == "" && $titlesfound)
            {
                break;
            }
            else if(trim($line) == "")
            {
                continue;
            }
            
            $parts = explode(",", $line);
            if(!$titlesfound)
            {
                if(strcmp(trim($parts[0]), "interface") == 0)
                {                
                    $titlesfound = true;
                }
                
            }
            else if($titlesfound)
            {
                $result = parseLineVersion1($line);
                if($result === false)
                {
                    print("could not parse line $idx!\n");
                    continue;
                }
                else
                {
                    $interfaces[] = $result;
					$handledinterfaces++;
					if($handledinterfaces >= $maxnrofinterfaces)
					{
						return $interfaces;
					}
                }
            }
        }
        return $interfaces;
    }
    
    function parseVersion2($lines, $maxnrofinterfaces)
    {
        $titlesfound = false;
		$handledinterfaces = 0;
        $interfaces = array();
		$handledinterfaces = 0;
        foreach($lines as $idx => $line)
        {
            if(trim($line) == "" && $titlesfound)
            {
                break;
            }
            else if(trim($line) == "")
            {
                continue;
            }
            
            $parts = explode(",", $line);
            if(!$titlesfound)
            {
                if(strcmp(trim($parts[0]), "interface") == 0)
                {                
                    $titlesfound = true;
                }
                
            }
            else if($titlesfound)
            {
                $result = parseLineVersion2($line);
                if($result === false)
                {
                    print("could not parse line $idx!\n");
                    continue;
                }
                else
                {
                    $interfaces[] = $result;
					$handledinterfaces++;
					if($handledinterfaces >= $maxnrofinterfaces)
					{
						return $interfaces;
					}
                }
            }
        }
        return $interfaces;
    }
	
	function getNrOfLicensedInterfaces()
	{
		$nrofinterfaces = file_get_contents("/etc/delayerdb/nrofstreaminterfaces");
		if($nrofinterfaces === false)
		{
			return 0;
		}
		return trim($nrofinterfaces);
	}
	
    function printUsage($progname)
    {
        print("usage: " . $progname . " [-h] <importfile>\n\n");
    }
    
    // Main program start
    
    if($argc < 1)
    {
        printUsage($argv[0]);
        exit(1);
    }
    else if($argv[1] == "-h")
    {
        printUsage($argv[0]);
        exit(0);
    }
    
    $importfile = $argv[1];
    
    if(!is_file($importfile))
    {
        printf("Could not access importfile!\n");
        exit(2);
    }
    
    $lines = file($importfile);
  
  	$maxnrofinterfaces = getNrOfLicensedInterfaces();
    $handledfile = false;
    foreach($lines as $line)
    {
        if(trim($line)  == "")
        {
            continue;
        }

        if(strncmp(trim($line), "configversion",13) == 0)
        {
            
            $parts = explode(",",$line);
            if(trim($parts[1]) == "1")
            {
                $interfaces = parseVersion1($lines, $maxnrofinterfaces);
                writeConfig($interfaces);
                $handledfile = true;
                break;
            }
            else if(trim($parts[1]) == "2")
            {
                $interfaces = parseVersion2($lines, $maxnrofinterfaces);
                writeConfig($interfaces);
                $handledfile = true;
                break;
            }
            else
            {
               print("Unhandled verson " . trim($parts[1]). "\n" );
               break; 
            }
            break;
        }
    }
    
    if(!$handledfile)
    {
        print("Could not parse file!\n");
        exit(3);
    }
    
    exit(0);
?>
