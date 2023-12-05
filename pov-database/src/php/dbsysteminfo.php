<?php
    require_once("dblib.php");

    function DBFetchSysteminfo()
    {
		global $DB;
		$sth = $DB->prepare("select * from systeminfo");
        $sth->execute();
		$result = $sth->fetch();
		if($result === false){
			return array();
		}
		return $result;
    }
    
    function DBUpdateSysteminfoSystemName($systemname)
    {
        global $DB;
        $systemname = trim($systemname);
        $sth = $DB->prepare("update systeminfo set systemname = :systemname");
        $result = $sth->execute(array(":systemname" => $systemname));
        
		if(!$result){
			return 0;
		}
			return 1;
    }

    function DBUpdateSysteminfoLabelAPIPassword($labelapipassw)
    {
        global $DB;
        $labelapipassw = trim($labelapipassw);
        $sth = $DB->prepare("update systeminfo set labelapipassw = :labelapipassw");
        $result = $sth->execute(array(":labelapipassw" => $labelapipassw));
        
		if(!$result){
			return 0;
		}
			return 1;
    }
	
	function DBFetchSysteminfoStatdb()
	{
		global $DB;
		
		$sth = $DB->prepare("select statdb_remote_url, statdb_sleeptime from systeminfo");
		$sth->execute(array());
		$result = $sth->fetch();
		
		if($result === false)
		{
			return array();
		}
		
		return $result;
	}

	function DBFetchLabelAPIPassword()
	{
		global $DB;
		
		$sth = $DB->prepare("select labelapipassw from systeminfo");
		$sth->execute(array());
		$result = $sth->fetch();
		
		if($result === false)
		{
			return array();
		}
		
		return $result;
	}
	
	function DBSysteminfoSetStatdb($remoteurl, $sleeptime)
	{
		global $DB;
		
		if( $sleeptime < 1 )
		{
			return 0;
		}
		
		$remoteurl = trim($remoteurl);
		$sth = $DB->prepare("update systeminfo set statdb_remote_url=:remoteurl, statdb_sleeptime=:sleeptime");
		$result = $sth->execute(array( ":remoteurl" => $remoteurl,
									   ":sleeptime" => $sleeptime));
		
		if(!$result){
			return 0;
		}
			return 1;					
	}
?>
