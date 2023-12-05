#!/usr/bin/php

<?php
    require_once("/var/db/dbstreaminterfaces.php");
    require_once("/var/db/dbstreaminterfaceconfigs.php");
    require_once("/var/db/dbstreaminterfacesources.php");
    require_once("/var/db/dbstreaminterfacesinks.php");
    require_once("/var/db/dbstreaminterfacebackupsources.php");
    require_once("/var/db/dbstreaminterfacebackupsinks.php");
    
    define("MAX_BACKUP_INTERFACES", 8);
    define("CSV_CONFIG_VERSION", 2);
    
    function fixStr($str)
    {
        $str = str_replace("," , " ", $str);
        
        return $str;
    }
        
    function getCSVConfigForInterface($stinterface)
    {
        $stconfig = DBFetchStreaminterfaceConfigByStreaminterfaceId($stinterface['id']);
        $stsource = DBFetchStreaminterfaceSourceByStreaminterfaceConfigId($stconfig['id']);
        $stsink = DBFetchStreaminterfaceSinkByStreaminterfaceConfigId($stconfig['id']);
        $stbackupsources = DBFetchStreaminterfaceBackupSourcesByStreaminterfacesourceId($stsource['id']);
        $stbackupsinks = DBFetchStreaminterfaceBackupSinksByStreaminterfacesinkId($stsink['id']);
            
        $stinterface['name'] = fixStr($stinterface['name']);
        $stinterface['usertext'] = fixStr($stinterface['usertext']);

        if($stconfig['autostart']){ $stconfig['autostart'] = "yes";} else {$stconfig['autostart'] = "no";}
        if($stconfig['nosessionid']){ $stconfig['legacy'] = "yes";} else {$stconfig['legacy'] = "no";}
        if($stconfig['mcinuecheck']){ $stconfig['mcinusecheck'] = "yes";} else {$stconfig['mcinusecheck'] = "no";}
        if($stconfig['multipath']){ $stconfig['multipath'] = "yes";} else {$stconfig['multipath'] = "no";}
        if($stconfig['mploadbalance']){ $stconfig['mploadbalance'] = "yes";} else {$stconfig['mploadbalance'] = "no";}
            
        $virtualsource = $stsource['virtualsource_id'];

        if($stconfig['mode'] == "transmitter")
        {
            $hostcertificate = $stsink['hostclientcertificate_id'];
            $talkbackcertificate = $stsink['talkbackclientcertificate_id'];
            $sourcehost = $stsource['host'];
            $sourceport = $stsource['port'];
            $sourcetalkbackhost = "";
            $sourcetalkbackport = "";

            $sinkhost = $stsink['host'];
            $sinkport = $stsink['port'];
            $sinktalkbackhost = $stsink['talkbackhost'];
            $sinktalkbackport = $stsink['talkbackport'];

            $streammisstimeout = $stsink['streammissfailovertimeout'];
            $retrytime = $stsink['maxretrytime'];
        }
        else
        {
            $hostcertificate = $stsource['hostclientcertificate_id'];
            $talkbackcertificate = $stsource['talkbackclientcertificate_id'];
            $sourcehost = $stsource['host'];
            $sourceport = $stsource['port'];
            $sourcetalkbackhost = $stsource['talkbackhost'];
            $sourcetalkbackport = $stsource['talkbackport'];
            
            $sinkhost = $stsink['host'];
            $sinkport = $stsink['port'];
            $sinktalkbackhost = "";
            $sinktalkbackport = "";

            $streammisstimeout = $stsource['streammissfailovertimeout'];
            $retrytime = $stsource['maxretrytime'];
        }
        
        $csv = "";
        $csv .= $stinterface['name'] . "," . $stinterface['usertext'] . "," . $stconfig['mode'] . "," . $stconfig['autostart'] . ',' . $stconfig['mcinusecheck'] . "," . $stconfig['multipath'] . "," . $stconfig['mploadbalance'] . "," . $stconfig['msbuffer'];
        $csv .= "," . $stconfig['legacy'] . "," . $stsink['ttl'] . "," . $hostcertificate . "," . $talkbackcertificate . "," . $stsource['sourcetype']  . ",". $sourcehost . "," . $sourceport . "," . $sourcetalkbackhost . "," . $sourcetalkbackport;
        $csv .= "," . $stsink['sinktype'] . "," . $sinkhost . "," . $sinkport . "," . $sinktalkbackhost . "," . $sinktalkbackport . "," . $virtualsource . "," . $streammisstimeout . "," . $retrytime;

        for( $i=0; $i<MAX_BACKUP_INTERFACES; $i++)
        {
            
            if(!isset($stbackupsources[$i]) || sizeof($stbackupsources) == 0)
            {
                $stbackupsources[$i] = array("name" => "", "priority" => "", "maxretrytime" => "", "streammissfailovertimeout" => "", "hostclientcertificate_id" => "", "talkbackclientcertificate_id" => "", "sourcetype" => "", "host" => "", "port" => "", "talkbackhost" => "", "talkbackport" => "");
            }
            
            if(!isset($stbackupsinks[$i]) || sizeof($stbackupsinks) == 0)
            {
                $stbackupsinks[$i] = array("name" => "", "priority" => "", "maxretrytime" => "", "streammissfailovertimeout" => "", "hostclientcertificate_id" => "", "talkbackclientcertificate_id" => "", "sinktype" => "", "host" => "", "port" => "", "talkbackhost" => "", "talkbackport" => "");
            }
            
            $csv .= "," . $stbackupsources[$i]['name'] . "," . $stbackupsources[$i]['priority'] . "," . $stbackupsources[$i]['maxretrytime'] . "," . $stbackupsources[$i]['streammissfailovertimeout'] . "," . $stbackupsources[$i]['hostclientcertificate_id'] . "," . $stbackupsources[$i]['talkbackclientcertificate_id'] . "," .  $stbackupsources[$i]['sourcetype'] . "," . $stbackupsources[$i]['host'] . "," . $stbackupsources[$i]['port'] . "," . $stbackupsources[$i]['talkbackhost'] . "," . $stbackupsources[$i]['talkbackport'];

            $csv .= "," . $stbackupsinks[$i]['name'] . "," . $stbackupsinks[$i]['priority'] . "," . $stbackupsinks[$i]['maxretrytime'] . "," . $stbackupsinks[$i]['streammissfailovertimeout'] . "," . $stbackupsinks[$i]['hostclientcertificate_id'] . "," . $stbackupsinks[$i]['talkbackclientcertificate_id'] . ","  . $stbackupsinks[$i]['sinktype'] . "," . $stbackupsinks[$i]['host'] . "," . $stbackupsinks[$i]['port'] . "," . $stbackupsinks[$i]['talkbackhost'] . "," . $stbackupsinks[$i]['talkbackport'];
            
        }
        
        $csv .= "\n";
        
        return $csv;
    }

    function getMaxInterfaces()
    {
        $contents = file_get_contents("/etc/poverix/nrofstreaminterfaces");
        if($contents === false)
        {
            return 0;
        }
        
        return intVal($contents);
    }
    
    function printHelp($progname)
    {
        print("usage: " . $progname  . " [-h|interfacenumber]\n\n");
    }
    
    // Main program start
    $fetchallinterfaces = true;
    $maxinterfaces = getMaxInterfaces();
    if($argc > 1)
    {
        if($argv[1] != "-h" && !is_numeric($argv[1]))
        {
            printHelp($argv[0]);
            exit(0);
        }
        
        $interfacenumber = intVal($argv[1]);
        if( $interfacenumber < 0 || $interfacenumber > $maxinterfaces )
        {
            print("Interfacenumber is out of range!\n\n");
            exit(2);
        }
        $fetchallinterfaces = false;
    }
    
    $serialnumber = file_get_contents("/etc/serialnumber");
    if( $serialnumber === false )
    {
        $csv = "serialnumber, not found\n\n";
        $serialnumber="";
    }
    else
    {
        $serialnumber= trim($serialnumber);
        $csv="serialnumber, $serialnumber\n";
    }
    
    $csv .= "configversion, " . CSV_CONFIG_VERSION . "\n\n";
    $csv .= "interface,name,mode,autostart,norecvmulticastcheck, multipath, multipath loadbalance, buffer,legacy,ttl,hostcertificate,talkbackcertificate,sourcetype, sourcehost,sourceport,sourcetalkbackhost, sourcetalkbackport,sinktype,desthost,destport,desttalkbackhost,desttalkbackport,virtualsource,streammisstimeout, retrytime";
    
    // construct headers for backup sources and destinations
    for($i = 1 ; $i<=MAX_BACKUP_INTERFACES; $i++)
    {
        
        $csv .= ",baksrcname_$i, baksrcpriority_$i, baksrcmaxretrytime_$i, baksrctimeout_$i, backsrchostcertificate_$i, backsrctbcertificate_$i, baksrctype_$i, baksrchost_$i, baksrcport_$i, baksrctalkbackhost_$i, baksrctalkbackport_$i, baksrcvirtid_$i";
        $csv .= ",bakdestname_$i, bakdestpriority_$i, bakdestmaxretrytime_$i, bakdesttimeout_$i, backdesthostcertificate_$i, backdesttbcertificate_$i, bakdesttype_$i, bakdesthost_$i, bakdestport_$i, bakdesttalkbackhost_$i, bakdesttalkbackport_$i";
    }
    $csv .= "\n";
    
    if( $fetchallinterfaces )
    {
//        print("Fetching config for ALL interfaces\n");
        $stinterfaces = DBFetchAllStreaminterfaces();
        foreach( $stinterfaces as $stinterface )
        {
            $csv .= getCSVConfigForInterface($stinterface);
        } 
        $filepost="all";  
    }
    else
    {
  //      print("Fetching config for interface $interfacenumber\n");
        $stinterface = DBFetchStreaminterfaceByName("st" . $interfacenumber);
        $csv .= getCSVConfigForInterface($stinterface);
        $filepost=trim($interfacenumber);
    }
    
    $date = date("Y-m-d");
    
    $csv .= "\n ,generated,$date\n";
    $csv .= " ,(c) po<ver>ix 2014\n";

	$csv = utf8_encode($csv);
    file_put_contents("/tmp/stconfigs_${serialnumber}_$filepost.csv", $csv);
    
?>