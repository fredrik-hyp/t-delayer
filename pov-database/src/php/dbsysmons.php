<?php
	require_once("dblib.php");
	
	function DBFetchSysmon()
	{
		global $DB;
		
		$sth = $DB->prepare("SELECT * FROM sysmons");
		$sth->execute(array());
		if($sth === false)
		{
			return array();
		}
		
		$result = $sth->fetch();
		
		return $result;
	}
?>